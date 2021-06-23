use std::collections::HashMap;
use super::{Tokenizer, Token, token::*, Result};

pub type Statements = Vec<Statement>;

#[derive(Debug)]
#[non_exhaustive]
pub enum Statement {
	Class(Class),
	Func(Function),
	TryCatch(TryCatch),
	Throw(Throw),
	Return(Return),

	Global(Scope),
	Local(Scope),

	If(If),
	While(While),
	Switch(Switch),
	Label(String),
	ComeFrom(String),

	Expression(Expression),
}

#[derive(Debug)]
pub struct Scope {
	pub name: String,
	pub initializer: Option<Expression>
}

#[derive(Debug)]
pub struct Throw(pub Expression);

#[derive(Debug)]
pub struct Class {
	pub name: String,

	pub essences: HashMap<String, Expression>,
	pub methods: Vec<Function>,

	pub fields: Vec<String>,
	pub functions: Vec<Function>,
	pub constructor: Option<Function>
}

#[derive(Debug)]
pub struct Function {
	pub name: String,
	pub args: Vec<String>,
	pub body: Statements
}

#[derive(Debug)]
pub struct If {
	pub condition: Expression,
	pub if_true: Statements,
	pub if_false: Option<Statements>,
}

#[derive(Debug)]
pub struct Switch {
	pub condition: Expression,
	pub cases: Vec<(Expression, Statements)>,
	pub alas: Option<Statements>
}

#[derive(Debug)]
pub struct While {
	pub condition: Expression,
	pub body: Statements
}

#[derive(Debug)]
pub struct Return {
	pub value: Option<Expression>
}

#[derive(Debug)]
pub struct TryCatch {
	pub try_block: Statements,
	pub catch_block: Statements,
	pub exception: String
}

#[derive(Debug)]
#[non_exhaustive]
pub enum BinaryOperator {
	Add, Sub, Mul, Div, Mod, Pow,
	Eql, Neq, Lth, Gth, Leq, Geq, Cmp,
}

#[derive(Debug)]
#[non_exhaustive]
pub enum Expression {
	Assignment(Box<Assignment>),
	BinaryOperator(BinaryOperator, Vec<Expression>),
	Primary(Primary)
}

#[derive(Debug)]
pub enum Assignment {
	Simple { variable: String, value: Expression },
	Index { into: Primary, key: Expression, value: Expression },
	SetAttr { into: Primary, key: String, value: Expression },
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum UnaryOperator {
	Pos, Neg, Not	
}

#[derive(Debug)]
#[non_exhaustive]
pub enum Primary {
	UnaryOperator(UnaryOperator, Box<Expression>),
	FunctionCall(Box<Primary>, Vec<Expression>),
	Index(Box<Primary>, Vec<Expression>),
	Grouping(Statements),
	Lambda(Function),
	Literal(super::token::Literal),
	Array(Vec<Expression>),
	Codex(Vec<(Expression, Expression)>),
	Variable(String),
	GetAttr(Box<Primary>, String),
}

pub trait Parsable : Sized {
	fn parse<I: Iterator<Item=char>>(tokenizer: &mut Tokenizer<I>) -> Result<Option<Self>>;
}

macro_rules! expect {
	($tokenizer:expr, $($rest:tt)*) => {
		match $tokenizer.next().transpose()? {
			Some(what @ $($rest)*) => what,
			other => return expected!($tokenizer, given other, expected $($rest)*)
		}
	};
}

macro_rules! expected {
	($tokenizer:expr, given $given:expr, expected $($expected:expr),*) => {
		Err($tokenizer.error(super::ErrorKind::BadToken { given: $given, expected: vec![$($expected),*] }))
	};
}

impl Statement {
	fn parse_statements<I: Iterator<Item=char>>(tokenizer: &mut Tokenizer<I>) -> Result<Statements> {
		let _ = tokenizer;
		todo!()
	}
}

impl Parsable for Class {
	fn parse<I: Iterator<Item=char>>(tokenizer: &mut Tokenizer<I>) -> Result<Option<Self>> {
		let _ = tokenizer; todo!();
	}
}

impl Parsable for Function {
	fn parse<I: Iterator<Item=char>>(tokenizer: &mut Tokenizer<I>) -> Result<Option<Self>> {
		let _ = tokenizer; todo!();
	}
}

impl Parsable for TryCatch {
	fn parse<I: Iterator<Item=char>>(tokenizer: &mut Tokenizer<I>) -> Result<Option<Self>> {
		let _ = tokenizer; todo!();
	}
}

impl Parsable for Throw {
	fn parse<I: Iterator<Item=char>>(tokenizer: &mut Tokenizer<I>) -> Result<Option<Self>> {
		let _ = tokenizer; todo!();
	}
}

impl Parsable for Return {
	fn parse<I: Iterator<Item=char>>(tokenizer: &mut Tokenizer<I>) -> Result<Option<Self>> {
		let _ = tokenizer; todo!();
	}
}

impl Parsable for Scope {
	fn parse<I: Iterator<Item=char>>(tokenizer: &mut Tokenizer<I>) -> Result<Option<Self>> {
		let _ = tokenizer; todo!();
	}
}

impl Parsable for If {
	fn parse<I: Iterator<Item=char>>(tokenizer: &mut Tokenizer<I>) -> Result<Option<Self>> {
		let _ = tokenizer; todo!();
	}
}

impl Parsable for While {
	fn parse<I: Iterator<Item=char>>(tokenizer: &mut Tokenizer<I>) -> Result<Option<Self>> {
		let _ = tokenizer; todo!();
	}
}

impl Parsable for Switch {
	fn parse<I: Iterator<Item=char>>(tokenizer: &mut Tokenizer<I>) -> Result<Option<Self>> {
		let _ = tokenizer; todo!();
	}
}

impl Parsable for Statement {
	fn parse<I: Iterator<Item=char>>(tokenizer: &mut Tokenizer<I>) -> Result<Option<Self>> {
		let token = 
			if let Some(token) = tokenizer.next().transpose()? {
				tokenizer.undo();
				token
			} else {
				return Ok(None);
			};

		Ok(Some(match token {
			Token::Keyword(Keyword::Class) => Self::Class(Class::parse(tokenizer)?.unwrap()),
			Token::Keyword(Keyword::Function) => Self::Func(Function::parse(tokenizer)?.unwrap()),
			Token::Keyword(Keyword::Try) => Self::TryCatch(TryCatch::parse(tokenizer)?.unwrap()),
			Token::Keyword(Keyword::Throw) => Self::Throw(Throw::parse(tokenizer)?.unwrap()),
			Token::Keyword(Keyword::Return) => Self::Return(Return::parse(tokenizer)?.unwrap()),

			Token::Keyword(Keyword::Global) => {
				let _ = tokenizer.next();
				Self::Global(Scope::parse(tokenizer)?.unwrap())
			},
			Token::Keyword(Keyword::Local) => {
				let _ = tokenizer.next();
				Self::Local(Scope::parse(tokenizer)?.unwrap())
			},

			Token::Keyword(Keyword::If) => Self::If(If::parse(tokenizer)?.unwrap()),
			Token::Keyword(Keyword::While) => Self::While(While::parse(tokenizer)?.unwrap()),
			Token::Keyword(Keyword::Switch) => Self::Switch(Switch::parse(tokenizer)?.unwrap()),

			Token::Keyword(Keyword::ComeFrom) => {
				let _ = tokenizer.next();

				Self::ComeFrom(tokenizer.next_identifier().expect("todo: proper exception here lol)"))
			},

			Token::Identifier(ident) if tokenizer._hack_is_next_token_colon() => Self::Label(ident),
			_ => return Expression::parse(tokenizer).map(|opt| opt.map(Self::Expression)),
		}))
	}
}

impl Expression {
	fn parse_assignment<I: Iterator<Item=char>>(op: Symbol, primary: Primary, tokenizer: &mut Tokenizer<I>) -> Result<Self> {
		let _ = (op, primary, tokenizer);
		todo!();
	}

	fn parse_operator<I: Iterator<Item=char>>(op: Symbol, primary: Primary, tokenizer: &mut Tokenizer<I>) -> Result<Self> {
		let _ = (op, primary, tokenizer);
		todo!();
	}
}

impl Parsable for Expression {
	fn parse<I: Iterator<Item=char>>(tokenizer: &mut Tokenizer<I>) -> Result<Option<Self>> {
		use Symbol::*;

		let primary = 
			if let Some(primary) = Primary::parse(tokenizer)? {
				primary
			} else {
				return Ok(None);
			};

		match tokenizer.next().transpose()? {
			Some(Token::Symbol(asgn @ (
				Symbol::Equal |
				PlusEqual | 
				HyphenEqual | 
				AsteriskEqual | 
				AsteriskAsteriskEqual | 
				SolidusEqual | 
				PercentSignEqual
			))) => Self::parse_assignment(asgn, primary, tokenizer).map(Some),

			Some(Token::Symbol(op @ (
				EqualEqual | NotEqual | LessThan | LessThanOrEqual | GreaterThan |
				GreaterThanOrEqual | Compare | Plus | Hyphen | Asterisk |
				AsteriskAsterisk | Solidus | PercentSign | AndAnd | OrOr
			))) => Self::parse_operator(op, primary, tokenizer).map(Some),

			opt @ (Some(_) | None) => {
				if opt.is_some() {
					tokenizer.undo()
				}
				Ok(Some(Self::Primary(primary)))
			},
		}
	}
}

fn take_comma_sep_list<I, F, T>(tokenizer: &mut Tokenizer<I>, sep: Token, until: Token, func: F) -> Result<Vec<T>>
where
	I: Iterator<Item=char>,
	F: Fn(&mut Tokenizer<I>) -> Result<T>
{
	let mut args = vec![];

	loop {
		match tokenizer.next().transpose()? {
			Some(token) if token == until => break,
			Some(_) => tokenizer.undo(),
			None => return expected!(tokenizer, given None, expected until)
		}

		args.push((func)(tokenizer)?);

		match tokenizer.next().transpose()? {
			Some(token) if token == sep => continue,
			Some(token) if token == until => break,
			other => return expected!(tokenizer, given other, expected until, sep)
		}
	}

	Ok(args)
}

impl Primary {
	fn parse_paren_group<I: Iterator<Item=char>>(tokenizer: &mut Tokenizer<I>) -> Result<Self> {
		let args =
			take_comma_sep_list(
				tokenizer,
				Token::Symbol(Symbol::Endline),
				Token::RightParen(ParenKind::Round),
				|tokenizer| {
					Statement::parse(tokenizer)?.ok_or_else(|| tokenizer.error("expected value in paren group"))
				}
			)?;

		Ok(Self::Grouping(args))
	}

	fn parse_array_literal<I: Iterator<Item=char>>(tokenizer: &mut Tokenizer<I>) -> Result<Self> {
		let ary = 
			take_comma_sep_list(
				tokenizer,
				Token::Symbol(Symbol::Comma),
				Token::RightParen(ParenKind::Square),
				|tokenizer| {
					Expression::parse(tokenizer)?.ok_or_else(|| tokenizer.error("expected expression in array literal"))
				}
			)?;

		Ok(Self::Array(ary))
	}

	fn parse_codex_literal<I: Iterator<Item=char>>(tokenizer: &mut Tokenizer<I>) -> Result<Self> {
		let pages = 
			take_comma_sep_list(
				tokenizer,
				Token::Symbol(Symbol::Comma),
				Token::RightParen(ParenKind::Square),
				|tokenizer| {
					let key = Expression::parse(tokenizer)?.ok_or_else(|| tokenizer.error("expected key in codex literal"))?;
					expect!(tokenizer, Token::Symbol(Symbol::Colon));
					let value = Expression::parse(tokenizer)?.ok_or_else(|| tokenizer.error("expected value in codex literal"))?;
					Ok((key, value))
				}
			)?;

		Ok(Self::Codex(pages))
	}

	fn parse_function_call<I: Iterator<Item=char>>(primary: Primary, tokenizer: &mut Tokenizer<I>) -> Result<Self> {
		let args =
			take_comma_sep_list(
				tokenizer,
				Token::Symbol(Symbol::Comma),
				Token::RightParen(ParenKind::Round),
				|tokenizer| {
					Expression::parse(tokenizer)?.ok_or_else(|| tokenizer.error("expected expression in function call"))
				}
			)?;

		Ok(Self::FunctionCall(Box::new(primary), args))
	}

	fn parse_index<I: Iterator<Item=char>>(primary: Primary, tokenizer: &mut Tokenizer<I>) -> Result<Self> {
		if let Self::Array(args) = Self::parse_array_literal(tokenizer)? {
			Ok(Self::Index(Box::new(primary), args))
		} else {
			unreachable!()
		}
	}

	fn parse_getattr<I: Iterator<Item=char>>(primary: Primary, tokenizer: &mut Tokenizer<I>) -> Result<Self> {
		match tokenizer.next().transpose()? {
			Some(Token::Identifier(var)) => Ok(Self::GetAttr(Box::new(primary), var)),
			other => expected!(tokenizer, given other, expected Token::Identifier("<any identifier>".to_string()))
		}
	}

	fn parse_unary_operator<I: Iterator<Item=char>>(op: Symbol, tokenizer: &mut Tokenizer<I>) -> Result<Self> {
		let arg = Expression::parse(tokenizer)?.ok_or_else(|| tokenizer.error("expected expression for unary operator"))?;

		Ok(Self::UnaryOperator(match op {
			Symbol::Plus => UnaryOperator::Pos,
			Symbol::Hyphen => UnaryOperator::Neg,
			Symbol::Exclamation => UnaryOperator::Not,
			_ => unreachable!()
		}, Box::new(arg)))
	}

}

impl Parsable for Primary {
	fn parse<I: Iterator<Item=char>>(tokenizer: &mut Tokenizer<I>) -> Result<Option<Self>> {
		let token =
			if let Some(token) = tokenizer.next().transpose()? {
				token
			} else {
				return Ok(None);
			};

		let mut primary = 
			match token {
				Token::Literal(literal) => Self::Literal(literal),
				Token::LeftParen(ParenKind::Round) => Self::parse_paren_group(tokenizer)?,
				Token::LeftParen(ParenKind::Square) => Self::parse_array_literal(tokenizer)?,
				Token::LeftParen(ParenKind::Curly) => Self::parse_codex_literal(tokenizer)?,
				Token::Keyword(Keyword::Function) => todo!("lambda"),
				Token::Symbol(op @ (Symbol::Plus | Symbol::Hyphen | Symbol::Exclamation)) =>
					Self::parse_unary_operator(op, tokenizer)?,
				Token::Identifier(name) => Self::Variable(name),
				_ => {
					tokenizer.undo();
					return Ok(None);
				}
			};

		loop {
			match tokenizer.next().transpose()? {
				None => return Ok(Some(primary)),
				Some(Token::LeftParen(ParenKind::Round)) => primary = Self::parse_function_call(primary, tokenizer)?,
				Some(Token::LeftParen(ParenKind::Square)) => primary = Self::parse_index(primary, tokenizer)?,
				Some(Token::Symbol(Symbol::Dot)) => primary = Self::parse_getattr(primary, tokenizer)?,
				Some(_) => {
					tokenizer.undo();
					return Ok(Some(primary))
				}
			}
		}
	}
}