use crate::runtime::{Value, CodeBlock, Args, Vm, Error as RuntimeError};
use std::hash::{Hash, Hasher};

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Journey {
	name: String,
	is_method: bool,
	args: Vec<String>,
	codeblock: CodeBlock
	// ...
}

impl PartialEq<str> for Journey {
	fn eq(&self, rhs: &str) -> bool {
		self.name == rhs
	}
}

impl std::borrow::Borrow<str> for Journey {
	fn borrow(&self) -> &str {
		&self.name
	}
}

impl Hash for Journey {
	fn hash<H: Hasher>(&self, hasher: &mut H) {
		self.name.hash(hasher)
	}
}

impl Journey {
	pub fn new(name: String, is_method: bool, args: Vec<String>, codeblock: CodeBlock) -> Self {
		Self { name, is_method, args, codeblock }
	}

	pub fn name(&self) -> &str {
		&self.name
	}

	pub fn call(&self, args: Args, vm: &mut Vm) -> Result<Value, RuntimeError> {
		if args._as_slice().len() != self.args.len() {
			Err(RuntimeError::ArgumentError { given: args._as_slice().len(), expected: self.args.len() })
		} else {
			self.codeblock.run(args, vm)
		}
	}
}

// struct sq_function {
// 	char *name;
// 	int refcount; // negative indicates a global function.

// 	unsigned argc, nlocals, nconsts, codelen;
// 	sq_value *consts;
// 	struct sq_program *program;
// 	union sq_bytecode *bytecode;
// 	bool is_method;
// };

