use super::{Error, Compilable};
use std::collections::HashMap;
use crate::Value;
use std::rc::Rc;
use std::cell::RefCell;

use crate::runtime::{Bytecode, Opcode, Interrupt, CodeBlock};
use crate::parse::{Parsable, Parser};

#[derive(Debug)]
struct Label {}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub struct Target(usize);

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub struct Constant(usize);

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub struct Global(usize);

type Globals = Rc<RefCell<HashMap<String, Global>>>;

#[derive(Debug)]
pub struct Compiler {
	globals: Globals,
	code: Vec<Bytecode>,
	ntargets: usize,
	labels: Vec<Label>,
	locals: HashMap<String, Target>,
	constants: Vec<Value>
}

impl Default for Compiler {
	fn default() -> Self {
		let mut globals = HashMap::new();

		for default in crate::value::builtin::defaults() {
			globals.insert(default.name().to_string(), Global(globals.len()));
		}

		Self {
			globals: Rc::new(RefCell::new(globals)),
			code: Default::default(),
			ntargets: Default::default(),
			labels: Default::default(),
			locals: Default::default(),
			constants: Default::default(),
		}
	}
}

impl Compiler {
	pub fn with_globals(globals: Globals) -> Self {
		Self { globals, ..Self::default() }
	}

	pub fn compile_with<I: Iterator<Item=char>>(&mut self, parser: &mut Parser<I>) -> Result<(), Error> {
		while let Some(statement) = crate::ast::Statement::parse(parser)? {
			statement.compile(self, None)?;
		}

		Ok(())
	}

	pub fn finish(self) -> CodeBlock {
		CodeBlock::new(self.ntargets, self.code, self.constants)
	}

	fn bytecode(&mut self, bytecode: Bytecode) {
		self.code.push(bytecode);
	}

	pub fn opcode(&mut self, opcode: Opcode) {
		self.bytecode(Bytecode::Opcode(opcode));
	}

	pub fn count(&mut self, amount: usize) {
		self.bytecode(Bytecode::Count(amount));
	}

	pub fn interrupt(&mut self, interrupt: Interrupt) {
		self.bytecode(Bytecode::Interrupt(interrupt));
	}

	pub fn target(&mut self, target: Target) {
		debug_assert!(target.0 <= self.ntargets);

		self.bytecode(Bytecode::Local(target.0));
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

	pub fn define_local(&mut self, name: String) -> Target {
		if let Some(&target) = self.locals.get(&name) {
			target
		} else {
			let target = self.next_target();
			self.locals.insert(name, target);
			target
		}
	}

	pub fn get_global(&mut self, name: &str) -> Option<Global> {
		self.globals.borrow().get(name).cloned()
	}

	pub fn next_target(&mut self) -> Target {
		self.ntargets += 1;
		Target(self.ntargets - 1)
	}
}
