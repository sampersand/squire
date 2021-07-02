use std::fmt::{self, Display, Formatter};
use std::str::FromStr;
use std::cmp::Ordering;
use crate::runtime::{Vm, Error as RuntimeError};
use crate::value::{Value, Veracity, Text};
use crate::value::ops::{
	ConvertTo, Dump,
	Negate, Add, Subtract, Multiply, Divide, Modulo, Power,
	IsEqual, Compare,
	GetAttr
};

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, Default)]
pub struct Numeral(i64);

impl Numeral {
	pub const fn new(num: i64) -> Self {
		Self(num)
	}

	pub const fn get(self) -> i64 {
		self.0
	}

	pub const fn abs(self) -> Self {
		Self::new(self.get().abs())
	}

	pub const fn is_zero(self) -> bool {
		self.get() == 0
	}

	pub fn pow(self, amount: u32) -> Self {
		Self::new(self.get().pow(amount))
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

impl From<Ordering> for Numeral {
	#[inline]
	fn from(ord: Ordering) -> Self {
		match ord {
			Ordering::Less => Self::new(-1),
			Ordering::Equal => Self::new(0),
			Ordering::Greater => Self::new(1),
		}
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
	fn partial_cmp(&self, rhs: &i64) -> Option<Ordering> {
		self.get().partial_cmp(rhs)
	}
}

impl Dump for Numeral {
	fn dump(&self, to: &mut String, _: &mut Vm) -> Result<(), RuntimeError> {
		// todo: optimize this 
		to.push_str(&self.to_string());

		Ok(())
	}
}

impl ConvertTo<Veracity> for Numeral {
	fn convert(&self, _: &mut Vm) -> Result<Veracity, RuntimeError> {
		Ok(!self.is_zero())
	}
}

impl ConvertTo<Text> for Numeral {
	fn convert(&self, _: &mut Vm) -> Result<Text, RuntimeError> {
		Ok(Text::new(self))
	}
}

impl Negate for Numeral {
	fn negate(&self, _: &mut Vm) -> Result<Value, RuntimeError> {
		Ok(Self::new(-self.get()).into())
	}
}

impl Add for Numeral {
	fn add(&self, rhs: &Value, vm: &mut Vm) -> Result<Value, RuntimeError> {
		Ok(Self::new(self.get() + rhs.convert_to::<Self>(vm)?.get()).into())
	}
}

impl Subtract for Numeral {
	fn subtract(&self, rhs: &Value, vm: &mut Vm) -> Result<Value, RuntimeError> {
		Ok(Self::new(self.get() - rhs.convert_to::<Self>(vm)?.get()).into())
	}
}

impl Multiply for Numeral {
	fn multiply(&self, rhs: &Value, vm: &mut Vm) -> Result<Value, RuntimeError> {
		Ok(Self::new(self.get() * rhs.convert_to::<Self>(vm)?.get()).into())
	}
}

impl Divide for Numeral {
	fn divide(&self, rhs: &Value, vm: &mut Vm) -> Result<Value, RuntimeError> {
		self.get().checked_div(rhs.convert_to::<Self>(vm)?.get())
			.map(Self::new)
			.map(Into::into)
			.ok_or(RuntimeError::DivisionByZero)
	}
}

impl Modulo for Numeral {
	fn modulo(&self, rhs: &Value, vm: &mut Vm) -> Result<Value, RuntimeError> {
		self.get().checked_rem(rhs.convert_to::<Self>(vm)?.get())
			.map(|num| Value::Numeral(Self::new(num)))
			.ok_or(RuntimeError::DivisionByZero)
	}
}

impl Power for Numeral {
	fn power(&self, rhs: &Value, vm: &mut Vm) -> Result<Value, RuntimeError> {
		const U32MAX_AS_I64: i64 = u32::MAX as i64;

		match rhs.convert_to::<Self>(vm)?.get() {
			exp @ (i64::MIN..=-1) =>
				match self.get() {
					0                        => Err(RuntimeError::DivisionByZero),
					-1 if exp.abs() % 2 == 0 => Ok(Self::new(1).into()),
					1 | -1                   => Ok((*self).into()),
					_                        => Ok(Self::new(0).into())
				},
			exp @ (0..=U32MAX_AS_I64) => Ok(Self::new(self.get().pow(exp as u32)).into()),
			_ => Err(RuntimeError::OutOfBounds) // or should it be infinity or somethin? idk.
		}
	}
}

impl IsEqual for Numeral {
	fn is_equal(&self, rhs: &Value, _: &mut Vm) -> Result<bool, RuntimeError> {
		if let Value::Numeral(rhs) = rhs {
			Ok(*self == *rhs)
		} else {
			Ok(false)
		}
	}
}

impl Compare for Numeral {
	fn compare(&self, rhs: &Value, vm: &mut Vm) -> Result<Option<std::cmp::Ordering>, RuntimeError> {
		Ok(self.partial_cmp(&rhs.convert_to::<Self>(vm)?))
	}
}

impl GetAttr for Numeral {
	fn get_attr(&self, attr: &str, vm: &mut Vm) -> Result<Value, RuntimeError> {
		let _ = (attr, vm); todo!();
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
		unimplemented!()
	}
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash)]
pub enum RomanNumeral {
	N    =  0,
	I    = 1,
	V    = 5,
	X    = 10,
	L    = 50,
	C    = 100,
	D    = 500,
	M    = 1000,
	DD   = 5000,
	CCDD = 10_000,
	SEP  = 100001,
}

const ASCII_ROMAN: [char; 8] = [
	'N', 'I', 'V', 'X', 'L', 'C', 'D', 'M'
];

const UPPER_ROMAN_UNICODE: [char; 20] = [
	'Ⅰ', 'Ⅱ', 'Ⅲ', 'Ⅳ', 'Ⅴ', 'Ⅵ', 'Ⅶ', 'Ⅷ', 'Ⅸ', 'Ⅹ', 'Ⅺ', 'Ⅻ', 'Ⅼ', 'Ⅽ', 'Ⅾ', 'Ⅿ',
	'ↀ', 'ↁ', 'ↂ', 'Ↄ'
];

const LOWER_ROMAN_UNICODE: [char; 16] = [
	'ⅰ', 'ⅱ', 'ⅲ', 'ⅳ', 'ⅴ', 'ⅵ', 'ⅶ', 'ⅷ', 'ⅸ', 'ⅹ', 'ⅺ', 'ⅻ', 'ⅼ', 'ⅽ', 'ⅾ', 'ⅿ'
];

impl Display for RomanNumeral {
	fn fmt(&self, f: &mut Formatter<'_>) -> fmt::Result {
		if f.alternate() {
			match self {
				Self::N    => write!(f, "N"),
				Self::I    => write!(f, "Ⅰ"),
				Self::V    => write!(f, "Ⅴ"),
				Self::X    => write!(f, "Ⅹ"),
				Self::L    => write!(f, "Ⅼ"),
				Self::C    => write!(f, "Ⅽ"),
				Self::D    => write!(f, "Ⅾ"),
				Self::M    => write!(f, "Ⅿ"),
				Self::DD   => write!(f, "ↁ"),
				Self::CCDD => write!(f, "ↂ"),
				Self::SEP  => write!(f, "Ↄ"),
			}
		} else {
			match self {
				Self::N    => write!(f, "N"),
				Self::I    => write!(f, "I"),
				Self::V    => write!(f, "V"),
				Self::X    => write!(f, "X"),
				Self::L    => write!(f, "L"),
				Self::C    => write!(f, "C"),
				Self::D    => write!(f, "D"),
				Self::M    => write!(f, "M"),
				Self::DD   => write!(f, "DD"), // `|))` ?
				Self::CCDD => write!(f, "CCDD"), // `((|))` ?
				Self::SEP  => write!(f, "D"),
			}
		}
	}
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub enum NumeralParseError {
	Empty,
	UnexpectedStartingChar(char),
	BadTrailingChar(char)
}

impl Display for NumeralParseError {
	fn fmt(&self, f: &mut Formatter<'_>) -> fmt::Result {
		write!(f, "bad numeral text: ")?;

		match self {
			Self::Empty => write!(f, "an empty string was given"),
			Self::UnexpectedStartingChar(chr) => write!(f, "invalid leading character {:?}", chr),
			Self::BadTrailingChar(chr) => write!(f, "invalid trailing character {:?}", chr),
		}
	}
}

impl std::error::Error for NumeralParseError {}

impl Numeral {
	pub fn from_str_arabic(input: &str) -> Result<Self, NumeralParseError> {
		let mut chars = input.trim_start().chars();
		let mut is_neg = false;
		let mut numeral = 0;

		match chars.next() {
			Some('-') => is_neg = true,
			Some(digit) => numeral += digit.to_digit(10).ok_or(NumeralParseError::UnexpectedStartingChar(digit))? as i64,
			None => return Err(NumeralParseError::Empty)
		}

		for digit in chars.by_ref() {
			if digit == '_' {
				continue;
			} else if let Some(num) = digit.to_digit(10) {
				numeral = numeral * 10 + (num as i64);
			} else if digit.is_alphanumeric() {
				return Err(NumeralParseError::BadTrailingChar(digit))
			} else {
				break;
			}
		}

		if is_neg {
			numeral = -numeral;
		}

		Ok(Self::new(numeral))
	}

	pub fn from_str_roman(input: &str) -> Result<Self, NumeralParseError> {
		let input = input.trim();

		// todo: make this work with ascii and unicode
		let mut number = 0_i64;
		let mut stage = RomanNumeral::N;

		let mut chars = input.chars();

		let is_negative =
			if chars.next() == Some('-') {
				true
			} else {
				chars = input.chars();
				false
			};

		if input.chars().next() == Some('N') && !input.chars().skip(1).next().map_or(false, char::is_alphanumeric) {
			return Ok(Self::new(0));
		}

		while let Some(chr) = chars.next() {
			let parsed = 
				match chr {
					'I' => RomanNumeral::I,
					'V' => RomanNumeral::V,
					'X' => RomanNumeral::X,
					'L' => RomanNumeral::L,
					'C' => RomanNumeral::C,
					'D' => RomanNumeral::D,
					'M' => RomanNumeral::M,
					'_' => continue,
					_ => return Err(NumeralParseError::BadTrailingChar(chr))
				};

			number += parsed as i64;

			if stage == RomanNumeral::N || parsed <= stage {
				stage = parsed;
			} else {
				number -= (stage as i64) * 2;
			}
		}

		if is_negative {
			number = -number;
		}

		Ok(Self::new(number))
	}

	// we'll output unicode regardless of the roman numeral
	pub fn is_roman_numeral(chr: char) -> bool {
		ASCII_ROMAN.contains(&chr)
			|| UPPER_ROMAN_UNICODE.contains(&chr)
			|| LOWER_ROMAN_UNICODE.contains(&chr)
	}

	pub fn to_roman(self) -> String {
		if cfg!(feature="unicode-roman-numerals") {
			format!("{:#}", self.display_roman())
		} else {
			self.display_roman().to_string()
		}
	}

	pub fn to_arabic(self) -> String {
		self.display_arabic().to_string()
	}

	#[inline]
	pub fn display_roman(self) -> RomanDisplay {
		RomanDisplay(self)
	}

	#[inline]
	pub fn display_arabic(self) -> ArabicDisplay {
		ArabicDisplay(self)
	}
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct ArabicDisplay(Numeral);

impl Display for ArabicDisplay {
	#[inline]
	fn fmt(&self, f: &mut Formatter<'_>) -> fmt::Result {
		Display::fmt(&self.0, f)
	}
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct RomanDisplay(Numeral);

impl Display for RomanDisplay {
	// todo: adhere to padding and other fun stuff like that.
	fn fmt(&self, f: &mut Formatter<'_>) -> fmt::Result {
		const NONZERO_NUMERALS: [RomanNumeral; 9] = [
			RomanNumeral::I, RomanNumeral::V,
			RomanNumeral::X, RomanNumeral::L,
			RomanNumeral::C, RomanNumeral::D,
			RomanNumeral::M, RomanNumeral::DD,
			RomanNumeral::CCDD
		];

		if self.0 == 0 {
			return Display::fmt(&'N', f)
		} else if self.0 < 0 {
			write!(f, "-")?;
		}

		let mut numeral = self.0.abs().get() as u64;

		'outer:
		while numeral != 0 {
			if numeral <= 12 && f.alternate() {
				write!(f, "{}", UPPER_ROMAN_UNICODE[numeral as usize - 1])?;
				break;
			}

			for (i, &suffix) in NONZERO_NUMERALS.iter().enumerate().rev() {
				if (suffix as u64) <= numeral {
					break; // nothing else will match,  as suffixes shrink.
				}

				let mut smallest: Option<(RomanNumeral, u64)> = None;

	
				for &prefix in NONZERO_NUMERALS[..i].iter() {
					// dont use prefixes as replacements
					if prefix == RomanNumeral::V || prefix == RomanNumeral::L || prefix == RomanNumeral::D {
						continue;
					}

					let added = numeral + prefix as u64;

					if (suffix as u64) <= added {
						let diff = added - (suffix as u64);

						if smallest.map_or(true, |(_, old)| diff < old) {
							smallest = Some((prefix, diff));
						}
					}
				}

				if let Some((smallest, _)) = smallest {
					Display::fmt(&smallest, f)?;
					numeral += smallest as u64;
				}
			}

			for &roman_numeral in NONZERO_NUMERALS.iter().rev() {
				if (roman_numeral as u64) <= numeral {
					Display::fmt(&roman_numeral, f)?;
					numeral -= roman_numeral as u64;
					continue 'outer;
				}
			}

			unreachable!()
		}

		Ok(())
	}
}

impl FromStr for Numeral {
	type Err = NumeralParseError;

	fn from_str(input: &str) -> Result<Self, Self::Err> {
		let mut input = input.trim_start();

		match Self::from_str_arabic(&mut input) {
			Err(NumeralParseError::UnexpectedStartingChar(_)) => Self::from_str_roman(&mut input),
			other => other
		}
	}
}