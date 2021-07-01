use std::fmt::{self, Display, Formatter};
use crate::runtime::{Vm, Error as RuntimeError};
use crate::value::{Value, Veracity, Numeral, Text, Array, Codex};
use crate::value::ops::{ConvertTo, IsEqual, GetAttr};

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

impl ConvertTo<Veracity> for Null {
	fn convert(&self, _: &mut Vm) -> Result<Veracity, RuntimeError> {
		Ok(Veracity::default())
	}
}

impl ConvertTo<Numeral> for Null {
	fn convert(&self, _: &mut Vm) -> Result<Numeral, RuntimeError> {
		Ok(Numeral::default())
	}
}

impl ConvertTo<Text> for Null {
	fn convert(&self, _: &mut Vm) -> Result<Text, RuntimeError> {
		Ok(Text::default())
	}
}

impl ConvertTo<Array> for Null {
	fn convert(&self, _: &mut Vm) -> Result<Array, RuntimeError> {
		Ok(Array::default())
	}
}

impl ConvertTo<Codex> for Null {
	fn convert(&self, _: &mut Vm) -> Result<Codex, RuntimeError> {
		Ok(Codex::default())
	}
}

impl IsEqual for Null {
	fn is_equal(&self, _: &Value, _: &mut Vm) -> Result<bool, RuntimeError> {
		Ok(true)
	}
}

