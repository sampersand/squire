use std::collections::HashMap;
use crate::Value;
// use std::cmp::Ordering;
use std::ops::Index;
use std::hash::{Hash, /*s*/};
use std::borrow::Borrow;

#[derive(Debug, Clone, PartialEq, Eq, Default)]
pub struct Codex(HashMap<Value, Value>);

impl Codex {
	pub fn new() -> Self {
		Self(HashMap::new())
	}

	pub fn with_capacity(capacity: usize) -> Self {
		Self(HashMap::with_capacity(capacity))
	}

	pub fn get(&self, key: &Value) -> Option<&Value> {
		self.0.get(key)
	}

	pub fn contains_key(&self, key: &Value) -> bool {
		self.0.contains_key(key)
	}

	pub fn get_mut(&mut self, key: &Value) -> Option<&mut Value> {
		self.0.get_mut(key)
	}

	pub fn insert(&mut self, key: Value, value: Value) -> Option<Value> {
		self.0.insert(key, value)
	}

	pub fn remove(&mut self, key: &Value) -> Option<Value> {
		self.0.remove(key)
	}

	pub fn len(&self) -> usize {
		self.0.len()
	}

	pub fn is_empty(&self) -> bool {
		self.0.is_empty()
	}
}

impl From<HashMap<Value, Value>> for Codex {
	#[inline]
	fn from(hashmap: HashMap<Value, Value>) -> Self {
		Self(hashmap)
	}
}
impl IntoIterator for Codex {
	type Item = (Value, Value);
	type IntoIter = <HashMap<Value, Value> as IntoIterator>::IntoIter;

	fn into_iter(self) -> Self::IntoIter {
		self.0.into_iter()	
	}
}

impl std::iter::FromIterator<(Value, Value)> for Codex {
	fn from_iter<I: IntoIterator<Item=(Value, Value)>>(iter: I) -> Self {
		Self(iter.into_iter().collect())
	}
}

impl std::iter::Extend<(Value, Value)> for Codex {
	fn extend<I: IntoIterator<Item=(Value, Value)>>(&mut self, iter: I) {
		self.0.extend(iter)
	}
}

impl<I> Index<&I> for Codex
where
	I: Eq + Hash,
	Value: Borrow<I>
{
	type Output = Value;

	#[inline]
	fn index(&self, index: &I) -> &Value {
		&self.0[index]
	}
}

impl Codex {
	pub fn merge<I: IntoIterator<Item=(Value, Value)>>(&mut self, rhs: I) {
		self.0.extend(rhs)
	}
}

// impl PartialOrd for Codex {
// 	fn partial_cmp(&self, rhs: &Self) -> Option<Ordering> {
// 		if (self as *const _) == (rhs as *const _) {
// 			return Some(Ordering::Equal);
// 		}

// 		let mut self_contains_keys_rhs_doesnt = false;

// 		// if at least one key in `self` doesn't exist in `rhs`, we're either
// 		// greater than `rhs`, or we cannot compare (as they have different elements.)
// 		for key in self.0.keys() {
// 			if !rhs.contains_key(key) {
// 				self_contains_keys_rhs_doesnt = true;
// 			}
// 		}

// 		for key in rhs.0.keys {
// 			if !self.contains_key(key) {
// 				if self_contains_keys_rhs_doesnt {
// 					return None; // Each has at least one key the other doesnt have.
// 				} else {
// 					return Some(Ordering::Less) // rhs has a key we do not
// 				}
// 			}
// 		}

// 		if self_contains_keys_rhs_doesnt {
// 			Some(Ordering::Greater) // we have a key rhs doesnt
// 		} else {
// 			Some(Ordering::Equal) // we both have the same keys and values
// 		}
// 		if self.0.keys() == rhs.0.keys() {
// 			return Ordering
// 		}
// 		Some(self.cmp(rhs))
// 	}
// }

