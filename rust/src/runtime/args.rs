use crate::value::Value;
use std::collections::HashMap;
use crate::runtime::Error as RuntimeError;

#[derive(Debug, Clone, PartialEq, Eq, Default)]
pub struct Args {
	positional: Vec<Value>,
	keyword: HashMap<String, Value>
}

impl Args {
	pub fn new(positional: &[Value]) -> Self {
		Self::with_kwargs(positional.iter().cloned().collect(), Default::default())
	}

	pub fn with_kwargs(positional: Vec<Value>, keyword: HashMap<String, Value>) -> Self {
		Self { positional, keyword }
	}

	pub fn _as_slice(&self) -> &[Value] {
		&self.positional
	}

	pub fn add_soul(&mut self, soul: Value) {
		self.positional.insert(0, soul);
	}

	pub fn guard_required_positional(&self, expected: usize) -> Result<(), RuntimeError> {
		let positional_length = self._as_slice().len();
		if expected == positional_length {
			Ok(())
		} else {
			Err(RuntimeError::ArgumentCountError { given: positional_length, expected })
		}
	}

	pub fn positional(&self) -> &[Value] {
		&self.positional
	}

	pub fn kwarg(&self, name: &str) -> Option<&Value> {
		self.keyword.get(name)
	}

	// pub fn positional(&self, index: usize) -> Option<&Value> {
	// 	self.positional.get(index)
	// }
}


impl Args {
	pub fn len(&self) -> usize {
		self.positional.len()
	}
}

impl std::ops::Index<usize> for Args {
	type Output = Value;

	fn index(&self, pos: usize) -> &Self::Output {
		&self.positional[pos]
	}
}