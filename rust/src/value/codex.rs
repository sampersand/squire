use std::collections::HashMap;
use std::fmt::{self, Debug, Formatter};
use std::sync::Arc;
use parking_lot::RwLock;
use crate::runtime::{Vm, Error as RuntimeError};
use crate::value::{Value, Veracity, Text, Book};
use crate::value::ops::{
	ConvertTo, Dump,
	Add, Subtract,
	Matches, IsEqual, Compare,
	GetIndex, SetIndex, GetAttr
};

use std::cmp::Ordering;
use std::hash::{Hash, Hasher};

#[derive(Clone, Default)]
pub struct Codex(Arc<RwLock<HashMap<Value, Value>>>);


impl Debug for Codex {
	fn fmt(&self, f: &mut Formatter<'_>) -> fmt::Result {
		// todo: debug to ensure we don't have infinite recursion		
		f.debug_map()
			.entries(self.0.read().iter())
			.finish()
	}
}

impl Eq for Codex {}
impl PartialEq for Codex {
	fn eq(&self, rhs: &Self) -> bool {
		Arc::ptr_eq(&self.0, &rhs.0) || *self.0.read() == *rhs.0.read()
	}
}

impl Codex {
	pub fn new() -> Self {
		Self(Arc::new(RwLock::new(HashMap::new())))
	}

	pub fn with_capacity(capacity: usize) -> Self {
		Self(Arc::new(RwLock::new(HashMap::with_capacity(capacity))))
	}

	pub fn get(&self, key: &Value) -> Option<Value> {
		self.0.read().get(key).cloned()
	}

	pub fn contains_key(&self, key: &Value) -> bool {
		self.0.read().contains_key(key)
	}

	pub fn insert(&self, key: Value, value: Value) -> Option<Value> {
		self.0.write().insert(key, value)
	}

	pub fn remove(&self, key: &Value) -> Option<Value> {
		self.0.write().remove(key)
	}

	pub fn len(&self) -> usize {
		self.0.read().len()
	}

	pub fn is_empty(&self) -> bool {
		self.0.read().is_empty()
	}
}

impl From<HashMap<Value, Value>> for Codex {
	#[inline]
	fn from(hashmap: HashMap<Value, Value>) -> Self {
		Self(Arc::new(RwLock::new(hashmap)))
	}
}

impl std::iter::FromIterator<(Value, Value)> for Codex {
	fn from_iter<I: IntoIterator<Item=(Value, Value)>>(iter: I) -> Self {
		Self(Arc::new(RwLock::new(iter.into_iter().collect())))
	}
}

// impl std::iter::Extend<(Value, Value)> for Codex {
// 	fn extend<I: IntoIterator<Item=(Value, Value)>>(&mut self, iter: I) {
// 		self.0.extend(iter)
// 	}
// }

// impl<I: IntoIterator<Item=(Value, Value)>> std::ops::Add<I> for Codex {
// 	type Output = Self;

// 	fn add(mut self, rhs: I) -> Self::Output {
// 		self.0.extend(rhs);
// 		self
// 	}
// }

// impl<'a, I: IntoIterator<Item=&'a Value>> std::ops::Sub<I> for Codex {
// 	type Output = Self;

// 	fn sub(mut self, rhs: I) -> Self::Output {
// 		// todo: optimize
// 		let rhs = rhs.into_iter().collect::<Vec<_>>();
// 		self.0.retain(|value, _| !rhs.contains(&value));
// 		self
// 	}
// }

impl Hash for Codex {
	fn hash<H: Hasher>(&self, h: &mut H) {
		0.hash(h); // todo: actually hash.
	}
}

impl From<Codex> for Value {
	#[inline]
	fn from(codex: Codex) -> Self {
		Self::Codex(codex)
	}
}

impl Dump for Codex {
	fn dump(&self, to: &mut String, vm: &mut Vm) -> Result<(), RuntimeError> {
		to.push('{');

		let mut is_first = true;

		for (key, value) in self.0.read().iter() {
			if is_first {
				is_first = false;
			} else {
				to.push_str(", ");
			}

			key.dump(to, vm)?;
			to.push_str(": ");
			value.dump(to, vm)?;
		}

		to.push('}');
		Ok(())
	}
}

impl ConvertTo<Veracity> for Codex {
	fn convert(&self, _: &mut Vm) -> Result<Veracity, RuntimeError> {
		Ok(!self.is_empty())
	}
}

impl ConvertTo<Text> for Codex {
	fn convert(&self, vm: &mut Vm) -> Result<Text, RuntimeError> {
		let mut dump = String::new();

		self.dump(&mut dump, vm)?;

		Ok(dump.into())
	}
}

impl ConvertTo<Book> for Codex {
	fn convert(&self, _: &mut Vm) -> Result<Book, RuntimeError> {
		Ok(self.0
			.read()
			.iter()
			.map(|(l, r)| vec![l.clone(), r.clone()])
			.map(Value::from)
			.collect::<Vec<_>>()
			.into())
	}
}

impl Add for Codex {
	fn add(&self, rhs: &Value, vm: &mut Vm) -> Result<Value, RuntimeError> {
		let _ = (rhs, vm);
		todo!()
	}
}

impl Subtract for Codex {
	fn subtract(&self, rhs: &Value, vm: &mut Vm) -> Result<Value, RuntimeError> {
		let _ = (rhs, vm);
		todo!()
	}
}

impl Matches for Codex {
	fn matches(&self, target: &Value, vm: &mut Vm) -> Result<bool, RuntimeError> {
		self.is_equal(target, vm)
	}
}

impl IsEqual for Codex {
	fn is_equal(&self, rhs: &Value, vm: &mut Vm) -> Result<bool, RuntimeError> {
		let _ = (rhs, vm);
		todo!()
	}
}

impl Compare for Codex {
	fn compare(&self, rhs: &Value, vm: &mut Vm) -> Result<Option<Ordering>, RuntimeError> {
		let _ = (rhs, vm);
		todo!()
	}
}

impl GetIndex for Codex {
	fn get_index(&self, key: &Value, vm: &mut Vm) -> Result<Value, RuntimeError> {
		let _ = (key, vm);
		todo!()
	}
}

impl SetIndex for Codex {
	fn set_index(&self, key: Value, value: Value, vm: &mut Vm) -> Result<(), RuntimeError> {
		let _ = (key, value, vm);
		todo!()
	}
}

impl GetAttr for Codex {
	fn get_attr(&self, attr: &str, vm: &mut Vm) -> Result<Value, RuntimeError> {
		let _ = (attr, vm); todo!();
	}
}

