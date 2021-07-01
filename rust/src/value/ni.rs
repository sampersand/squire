use std::fmt::{self, Display, Formatter};
use crate::runtime::{Vm, Error as RuntimeError};
use crate::value::{Value, Veracity, Numeral, Text, Book, Codex};
use crate::value::ops::{ConvertTo, Dump, IsEqual, GetAttr};

#[derive(Debug, Clone, PartialEq, Eq, PartialOrd, Ord, Hash, Default)]
pub struct Ni;

impl Display for Ni {
	fn fmt(&self, f: &mut Formatter<'_>) -> fmt::Result {
		Display::fmt(&"ni", f)
	}
}

impl Dump for Ni {
	fn dump(&self, to: &mut String, _: &mut Vm) -> Result<(), RuntimeError> {
		to.push_str(&self.to_string());

		Ok(())
	}
}


impl GetAttr for Ni {
	fn get_attr(&self, attr: &str, _: &mut Vm) -> Result<Value, RuntimeError> {
		let _ = attr;
		todo!();
	}
}

impl ConvertTo<Veracity> for Ni {
	fn convert(&self, _: &mut Vm) -> Result<Veracity, RuntimeError> {
		Ok(Veracity::default())
	}
}

impl ConvertTo<Numeral> for Ni {
	fn convert(&self, _: &mut Vm) -> Result<Numeral, RuntimeError> {
		Ok(Numeral::default())
	}
}

impl ConvertTo<Text> for Ni {
	fn convert(&self, _: &mut Vm) -> Result<Text, RuntimeError> {
		Ok(Text::default())
	}
}

impl ConvertTo<Book> for Ni {
	fn convert(&self, _: &mut Vm) -> Result<Book, RuntimeError> {
		Ok(Book::default())
	}
}

impl ConvertTo<Codex> for Ni {
	fn convert(&self, _: &mut Vm) -> Result<Codex, RuntimeError> {
		Ok(Codex::default())
	}
}

impl IsEqual for Ni {
	fn is_equal(&self, _: &Value, _: &mut Vm) -> Result<bool, RuntimeError> {
		Ok(true)
	}
}

