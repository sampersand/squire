use crate::runtime::{CodeBlock, Args, Vm, Error as RuntimeError};
use crate::value::Value;
use crate::value::ops::{Dump, IsEqual, Call, GetAttr};
use std::hash::{Hash, Hasher};
use std::fmt::{self, Debug, Formatter};
use std::sync::Arc;

mod arguments;
pub use arguments::Arguments;

#[derive(Clone)]
pub struct Journey(Arc<JourneyInner>);

struct JourneyInner {
	name: String,
	is_method: bool,
	args: Vec<String>,
	codeblock: CodeBlock
	// ...?
}

impl Debug for Journey {
	fn fmt(&self, f: &mut Formatter<'_>) -> fmt::Result {
		if !f.alternate() {
			return f.debug_tuple("Journey").field(&self.name()).finish()
		}

		f.debug_struct("Journey")
			.field("name", &self.name())
			.field("is_method", &self.0.is_method)
			.field("args", &self.0.args)
			.field("codeblock", &self.0.codeblock)
			.finish()
	}
}

impl Eq for Journey {}
impl PartialEq for Journey {
	fn eq(&self, rhs: &Self) -> bool {
		Arc::ptr_eq(&self.0, &rhs.0)
	}
}

impl Hash for Journey {
	fn hash<H: Hasher>(&self, hasher: &mut H) {
		self.name().hash(hasher)
	}
}

impl Journey {
	pub fn new(name: String, is_method: bool, args: Vec<String>, codeblock: CodeBlock) -> Self {
		Self(Arc::new(JourneyInner { name, is_method, args, codeblock }))
	}

	pub fn name(&self) -> &str {
		&self.0.name
	}
}

impl From<Journey> for Value {
	#[inline]
	fn from(journey: Journey) -> Self {
		Self::Journey(journey)
	}
}

impl Dump for Journey {
	fn dump(&self, to: &mut String, _: &mut Vm) -> Result<(), RuntimeError> {
		to.push_str(&format!("Journey({}: {:p})", self.name(), Arc::as_ptr(&self.0)));

		Ok(())
	}
}

impl IsEqual for Journey {
	fn is_equal(&self, rhs: &Value, _: &mut Vm) -> Result<bool, RuntimeError> {
		if let Value::Journey(rhs) = rhs {
			Ok(self == rhs)
		} else {
			Ok(false)
		}
	}
}

impl Call for Journey {
	fn call(&self, args: Args, vm: &mut Vm) -> Result<Value, RuntimeError> {
		if args._as_slice().len() == self.0.args.len() {
			return self.0.codeblock.run(args, vm)
		}

		Err(RuntimeError::ArgumentCountError {
			given: args._as_slice().len(),
			expected: self.0.args.len()
		})
	}
}

impl GetAttr for Journey {
	fn get_attr(&self, attr: &str, _: &mut Vm) -> Result<Value, RuntimeError> {
		match attr {
			"name" => Ok(self.0.name.clone().into()),
			"args" => Ok(self.0.args.iter().cloned().map(Value::from).collect::<Vec<_>>().into()),
			// "is_method" (?)
			_ => Err(RuntimeError::UnknownAttribute(attr.to_string()))
		}
	}
}
