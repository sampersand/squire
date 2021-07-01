use super::CodeBlock;
use crate::runtime::{Bytecode, Opcode, Vm, Result, Interrupt, Error};
use crate::value::{Value, Numeral, ops::*};

#[derive(Debug)]
pub struct StackFrame<'a> {
	codeblock: &'a CodeBlock,
	vm: &'a mut Vm,
	ip: usize,
	locals: &'a mut [Value],
	handlers: Vec<Handler>
}

#[derive(Debug)]
struct Handler {
	exception: usize,
	start: usize
}

impl<'a> StackFrame<'a> {
	pub fn new(codeblock: &'a CodeBlock, locals: &'a mut [Value], vm: &'a mut Vm) -> Self {
		Self { codeblock, vm, ip: 0, locals, handlers: Vec::new() }
	}

	fn is_finished(&self) -> bool {
		debug_assert!(self.ip <= self.codeblock.code().len(), "ip ({:?}) is too large (max={:?})",
			self.ip, self.codeblock.code().len());

		self.ip == self.codeblock.code().len()
	}

	fn next(&mut self) -> Bytecode {
		let code = self.codeblock.code()[self.ip];
		self.ip += 1;
		trace!(?self.ip, ?code, "next instruction read");
		code
	}

	fn next_opcode(&mut self) -> Opcode {
		match self.next()  {
			Bytecode::Opcode(opcode) => opcode,
			other => panic!("expected an Opcode but was given {:?}", other)
		}
	}

	fn next_local_index(&mut self) -> usize {
		match self.next() {
			Bytecode::Local(index) => index,
			other => panic!("expected a Local but was given {:?}", other)
		}
	}

	fn next_local(&mut self) -> &Value {
		&self.locals[self.next_local_index()]
	}

	fn next_local_mut(&mut self) -> &mut Value {
		&mut self.locals[self.next_local_index()]
	}

	fn next_global_index(&mut self) -> usize {
		match self.next() {
			Bytecode::Global(index) => index,
			other => panic!("expected a Global but was given {:?}", other)
		}
	}

	fn next_constant(&mut self) -> &Value {
		match self.next() {
			Bytecode::Constant(index) => &self.codeblock.constants()[index],
			other => panic!("expected a Constant but was given {:?}", other)
		}
	}

	fn next_offset(&mut self) -> isize {
		match self.next() {
			Bytecode::Offset(index) => index,
			other => panic!("expected a Offset but was given {:?}", other)
		}
	}

	fn next_count(&mut self) -> usize {
		match self.next()  {
			Bytecode::Count(count) => count,
			other => panic!("expected an Count but was given {:?}", other)
		}
	}

	fn next_locals_and_vm<const N: usize>(&mut self) -> ([&Value; N], &mut Vm) {
		let mut locals = std::mem::MaybeUninit::<[&Value; N]>::uninit();

		for i in 0..N {
			let local = self.next_local();

			unsafe {
				(locals.as_mut_ptr() as *mut &Value).offset(i as isize).write(local);
			}
		}

		(unsafe { locals.assume_init() }, self.vm)
	}

	fn jump(&mut self, to: isize) {
		let new_ip = (self.ip as isize + to) as usize;

		trace!(old_ip=%self.ip, new_ip=%new_ip, "jumped");

		self.ip = new_ip;
	}

	fn set_result(&mut self, value: Value) {
		*self.next_local_mut() = value;
	}
}

impl StackFrame<'_> {
	fn do_noop(&mut self) {
		// do nothing
	}

	fn do_move(&mut self) {
		let source = self.next_local_index();
		let target = self.next_local_index();

		if source != target {
			self.locals[target] = self.locals[source].clone();
		}
	}

	fn do_jump(&mut self) {
		let to = self.next_offset();
		self.jump(to - 1);
	}

	fn do_jump_if_false(&mut self) -> Result<()> {
		let cond = self.next_local().clone();
		let to = self.next_offset();

		if !cond.convert_to::<bool>(self.vm)? {
			self.jump(to - 1);
		}

		Ok(())
	}

	fn do_jump_if_true(&mut self) -> Result<()> {
		let cond = self.next_local().clone();
		let to = self.next_offset();

		if cond.convert_to::<bool>(self.vm)? {
			self.jump(to - 1);
		}

		Ok(())
	}

	fn do_call(&mut self) -> Result<()> {
		let func = self.next_local().clone();
		let argc = self.next_count();

		let mut args = Vec::with_capacity(argc);
		for _ in 0..argc {
			args.push(self.next_local().clone());
		}

		let args = crate::runtime::Args::new(&args);

		let result = func.call(args, self.vm)?;

		self.set_result(result);
		Ok(())
	}

	fn do_return(&mut self) -> Result<Value> {
		Ok(self.next_local().clone())
	}

	fn do_comefrom(&mut self) -> Result<()> {
		todo!();
	}

	fn do_throw(&mut self) -> Result<()> {
		let value = self.next_local();

		Err(Error::Throw(value.clone()))
	}

	fn do_attempt(&mut self) -> Result<()> {
		let start = ((self.ip as isize) + self.next_offset()) as usize;
		let exception = self.next_local_index();

		self.handlers.push(Handler { start, exception });

		Ok(())
	}

	fn do_pop_handler(&mut self) -> Result<()> {
		debug_assert_ne!(self.handlers.len(), 0);

		self.handlers.pop();

		Ok(())
	}

	fn do_unary_op(&mut self, op: fn(&Value, &mut Vm) -> Result<Value>) -> Result<()> {
		let ([arg], vm) = self.next_locals_and_vm();
		let result = (op)(arg, vm)?;

		self.set_result(result);
		Ok(())
	}

	fn do_binary_op(&mut self, op: fn(&Value, &Value, &mut Vm) -> Result<Value>) -> Result<()> {
		let ([lhs, rhs], vm) = self.next_locals_and_vm();
		let result = (op)(lhs, rhs, vm)?;

		self.set_result(result);
		Ok(())
	}

	fn do_not(&mut self) -> Result<()> {
		self.do_unary_op(|arg, vm| arg.convert_to::<bool>(vm).map(|x| Value::Veracity(!x)))
	}

	fn do_equals(&mut self) -> Result<()> {
		self.do_binary_op(|lhs, rhs, vm| lhs.is_equal(rhs, vm).map(Value::Veracity))
	}

	fn do_not_equals(&mut self) -> Result<()> {
		self.do_binary_op(|lhs, rhs, vm| lhs.is_equal(rhs, vm).map(|x| Value::Veracity(!x)))
	}

	fn do_less_than(&mut self) -> Result<()> {
		self.do_binary_op(|lhs, rhs, vm| lhs.compare(rhs, vm).map(|x| (x == Some(std::cmp::Ordering::Less)).into()))
	}

	fn do_less_than_or_equal(&mut self) -> Result<()> {
		self.do_binary_op(|lhs, rhs, vm| lhs.compare(rhs, vm).map(|x| (x != Some(std::cmp::Ordering::Greater)).into()))
	}

	fn do_greater_than(&mut self) -> Result<()> {
		self.do_binary_op(|lhs, rhs, vm| lhs.compare(rhs, vm).map(|x| (x == Some(std::cmp::Ordering::Greater)).into()))
	}

	fn do_greater_than_or_equal(&mut self) -> Result<()> {
		self.do_binary_op(|lhs, rhs, vm| lhs.compare(rhs, vm).map(|x| (x != Some(std::cmp::Ordering::Less)).into()))
	}

	fn do_compare(&mut self) -> Result<()> {
		self.do_binary_op(|lhs, rhs, vm| lhs.compare(rhs, vm).map(|x| x.map_or(Value::Null, |x| Numeral::from(x).into())))
	}

	fn do_pos(&mut self) -> Result<()> {
		// self.do_unary_op(Value::try_neg)
		todo!();
	}

	fn do_negate(&mut self) -> Result<()> {
		self.do_unary_op(Value::negate)
	}

	fn do_add(&mut self) -> Result<()> {
		self.do_binary_op(Value::add)
	}

	fn do_subtract(&mut self) -> Result<()> {
		self.do_binary_op(Value::subtract)
	}

	fn do_multiply(&mut self) -> Result<()> {
		self.do_binary_op(Value::multiply)
	}

	fn do_divide(&mut self) -> Result<()> {
		self.do_binary_op(Value::divide)
	}

	fn do_modulo(&mut self) -> Result<()> {
		self.do_binary_op(Value::modulo)
	}

	fn do_power(&mut self) -> Result<()> {
		self.do_binary_op(Value::power)
	}

	fn do_index(&mut self) -> Result<()> {
		self.do_binary_op(Value::get_index)
	}

	fn do_index_assign(&mut self) -> Result<()> {
		let index = self.next_local_index();
		let key = self.next_local().clone();
		let value = self.next_local().clone();

		self.locals[index].set_index(key, value, self.vm)
	}

	fn do_load_constant(&mut self) -> Result<()> {
		let constant = self.next_constant().clone();
		self.set_result(constant);
		Ok(())
	}

	fn do_load_global(&mut self) -> Result<()> {
		let global_index = self.next_global_index();
		let global = self.vm.get_global(global_index);
		self.set_result(global);
		Ok(())
	}

	fn do_store_global(&mut self) -> Result<()> {
		let global_index = self.next_global_index();
		let value = self.next_local().clone();
		self.vm.set_global(global_index, value);
		Ok(())
	}

	fn do_get_attribute(&mut self) -> Result<()> {
		let value = self.next_local().clone();

		match self.next_constant().clone() {
			Value::Text(text) => {
				let attr = value.get_attr(text.as_str(), self.vm)?;
				self.set_result(attr);
				Ok(())
			},
			other => unreachable!("tried getting a non-text attr {:?}", other)
		}
	}

	fn do_set_attribute(&mut self) -> Result<()> {
		let index = self.next_local_index();
		let attr = 
			match self.next_constant() {
				Value::Text(text) => text.clone(),
				other => unreachable!("tried setting a non-tex tattr {:?}", other)
			};
		let value = self.next_local().clone();

		self.locals[index].set_attr(attr.as_str(), value, self.vm)
	}

	#[tracing::instrument(level="debug", skip(self))]
	pub fn run(mut self) -> Result<Value> {
		while !self.is_finished() {
			trace!(?self.locals);

			match self.run_inner() {
				Ok(Some(return_value)) => return Ok(return_value),
				Ok(None) => continue,
				Err(err) if self.handlers.is_empty() => return Err(err),
				Err(err) => {
					let value = 
						if let Error::Throw(value) = err {
							value
						} else {
							err.to_string().into()
						};

					let Handler { start, exception } = self.handlers.pop().unwrap();
					self.locals[exception] = value;
					self.ip = start;
					continue;
				},
			}
		}

		// if we reach the end without a return, we just return Null.
		Ok(Value::Null)
	}

	fn run_inner(&mut self) -> Result<Option<Value>> {
		match self.next_opcode() {
			// Misc
			Opcode::NoOp => self.do_noop(),
			Opcode::Move => self.do_move(),
			Opcode::Interrupt => self.do_interrupt(),

			// Control flow
			Opcode::Jump => self.do_jump(),
			Opcode::JumpIfFalse => self.do_jump_if_false()?,
			Opcode::JumpIfTrue => self.do_jump_if_true()?,
			Opcode::Call => self.do_call()?,
			Opcode::Return => return self.do_return().map(Some),
			Opcode::ComeFrom => self.do_comefrom()?,
			Opcode::Throw => self.do_throw()?,
			Opcode::Attempt => self.do_attempt()?,
			Opcode::PopHandler => self.do_pop_handler()?,

			// Logical Operations
			Opcode::Not => self.do_not()?,
			Opcode::Equals => self.do_equals()?,
			Opcode::NotEquals => self.do_not_equals()?,
			Opcode::LessThan => self.do_less_than()?,
			Opcode::LessThanOrEqual => self.do_less_than_or_equal()?,
			Opcode::GreaterThan => self.do_greater_than()?,
			Opcode::GreaterThanOrEqual => self.do_greater_than_or_equal()?,
			Opcode::Compare => self.do_compare()?,

			// Math
			Opcode::Pos => self.do_pos()?,
			Opcode::Negate => self.do_negate()?,
			Opcode::Add => self.do_add()?,
			Opcode::Subtract => self.do_subtract()?,
			Opcode::Multiply => self.do_multiply()?,
			Opcode::Divide => self.do_divide()?,
			Opcode::Modulo => self.do_modulo()?,
			Opcode::Power => self.do_power()?,

			// Misc Operators
			Opcode::Index => self.do_index()?,
			Opcode::IndexAssign => self.do_index_assign()?,

			// VM-specific
			Opcode::LoadConstant => self.do_load_constant()?,
			Opcode::LoadGlobal => self.do_load_global()?,
			Opcode::StoreGlobal => self.do_store_global()?,
			Opcode::GetAttribute => self.do_get_attribute()?,
			Opcode::SetAttribute => self.do_set_attribute()?,
		}

		Ok(None)
	}

	fn do_interrupt(&mut self) {
		let interrupt = 
			match self.next() {
				Bytecode::Interrupt(interrupt) => interrupt,
				other => panic!("expected an Interrupt but was given {:?}", other)
			};

		match interrupt {
			Interrupt::NewArray => self.do_interrupt_new_array(),
			Interrupt::NewCodex => self.do_interrupt_new_codex()
		}
	}

	fn do_interrupt_new_array(&mut self) {
		let count = self.next_count();
		let mut eles = Vec::with_capacity(count);

		for _ in 0..count {
			eles.push(self.next_local().clone());
		}

		self.set_result(Value::Book(eles.into_iter().collect()))
	}

	fn do_interrupt_new_codex(&mut self) {
		let count = self.next_count();
		let mut pages = Vec::with_capacity(count);

		for _ in 0..count {
			let key = self.next_local().clone();
			let value = self.next_local().clone();

			pages.push((key, value));
		}

		self.set_result(Value::Codex(pages.into_iter().collect()))
	}


}

