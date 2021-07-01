use crate::runtime::{Vm, Error as RuntimeError};
use crate::value::{Value, Numeral, Text};
use crate::value::ops::{ConvertTo, IsEqual, Compare};

pub type Veracity = bool;

impl ConvertTo<Numeral> for Veracity {
	fn convert(&self, _: &mut Vm) -> Result<Numeral, RuntimeError> {
		Ok(Numeral::new(*self as i64))
	}
}

impl ConvertTo<Text> for Veracity {
	fn convert(&self, _: &mut Vm) -> Result<Text, RuntimeError> {
		Ok(Text::new(if *self { "yay" } else { "nay" }))
	}
}

impl IsEqual for Veracity {
	fn is_equal(&self, rhs: &Value, _: &mut Vm) -> Result<bool, RuntimeError> {
		if let Value::Veracity(rhs) = rhs {
			Ok(*self == *rhs)
		} else {
			Ok(false)
		}
	}
}

impl Compare for Veracity {
	fn compare(&self, rhs: &Value, vm: &mut Vm) -> Result<Option<std::cmp::Ordering>, RuntimeError> {
		Ok(self.partial_cmp(&rhs.convert_to::<Self>(vm)?))
	}
}
