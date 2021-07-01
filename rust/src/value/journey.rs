use crate::runtime::{CodeBlock, Args, Vm, Error as RuntimeError};
use crate::value::Value;
use crate::value::ops::{IsEqual, Call};
use std::hash::{Hash, Hasher};
use std::fmt::{self, Debug, Formatter};

mod arguments;
pub use arguments::Arguments;

#[derive(Clone, PartialEq, Eq)]
pub struct Journey {
	name: String,
	is_method: bool,
	args: Vec<String>,
	codeblock: CodeBlock
	// ...
}

impl Debug for Journey {
	fn fmt(&self, f: &mut Formatter<'_>) -> fmt::Result {
		if !f.alternate() {
			return f.debug_tuple("Journey").field(&self.name).finish()
		}

		f.debug_struct("Journey")
			.field("name", &self.name)
			.field("is_method", &self.is_method)
			.field("args", &self.args)
			.field("codeblock", &self.codeblock)
			.finish()
	}
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
}

impl IsEqual for Journey {
	fn is_equal(&self, rhs: &Value, vm: &mut Vm) -> Result<bool, RuntimeError> {
		let _ = (rhs, vm);
		todo!();
	}
}

impl Call for Journey {
	fn call(&self, args: Args, vm: &mut Vm) -> Result<Value, RuntimeError> {
		if args._as_slice().len() != self.args.len() {
			Err(RuntimeError::ArgumentError { given: args._as_slice().len(), expected: self.args.len() })
		} else {
			self.codeblock.run(args, vm)
		}
	}
}
