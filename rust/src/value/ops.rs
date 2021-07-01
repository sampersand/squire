use crate::runtime::{Vm, Args, Error as RuntimeError};
use crate::value::{Value, Veracity, Numeral, Text, Array, Codex};

/// A trait for converting to a [`Veracity`].
pub trait ToVeracity {
	/// Converts `self` to a [`Veracity`], returning a [`RuntimeError`] if the conversion failed.
	fn to_veracity(&self, vm: &mut Vm) -> Result<Veracity, RuntimeError>;
}

/// A trait for converting to a [`Numeral`].
pub trait ToNumeral {
	/// Converts `self` to a [`Numeral`], returning a [`RuntimeError`] if the conversion failed.
	fn to_numeral(&self, vm: &mut Vm) -> Result<Numeral, RuntimeError>;
}

/// A trait for converting to a [`Text`].
pub trait ToText {
	/// Converts `self` to a [`Text`], returning a [`RuntimeError`] if the conversion failed.
	fn to_text(&self, vm: &mut Vm) -> Result<Text, RuntimeError>;
}

/// A trait for converting to an [`Array`].
pub trait ToArray {
	/// Converts `self` to an [`Array`], returning a [`RuntimeError`] if the conversion failed.
	fn to_array(&self, vm: &mut Vm) -> Result<Array, RuntimeError>;
}

/// A trait for converting to a [`Codex`].
pub trait ToCodex {
	/// Converts `self` to an [`Codex`], returning a [`RuntimeError`] if the conversion failed.
	fn to_codex(&self, vm: &mut Vm) -> Result<Codex, RuntimeError>;
}

pub trait ConvertTo<T> {
	fn convert(&self, vm: &mut Vm) -> Result<T, RuntimeError>;
}

pub trait Negate {
	fn negate(&self, vm: &mut Vm) -> Result<Value, RuntimeError>;
}

pub trait Add {
	fn add(&self, rhs: &Value, vm: &mut Vm) -> Result<Value, RuntimeError>;
}

pub trait Subtract {
	fn subtract(&self, rhs: &Value, vm: &mut Vm) -> Result<Value, RuntimeError>;
}

pub trait Multiply {
	fn multiply(&self, rhs: &Value, vm: &mut Vm) -> Result<Value, RuntimeError>;
}

pub trait Divide {
	fn divide(&self, rhs: &Value, vm: &mut Vm) -> Result<Value, RuntimeError>;
}

pub trait Modulo {
	fn modulo(&self, rhs: &Value, vm: &mut Vm) -> Result<Value, RuntimeError>;
}

pub trait Power {
	fn power(&self, rhs: &Value, vm: &mut Vm) -> Result<Value, RuntimeError>;
}

pub trait IsEqual {
	fn is_equal(&self, rhs: &Value, vm: &mut Vm) -> Result<bool, RuntimeError>;
}

pub trait Compare {
	fn compare(&self, rhs: &Value, vm: &mut Vm) -> Result<Option<std::cmp::Ordering>, RuntimeError>;
}

pub trait Call {
	fn call(&self, args: Args, vm: &mut Vm) -> Result<Value, RuntimeError>;
}

pub trait GetAttr {
	fn get_attr(&self, attr: &str, vm: &mut Vm) -> Result<Value, RuntimeError>;
}

pub trait SetAttr {
	fn set_attr(&self, attr: &str, value: Value, vm: &mut Vm) -> Result<(), RuntimeError>;
}

pub trait GetIndex {
	fn get_index(&self, index: &Value, vm: &mut Vm) -> Result<Value, RuntimeError>;
}

pub trait SetIndex {
	fn set_index(&self, index: Value, value: Value, vm: &mut Vm) -> Result<(), RuntimeError>;
}