use crate::value::{Numeral, Text};
use super::{Stream, Result, ErrorKind};

mod macros;
use macros::Macros;

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum Keyword {
	Form,
	Change,
	Matter,
	Essence,
	Recall,
	Imitate,
	Journey,

	Renowned,
	Nigh,

	If,
	Alas,
	Whence,
	Whilst,
	Reward,
	Attempt,
	Notwithstanding,
	Catapult,
	Fork,
	Path,
	Challenge,
}

impl Keyword {
	pub const fn repr(self) -> &'static str {
		match self {
			Self::Form => "form",
			Self::Change => "change",
			Self::Matter => "matter",
			Self::Essence => "essence",
			Self::Recall => "recall",
			Self::Imitate => "imitate",
			Self::Journey => "journey",

			Self::Renowned => "renowned",
			Self::Nigh => "nigh",

			Self::If => "if",
			Self::Alas => "alas",
			Self::Whence => "whence",
			Self::Whilst => "whilst",
			Self::Reward => "reward",
			Self::Attempt => "attempt",
			Self::Notwithstanding => "notwithstanding",
			Self::Catapult => "catapult",
			Self::Fork => "fork",
			Self::Path => "path",
			Self::Challenge => "challenge",
		}
	}
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum ParenKind {
	Round,
	Square,
	Curly
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum Symbol {
	Endline,
	Comma,
	Colon,
	Dot,
	Equal,

	EqualEqual,
	NotEqual,
	LessThan,
	LessThanOrEqual,
	GreaterThan,
	GreaterThanOrEqual,
	Compare,

	Plus,
	PlusEqual,
	Hyphen,
	HyphenEqual,
	Asterisk,
	AsteriskEqual,
	AsteriskAsterisk,
	AsteriskAsteriskEqual,
	Solidus,
	SolidusEqual,
	PercentSign,
	PercentSignEqual,

	Exclamation,
	AndAnd,
	OrOr,
	Pipe
}

#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub enum Literal {
	Ni,
	Boolean(bool),
	Numeral(Numeral),
	Text(Text),
	TextInterpolation(Vec<(Text, Vec<Token>)>, Text),
}

#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub enum Token {
	Keyword(Keyword),
	Symbol(Symbol),
	LeftParen(ParenKind),
	RightParen(ParenKind),
	Literal(Literal),
	Identifier(String),
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum LiteralKind {
	Any,
	Ni,
	Veracity,
	Numeral,
	Text
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum TokenKind {
	Keyword(Keyword),
	Symbol(Symbol),
	LeftParen(ParenKind),
	RightParen(ParenKind),
	Literal(LiteralKind),
	Identifier,
}

#[derive(Debug)]
pub struct Tokenizer<'a, I> {
	stream: &'a mut Stream<'a, I>,
	macros: Macros,
	last: Option<Token>,
	put_back: bool
}

impl<'a, I> Tokenizer<'a, I> {
	pub fn new(stream: &'a mut Stream<'a, I>) -> Self {
		Self { stream, macros: Macros::default(), last: None, put_back: false }
	}
}

impl<I: Iterator<Item=char>> Tokenizer<'_, I> {
	pub const TRUE: &'static str            = "yay";
	pub const FALSE: &'static str           = "nay";
	pub const NULL: &'static str            = "ni";

	pub fn error(&self, error: impl Into<ErrorKind>) -> super::Error {
		self.stream.error(error)
	}

	#[doc(hidden)]
	pub fn _hack_is_next_token_colon(&mut self) -> bool {
		self.stream.strip_whitespace_and_comments();
		self.stream.peek() == Some(':')
	}

	pub fn next_keyword(&mut self) -> Option<Keyword> {
		macro_rules! keyword {
			($name:ident) => {
				if self.stream.take_identifier(Keyword::$name.repr()) {
					return Some(Keyword::$name)
				}
			};
		}

		keyword!(Form);
		keyword!(Change);
		keyword!(Matter);
		keyword!(Essence);
		keyword!(Recall);
		keyword!(Imitate);
		keyword!(Journey);

		keyword!(Renowned);
		keyword!(Nigh);

		keyword!(If);
		keyword!(Alas);
		keyword!(Whence);
		keyword!(Whilst);
		keyword!(Reward);
		keyword!(Attempt);
		keyword!(Notwithstanding);
		keyword!(Catapult);
		keyword!(Fork);
		keyword!(Path);
		keyword!(Challenge);

		None
	}

	fn parse_arabic_numeral(&mut self) -> Option<Result<Numeral>> {
		use crate::value::numeral::NumeralParseError;

		let parsed = self.stream.take_while(|chr| chr.is_ascii_digit() || chr == '_')?;

		match self.stream.peek() {
			Some(chr) if chr.is_alphanumeric() => Some(Err(self.error(NumeralParseError::BadTrailingChar(chr)))),
			_ => Some(Ok(parsed.parse().unwrap()))
		}
	}

	fn parse_roman_numeral(&mut self) -> Option<Result<Numeral>> {
		let parsed = self.stream.take_while(|chr| Numeral::is_roman_numeral(chr) || chr == '_')?;

		// if it's an identifier, then don't raise an error.
		if self.stream.peek().map_or(false, char::is_alphanumeric) {
			self.stream.put_back(parsed.chars());
			return None;
		}

		Some(parsed.parse().map_err(|err| self.error(err)))
	}

	fn parse_numeral(&mut self) -> Option<Result<Token>> {
		self.parse_roman_numeral()
			.or_else(|| self.parse_arabic_numeral())
			.map(|val| val.map(Literal::Numeral).map(Token::Literal))
	}

	fn parse_fraktur(&mut self) -> Result<Text> {
		use crate::value::text::is_fraktur;

		let fraktur = self.stream.take_while(|chr| is_fraktur(chr) || chr.is_whitespace()).unwrap();

		if self.stream.peek().map_or(false, |chr| chr.is_alphanumeric()) {
			Err(self.error(ErrorKind::BadFrakturSuffix))
		} else {
			Ok(Text::new_fraktur(fraktur.trim().to_string()))
		}
	}

	pub fn undo(&mut self) {
		assert!(!self.put_back);
		self.put_back = true;
	}

	fn parse_quoted(&mut self) -> Result<Token> {
		let mut text = String::new();

		let quote = self.stream.next().unwrap();
		debug_assert!(quote == '\'' || quote == '\"');

		let mut interpolations = vec![];

		while let Some(chr) = self.stream.next() {
			if chr == quote {
				break;
			} else if chr != '\\' {
				text.push(chr);
				continue;
			} else if quote == '\'' {
				// for single quoting, we only have basic escapes
				match self.stream.next().ok_or_else(|| self.error(ErrorKind::UnterminatedEscapeSequence))? {
					chr @ ('\\' | '\'') => text.push(chr),
					other => { text.push('\\'); text.push(other); }
				}

				continue;
			}

			macro_rules! next_hex_char {
				() => {
					match self.stream.next().map(|chr| (chr, chr.to_digit(16))) {
						Some((_, Some(digit))) => Ok(digit),
						Some((bad, None)) => Err(self.error(ErrorKind::InvalidHexDigit(bad))),
						None => Err(self.error(ErrorKind::UnterminatedEscapeSequence))
					};
				}
			}

			match self.stream.next().ok_or_else(|| self.error(ErrorKind::UnterminatedEscapeSequence))? {
				chr @ ('\\' | '\"' | '\'') => text.push(chr),
				'\r' if self.stream.next() == Some('\n') => continue, // ignore `\` at the end of lines
				'\n' => continue, // ignore `\` at the end of lines
				'n' => text.push('\n'),
				't' => text.push('\t'),
				'r' => text.push('\r'),
				'f' => text.push('\t'),
				'0' => text.push('\0'),
				'x' => {
					let upper = next_hex_char!()? * 0x10;
					let lower = next_hex_char!()? * 0x00;
					let escape = upper | lower;
					text.push(char::from_u32(escape).unwrap());
				},
				'u' => {
					let uppermost = next_hex_char!()? * 0x30;
					let upper     = next_hex_char!()? * 0x20;
					let lower     = next_hex_char!()? * 0x10;
					let lowermost = next_hex_char!()? * 0x00;
					let escape = uppermost | upper | lower | lowermost;

					text.push(char::from_u32(escape).ok_or_else(|| self.error(ErrorKind::InvalidHexEscape(escape)))?);
				},
				'(' => {
					let mut inner = vec![];
					let mut nesting = 1;

					loop {
						let token = self.next()
							.unwrap_or_else(|| Err(self.error(ErrorKind::UnterminatedEscapeSequence)))?;

						if token == Token::LeftParen(ParenKind::Round) {
							nesting += 1;
						} else if token == Token::RightParen(ParenKind::Round) {
							nesting -= 1;
							if nesting == 0 {
								break;
							}
						}

						inner.push(token);
					}

					interpolations.push((Text::new(text), inner));
					text = String::new();
				},
				other => return Err(self.error(ErrorKind::UnknownEscapeCharacter(other)))
			}
		}

		if interpolations.is_empty() {
			Ok(Token::Literal(Literal::Text(Text::new(text))))
		} else {
			Ok(Token::Literal(Literal::TextInterpolation(interpolations, Text::new(text))))
		}
	}

	fn parse_text(&mut self) -> Option<Result<Token>> {
		let peeked = self.stream.peek()?;

		if crate::value::text::is_fraktur(peeked) {
			Some(self.parse_fraktur().map(Literal::Text).map(Token::Literal))
		} else if peeked == '\'' || peeked == '\"' {
			Some(self.parse_quoted())
		} else {
			None
		}
	}

	pub fn next_literal(&mut self) -> Option<Result<Token>> {
		if let Some(numeral) = self.parse_numeral() {
			Some(numeral)
		} else if let Some(text) = self.parse_text() {
			Some(text)
		} else if self.stream.take_identifier(Self::TRUE) {
			Some(Ok(Token::Literal(Literal::Boolean(true))))
		} else if self.stream.take_identifier(Self::FALSE) {
			Some(Ok(Token::Literal(Literal::Boolean(false))))
		} else if self.stream.take_identifier(Self::NULL) {
			Some(Ok(Token::Literal(Literal::Ni)))
		} else {
			None
		}
	}

	pub fn next_identifier(&mut self) -> Option<String> {
		fn is_alpha(c: char) -> bool {
			c.is_alphabetic() || c == '_'
		}

		if !self.stream.peek().map_or(false, is_alpha) {
			return None;
		}

		let mut identifier = String::new();
		let mut replace_camel = false;
		let mut has_non_underscore_been_reached = false;

		while let Some(chr) = self.stream.next() {
			// we only replace camelcase if the first non-underscore character is lowercase
			if !has_non_underscore_been_reached {
				has_non_underscore_been_reached |= chr != '_';

				if chr.is_lowercase() {
					replace_camel = true;
				}
			}

			match chr {
				_ if chr.is_uppercase() && replace_camel => {
					identifier.push('_');
					identifier.extend(chr.to_lowercase());
				},
				_ if chr.is_alphanumeric() || chr == '_' => identifier.push(chr),
				'-' => {
					if self.stream.peek().map_or(false, is_alpha) {
						identifier.push('_');
					} else {
						self.stream.put_back(['-']);
						break;
					}
				},
				' ' | '\t' => {
					let coll = self.stream.take_while(|c| c == ' ' || c == '\t').unwrap_or_default();
					if self.stream.peek().map_or(false, is_alpha) {
						identifier.push('_');
					} else {
						self.stream.put_back(coll.chars());
						break
					}
				},
				_ => {
					self.stream.put_back([chr]);
					break;
				}
			}
		}

		Some(identifier)
	}

	fn next_macro_invocation(&mut self) -> Option<Result<()>> {
		if self.stream.peek() != Some('@') {
			return None;
		}

		self.stream.strip_whitespace_and_comments();

		if let Some(identifier) = self.next_identifier() {
			// Some(self.parse_macro_invocation_for(&identifier))
			let _ = identifier;
			unimplemented!();
		} else {
			Some(Err(self.error("no macro invocation supplied")))
		}
	}

	fn next_macro_variable(&mut self) -> Option<String> {
		if self.stream.peek() != Some('$') {
			None
		} else {
			self.stream.strip_whitespace_and_comments();
			Some(self.next_identifier().unwrap())
		}
	}

	fn parse_macro_variable(&mut self) -> Result<Token> {
		unimplemented!();
	}

	fn next_from_stream(&mut self) -> Option<Result<Token>> {
		macro_rules! if_equals {
			($if_eql:ident, $if_not:ident) => {
				if self.stream.take_prefix("=") {
					Token::Symbol(Symbol::$if_eql)
				} else {
					Token::Symbol(Symbol::$if_not)
				}
			};
		}

		self.stream.strip_whitespace_and_comments();

		if self.stream.take_identifier("@__END__") {
			self.stream.put_back("@__END__".chars());
			return None;
		}

		if let Some(kw) = self.next_keyword() {
			return Some(Ok(Token::Keyword(kw)));
		} else if let Some(literal) = self.next_literal() {
			return Some(literal);
		} else if let Some(invocation) = self.next_macro_invocation() {
			if let Err(err) = invocation {
				return Some(Err(err));
			} else {
				return self.next();
			}
		} else if let Some(identifier) = self.next_identifier() {
			return Some(Ok(Token::Identifier(identifier)))
		}
		// } else if let Some(identifier) = self.next_macro_variable() {
		// 	self.parse_

		Some(Ok(match self.stream.next()? {
			// macros
			'$' => return Some(self.parse_macro_variable()),

			// parens
			'(' => Token::LeftParen(ParenKind::Round),
			'[' => Token::LeftParen(ParenKind::Square),
			'{' => Token::LeftParen(ParenKind::Curly),
			')' => Token::RightParen(ParenKind::Round),
			']' => Token::RightParen(ParenKind::Square),
			'}' => Token::RightParen(ParenKind::Curly),

			// symbols
			';' => Token::Symbol(Symbol::Endline),
			',' => Token::Symbol(Symbol::Comma),
			':' => Token::Symbol(Symbol::Colon),
			'.' => Token::Symbol(Symbol::Dot),
			'=' => if_equals!(EqualEqual, Equal),
			'!' => if_equals!(NotEqual, Exclamation),
			'<' => 
				if self.stream.take_prefix("=>") {
					Token::Symbol(Symbol::Compare)
				} else {
					if_equals!(LessThanOrEqual, LessThan)
				},
			'>' => if_equals!(GreaterThanOrEqual, GreaterThan),
			'+' => if_equals!(PlusEqual, Plus),
			'-' => if_equals!(HyphenEqual, Hyphen),
			'*' => 
				if self.stream.take_prefix("*") {
					if_equals!(AsteriskAsteriskEqual, AsteriskAsterisk)
				} else {
					if_equals!(AsteriskEqual, Asterisk)
				},
			'/' => if_equals!(SolidusEqual, Solidus),
			'%' => if_equals!(PercentSignEqual, PercentSign),
			'&' if self.stream.take_prefix("&") => Token::Symbol(Symbol::AndAnd),
			'|' =>
				if self.stream.take_prefix("|") {
					Token::Symbol(Symbol::OrOr)
				} else {
					Token::Symbol(Symbol::Pipe)
				},
			other => return Some(Err(self.error(ErrorKind::UnknownTokenStart(other))))
		}))
	}
}

impl<I: Iterator<Item=char>> Iterator for Tokenizer<'_, I> {
	type Item = Result<Token>;

	fn next(&mut self) -> Option<Self::Item> {
		if self.put_back {
			self.put_back = false;
			self.last.clone().map(Ok)
		} else {
			Some(self.next_from_stream()?.map(|value| { 
				self.last = Some(value.clone());
				value
			}))
		}
	// if let Some(macro_) = self.macros.last_mut() {
		// macro_.next().map(Ok).or_else(|| self.next())
	// } else {
	}
}
