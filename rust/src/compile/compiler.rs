// use super::Result;
use std::collections::HashMap;
use crate::Value;
use std::rc::Rc;
use std::cell::RefCell;
use std::num::NonZeroUsize;

use crate::runtime::{Bytecode, Opcode, Interrupt};

#[derive(Debug)]
struct Label {}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub struct Target(NonZeroUsize);

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub struct Constant(usize);

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub struct Global(usize);

type Globals = Rc<RefCell<HashMap<String, Global>>>;

#[derive(Debug, Default)]
pub struct Compiler {
	globals: Globals,
	code: Vec<Bytecode>,
	ntargets: usize,
	labels: Vec<Label>,
	locals: HashMap<String, Target>,
	constants: Vec<Value>
}

impl Compiler {
	pub fn with_globals(globals: Globals) -> Self {
		Self { globals, ..Self::default() }
	}

	fn bytecode(&mut self, bytecode: Bytecode) {
		self.code.push(bytecode);
	}

	pub fn opcode(&mut self, opcode: Opcode) {
		self.bytecode(Bytecode::Opcode(opcode));
	}

	pub fn interrupt(&mut self, interrupt: Interrupt) {
		self.bytecode(Bytecode::Interrupt(interrupt));
	}

	pub fn target(&mut self, target: Target) {
		debug_assert!(target.0.get() <= self.ntargets);

		self.bytecode(Bytecode::Local(target.0.get()));
	}

	pub fn constant(&mut self, constant: Constant) {
		debug_assert!(constant.0 < self.constants.len());

		self.bytecode(Bytecode::Constant(constant.0));
	}

	pub fn global(&mut self, global: Global) {
		debug_assert!(global.0 < self.globals.borrow().len());

		self.bytecode(Bytecode::Global(global.0));
	}

	pub fn get_constant(&mut self, constant: Value) -> Constant {
		if let Some(index) = self.constants.iter().position(|x| *x == constant) {
			Constant(index)
		} else {
			self.constants.push(constant);
			Constant(self.constants.len() - 1)
		}
	}

	pub fn get_local(&mut self, name: &str) -> Option<Target> {
		self.locals.get(name).cloned()
	}

	pub fn get_global(&mut self, name: &str) -> Option<Global> {
		self.globals.borrow().get(name).cloned()
	}

	pub fn next_target(&mut self) -> Target {
		self.ntargets += 1;
		Target(NonZeroUsize::new(self.ntargets).unwrap())
	}
}
