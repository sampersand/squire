mod error;
mod stream;
pub mod token;

pub use error::{Error, ErrorKind, Result};
pub use stream::Stream;
pub use token::{Token, TokenKind, Tokenizer};

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

pub trait TokenPattern {
	fn matches(&self, token: &Token) -> bool;
	fn to_kinds(&self, out: &mut Vec<TokenKind>);
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

	pub fn error(&self, error: impl Into<ErrorKind>) -> Error {
		self.tokenizer.error(error)
	}

	pub fn require_ident(&mut self) -> Result<String> {
		if let Token::Identifier(ident) = self.require(&TokenKind::Identifier)? {
			Ok(ident)
		} else {
			unreachable!()
		}
	}

	pub fn require<P: TokenPattern + ?Sized>(&mut self, patterns: &P) -> Result<Token> {
		if let Some(token) = self.guard(patterns)? {
			return Ok(token)
		}

		let given = self.next_token().unwrap();
		let mut expected = Vec::new();
		patterns.to_kinds(&mut expected);

		Err(self.error(ErrorKind::BadToken { given, expected }))
	}

	pub fn guard<P: TokenPattern + ?Sized>(&mut self, pattern: &P) -> Result<Option<Token>> {
		if let Some(token) = self.next_token()? {
			if pattern.matches(&token) {
				return Ok(Some(token));
			}

			self.undo_next_token();
		}

		Ok(None)
	}

	pub fn parse_statements<T, F>(&mut self) -> Result<crate::ast::statement::Statements> {
		self.take_paren_group(token::ParenKind::Curly, crate::ast::Statement::expect_parse)?
			.ok_or_else(|| self.error("expected statements, none given..."))
	}

	pub fn take_paren_group<T, F>(&mut self, kind: token::ParenKind, func: F) -> Result<Option<Vec<T>>>
	where
		F: FnMut(&mut Self) -> Result<T>
	{
		self.take_group(&Token::LeftParen(kind), &Token::RightParen(kind), func)
	}

	pub fn take_group<T, O, C, F>(&mut self, open: &O, close: &C, mut func: F) -> Result<Option<Vec<T>>>
	where
		O: TokenPattern + ?Sized,
		C: TokenPattern + ?Sized,
		F: FnMut(&mut Self) -> Result<T>
	{
		if self.guard(open)?.is_none() {
			return Ok(None)
		}

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

		Ok(Some(acc))
	}

	pub fn take_separated<T, S, C, F>(&mut self, sep: &S, close: &C, mut func: F) -> Result<Vec<T>>
	where
		S: TokenPattern + ?Sized,
		C: TokenPattern + ?Sized,
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
			self.require(close)?;
			break;
		}

		Ok(acc)
	}
}


impl TokenPattern for Token {
	fn matches(&self, token: &Token) -> bool {
		self == token
	}

	fn to_kinds(&self, kinds: &mut Vec<TokenKind>) {
		match self {
			Token::Keyword(keyword) => kinds.push(TokenKind::Keyword(*keyword)),
			Token::Symbol(symbol) => kinds.push(TokenKind::Symbol(*symbol)),
			Token::LeftParen(paren) => kinds.push(TokenKind::LeftParen(*paren)),
			Token::RightParen(paren) => kinds.push(TokenKind::RightParen(*paren)),
			Token::Literal(_) => kinds.push(TokenKind::Literal),
			Token::Identifier(_) => kinds.push(TokenKind::Identifier),
		}
	}
}

impl TokenPattern for TokenKind {
	fn matches(&self, token: &Token) -> bool {
		match (self, token) {
			(TokenKind::Keyword(lhs), Token::Keyword(rhs)) => lhs == rhs,
			(TokenKind::Symbol(lhs), Token::Symbol(rhs)) => lhs == rhs,
			(TokenKind::LeftParen(lhs), Token::LeftParen(rhs)) => lhs == rhs,
			(TokenKind::RightParen(lhs), Token::RightParen(rhs)) => lhs == rhs,
			(TokenKind::Literal, Token::Literal(_)) => true,
			(TokenKind::Identifier, Token::Identifier(_)) => true,
			_ => false
		}
	}

	fn to_kinds(&self, kinds: &mut Vec<TokenKind>) {
		kinds.push(*self)
	}
}

impl<P: TokenPattern> TokenPattern for [P] {
	fn matches(&self, token: &Token) -> bool {
		self.iter().any(|pattern| pattern.matches(token))
	}

	fn to_kinds(&self, kinds: &mut Vec<TokenKind>) {
		for pattern in self {
			pattern.to_kinds(kinds);
		}
	}
}