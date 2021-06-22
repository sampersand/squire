use std::fmt::{self, Display, Formatter};
use std::str::FromStr;

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, Default)]
pub struct Numeral(i64);

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash)]
pub enum RomanNumeral {
	N = 0,
	I = 1,
	V = 5,
	X = 10,
	L = 50,
	C = 100,
	D = 500,
	M = 1000,
}

impl Numeral {
	pub const fn new(num: i64) -> Self {
		Self(num)
	}

	pub const fn get(self) -> i64 {
		self.0
	}
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub enum NumeralParseError {
	Empty,
	UnexpectedStartingChar,
	AlphanumericTrailingChar {
		so_far: Numeral,
		bad_byte_index: usize
	}
}

impl Numeral {
	pub fn parse_from_arabic_str(input: &mut &str) -> Result<Self, NumeralParseError> {
		let mut chars = input.trim_start().chars();
		let mut is_neg = false;
		let mut numeral = Self::default();

		match chars.next() {
			Some('-') => is_neg = true,
			Some(digit) => numeral += digit.to_digit(10).ok_or(NumeralParseError::Empty)? as i64,
			None => return Err(NumeralParseError::UnexpectedStartingChar)
		}

		let mut is_valid = true;
		for digit in chars.by_ref() {
			if digit == '_' {
				continue;
			} else if let Some(num) = digit.to_digit(10) {
				numeral = numeral * 10 + (num as i64);
			} else {
				is_valid = digit.is_alphanumeric();
				break;
			}
		}

		if is_neg {
			numeral = -numeral;
		}

		if is_valid {
			*input = chars.as_str();
			Ok(numeral)
		} else {
			Err(NumeralParseError::AlphanumericTrailingChar {
				so_far: numeral,
				bad_byte_index: 0 // todo
			})
		}
	}

	pub fn parse_from_roman_str(_input: &mut &str) -> Result<Self, NumeralParseError> {
		todo!();
	}
// // 	pub fn parse_from_arabic_str(input: &mut &str) -> Option<Self> {
// 		let mut chars = input.trim_start().chars();
// 		let mut numeral = 0;
// 		let mut stage = RomanNumeral::N;
// 		let mut is_neg = false;

// 		match chars.next()? {
// 			'-' => is_neg = true,
// 			chr if chr == RomanNumeral::N.chr() => {
// 				if chars.next()? == RomanNumeral::N.chr() {
// 					// strip all trailing `_`s, then check to see if we end with an alphanumeric.
// 					// if we do, it's not an arabic string.
// 					while let Some(chr) = chars.next() {
// 						match chr {
// 							'_' => { /* find the next non-underscore character */ },
// 							_ if chr.is_alphanumeric() => return None, // alphanumeric at the end = dont parse
// 							_ => break // alphanumeric
// 						}
// 					}

// 					*input = chars.as_str();
// 					return Some(Self::new(0));
// 				},
// 			roman if


// 		// 	sq_number number = 0;
// // 	enum roman_numeral stage = 0, parsed;

// 		while let Some(chr) = chars.next() {
// 			if chr == '_' {
// 				continue; // ignore `_` in roman numerals
// 			}

// 			let numeral = 
// 				if let Some(numeral) = RomanNumeral::from_char(chr) {
// 					numeral
// 				} else if chr.is_alphanumeric() {
// 					return None; // trailing alphanumerics means not a roman numeral literal.
// 				};

// 			number += numeral as u64;
// // 	while (true) {
// // 		switch(*input) {
// // 		case 'I': parsed = SQ_TK_ROMAN_I; break;
// // 		case 'V': parsed = SQ_TK_ROMAN_V; break;
// // 		case 'X': parsed = SQ_TK_ROMAN_X; break;
// // 		case 'L': parsed = SQ_TK_ROMAN_L; break;
// // 		case 'C': parsed = SQ_TK_ROMAN_C; break;
// // 		case 'D': parsed = SQ_TK_ROMAN_D; break;
// // 		case 'M': parsed = SQ_TK_ROMAN_M; break;
// // 		case '_': continue; // ignore `_` in roman numeral literals
// // 		default:
// // 			// followed by any other alphanumerics, we aren't a roman numeral.
// // 			if (isalnum(*input)) return -1;
// // 			goto done;
// // 		}


// // 		number += parsed;

// // 		if (stage == 0 || parsed <= stage) stage = parsed;
// // 		else number -= stage * 2;

// // 		++input;
// // 	}

// // done:

// // 	if (output)
// // 		*output = input;

// // 	return number;



// 	}

	pub fn to_arabic(self) -> String {
		self.0.to_string()
	}

	pub fn to_roman(self) -> String {
		fn convert(digit: u64, one: RomanNumeral, five: RomanNumeral, ten: RomanNumeral, out: &mut String) {
			macro_rules! push {
				($($name:ident),*) => {{ $(out.push($name.chr());)* }};
			}

			match digit {
				1 => push!(one),
				2 => push!(one, one),
				3 => push!(one, one, one),
				4 => push!(one, five),
				5 => push!(five),
				6 => push!(five, one),
				7 => push!(five, one, one),
				8 => push!(five, one, one, one),
				9 => push!(one, ten),
				10 => push!(ten),
				other => unreachable!("invalid digit: {:?}", other)
			}
		}

		if self == 0 {
			return RomanNumeral::N.to_string();
		}

		let mut roman = String::new();
		let mut number;

		if self < 0 {
			roman.push('-');
			number = self.get().abs() as u64;
		} else {
			number = self.get() as u64;
		}

		while 0 < number {
			if number <= 10 {
				convert(number, RomanNumeral::I, RomanNumeral::V, RomanNumeral::X, &mut roman);
				break;
			} else if number <= 100 {
				convert(number / 10, RomanNumeral::X, RomanNumeral::L, RomanNumeral::C, &mut roman);
				number %= 10;
			} else if number <= 1000 {
				convert(number / 100, RomanNumeral::C, RomanNumeral::D, RomanNumeral::M, &mut roman);
				number %= 100;
			} else {
				roman.push(RomanNumeral::M.chr());
				number -= RomanNumeral::M as u64;
			}
		}

		roman
	}
}

impl FromStr for Numeral {
	type Err = NumeralParseError;

	fn from_str(input: &str) -> Result<Self, Self::Err> {
		let mut input = input.trim_start();

		match Self::parse_from_arabic_str(&mut input) {
			Err(NumeralParseError::UnexpectedStartingChar) => Self::parse_from_roman_str(&mut input),
			other => other
		}
	}
}


impl From<i64> for Numeral {
	#[inline]
	fn from(num: i64) -> Self {
		Self::new(num)
	}
}

impl From<Numeral> for i64 {
	#[inline]
	fn from(numeral: Numeral) -> Self {
		numeral.get()
	}
}

impl Display for Numeral {
	fn fmt(&self, f: &mut Formatter<'_>) -> fmt::Result {
		if cfg!(feature="roman-numeral-tostring") {
			Display::fmt(&self.to_roman(), f)
		} else {
			Display::fmt(&self.to_arabic(), f)
		}
	}
}

impl PartialEq<i64> for Numeral {
	fn eq(&self, rhs: &i64) -> bool {
		self.get() == *rhs
	}
}
impl PartialOrd<i64> for Numeral {
	fn partial_cmp(&self, rhs: &i64) -> Option<std::cmp::Ordering> {
		self.get().partial_cmp(rhs)
	}
}

impl std::ops::Neg for Numeral {
	type Output = Self;

	#[inline]
	fn neg(self) -> Self {
		Self::new(-self.get())
	}
}

macro_rules! impl_op {
	($trait:ident: $fn:ident, $trait_assign:ident: $fn_assign:ident) => {
		impl std::ops::$trait for Numeral {
			type Output = Self;

			#[inline]
			fn $fn(mut self, rhs: Self) -> Self::Output {
				use std::ops::$trait_assign;

				self.$fn_assign(rhs);
				self
			}
		}

		impl std::ops::$trait<i64> for Numeral {
			type Output = Self;

			#[inline]
			fn $fn(self, rhs: i64) -> Self::Output {
				self.$fn(Self::new(rhs))
			}
		}

		impl std::ops::$trait_assign for Numeral {
			#[inline]
			fn $fn_assign(&mut self, rhs: Self) {
				self.0.$fn_assign(rhs.get());
			}
		}

		impl std::ops::$trait_assign<i64> for Numeral {
			#[inline]
			fn $fn_assign(&mut self, rhs: i64) {
				self.$fn_assign(Self::new(rhs))
			}
		}
	};
}

impl_op!(Add: add, AddAssign: add_assign);
impl_op!(Sub: sub, SubAssign: sub_assign);
impl_op!(Mul: mul, MulAssign: mul_assign);
impl_op!(Div: div, DivAssign: div_assign);
impl_op!(Rem: rem, RemAssign: rem_assign);


impl Display for RomanNumeral {
	fn fmt(&self, f: &mut Formatter<'_>) -> fmt::Result {
		Display::fmt(&self.chr(), f)
	}
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum RomanNumeralParseError {
	NotARomanNumeral(char),
	NotExactlyOneChar
}

impl Display for RomanNumeralParseError {
	fn fmt(&self, f: &mut Formatter<'_>) -> fmt::Result {
		match self {
			Self::NotARomanNumeral(chr) => write!(f, "character '{}' is not a roman numeral", chr),
			Self::NotExactlyOneChar => write!(f, "not exactly one char was given.")
		}
	}
}

impl std::error::Error for RomanNumeralParseError {}

impl FromStr for RomanNumeral {
	type Err = RomanNumeralParseError;

	fn from_str(input: &str) -> Result<Self, Self::Err> {
		let _ = input;
		// let mut chars = input.chars();
		// let chr = chars.next()
		// let chr = input.chars
		todo!()
	}
}

impl RomanNumeral {
	pub const fn from_char(chr: char) -> Option<Self> {
		match chr {
			'N' => Some(Self::N),
			'I' => Some(Self::I),
			'V' => Some(Self::V),
			'X' => Some(Self::X),
			'L' => Some(Self::L),
			'C' => Some(Self::C),
			'D' => Some(Self::D),
			'M' => Some(Self::M),
			_ => None
		}
	}

	pub const fn chr(self) -> char {
		match self {
			Self::N => 'N',
			Self::I => 'I',
			Self::V => 'V',
			Self::X => 'X',
			Self::L => 'L',
			Self::C => 'C',
			Self::D => 'D',
			Self::M => 'M',
		}
	}
}