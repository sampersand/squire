use crate::vm::{Args, Vm, Error as RuntimeError};
use crate::value::Value;
use crate::value::ops::{Dump, Matches, IsEqual, Call, GetAttr};
use std::hash::{Hash, Hasher};

mod arguments;
pub mod builtin;
mod bound;
mod user_defined;
pub use arguments::Arguments;
pub use builtin::Builtin;
pub use bound::Bound;
pub use user_defined::UserDefined;

#[derive(Debug, Clone)]
pub enum Journey {
	Builtin(Builtin),
	Bound(Bound),
	UserDefined(UserDefined)
}

impl Eq for Journey {}
impl PartialEq for Journey {
	fn eq(&self, rhs: &Self) -> bool {
		match (self, rhs) {
			(Self::Builtin(lhs), Self::Builtin(rhs)) => lhs == rhs,
			(Self::Bound(lhs), Self::Bound(rhs)) => lhs == rhs,
			(Self::UserDefined(lhs), Self::UserDefined(rhs)) => lhs == rhs,
			_ => false
		}
	}
}

impl Hash for Journey {
	fn hash<H: Hasher>(&self, h: &mut H) {
		const BUILTIN_TAG: u8 = 0;
		const BOUND_TAG: u8 = 1;
		const USER_DEFINED_TAG: u8 = 2;

		match self {
			Self::Builtin(builtin) => {
				h.write_u8(BUILTIN_TAG);
				builtin.hash(h);
			},

			Self::Bound(bound) => {
				h.write_u8(BOUND_TAG);
				bound.hash(h);
			},

			Self::UserDefined(user_defined) => {
				h.write_u8(USER_DEFINED_TAG);
				user_defined.hash(h);
			},
		}
	}
}
// impl Journey {
// 	pub fn name(&self) -> &str {
// 		match self {
// 			Self::Builtin(builtin) => builtin.name(),
// 			Self::Bound(bound) => bound.name(),
// 			Self::UserDefined(user_defined) => user_defined.name(),
// 		}
// 	}
// }

impl From<Builtin> for Journey {
	#[inline]
	fn from(builtin: Builtin) -> Self {
		Self::Builtin(builtin)
	}
}

impl From<Bound> for Journey {
	#[inline]
	fn from(bound: Bound) -> Self {
		Self::Bound(bound)
	}
}

impl From<UserDefined> for Journey {
	#[inline]
	fn from(user_defined: UserDefined) -> Self {
		Self::UserDefined(user_defined)
	}
}

impl From<Journey> for Value {
	#[inline]
	fn from(journey: Journey) -> Self {
		Self::Journey(journey)
	}
}

impl Dump for Journey {
	fn dump(&self, to: &mut String, vm: &mut Vm) -> Result<(), RuntimeError> {
		match self{  
			Self::Builtin(builtin) => builtin.dump(to, vm),
			Self::Bound(bound) => bound.dump(to, vm),
			Self::UserDefined(user_defined) => user_defined.dump(to, vm),
		}
	}
}

impl Matches for Journey {
	fn matches(&self, target: &Value, vm: &mut Vm) -> Result<bool, RuntimeError> {
		if self.is_equal(target, vm)? {
			Ok(true)
		} else {
			self.call(Args::new(&[target.clone()]), vm)?.convert_to::<bool>(vm)
		}
	}
}

impl IsEqual for Journey {
	fn is_equal(&self, rhs: &Value, vm: &mut Vm) -> Result<bool, RuntimeError> {
		match self {
			Self::Builtin(builtin) => builtin.is_equal(rhs, vm),
			Self::Bound(bound) => bound.is_equal(rhs, vm),
			Self::UserDefined(user_defined) => user_defined.is_equal(rhs, vm),
		}
	}
}

impl Call for Journey {
	fn call(&self, args: Args, vm: &mut Vm) -> Result<Value, RuntimeError> {
		match self {
			Self::Builtin(builtin) => builtin.call(args, vm),
			Self::Bound(bound) => bound.call(args, vm),
			Self::UserDefined(user_defined) => user_defined.call(args, vm)
		}
	}
}

impl GetAttr for Journey {
	fn get_attr(&self, attr: &str, vm: &mut Vm) -> Result<Value, RuntimeError> {
		match self {
			Self::Builtin(builtin) => builtin.get_attr(attr, vm),
			Self::Bound(bound) => bound.get_attr(attr, vm),
			Self::UserDefined(user_defined) => user_defined.get_attr(attr, vm)
		}
	}
}
