use crate::runtime::{Vm, Args, Error as RuntimeError};
use crate::value::Value;

pub trait ConvertTo<T> {
	fn convert(&self, vm: &mut Vm) -> Result<T, RuntimeError>;
}

impl<T: Clone> ConvertTo<T> for T {
	fn convert(&self, _: &mut Vm) -> Result<Self, RuntimeError> {
		Ok(self.clone())
	}
}

pub trait Duplicate : Sized {
	fn duplicate(&self) -> Self;
}

pub trait Dump {
	fn dump(&self, to: &mut String, vm: &mut Vm) -> Result<(), RuntimeError>;
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

pub trait Matches {
	fn matches(&self, target: &Value, vm: &mut Vm) -> Result<bool, RuntimeError>;
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