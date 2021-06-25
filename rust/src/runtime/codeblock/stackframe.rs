use crate::Value;
use super::CodeBlock;
use crate::runtime::{Bytecode, Opcode, Vm, Result};

#[derive(Debug, PartialEq, Eq)]
pub struct StackFrame<'a> {
	codeblock: &'a CodeBlock,
	vm: &'a mut Vm,
	args: &'a [Value],
	ip: usize,
	locals: &'a mut [Value]
}

impl<'a> StackFrame<'a> {
	pub fn new(codeblock: &'a CodeBlock, args: &'a [Value], locals: &'a mut [Value], vm: &'a mut Vm) -> Self {
		Self { codeblock, vm, args, ip: 0, locals }
	}

	fn is_finished(&self) -> bool {
		debug_assert!(self.ip <= self.codeblock.code().len());

		self.ip == self.codeblock.code().len()
	}

	fn next(&mut self) -> Bytecode {
		let code = self.codeblock.code()[self.ip];
		self.ip += 1;
		trace!(?code, ?self.ip, "next instruction read");
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

	fn next_global(&mut self) -> &Value {
		todo!()
	}

	fn next_constant(&mut self) -> &Value {
		match self.next() {
			Bytecode::Constant(index) => &self.codeblock.constants()[index],
			other => panic!("expected a 'Local' but was given {:?}", other)
		}
	}

	fn next_label(&mut self) -> usize {
		match self.next() {
			Bytecode::Offset(index) => (self.ip as isize).checked_add(index).unwrap() as usize,
			other => panic!("expected a 'Local' but was given {:?}", other)
		}
	}

	fn next_locals_and_vm<const N: usize>(&mut self) -> ([&Value; N], &mut Vm) {
		todo!()
	}

	#[tracing::instrument(level="trace")]
	fn jump(&mut self, to: usize) {
		self.ip += to;
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
		let target = self.next_local_index();
		let source = self.next_local_index();

		if source != target {
			self.locals[target] = self.locals[source].clone();
		}
	}

	fn do_jump(&mut self) {
		let to = self.next_label();
		self.jump(to);
	}

	fn do_jump_if_false(&mut self) -> Result<()> {
		let to = self.next_label();
		let cond = self.next_local().clone();

		if !cond.to_boolean(self.vm)? {
			self.jump(to);
		}

		Ok(())
	}

	fn do_jump_if_true(&mut self) -> Result<()> {
		let to = self.next_label();
		let cond = self.next_local().clone();

		if cond.to_boolean(self.vm)? {
			self.jump(to);
		}

		Ok(())
	}

	fn do_call(&mut self) -> Result<()> {
		todo!()
		// let target = self.next_local();
		// let argc = self.next_argc
		// match self.next_local() {

		// }
		// 	sq_value imitation_value = NEXT_LOCAL();
		// 	unsigned argc = NEXT_INDEX();

		// 	sq_value newargs[argc];

		// 	for (unsigned i = 0; i < argc; ++i)
		// 		newargs[i] = NEXT_LOCAL();

		// 	if (sq_value_is_function(imitation_value)) {
		// 		struct sq_function *fn = sq_value_as_function(imitation_value);

		// 		if (argc != fn->argc)
		// 			die("argc mismatch (given %d, expected %d) for func '%s'", argc, fn->argc, fn->name);

		// 		NEXT_LOCAL() = sq_function_run(fn, argc, newargs);
		// 	} else if (sq_value_is_form(imitation_value)) {
		// 		struct sq_form *form = sq_value_as_form(imitation_value);
		// 		NEXT_LOCAL() = create_form_imitation(
		// 			form,
		// 			argc,
		// 			memdup(newargs, sizeof(sq_value[argc]))
		// 		);
		// 	} else {
		// 		die("can only call funcs, not '%s'", sq_value_typename(imitation_value));
		// 	}

		// 	continue;
		// }
	}

	fn do_callbuiltin(&mut self) -> Result<()> {
		todo!();
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

	fn do_ternary_op(&mut self, op: fn(&Value, &Value, Value, &mut Vm) -> Result<Value>) -> Result<()> {
		let ([this, key, value], vm) = self.next_locals_and_vm();
		let result = (op)(this, key, value.clone(), vm)?;

		self.set_result(result);
		Ok(())
	}

	fn do_not(&mut self) -> Result<()> {
		self.do_unary_op(Value::try_not)
	}

	fn do_equals(&mut self) -> Result<()> {
		self.do_binary_op(Value::try_eql)
	}

	fn do_not_equals(&mut self) -> Result<()> {
		self.do_binary_op(Value::try_neq)
	}

	fn do_less_than(&mut self) -> Result<()> {
		self.do_binary_op(Value::try_lth)
	}

	fn do_less_than_or_equal(&mut self) -> Result<()> {
		self.do_binary_op(Value::try_gth)
	}

	fn do_greater_than(&mut self) -> Result<()> {
		self.do_binary_op(Value::try_gth)
	}

	fn do_greater_than_or_equal(&mut self) -> Result<()> {
		self.do_binary_op(Value::try_geq)
	}

	fn do_compare(&mut self) -> Result<()> {
		self.do_binary_op(Value::try_cmp)
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

	fn do_exponentiate(&mut self) -> Result<()> {
		self.do_binary_op(Value::try_pow)
	}

	fn do_index(&mut self) -> Result<()> {
		self.do_binary_op(Value::try_index)
	}

	fn do_index_assign(&mut self) -> Result<()> {
		self.do_ternary_op(Value::try_index_assign)
	}

	fn do_load_constant(&mut self) -> Result<()> {
		let constant = self.next_constant().clone();
		self.set_result(constant);
		Ok(())
	}

	fn do_load_global(&mut self) -> Result<()> {
		todo!();
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


	#[tracing::instrument(level="debug")]
	pub fn run(mut self) -> Result<Value> {
		while !self.is_finished() {
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
				Opcode::CallBuiltin => self.do_callbuiltin()?,
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
				Opcode::Negate => self.do_negate()?,
				Opcode::Add => self.do_add()?,
				Opcode::Subtract => self.do_subtract()?,
				Opcode::Multiply => self.do_multiply()?,
				Opcode::Divide => self.do_divide()?,
				Opcode::Modulo => self.do_modulo()?,
				Opcode::Exponentiate => self.do_exponentiate()?,

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
