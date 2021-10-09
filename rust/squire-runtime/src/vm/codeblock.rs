use crate::vm::{Bytecode, Vm, Args, Result};
use crate::value::Value;
use std::collections::HashMap;

mod stackframe;
use stackframe::StackFrame;

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct CodeBlock {
	num_locals: usize,
	code: Vec<Bytecode>,
	constants: Vec<Value>,
	whences: HashMap<usize, Vec<usize>>
}

impl CodeBlock {
	pub fn new(num_locals: usize, code: Vec<Bytecode>, constants: Vec<Value>, whences: HashMap<usize, Vec<usize>>) -> Self {
		Self { num_locals, code, constants, whences }
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

	pub fn whences_for(&self, index: usize) -> Option<&[usize]> {
		self.whences.get(&index).map(Vec::as_ref)
	}

	pub fn run(&self, args: Args, vm: &mut Vm) -> Result<Value> {
		assert!(self.num_locals >= args._as_slice().len(), "not enough locals to store arguments!");

		let mut locals = Vec::with_capacity(self.num_locals + 1);
		locals.push(Value::Ni); // this is for the "scratch" target
		locals.extend(args._as_slice().iter().cloned());

		while locals.len() < self.num_locals {
			locals.push(Value::Ni);
		}

		StackFrame::new(self, &mut locals, vm).run()
	}
}
