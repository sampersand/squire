use crate::Value;
use super::CodeBlock;
use crate::runtime::{Bytecode, Opcode, Vm, Result};

#[derive(Debug, PartialEq, Eq)]
pub struct StackFrame<'a> {
	codeblock: &'a CodeBlock,
	vm: &'a mut Vm,
	ip: usize,
	locals: &'a mut [Value]
}

impl<'a> StackFrame<'a> {
	pub fn new(codeblock: &'a CodeBlock, locals: &'a mut [Value], vm: &'a mut Vm) -> Self {
		Self { codeblock, vm, ip: 0, locals }
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
			other => panic!("expected an 'Opcode' but was given {:?}", other)
		}
	}

	fn next_local_index(&mut self) -> usize {
		match self.next() {
			Bytecode::Local(index) => index,
			other => panic!("expected a 'Local' but was given {:?}", other)
		}
	}

	fn next_local(&mut self) -> &Value {
		&self.locals[self.next_local_index()]
	}

	fn next_local_mut(&mut self) -> &mut Value {
		&mut self.locals[self.next_local_index()]
	}

	fn next_global(&mut self) -> Value {
		match self.next() {
			Bytecode::Global(index) => self.vm.get_global(index),
			other => panic!("expected a 'Global' but was given {:?}", other)
		}
	}

	fn next_constant(&mut self) -> &Value {
		match self.next() {
			Bytecode::Constant(index) => &self.codeblock.constants()[index],
			other => panic!("expected a 'Constant' but was given {:?}", other)
		}
	}

	fn next_offset(&mut self) -> isize {
		match self.next() {
			Bytecode::Offset(index) => index,
			other => panic!("expected a 'Offset' but was given {:?}", other)
		}
	}

	fn next_count(&mut self) -> usize {
		match self.next()  {
			Bytecode::Count(count) => count,
			other => panic!("expected an 'Count' but was given {:?}", other)
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

		if !cond.to_veracity(self.vm)? {
			self.jump(to - 1);
		}

		Ok(())
	}

	fn do_jump_if_true(&mut self) -> Result<()> {
		let cond = self.next_local().clone();
		let to = self.next_offset();

		if cond.to_veracity(self.vm)? {
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

		let args = crate::runtime::Args::new(args, Default::default());

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
		todo!();
	}

	fn do_trycatch(&mut self) -> Result<()> {
		todo!();
	}

	fn do_poptrycatch(&mut self) -> Result<()> {
		todo!();
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
		self.do_unary_op(|arg, vm| arg.try_not(vm).map(Value::Veracity))
	}

	fn do_equals(&mut self) -> Result<()> {
		self.do_binary_op(|lhs, rhs, vm| lhs.try_eql(rhs, vm).map(Value::Veracity))
	}

	fn do_not_equals(&mut self) -> Result<()> {
		self.do_binary_op(|lhs, rhs, vm| lhs.try_neq(rhs, vm).map(Value::Veracity))
	}

	fn do_less_than(&mut self) -> Result<()> {
		self.do_binary_op(|lhs, rhs, vm| lhs.try_lth(rhs, vm).map(Value::Veracity))
	}

	fn do_less_than_or_equal(&mut self) -> Result<()> {
		self.do_binary_op(|lhs, rhs, vm| lhs.try_gth(rhs, vm).map(Value::Veracity))
	}

	fn do_greater_than(&mut self) -> Result<()> {
		self.do_binary_op(|lhs, rhs, vm| lhs.try_gth(rhs, vm).map(Value::Veracity))
	}

	fn do_greater_than_or_equal(&mut self) -> Result<()> {
		self.do_binary_op(|lhs, rhs, vm| lhs.try_geq(rhs, vm).map(Value::Veracity))
	}

	fn do_compare(&mut self) -> Result<()> {
		self.do_binary_op(Value::try_cmp)
	}

	fn do_pos(&mut self) -> Result<()> {
		// self.do_unary_op(Value::try_neg)
		todo!();
	}

	fn do_negate(&mut self) -> Result<()> {
		self.do_unary_op(Value::try_neg)
	}

	fn do_add(&mut self) -> Result<()> {
		self.do_binary_op(Value::try_add)
	}

	fn do_subtract(&mut self) -> Result<()> {
		self.do_binary_op(Value::try_sub)
	}

	fn do_multiply(&mut self) -> Result<()> {
		self.do_binary_op(Value::try_mul)
	}

	fn do_divide(&mut self) -> Result<()> {
		self.do_binary_op(Value::try_div)
	}

	fn do_modulo(&mut self) -> Result<()> {
		self.do_binary_op(Value::try_rem)
	}

	fn do_power(&mut self) -> Result<()> {
		self.do_binary_op(Value::try_pow)
	}

	fn do_index(&mut self) -> Result<()> {
		todo!("the second value's going to be a constant index, not a target.");
		// self.do_binary_op(Value::try_index)
	}

	fn do_index_assign(&mut self) -> Result<()> {
		let index = self.next_local_index();
		let key = self.next_local().clone();
		let value = self.next_local().clone();

		self.locals[index].try_index_assign(key, value, self.vm)
	}

	fn do_load_constant(&mut self) -> Result<()> {
		let constant = self.next_constant().clone();
		self.set_result(constant);
		Ok(())
	}

	fn do_load_global(&mut self) -> Result<()> {
		let global = self.next_global();
		self.set_result(global);
		Ok(())
	}

	fn do_store_global(&mut self) -> Result<()> {
		let _ = self.next_global();
		todo!();
	}

	fn do_get_attribute(&mut self) -> Result<()> {
		todo!();
	}

	fn do_set_attribute(&mut self) -> Result<()> {
		todo!();
	}


	#[tracing::instrument(level="debug", skip(self))]
	pub fn run(mut self) -> Result<Value> {
		while !self.is_finished() {
			trace!(?self.locals);

			match self.next_opcode() {
				// Misc
				Opcode::NoOp => self.do_noop(),
				Opcode::Move => self.do_move(),
				Opcode::Interrupt => todo!(),

				// Control flow
				Opcode::Jump => self.do_jump(),
				Opcode::JumpIfFalse => self.do_jump_if_false()?,
				Opcode::JumpIfTrue => self.do_jump_if_true()?,
				Opcode::Call => self.do_call()?,
				Opcode::Return => return self.do_return(),
				Opcode::ComeFrom => self.do_comefrom()?,
				Opcode::Throw => self.do_throw()?,
				Opcode::TryCatch => self.do_trycatch()?,
				Opcode::PopTryCatch => self.do_poptrycatch()?,

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
		}

		// if we reach the end without a return, we just return Null.
		Ok(Value::Null)
	}
}

