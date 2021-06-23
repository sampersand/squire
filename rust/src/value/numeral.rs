use std::fmt::{self, Display, Formatter};
use std::str::FromStr;

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

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash)]
pub enum RomanNumeral {
	N   =  0,
	I    = 1,
	V    = 5,
	X    = 10,
	L    = 50,
	C    = 100,
	D    = 500,
	M    = 1000,
	DD   = 5000,
	CCDD = 10_000,
	SEP = 100001,
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
				Self::DD   => write!(f, "DD"),
				Self::CCDD  => write!(f, "CCDD"),
				Self::SEP  => write!(f, "D"),
			}
		}
	}
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub enum NumeralParseError {
	Empty,
	UnexpectedStartingChar,
	BadTrailingChar(char)
}

impl Numeral {
	pub fn from_str_arabic(input: &str) -> Result<Self, NumeralParseError> {
		let mut chars = input.trim_start().chars();
		let mut is_neg = false;
		let mut numeral = Self::default();

		match chars.next() {
			Some('-') => is_neg = true,
			Some(digit) => numeral += digit.to_digit(10).ok_or(NumeralParseError::UnexpectedStartingChar)? as i64,
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

		Ok(numeral)
	}

	pub fn from_str_roman(_input: &str) -> Result<Self, NumeralParseError> {
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
				if (suffix as u64) < numeral {
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
			Err(NumeralParseError::UnexpectedStartingChar) => Self::from_str_roman(&mut input),
			other => other
		}
	}
}