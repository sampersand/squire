use std::ops::{Add, Mul};

#[derive(Debug, Clone, PartialEq, Eq, PartialOrd, Ord, Hash, Default)]
pub struct Text(String);

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
		Self(text.to_string())
	}

	pub fn new_fraktur(text: String) -> Self {
		// todo: do we translate fraktur over to ascii?
		Self::new(text)
	}

	pub fn is_empty(&self) -> bool {
		self.0.is_empty()
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

impl From<String> for Text {
	#[inline]
	fn from(text: String) -> Self {
		Self::new(text)
	}
}

impl AsRef<str> for Text {
	fn as_ref(&self) -> &str {
		self.as_str()
	}
}

impl<T: AsRef<str>> Add<T> for Text {
	type Output = Self;

	fn add(mut self, rhs: T) -> Self::Output {
		self.0 += rhs.as_ref();
		self
	}
}

impl Mul<usize> for Text {
	type Output = Self;

	fn mul(mut self, amount: usize) -> Self::Output {
		match amount {
			0 => Self::default(),
			1 => self,
			_ => {
				self.0 = self.0.repeat(amount);
				self
			}
		}
	}
}