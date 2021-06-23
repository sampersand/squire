use crate::Value;

#[derive(Debug, Clone, PartialEq, Eq, Default)]
pub struct Vm {
	globals: Vec<Value>
}

// impl Vm {