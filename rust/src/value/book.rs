use std::sync::Arc;
use parking_lot::RwLock;
use crate::runtime::{Vm, Error as RuntimeError};
use crate::value::{Value, Veracity, Numeral, Text, Codex};
use std::hash::{Hash, Hasher};
use std::fmt::{self, Debug, Display, Formatter};
use std::ops::{Deref, DerefMut};
use crate::value::ops::{
	ConvertTo, Duplicate,
	Add, Subtract, Multiply, IsEqual, Compare,
	GetIndex, SetIndex,
	Dump, GetAttr
};

#[derive(Clone, Default)]
pub struct Book(Arc<RwLock<Vec<Value>>>);

impl Debug for Book {
	fn fmt(&self, f: &mut Formatter<'_>) -> fmt::Result {
		let slice = &*self.as_slice();

		if f.alternate() {
			f.debug_tuple("Book")
				.field(&slice)
				.finish()		
		} else {
			Debug::fmt(slice, f)
		}
	}
}

impl Eq for Book {}
impl PartialEq for Book {
	fn eq(&self, rhs: &Self) -> bool {
		Arc::ptr_eq(&self.0, &rhs.0) || *self.as_slice() == *rhs.as_slice()
	}
}

impl Hash for Book {
	fn hash<H: Hasher>(&self, h: &mut H) {
		for i in &*self.as_slice() {
			i.hash(h);
		}
	}
}

impl From<Vec<Value>> for Book {
	#[inline]
	fn from(vec: Vec<Value>) -> Self {
		vec.into_iter().collect()
	}
}

impl From<Vec<Value>> for Value {
	#[inline]
	fn from(vec: Vec<Value>) -> Self {
		Book::from(vec).into()
	}
}


impl Book {
	pub fn new() -> Self {
		Self(Arc::new(RwLock::new(Vec::new())))
	}

	pub fn with_capacity(capacity: usize) -> Self {
		Self(Arc::new(RwLock::new(Vec::with_capacity(capacity))))
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

	pub fn remove(&self, index: usize) -> Option<Value> {
		// todo: lock before checking for out of bounds.
		if self.is_index_out_of_bounds(index) {
			None
		} else {
			Some(self.as_vec_mut().remove(index))
		}
	}

	pub fn get(&self, index: isize) -> Result<Option<Value>, RuntimeError> {
		let slice = &*self.as_slice();
		let len = slice.len() as isize;

		match index {
			0 => return Err(RuntimeError::ArgumentError("cannot index by zero".into())),
			_ if (1..=len).contains(&index) => Ok(Some(slice[index as usize - 1].clone())),
			_ if (-len..=-1).contains(&index) => Ok(Some(slice[(index + len) as usize].clone())),
			_ if index.is_negative() => return Err(RuntimeError::ArgumentError("index out of bounds".into())),
			_ => Ok(None)
		}
	}

	pub fn set(&self, index: isize, value: Value) -> Result<(), RuntimeError> {
		let mut vec = self.as_vec_mut();
		let len = vec.len() as isize;

		match index {
			0 => return Err(RuntimeError::ArgumentError("cannot index by zero".into())),
			_ if (1..=len).contains(&index) => vec[index as usize - 1] = value,
			_ if (-len..=-1).contains(&index) => vec[(index + len) as usize] = value,
			_ if index.is_negative() => return Err(RuntimeError::ArgumentError("index out of bounds".into())),
			_ => {
				vec.resize_with(index as usize, Value::default);
				vec.push(value);
			},
		}

		Ok(())
	}

	pub fn len(&self) -> usize {
		self.as_slice().len()
	}

	pub fn is_empty(&self) -> bool {
		self.as_slice().is_empty()
	}

	pub fn clear(&self) {
		self.as_vec_mut().clear();
	}

	pub fn iter(&self) -> impl Iterator<Item=Value> + '_ {
		struct Iter<'a>(parking_lot::RwLockReadGuard<'a, Vec<Value>>, usize);

		impl Iterator for Iter<'_> {
			type Item = Value;
		
			fn next(&mut self) -> Option<Self::Item> {
				let page = self.0.get(self.1)?;
				self.1 += 1;
				Some(page.clone())
			}
		}

		Iter(self.0.read(), 0)
	}

	pub fn shrink_to_fit(&self) {
		self.as_vec_mut().shrink_to_fit();
	}
}

impl Duplicate for Book {
	fn duplicate(&self) -> Self {
		self.iter().collect()
	}
}

impl std::iter::FromIterator<Value> for Book {
    fn from_iter<T: IntoIterator<Item = Value>>(iter: T) -> Self {
    	Self(Arc::new(RwLock::new(iter.into_iter().collect())))
    }
}

// impl IntoIterator for Book {
// 	type Item = Value;
// 	type IntoIter = <Vec<Value> as IntoIterator>::IntoIter;

// 	fn into_iter(self) -> Self::IntoIter {
// 		self.0.into_iter()
// 	}
// }

// impl AsRef<[Value]> for Book {
// 	fn as_ref(&self) -> &[Value] {
// 		&self.as_slice()
// 	}
// }

// impl AsMut<[Value]> for Book {
// 	fn as_mut(&mut self) -> &mut [Value] {
// 		self.as_slice_mut()
// 	}
// }

impl Display for Book {
	fn fmt(&self, f: &mut Formatter<'_>) -> fmt::Result {
		write!(f, "[")?;

		for value in &*self.as_slice() {
			write!(f, "{:?}", value)?
		}

		write!(f, "]")
	}
}

// impl std::iter::Extend<Value> for Book {
// 	fn extend<I: IntoIterator<Item=Value>>(&mut self, iter: I) {
// 		// todo: if we're currently deadlocked, bail.
// 		self.0.extend(iter)
// 	}
// }

// impl<I: std::slice::SliceIndex<[Value]>> Index<I> for Book {
// 	type Output = I::Output;

// 	#[inline]
// 	fn index(&self, index: I) -> &Self::Output {
// 		&self.as_slice()[index]
// 	}
// }

// impl<I: std::slice::SliceIndex<[Value]>> IndexMut<I> for Book {
// 	#[inline]
// 	fn index_mut(&mut self, index: I) -> &mut Self::Output {
// 		// &mut self.0[index]
// 		todo!()
// 	}
// }

// impl<I: IntoIterator<Item=Value>> Add<I> for Book {
// 	type Output = Self;

// 	#[inline]
// 	fn add(mut self, rhs: I) -> Self {
// 		self += rhs;
// 		self
// 	}
// }

// impl<I: IntoIterator<Item=Value>> AddAssign<I> for Book {
// 	#[inline]
// 	fn add_assign(&mut self, rhs: I) {
// 		self.extend(rhs);
// 	}
// }

// impl<'a, I: IntoIterator<Item=&'a Value>> Sub<I> for Book {
// 	type Output = Book;

// 	#[inline]
// 	fn sub(mut self, rhs: I) -> Book {
// 		self -= rhs;
// 		self
// 	}
// }


// impl<'a, I: IntoIterator<Item=&'a Value>> SubAssign<I> for Book {
// 	fn sub_assign(&mut self, rhs: I) {
// 		// todo: optimize
// 		let rhs = rhs.into_iter().collect::<Vec<_>>();
// 		self.0.retain(|value| !rhs.contains(&value))
// 	}
// }

// impl Mul<usize> for Book {
// 	type Output = Book;

// 	#[inline]
// 	fn mul(mut self, amount: usize) -> Book {
// 		self *= amount;
// 		self
// 	}
// }

// impl MulAssign<usize> for Book {
// 	fn mul_assign(&mut self, amount: usize) {
// 		if amount <= 1 {
// 			if amount == 0 {
// 				self.clear();
// 			}

// 			return;
// 		}

// 		self.expand_to(self.len() * amount);
// 		let dup = self.iter().map(Clone::clone).collect::<Vec<_>>();
// 		self.extend(dup.into_iter().cycle().take(amount - 1));
// 	}
// }

impl From<Book> for Value {
	#[inline]
	fn from(book: Book) -> Self {
		Self::Book(book)
	}
}

impl Dump for Book {
	fn dump(&self, to: &mut String, vm: &mut Vm) -> Result<(), RuntimeError> {
		let slice = &*self.as_slice();
		to.push('[');

		if let Some(first) = slice.get(0) {
			first.dump(to, vm)?;

			for page in &slice[1..] {
				to.push_str(", ");
				page.dump(to, vm)?;
			}
		}

		to.push(']');
		Ok(())
	}
}

impl ConvertTo<Veracity> for Book {
	fn convert(&self, _: &mut Vm) -> Result<Veracity, RuntimeError> {
		Ok(!self.is_empty())
	}
}

impl ConvertTo<Text> for Book {
	fn convert(&self, vm: &mut Vm) -> Result<Text, RuntimeError> {
		let mut out = String::new();

		self.dump(&mut out, vm)?;

		Ok(Text::new(out))
	}
}

impl ConvertTo<Codex> for Book {
	fn convert(&self, vm: &mut Vm) -> Result<Codex, RuntimeError> {
		use std::collections::HashMap;
		let mut codex = HashMap::with_capacity(self.len());

		for ele in &*self.as_slice() {
			let ele = ele.convert_to::<Book>(vm)?;
			let slice = ele.as_slice();

			if slice.len() != 2 {
				return Err(RuntimeError::ValueError(format!("expected 2 elements, got {}", slice.len())));
			}

			if codex.insert(slice[0].clone(), slice[1].clone()).is_some() {
				warn!(key=?slice[0], "duplicate key encountered");
			}
		}

		Ok(Codex::from(codex))
	}
}

impl Add for Book {
	fn add(&self, rhs: &Value, vm: &mut Vm) -> Result<Value, RuntimeError> {
		let slice = self.as_slice();
		let rhs = rhs.convert_to::<Self>(vm)?;
		let rhs = rhs.as_slice();

		let mut sum = Vec::with_capacity(slice.len() + rhs.len());

		sum.extend(slice.iter().cloned());
		sum.extend(rhs.iter().cloned());

		Ok(Self::from(sum).into())
	}
}

impl Subtract for Book {
	fn subtract(&self, rhs: &Value, vm: &mut Vm) -> Result<Value, RuntimeError> {
		let slice = self.as_slice();
		let rhs = rhs.convert_to::<Self>(vm)?;
		let rhs = rhs.as_slice();

		let mut result = Vec::with_capacity(self.len() - rhs.len());

		for ele in &*slice {
			if !rhs.contains(ele) {
				result.push(ele.clone());
			}
		}

		Ok(result.into())
	}
}

impl Multiply for Book {
	fn multiply(&self, rhs: &Value, vm: &mut Vm) -> Result<Value, RuntimeError> {
		match rhs {
			Value::Numeral(amount) => 
				if *amount < 0 {
					Err(RuntimeError::ArgumentError("cannot replicate by a negative amount".into()))
				} else {
					let slice = self.as_slice();
					Ok(slice
						.iter()
						.cloned()
						.cycle()
						.take((amount.get() as usize) * slice.len())
						.collect::<Self>()
						.into())
				},
			Value::Text(text) => {
				let mut join = String::new();
				let mut is_first = false;

				for page in &*self.as_slice() {
					if is_first {
						is_first = false;
					} else {
						join.push_str(text.as_str());
					}

					join.push_str(page.convert_to::<Text>(vm)?.as_str());
				}

				Ok(join.into())
			},
			Value::Book(other) => {
				let slice = self.as_slice();
				let other = other.as_slice();
				let mut prod = Vec::with_capacity(slice.len() * other.len());

				for lhs in &*slice {
					for rhs in &*other {
						prod.push(vec![lhs.clone(), rhs.clone()].into());
					} 
				}

				Ok(prod.into())
			},
			_ => Err(RuntimeError::InvalidOperand { kind: rhs.kind(), func: "Book.*" })
		}
	}
}


impl IsEqual for Book {
	fn is_equal(&self, rhs: &Value, vm: &mut Vm) -> Result<bool, RuntimeError> {
		let rhs = rhs.convert_to::<Self>(vm)?;

		if Arc::ptr_eq(&self.0, &rhs.0) {
			return Ok(true);
		} else if self.len() != rhs.len() {
			return Ok(false);
		}

		for (lhs, rhs) in self.iter().zip(rhs.iter()) {
			if !lhs.is_equal(&rhs, vm)? {
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
	fn get_index(&self, key: &Value, _: &mut Vm) -> Result<Value, RuntimeError> {
		match key {
			Value::Book(_book) => todo!("index into a book with another one."),
			Value::Numeral(numeral) => Ok(self.get(numeral.get() as isize)?.unwrap_or_default()),
			key => Err(RuntimeError::InvalidOperand { kind: key.kind(), func: "Book.[]" })
		}
	}
}


impl SetIndex for Book {
	fn set_index(&self, key: Value, value: Value, _: &mut Vm) -> Result<(), RuntimeError> {
		match key {
			Value::Book(_book) => todo!("index into a book with another one."),
			Value::Numeral(numeral) => self.set(numeral.get() as isize, value),
			key => Err(RuntimeError::InvalidOperand { kind: key.kind(), func: "Book.[]=" })
		}
	}
}

impl GetAttr for Book {
	fn get_attr(&self, attr: &str, _: &mut Vm) -> Result<Value, RuntimeError> {
		match attr {
			"pages" | "count" => Ok(Numeral::new(self.len() as i64).into()),
			other => Err(RuntimeError::UnknownAttribute(other.to_string()))
		}
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
