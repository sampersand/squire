use std::fmt::{self, Display, Formatter};
use super::GetAttr;
use crate::runtime::{Value, Vm, Error as RuntimeError};

#[derive(Debug, Clone, PartialEq, Eq, PartialOrd, Ord, Hash, Default)]
pub struct Null;

impl Display for Null {
	fn fmt(&self, f: &mut Formatter<'_>) -> fmt::Result {
		Display::fmt(&"null", f)
	}
}


impl GetAttr for Null {
	fn get_attr(&self, attr: &str, _: &mut Vm) -> Result<Value, RuntimeError> {
		let _ = attr;
		todo!();
	}
}
