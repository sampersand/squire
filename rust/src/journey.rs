use crate::Value;
use std::hash::{Hash, Hasher};

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Journey {
	name: String,
	nlocals: usize,
	consts: Vec<Value>,
	is_method: bool,
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

// struct sq_function {
// 	char *name;
// 	int refcount; // negative indicates a global function.

// 	unsigned argc, nlocals, nconsts, codelen;
// 	sq_value *consts;
// 	struct sq_program *program;
// 	union sq_bytecode *bytecode;
// 	bool is_method;
// };

