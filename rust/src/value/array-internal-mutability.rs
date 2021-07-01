use std::sync::Arc;
use parking_lot::RwLock;
use crate::runtime::{Vm, Error as RuntimeError};
use crate::value::Value;
use std::fmt::{self, Display, Formatter};
use std::ops::{
	Deref, DerefMut,
	Add, AddAssign,
	Sub, SubAssign,
	Mul, MulAssign,
	// BitAnd, BitAndAssign,
	// BitOr, BitOrAssign,
	// BitXor, BitXorAssign,
	Index, IndexMut
};

#[derive(Debug, Clone, Default)]
pub struct Array(Arc<RwLock<Vec<Value>>>);

impl From<Vec<Value>> for Array {
	#[inline]
	fn from(vec: Vec<Value>) -> Self {
		Self(Arc::new(RwLock::new(vec)))
	}
}

impl PartialEq for Array {
	fn eq(&self, rhs: &Self) -> bool {
		if Arc::ptr_eq(&self.0, &rhs.0) {
			return true;
		}

		let mine = self.0.read();
		let theirs = rhs.0.read();

		if mine.len() != theirs.len() {
			return false;
		}

		mine.iter().zip(&*theirs).all(|(lhs, rhs)| lhs == rhs)
	}
}

impl Array {
	pub const fn new() -> Self {
		Self(Arc::new(RwLock::new(Vec::new())))
	}

	pub fn with_capacity(capacity: usize) -> Self {
		Self::from(Vec::with_capacity(capacity))
	}

	fn is_index_out_of_bounds(&self, index: usize) -> bool {
		self.len() <= index
	}

	pub fn expand_to(&self, new_len: usize) {
		if self.len() < new_len {
			self.as_vec_mut().resize_with(new_len, Value::default)
		}
	}

	pub fn push(&self, value: Value) {
		self.as_vec_mut().push(value);
	}

	pub fn pop(&self) -> Option<Value> {
		self.as_vec_mut().pop()
	}

	pub fn contains(&self, value: &Value) -> bool {
		self.as_slice().contains(value)
	}

	// insert into the index, possibly filling empty slots with Ni.
	pub fn insert(&self, index: usize, value: Value) {
		self.expand_to(index + 1); // TODO: _do_ we need the extra slot?
		self.as_vec_mut().insert(index, value);
	}

	pub fn remove(&mut self, index: usize) -> Option<Value> {
		if self.is_index_out_of_bounds(index) {
			None
		} else {
			Some(self.as_vec_mut().remove(index))
		}
	}

	pub fn get(&self, index: usize) -> Option<Value> {
		self.as_slice().get(index).cloned()
	}

	pub fn get2_maybe_a_better_name(&self, index: isize) -> Option<Value> {
		if 0 <= index {
			self.get(index as usize)
		} else if let Ok(index) = <usize as std::convert::TryFrom<isize>>::try_from(index) {
			self.get(index)
		} else {
			None
		}
	}

	pub fn set(&self, index: usize, value: Value) {
		self.expand_to(index + 1);
		self.as_slice_mut()[index] = value;
	}


	pub fn set2_maybe_a_better_name(&self, index: isize, value: Value) {
		if 0 <= index {
			self.set(index as usize, value)
		} else if let Ok(index) = <usize as std::convert::TryFrom<isize>>::try_from(index) {
			self.set(index, value)
		} else {
			todo!()
		}
	}

	pub fn len(&self) -> usize {
		self.as_slice().len()
	}

	pub fn is_empty(&self) -> bool {
		self.as_slice().is_empty()
	}

	pub fn as_slice(&self) -> impl Deref<Target=[Value]> + '_ {
		struct SliceDeref<'a>(parking_lot::RwLockReadGuard<'a, Vec<Value>>);

		impl Deref for SliceDeref<'_> {
			type Target = [Value];
		
			fn deref(&self) -> &Self::Target {
				&self.0
			}
		}

		SliceDeref(self.0.read())
	}

	pub fn as_vec_mut(&self) -> impl DerefMut<Target=Vec<Value>> + '_ {
		struct VecDerefMut<'a>(parking_lot::RwLockWriteGuard<'a, Vec<Value>>);

		impl Deref for VecDerefMut<'_> {
			type Target = Vec<Value>;
		
			fn deref(&self) -> &Self::Target {
				&self.0
			}
		}

		impl DerefMut for VecDerefMut<'_> {
			fn deref_mut(&mut self) -> &mut Self::Target {
				&mut self.0
			}
		}

		VecDerefMut(self.0.write())
	}

	pub fn as_slice_mut(&self) -> impl DerefMut<Target=[Value]> + '_ {
		struct SliceDerefMut<'a>(parking_lot::RwLockWriteGuard<'a, Vec<Value>>);

		impl Deref for SliceDerefMut<'_> {
			type Target = [Value];
		
			fn deref(&self) -> &Self::Target {
				&self.0
			}
		}

		impl DerefMut for SliceDerefMut<'_> {
			fn deref_mut(&mut self) -> &mut Self::Target {
				&mut self.0
			}
		}

		SliceDerefMut(self.0.write())
	}

	pub fn clear(&mut self) {
		self.0.write().clear();
	}

	pub fn iter(&self) -> impl Iterator<Item=Value> + '_ {
		struct Iter<'a>(parking_lot::RwLockReadGuard<'a, Vec<Value>>, usize);

		impl Iterator for Iter<'_> {
			type Item = Value;
		
			fn next(&mut self) -> Option<Self::Item> {
				let ele = self.0.get(self.1)?;
				self.1 += 1;
				Some(ele.clone())
			}
		}

		Iter(self.0.read(), 0)
	}

	// pub fn iter_mut<'a>(&'a mut self) -> impl Iterator<Item=&'a mut Value> + 'a {
	// 	self.0.iter_mut()
	// }

	pub fn shrink_to_fit(&self) {
		self.as_vec_mut().shrink_to_fit();
	}
}

impl std::iter::FromIterator<Value> for Array {
    fn from_iter<T: IntoIterator<Item = Value>>(iter: T) -> Self {
    	Self::from(iter.into_iter().collect::<Vec<_>>())
    }
}

impl IntoIterator for Array {
	type Item = Value;
	type IntoIter = <Vec<Value> as IntoIterator>::IntoIter;

	fn into_iter(self) -> Self::IntoIter {
		self.0.read().into_iter()
	}
}

impl AsRef<[Value]> for Array {
	fn as_ref(&self) -> &[Value] {
		self.as_slice()
	}
}

impl AsMut<[Value]> for Array {
	fn as_mut(&mut self) -> &mut [Value] {
		self.as_slice_mut()
	}
}

impl Display for Array {
	fn fmt(&self, f: &mut Formatter<'_>) -> fmt::Result {
		write!(f, "[")?;

		for value in self.as_slice() {
			write!(f, "{:#}", value)?
		}

		write!(f, "]")
	}
}

impl std::iter::Extend<Value> for Array {
	fn extend<I: IntoIterator<Item=Value>>(&mut self, iter: I) {
		self.0.extend(iter)
	}
}

impl<I: std::slice::SliceIndex<[Value]>> Index<I> for Array {
	type Output = I::Output;

	#[inline]
	fn index(&self, index: I) -> &Self::Output {
		&self.0[index]
	}
}

impl<I: std::slice::SliceIndex<[Value]>> IndexMut<I> for Array {
	#[inline]
	fn index_mut(&mut self, index: I) -> &mut Self::Output {
		&mut self.0[index]
	}
}

impl<I: IntoIterator<Item=Value>> Add<I> for Array {
	type Output = Self;

	#[inline]
	fn add(mut self, rhs: I) -> Self {
		self += rhs;
		self
	}
}

impl<I: IntoIterator<Item=Value>> AddAssign<I> for Array {
	#[inline]
	fn add_assign(&mut self, rhs: I) {
		self.extend(rhs);
	}
}

impl<'a, I: IntoIterator<Item=&'a Value>> Sub<I> for Array {
	type Output = Array;

	#[inline]
	fn sub(mut self, rhs: I) -> Array {
		self -= rhs;
		self
	}
}


impl<'a, I: IntoIterator<Item=&'a Value>> SubAssign<I> for Array {
	fn sub_assign(&mut self, rhs: I) {
		// todo: optimize
		let rhs = rhs.into_iter().collect::<Vec<_>>();
		self.0.retain(|value| !rhs.contains(&value))
	}
}

impl Mul<usize> for Array {
	type Output = Array;

	#[inline]
	fn mul(mut self, amount: usize) -> Array {
		self *= amount;
		self
	}
}

impl MulAssign<usize> for Array {
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

impl Array {
	pub fn try_eql(&self, rhs: &Self, vm: &mut Vm) -> Result<bool, RuntimeError> {
		if (self as *const _) == (rhs as *const _) {
			return Ok(true);
		} else if self.len() != rhs.len() {
			return Ok(false);
		}

		for (lhs, rhs) in self.iter().zip(rhs.iter()) {
			if !lhs.try_eql(rhs, vm)? {
				return Ok(false)
			}
		}

		Ok(true)
	}

	pub fn try_cmp(&self, rhs: &Self, vm: &mut Vm) -> Result<std::cmp::Ordering, RuntimeError> {
		let _ = (rhs, vm); todo!()
	}
}

// impl<I: IntoIterator<Item=Value>> BitAnd<I> for &Array {
// 	type Output = Array;
// 	fn bitand(self, rhs: I) -> Array {
// 		let mut new = Array::with_capacity(self.len());

// 		for ele in rhs {
// 			if self.contains(ele.borrow()) {
// 				new.push(ele.into());
// 			}
// 		}

// 		new
// 	}
// }

// // impl<I: AsRef<[Value]>> BitAndAssign<&I> for Array {
// // 	fn bitand_assign(&mut self, rhs: &I) {
// // 		let rhs = rhs.as_ref();

// // 		self.0.retain(|value| rhs.contains(value));
// // 	}
// // }

// // impl<I: AsRef<[Value]>> BitOr<&I> for &Array {
// // 	type Output = Array;
// // 	fn bitor(self, rhs: &T) -> Array {
// // 		let rhs = rhs.as_ref();

// // 		let mut new = Array::with_capacity(self.len());

// // 		for ele in self.iter() {
// // 			if rhs.contains(ele) {
// // 				new.push(ele.clone());
// // 			}
// // 		}

// // 		new.shrink_to_fit();
// // 		new
// // 	}
// // }

// // impl<T: AsRef<[Value]>> BitOrAssign<&T> for Array {
// // 	fn bitor_assign(&mut self, rhs: &T) {
// // 		let rhs = rhs.as_ref().iter().filter(|value| !self.contains(value));
// // 		self.extend(rhs.iter().filter(|value| !self.contains(value)).cloned());
// // 	}
// // }
