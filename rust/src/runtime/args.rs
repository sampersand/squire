use crate::value::Value;
use std::collections::HashMap;

#[derive(Debug, Clone, PartialEq, Eq, Default)]
pub struct Args {
	positional: Vec<Value>,
	keyword: HashMap<String, Value>
}

impl Args {
	pub fn new(positional: Vec<Value>, keyword: HashMap<String, Value>) -> Self {
		Self { positional, keyword }
	}

	pub fn _as_slice(&self) -> &[Value] {
		&self.positional
	}

	pub fn add_me(&mut self, me: Value) {
		self.positional.insert(0, me);
	}

	pub fn positional(&self, index: usize) -> Option<&Value> {
		self.positional.get(index)
	}
}


impl Args {
	pub fn len(&self) -> usize {
		self.positional.len()
	}
}