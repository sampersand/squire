#![allow(unused)]
// use parking_lot::RwLock;
use crate::runtime::{Vm, Error as RuntimeError};
use crate::value::{Value, Veracity, Numeral, Text, Codex};
use std::fmt::{self, Display, Formatter};
use std::ops::{
	Add, AddAssign,
	Sub, SubAssign,
	Mul, MulAssign,
	// BitAnd, BitAndAssign,
	// BitOr, BitOrAssign,
	// BitXor, BitXorAssign,
	Index, IndexMut
};
use crate::value::ops::{
	ConvertTo,
	Add as OpsAdd, Subtract, Multiply, IsEqual, Compare,
	GetIndex, SetIndex
};

#[derive(Debug, Clone, PartialEq, Eq, PartialOrd, Hash, Default)]
pub struct Book(Vec<Value>);

impl From<Vec<Value>> for Book {
	#[inline]
	fn from(vec: Vec<Value>) -> Self {
		Self(vec)
	}
}

impl Book {
	pub const fn new() -> Self {
		Self(Vec::new())
	}

	pub fn with_capacity(capacity: usize) -> Self {
		Self(Vec::with_capacity(capacity))
	}

	fn is_index_out_of_bounds(&self, index: usize) -> bool {
		self.len() <= index
	}

	pub fn expand_to(&mut self, new_len: usize) {
		if self.len() < new_len {
			self.0.resize_with(new_len, Value::default)
		}
	}

	pub fn push(&mut self, value: Value) {
		self.0.push(value);
	}

	pub fn pop(&mut self) -> Option<Value> {
		self.0.pop()
	}

	pub fn contains(&self, value: &Value) -> bool {
		self.0.contains(value)
	}

	// insert into the index, possibly filling empty slots with Null.
	pub fn insert(&mut self, index: usize, value: Value) {
		self.expand_to(index + 1); // TODO: _do_ we need the extra slot?
		self.0.insert(index, value);
	}

	pub fn remove(&mut self, index: usize) -> Option<Value> {
		if self.is_index_out_of_bounds(index) {
			None
		} else {
			Some(self.0.remove(index))
		}
	}

	pub fn get(&self, index: usize) -> Option<&Value> {
		self.0.get(index)
	}

	pub fn get2_maybe_a_better_name(&self, index: isize) -> Option<&Value> {
		if 0 <= index {
			self.get(index as usize)
		} else if let Ok(index) = <usize as std::convert::TryFrom<isize>>::try_from(index) {
			self.get(index)
		} else {
			None
		}
	}

	pub fn set(&mut self, index: usize, value: Value) {
		self.expand_to(index + 1);
		self.0[index] = value;
	}


	pub fn set2_maybe_a_better_name(&mut self, index: isize, value: Value) {
		if 0 <= index {
			self.set(index as usize, value)
		} else if let Ok(index) = <usize as std::convert::TryFrom<isize>>::try_from(index) {
			self.set(index, value)
		} else {
			todo!()
		}
	}

	pub fn get_mut(&mut self, index: usize) -> Option<&mut Value> {
		self.0.get_mut(index)
	}

	pub fn len(&self) -> usize {
		self.0.len()
	}

	pub fn is_empty(&self) -> bool {
		self.0.is_empty()
	}

	pub fn as_slice(&self) -> &[Value] {
		&self.0
	}

	pub fn as_slice_mut(&mut self) -> &mut [Value] {
		&mut self.0
	}

	pub fn clear(&mut self) {
		self.0.clear();
	}

	pub fn iter<'a>(&'a self) -> impl Iterator<Item=&'a Value> + 'a {
		self.0.iter()
	}

	pub fn iter_mut<'a>(&'a mut self) -> impl Iterator<Item=&'a mut Value> + 'a {
		self.0.iter_mut()
	}

	pub fn shrink_to_fit(&mut self) {
		self.0.shrink_to_fit();
	}
}

impl std::iter::FromIterator<Value> for Book {
    fn from_iter<T: IntoIterator<Item = Value>>(iter: T) -> Self {
    	Self(iter.into_iter().collect())
    }
}

impl IntoIterator for Book {
	type Item = Value;
	type IntoIter = <Vec<Value> as IntoIterator>::IntoIter;

	fn into_iter(self) -> Self::IntoIter {
		self.0.into_iter()
	}
}

impl AsRef<[Value]> for Book {
	fn as_ref(&self) -> &[Value] {
		self.as_slice()
	}
}

impl AsMut<[Value]> for Book {
	fn as_mut(&mut self) -> &mut [Value] {
		self.as_slice_mut()
	}
}

impl Display for Book {
	fn fmt(&self, f: &mut Formatter<'_>) -> fmt::Result {
		write!(f, "[")?;

		for value in self.as_slice() {
			write!(f, "{:#}", value)?
		}

		write!(f, "]")
	}
}

impl std::iter::Extend<Value> for Book {
	fn extend<I: IntoIterator<Item=Value>>(&mut self, iter: I) {
		self.0.extend(iter)
	}
}

impl<I: std::slice::SliceIndex<[Value]>> Index<I> for Book {
	type Output = I::Output;

	#[inline]
	fn index(&self, index: I) -> &Self::Output {
		&self.0[index]
	}
}

impl<I: std::slice::SliceIndex<[Value]>> IndexMut<I> for Book {
	#[inline]
	fn index_mut(&mut self, index: I) -> &mut Self::Output {
		&mut self.0[index]
	}
}

impl<I: IntoIterator<Item=Value>> Add<I> for Book {
	type Output = Self;

	#[inline]
	fn add(mut self, rhs: I) -> Self {
		self += rhs;
		self
	}
}

impl<I: IntoIterator<Item=Value>> AddAssign<I> for Book {
	#[inline]
	fn add_assign(&mut self, rhs: I) {
		self.extend(rhs);
	}
}

impl<'a, I: IntoIterator<Item=&'a Value>> Sub<I> for Book {
	type Output = Book;

	#[inline]
	fn sub(mut self, rhs: I) -> Book {
		self -= rhs;
		self
	}
}


impl<'a, I: IntoIterator<Item=&'a Value>> SubAssign<I> for Book {
	fn sub_assign(&mut self, rhs: I) {
		// todo: optimize
		let rhs = rhs.into_iter().collect::<Vec<_>>();
		self.0.retain(|value| !rhs.contains(&value))
	}
}

impl Mul<usize> for Book {
	type Output = Book;

	#[inline]
	fn mul(mut self, amount: usize) -> Book {
		self *= amount;
		self
	}
}

impl MulAssign<usize> for Book {
	fn mul_assign(&mut self, amount: usize) {
		if amount <= 1 {
			if amount == 0 {
				self.clear();
			}

			return;
		}

		self.expand_to(self.len() * amount);
		let dup = self.iter().map(Clone::clone).collect::<Vec<_>>();
		self.extend(dup.into_iter().cycle().take(amount - 1));
	}
}

impl ConvertTo<Veracity> for Book {
	fn convert(&self, _: &mut Vm) -> Result<Veracity, RuntimeError> {
		Ok(!self.is_empty())
	}
}

impl ConvertTo<Text> for Book {
	fn convert(&self, _: &mut Vm) -> Result<Text, RuntimeError> {
		todo!()
	}
}

impl ConvertTo<Codex> for Book {
	fn convert(&self, _: &mut Vm) -> Result<Codex, RuntimeError> {
		todo!()
	}
}

impl OpsAdd for Book {
	fn add(&self, rhs: &Value, vm: &mut Vm) -> Result<Value, RuntimeError> {
		let _ = (rhs, vm);
		todo!()
	}
}

impl Subtract for Book {
	fn subtract(&self, rhs: &Value, vm: &mut Vm) -> Result<Value, RuntimeError> {
		let _ = (rhs, vm);
		todo!()
	}
}

impl Multiply for Book {
	fn multiply(&self, rhs: &Value, vm: &mut Vm) -> Result<Value, RuntimeError> {
		let _ = (rhs, vm);
		todo!()
	}
}


impl IsEqual for Book {
	fn is_equal(&self, rhs: &Value, vm: &mut Vm) -> Result<bool, RuntimeError> {
		let rhs = rhs.convert_to::<Self>(vm)?;

		if (self as *const _) == (&rhs as *const _) {
			return Ok(true);
		} else if self.len() != rhs.len() {
			return Ok(false);
		}

		for (lhs, rhs) in self.iter().zip(rhs.iter()) {
			if !lhs.is_equal(rhs, vm)? {
				return Ok(false)
			}
		}

		Ok(true)
	}
}

impl Compare for Book {
	fn compare(&self, rhs: &Value, vm: &mut Vm) -> Result<Option<std::cmp::Ordering>, RuntimeError> {
		let _ = (rhs, vm); todo!()
	}
}

impl GetIndex for Book {
	fn get_index(&self, key: &Value, vm: &mut Vm) -> Result<Value, RuntimeError> {
		let _ = (key, vm); todo!()
	}
}


impl SetIndex for Book {
	fn set_index(&self, key: Value, value: Value, vm: &mut Vm) -> Result<(), RuntimeError> {
		let _ = (key, value, vm); todo!()
	}
}

// impl<I: IntoIterator<Item=Value>> BitAnd<I> for &Book {
// 	type Output = Book;
// 	fn bitand(self, rhs: I) -> Book {
// 		let mut new = Book::with_capacity(self.len());

// 		for ele in rhs {
// 			if self.contains(ele.borrow()) {
// 				new.push(ele.into());
// 			}
// 		}

// 		new
// 	}
// }

// // impl<I: AsRef<[Value]>> BitAndAssign<&I> for Book {
// // 	fn bitand_assign(&mut self, rhs: &I) {
// // 		let rhs = rhs.as_ref();

// // 		self.0.retain(|value| rhs.contains(value));
// // 	}
// // }

// // impl<I: AsRef<[Value]>> BitOr<&I> for &Book {
// // 	type Output = Book;
// // 	fn bitor(self, rhs: &T) -> Book {
// // 		let rhs = rhs.as_ref();

// // 		let mut new = Book::with_capacity(self.len());

// // 		for ele in self.iter() {
// // 			if rhs.contains(ele) {
// // 				new.push(ele.clone());
// // 			}
// // 		}

// // 		new.shrink_to_fit();
// // 		new
// // 	}
// // }

// // impl<T: AsRef<[Value]>> BitOrAssign<&T> for Book {
// // 	fn bitor_assign(&mut self, rhs: &T) {
// // 		let rhs = rhs.as_ref().iter().filter(|value| !self.contains(value));
// // 		self.extend(rhs.iter().filter(|value| !self.contains(value)).cloned());
// // 	}
// // }
