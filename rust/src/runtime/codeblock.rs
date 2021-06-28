use crate::runtime::{Bytecode, Vm, Args, Result};
use crate::value::Value;

mod stackframe;
use stackframe::StackFrame;

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct CodeBlock {
	num_locals: usize,
	code: Vec<Bytecode>,
	constants: Vec<Value>,
}

impl CodeBlock {
	pub fn new(num_locals: usize, code: Vec<Bytecode>, constants: Vec<Value>) -> Self {
		Self { num_locals, code, constants }
	}

	pub fn num_locals(&self) -> usize {
		self.num_locals
	}

	pub fn code(&self) -> &[Bytecode] {
		&self.code
	}

	pub fn constants(&self) -> &[Value] {
		&self.constants
	}

	pub fn run(&self, args: Args, vm: &mut Vm) -> Result<Value> {

		assert!(self.num_locals >= args._as_slice().len(), "not enough locals to store arguments!");

		let mut locals = Vec::with_capacity(self.num_locals);
		locals.extend(args._as_slice().iter().cloned());

		while locals.len() < self.num_locals {
			locals.push(Value::Null);
		}

		StackFrame::new(self, &mut locals, vm).run()
	}
}
