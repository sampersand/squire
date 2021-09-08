mod error;
mod stream;
pub mod token;

pub use error::{Error, ErrorKind, Result};
pub use stream::{Stream, Context};
pub use token::{Token, TokenKind, Literal, LiteralKind, Tokenizer};

pub trait Parsable : Sized {
	const TYPE_NAME: &'static str;

	fn parse<I: Iterator<Item=char>>(parser: &mut Parser<'_, I>) -> Result<Option<Self>>;

	fn expect_parse<I: Iterator<Item=char>>(parser: &mut Parser<'_, I>) -> Result<Self> {
		Self::parse(parser)?.ok_or_else(|| parser.error(ErrorKind::MissingRequiredAst(Self::TYPE_NAME)))
	}
}

#[derive(Debug)]
pub struct Parser<'a, I> {
	tokenizer: &'a mut Tokenizer<'a, I>
}

pub trait TokenPattern : Copy {
	fn matches(&self, token: &Token) -> bool;
	fn to_kinds(self, out: &mut Vec<TokenKind>);
}

impl<'a, I: Iterator<Item=char>> Parser<'a, I> {
	pub fn new(tokenizer: &'a mut Tokenizer<'a, I>) -> Self {
		Self { tokenizer }
	}

	pub fn next_token(&mut self) -> Result<Option<Token>> {
		self.tokenizer.next().transpose()
	}

	pub fn undo_next_token(&mut self) {
		self.tokenizer.undo();
	}

	#[doc(hidden)]
	pub fn _hack_is_next_token_colon(&mut self) -> bool {
		// this wouldn't be necessary if `parser` also kept state
		self.tokenizer._hack_is_next_token_colon()
	}


	pub fn error(&self, error: impl Into<ErrorKind>) -> Error {
		self.tokenizer.error(error)
	}

	pub fn expect_identifier(&mut self) -> Result<String> {
		if let Token::Identifier(ident) = self.expect(TokenKind::Identifier)? {
			Ok(ident)
		} else {
			unreachable!()
		}
	}

	pub fn expect_identifier_or_operator(&mut self) -> Result<String> {
		use token::{Symbol as Sym, ParenKind};
		use TokenKind as Tk;
		use Token as Tkn;

		Ok(match self.expect([
			Tk::Identifier,
			Tk::Symbol(Sym::EqualEqual),
			Tk::Symbol(Sym::NotEqual),
			Tk::Symbol(Sym::LessThan),
			Tk::Symbol(Sym::LessThanOrEqual),
			Tk::Symbol(Sym::GreaterThan),
			Tk::Symbol(Sym::GreaterThanOrEqual),
			Tk::Symbol(Sym::Compare),

			Tk::Symbol(Sym::Plus),
			Tk::Symbol(Sym::Hyphen),
			Tk::Symbol(Sym::Asterisk),
			Tk::Symbol(Sym::AsteriskAsterisk),
			Tk::Symbol(Sym::Solidus),
			Tk::Symbol(Sym::PercentSign),
			Tk::Symbol(Sym::Exclamation),
			Tk::LeftParen(ParenKind::Square),
			Tk::LeftParen(ParenKind::Round),
		])? {
			Tkn::Identifier(ident) => ident,
			Tkn::Symbol(Sym::EqualEqual) => "==".to_string(),
			Tkn::Symbol(Sym::NotEqual) => "!=".to_string(),
			Tkn::Symbol(Sym::LessThan) => "<".to_string(),
			Tkn::Symbol(Sym::LessThanOrEqual) => "<=".to_string(),
			Tkn::Symbol(Sym::GreaterThan) => ">".to_string(),
			Tkn::Symbol(Sym::GreaterThanOrEqual) => ">=".to_string(),
			Tkn::Symbol(Sym::Compare) => "<=>".to_string(),

			Tkn::Symbol(Sym::Plus) => "+".to_string(),
			Tkn::Symbol(Sym::Hyphen) => "-".to_string(),
			Tkn::Symbol(Sym::Asterisk) => "*".to_string(),
			Tkn::Symbol(Sym::AsteriskAsterisk) => "**".to_string(),
			Tkn::Symbol(Sym::Solidus) => "/".to_string(),
			Tkn::Symbol(Sym::PercentSign) => "%".to_string(),
			Tkn::Symbol(Sym::Exclamation) => "!".to_string(),
			Tkn::LeftParen(ParenKind::Round) => {
				self.expect(Tk::RightParen(ParenKind::Round))?;
				"()".to_string()
			},
			Tkn::LeftParen(ParenKind::Square) => {
				self.expect(Tk::RightParen(ParenKind::Square))?;
				if self.guard(Tk::Symbol(Sym::Equal))?.is_some() {
					"[]=".to_string()
				} else {
					"[]".to_string()
				}
			}

			other => unreachable!("`parser.expect` returned a bad token: {:?}", other)
		})
	}

	pub fn guard_identifier(&mut self) -> Result<Option<String>> {
		match self.guard(TokenKind::Identifier)? {
			Some(Token::Identifier(ident)) => Ok(Some(ident)),
			Some(_) => unreachable!(),
			None => Ok(None)
		}
	}

	pub fn expect<P: TokenPattern>(&mut self, patterns: P) -> Result<Token> {
		if let Some(token) = self.guard(patterns)? {
			return Ok(token)
		}

		let given = self.next_token().unwrap();
		let mut expected = Vec::new();
		patterns.to_kinds(&mut expected);

		Err(self.error(ErrorKind::BadToken { given, expected }))
	}

	pub fn guard<P: TokenPattern>(&mut self, pattern: P) -> Result<Option<Token>> {
		if let Some(token) = self.next_token()? {
			if pattern.matches(&token) {
				return Ok(Some(token));
			}

			self.undo_next_token();
		}

		Ok(None)
	}

	pub fn collect<T, F>(&mut self, mut func: F) -> Result<Vec<T>>
	where
		F: FnMut(&mut Self) -> Result<Option<T>>
	{
		std::iter::from_fn(|| func(self).transpose()).collect()
	}

	// pub fn collect_until<T, C, F>(&mut self, close: C, mut func: F) -> Result<Vec<T>>
	// where
	// 	C: TokenPattern,
	// 	F: FnMut(&mut Self) -> Result<Option<T>>
	// {
	// 	todo!()
	// 	// std::iter::from_fn(|| func(self).transpose()).collect()
	// }


	pub fn take_enclosed<T, O, C, F>(&mut self, open: O, close: C, mut func: F) -> Result<Option<T>>
	where
		O: TokenPattern,
		C: TokenPattern,
		F: FnMut(&mut Self) -> Result<T>
	{
		if self.guard(open)?.is_none() {
			Ok(None)
		} else {
			func(self).and_then(|t| self.expect(close).and(Ok(Some(t))))
		}
	}

	pub fn take_paren_group<T, F>(&mut self, kind: token::ParenKind, func: F) -> Result<Option<Vec<T>>>
	where
		F: FnMut(&mut Self) -> Result<T>
	{
		self.take_group(TokenKind::LeftParen(kind), TokenKind::RightParen(kind), func)
	}

	pub fn take_group<T, O, C, F>(&mut self, open: O, close: C, func: F) -> Result<Option<Vec<T>>>
	where
		O: TokenPattern,
		C: TokenPattern,
		F: FnMut(&mut Self) -> Result<T>
	{
		if self.guard(open)?.is_none() {
			Ok(None)
		} else {
			self.take_group_opened(open, close, func).map(Some)
		}
	}

	pub fn take_group_opened<T, O, C, F>(&mut self, open: O, close: C, mut func: F) -> Result<Vec<T>>
	where
		O: TokenPattern,
		C: TokenPattern,
		F: FnMut(&mut Self) -> Result<T>
	{
		let mut depth = 1;
		let mut acc = Vec::new();

		while depth != 0 {
			// todo: somehow handle whether we're empty or not (maybe?)
			if self.guard(open)?.is_some() {
				depth += 1
			} else if self.guard(close)?.is_some() {
				depth -= 1
			} else {
				acc.push(func(self)?);
			}
		}

		Ok(acc)
	}


	pub fn take_separated<T, S, C, F>(&mut self, sep: S, close: C, mut func: F) -> Result<Vec<T>>
	where
		S: TokenPattern,
		C: TokenPattern,
		F: FnMut(&mut Self) -> Result<T>
	{
		let mut acc = Vec::new();

		while self.guard(close)?.is_none() {
			acc.push(func(self)?);

			// if we have a separator, we can just continue.
			if self.guard(sep)?.is_some() {
				continue;
			}

			// We now don't have a separator. If we don't have a `close`, then it's an error.
			self.expect(close)?;
			break;
		}

		Ok(acc)
	}
}


impl TokenPattern for TokenKind {
	fn matches(&self, token: &Token) -> bool {
		match (self, token) {
			(TokenKind::Keyword(lhs), Token::Keyword(rhs)) => lhs == rhs,
			(TokenKind::Symbol(lhs), Token::Symbol(rhs)) => lhs == rhs,
			(TokenKind::LeftParen(lhs), Token::LeftParen(rhs)) => lhs == rhs,
			(TokenKind::RightParen(lhs), Token::RightParen(rhs)) => lhs == rhs,
			(TokenKind::Literal(LiteralKind::Any), Token::Literal(_)) => true,
			(TokenKind::Literal(LiteralKind::Ni), Token::Literal(Literal::Ni)) => true,
			(TokenKind::Literal(LiteralKind::Veracity), Token::Literal(Literal::Boolean(_))) => true,
			(TokenKind::Literal(LiteralKind::Numeral), Token::Literal(Literal::Numeral(_))) => true,
			(TokenKind::Literal(LiteralKind::Text), Token::Literal(Literal::Text(_))) => true,
			(TokenKind::Identifier, Token::Identifier(_)) => true,
			_ => false
		}
	}

	fn to_kinds(self, kinds: &mut Vec<TokenKind>) {
		kinds.push(self)
	}
}

impl<P: TokenPattern, const N: usize> TokenPattern for [P; N] {
	fn matches(&self, token: &Token) -> bool {
		self.iter().any(|pattern| pattern.matches(token))
	}

	fn to_kinds(self, kinds: &mut Vec<TokenKind>) {
		for pattern in self {
			pattern.to_kinds(kinds);
		}
	}
}