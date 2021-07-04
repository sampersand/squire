use crate::runtime::{Vm, Error as RuntimeError, Args, CodeBlock};
use crate::value::Value;
use crate::value::ops::{Dump, IsEqual, Call, GetAttr};
use std::hash::{Hash, Hasher};
use std::sync::Arc;
use std::fmt::{self, Debug, Formatter};
use super::Journey;

#[derive(Clone)]
pub struct UserDefined(Arc<UserDefinedInner>);

struct UserDefinedInner {
	name: String,
	is_method: bool,
	args: Vec<String>,
	codeblock: CodeBlock
	// ...?
}

impl Debug for UserDefined {
	fn fmt(&self, f: &mut Formatter<'_>) -> fmt::Result {
		if !f.alternate() {
			return f.debug_tuple("UserDefined").field(&self.name()).finish()
		}

		f.debug_struct("UserDefined")
			.field("name", &self.name())
			.field("is_method", &self.0.is_method)
			.field("args", &self.0.args)
			.field("codeblock", &self.0.codeblock)
			.finish()
	}
}

impl Eq for UserDefined {}
impl PartialEq for UserDefined {
	fn eq(&self, rhs: &Self) -> bool {
		Arc::ptr_eq(&self.0, &rhs.0)
	}
}

impl Hash for UserDefined {
	fn hash<H: Hasher>(&self, hasher: &mut H) {
		self.name().hash(hasher)
	}
}

impl UserDefined {
	pub fn new(name: String, is_method: bool, args: Vec<String>, codeblock: CodeBlock) -> Self {
		Self(Arc::new(UserDefinedInner { name, is_method, args, codeblock }))
	}

	pub fn name(&self) -> &str {
		&self.0.name
	}
}

impl From<UserDefined> for Value {
	#[inline]
	fn from(user_defined: UserDefined) -> Self {
		Self::Journey(user_defined.into())
	}
}

impl Dump for UserDefined {
	fn dump(&self, to: &mut String, _: &mut Vm) -> Result<(), RuntimeError> {
		to.push_str(&format!("UserDefined({}: {:p})", self.name(), Arc::as_ptr(&self.0)));

		Ok(())
	}
}

impl IsEqual for UserDefined {
	fn is_equal(&self, rhs: &Value, _: &mut Vm) -> Result<bool, RuntimeError> {
		if let Value::Journey(Journey::UserDefined(rhs)) = rhs {
			Ok(self == rhs)
		} else {
			Ok(false)
		}
	}
}

impl Call for UserDefined {
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

impl GetAttr for UserDefined {
	fn get_attr(&self, attr: &str, _: &mut Vm) -> Result<Value, RuntimeError> {
		match attr {
			"name" => Ok(self.0.name.clone().into()),
			"args" => Ok(self.0.args.iter().cloned().map(Value::from).collect::<Vec<_>>().into()),
			// "is_method" (?)
			_ => Err(RuntimeError::UnknownAttribute(attr.to_string()))
		}
	}
}
