use crate::runtime::{Vm, Error as RuntimeError};
use std::sync::Arc;
use crate::value::{Value, Veracity, Numeral, Book};
use crate::value::ops::{ConvertTo, Dump, IsEqual, Compare, Add, Multiply, Modulo, GetIndex, GetAttr};
use std::fmt::{self, Display, Formatter};

#[derive(Debug, Clone, PartialEq, Eq, PartialOrd, Ord, Hash)]
pub struct Text(Arc<str>);

impl Default for Text {
	fn default() -> Self {
		Self(Arc::from(""))
	}
}

pub const FRAKTUR_UPPER: [char; 26] = [
	'𝔄', '𝔅', 'ℭ', '𝔇', '𝔈', '𝔉', '𝔊', // A, B, C, D, E, F, G
	'ℌ', 'ℑ', '𝔍', '𝔎', '𝔏', '𝔐', '𝔑', // H, I, J, K, L, M N
	'𝔒', '𝔓', '𝔔', 'ℜ', '𝔖', '𝔗', '𝔘', // O, P, Q, R, S, T, U,
	'𝔙', '𝔚', '𝔛', '𝔜', 'ℨ' // V, W, X, Y, Z
];

const ASCII_UPPER: [char; 26] = [
	'A', 'B', 'C', 'D', 'E', 'F', 'G',
	'H', 'I', 'J', 'K', 'L', 'M', 'N',
	'O', 'P', 'Q', 'R', 'S', 'T', 'U',
	'V', 'W', 'X', 'Y', 'Z',
];

pub const FRAKTUR_LOWER: [char; 26] = [
	'𝔞', '𝔟', '𝔠', '𝔡', '𝔢', '𝔣', '𝔤',
	'𝔥', '𝔦', '𝔧', '𝔨', '𝔩', '𝔪', '𝔫',
	'𝔬', '𝔭', '𝔮', '𝔯', '𝔰', '𝔱', '𝔲',
	'𝔳', '𝔴', '𝔵', '𝔶', '𝔷', 
];

const ASCII_LOWER: [char; 26] = [
	'a', 'b', 'c', 'd', 'e', 'f', 'g',
	'h', 'i', 'j', 'k', 'l', 'm', 'n',
	'o', 'p', 'q', 'r', 's', 't', 'u',
	'v', 'w', 'x', 'y', 'z',
];


pub fn is_fraktur(chr: char) -> bool {
	FRAKTUR_LOWER.contains(&chr) || FRAKTUR_UPPER.contains(&chr)
}

pub fn to_fraktur(chr: char) -> Option<char> {
	if let Some(index) = ASCII_UPPER.iter().position(|&c| c == chr) {
		Some(FRAKTUR_UPPER[index])
	} else if let Some(index) = ASCII_LOWER.iter().position(|&c| c == chr) {
		Some(FRAKTUR_LOWER[index])
	} else {
		None
	}
}

pub fn from_fraktur(chr: char) -> Option<char> {
	if let Some(index) = FRAKTUR_UPPER.iter().position(|&c| c == chr) {
		Some(ASCII_UPPER[index])
	} else if let Some(index) = FRAKTUR_LOWER.iter().position(|&c| c == chr) {
		Some(ASCII_LOWER[index])
	} else {
		None
	}
}

impl Text {
	pub fn new(text: impl ToString) -> Self {
		let mut text = text.to_string();
		text.shrink_to_fit();

		Self(Arc::from(text))
	}

	pub fn new_fraktur(text: String) -> Self {
		Self::new(text.chars().map(|chr| from_fraktur(chr).unwrap_or(chr)).collect::<String>())
	}

	pub fn is_empty(&self) -> bool {
		self.0.is_empty()
	}

	pub fn len(&self) -> usize {
		self.0.len()
	}

	pub fn as_str(&self) -> &str {
		&self.0
	}

	pub fn char_at(&self, index: isize) -> Option<char> {
		if 0 <= index {
			self.0.chars().nth(index as usize)
		} else {
			let chars = self.0.chars().collect::<Vec<_>>();

			if let Ok(index) = <usize as std::convert::TryFrom<isize>>::try_from(index + chars.len() as isize) {
				chars.get(index).cloned()
			} else {
				None
			}
		}
	}
}

impl Display for Text {
	fn fmt(&self, f: &mut Formatter<'_>) -> fmt::Result {
		Display::fmt(&self.as_str(), f)
	}
}

impl From<String> for Text {
	#[inline]
	fn from(text: String) -> Self {
		Self::new(text)
	}
}

impl From<Arc<str>> for Text {
	#[inline]
	fn from(text: Arc<str>) -> Self {
		Self(text)
	}
}

impl From<char> for Text {
	#[inline]
	fn from(chr: char) -> Self {
		Self::new(chr)
	}
}

impl AsRef<str> for Text {
	fn as_ref(&self) -> &str {
		self.as_str()
	}
}

impl Add for Text {
	fn add(&self, rhs: &Value, vm: &mut Vm) -> Result<Value, RuntimeError> {
		let rhs = rhs.convert_to::<Self>(vm)?;

		let mut text = String::with_capacity(self.len() + rhs.len());
		text += self.as_str();
		text += rhs.as_str();

		Ok(Self::new(text).into())
	}
}

impl Multiply for Text {
	fn multiply(&self, rhs: &Value,  vm: &mut Vm) -> Result<Value, RuntimeError> {
		match rhs.convert_to::<Numeral>(vm)?.get() {
			0 => Ok(Self::default().into()),
			1 => Ok(self.clone().into()),
			amount @ 2..=i64::MAX => Ok(self.as_str().repeat(amount as usize).into()),
			_ => Err(RuntimeError::ArgumentError("cannot repeat by a negative value".into())),
		}
	}
}

impl Modulo for Text {
	fn modulo(&self, rhs: &Value,  _: &mut Vm) -> Result<Value, RuntimeError> {
		let _ = rhs;
		todo!("printf formatting");
	}
}


impl Dump for Text {
	fn dump(&self, to: &mut String, _: &mut Vm) -> Result<(), RuntimeError> {
		to.push_str(&format!("{:?}", self.as_str()));

		Ok(())
	}
}


impl ConvertTo<Veracity> for Text {
	fn convert(&self, _: &mut Vm) -> Result<Veracity, RuntimeError> {
		Ok(!self.is_empty())
	}
}

impl ConvertTo<Numeral> for Text {
	fn convert(&self, _: &mut Vm) -> Result<Numeral, RuntimeError> {
		Ok(self.as_str().parse()?)
	}
}

impl ConvertTo<Book> for Text {
	fn convert(&self, _: &mut Vm) -> Result<Book, RuntimeError> {
		Ok(self
			.as_str()
			.chars()
			.map(Self::from)
			.map(Value::Text)
			.collect())
	}
}

impl IsEqual for Text {
	fn is_equal(&self, rhs: &Value, _: &mut Vm) -> Result<bool, RuntimeError> {
		if let Value::Text(rhs) = rhs {
			Ok(*self == *rhs)
		} else {
			Ok(false)
		}
	}
}

impl Compare for Text {
	fn compare(&self, rhs: &Value, vm: &mut Vm) -> Result<Option<std::cmp::Ordering>, RuntimeError> {
		Ok(self.partial_cmp(&rhs.convert_to::<Self>(vm)?))
	}
}

impl GetIndex for Text {
	fn get_index(&self, key: &Value, vm: &mut Vm) -> Result<Value, RuntimeError> {
		let _ = (key, vm);
		todo!();
	}
}

impl GetAttr for Text {
	fn get_attr(&self, attr: &str, _: &mut Vm) -> Result<Value, RuntimeError> {
		match attr {
			"count" => Ok(Numeral::new(self.len() as i64).into()),
			other => Err(RuntimeError::UnknownAttribute(other.to_string()))
		}
	}
}