use std::sync::Arc;
use crate::{Journey, Form, Imitation};
use crate::runtime::{Vm, Result};
use std::fmt::{self, Display, Formatter};

mod array;
mod codex;
pub mod numeral;
pub mod text;

pub use text::Text;
pub use array::Array;
pub use codex::Codex;
pub use numeral::Numeral;

#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub enum Value {
	Null,
	Veracity(bool),
	Numeral(Numeral),
	Text(Text),
	Journey(Journey),
	Form(Arc<Form>),
	Imitation(Arc<Imitation>)
}

impl Default for Value {
	fn default() -> Self {
		Self::Null
	}
}

impl PartialOrd for Value {
	fn partial_cmp(&self, _rhs: &Self) -> Option<std::cmp::Ordering> {
		todo!()
	}
}

impl Display for Value {
	fn fmt(&self, f: &mut Formatter<'_>) -> fmt::Result {
		let _ = f;
		todo!("display (note that `{{:#}}` should be `repr()`)")
	}
}

impl From<bool> for Value {
	#[inline]
	fn from(boolean: bool) -> Self {
		Self::Veracity(boolean)
	}
}

impl Value {
	pub fn to_boolean(&self, vm: &mut Vm) -> Result<bool> {
		let _ = vm; todo!()
	}

	pub fn to_text(&self, vm: &mut Vm) -> Result<Text> {
		let _ = vm; todo!();
	}

	pub fn to_number(&self, vm: &mut Vm) -> Result<Numeral> {
		let _ = vm; todo!();
	}

	pub fn try_neg(&self, vm: &mut Vm) -> Result<Value> { let _ = vm; todo!(); }
	pub fn try_add(&self, rhs: &Value, vm: &mut Vm) -> Result<Value> { let _ = (rhs, vm); todo!(); }
	pub fn try_sub(&self, rhs: &Value, vm: &mut Vm) -> Result<Value> { let _ = (rhs, vm); todo!(); }
	pub fn try_mul(&self, rhs: &Value, vm: &mut Vm) -> Result<Value> { let _ = (rhs, vm); todo!(); }
	pub fn try_div(&self, rhs: &Value, vm: &mut Vm) -> Result<Value> { let _ = (rhs, vm); todo!(); }
	pub fn try_rem(&self, rhs: &Value, vm: &mut Vm) -> Result<Value> { let _ = (rhs, vm); todo!(); }
	pub fn try_pow(&self, rhs: &Value, vm: &mut Vm) -> Result<Value> { let _ = (rhs, vm); todo!(); }

	pub fn try_not(&self, vm: &mut Vm) -> Result<Value> { let _ = vm; todo!(); }
	pub fn try_eql(&self, rhs: &Value, vm: &mut Vm) -> Result<Value> { let _ = (rhs, vm); todo!(); }
	pub fn try_neq(&self, rhs: &Value, vm: &mut Vm) -> Result<Value> { let _ = (rhs, vm); todo!(); }
	pub fn try_lth(&self, rhs: &Value, vm: &mut Vm) -> Result<Value> { let _ = (rhs, vm); todo!(); }
	pub fn try_leq(&self, rhs: &Value, vm: &mut Vm) -> Result<Value> { let _ = (rhs, vm); todo!(); }
	pub fn try_gth(&self, rhs: &Value, vm: &mut Vm) -> Result<Value> { let _ = (rhs, vm); todo!(); }
	pub fn try_geq(&self, rhs: &Value, vm: &mut Vm) -> Result<Value> { let _ = (rhs, vm); todo!(); }
	pub fn try_cmp(&self, rhs: &Value, vm: &mut Vm) -> Result<Value> { let _ = (rhs, vm); todo!(); }

	pub fn try_index(&self, by: &Value, vm: &mut Vm) -> Result<Value> { let _ = (by, vm); todo!(); }
	pub fn try_index_assign(&self, by: &Value, with: Value, vm: &mut Vm) -> Result<Value> { let _ = (by, with, vm); todo!(); }
}