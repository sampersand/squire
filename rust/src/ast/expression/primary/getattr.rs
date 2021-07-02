use crate::ast::expression::{Expression, Primary};
use crate::parse::{Error as ParseError, Parser};
use crate::parse::token::{Token, TokenKind, Symbol, ParenKind};
use crate::compile::{Compiler, Compilable, Target, Error as CompileError};
use crate::runtime::Opcode;

#[derive(Debug)]
pub struct GetAttr {
	from: Box<Primary>,
	attribute: String
}

impl GetAttr {
	pub fn parse_with<I>(from: Primary, parser: &mut Parser<'_, I>) -> Result<Result<Self, Primary>, ParseError>
	where
		I: Iterator<Item=char>
	{
		use Symbol as Sym;
		use TokenKind as Tk;
		use Token as Tkn;

		if parser.guard(Tk::Symbol(Sym::Dot))?.is_none() {
			return Ok(Err(from));
		}

		let attribute =
			match parser.expect([
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
					parser.expect(Tk::RightParen(ParenKind::Round))?;
					"()".to_string()
				},
				Tkn::LeftParen(ParenKind::Square) => {
					parser.expect(Tk::RightParen(ParenKind::Square))?;
					if parser.guard(Tk::Symbol(Sym::Equal))?.is_some() {
						"[]=".to_string()
					} else {
						"[]".to_string()
					}
				}

				other => unreachable!("`parser.expect` returned a bad token: {:?}", other)
			};

		Ok(Ok(Self { from: Box::new(from), attribute }))
	}

	pub fn compile_assignment(
		self,
		op: Option<crate::ast::expression::binary_operator::Math>,
		rhs: Box<Expression>,
		compiler: &mut Compiler,
		target: Option<Target>
	) -> Result<(), CompileError> {
		if op.is_some() { todo!(); }

		let lhs_target = target.unwrap_or_else(|| compiler.next_target());
		self.from.compile(compiler, Some(lhs_target))?;
		let rhs_target = compiler.next_target();
		rhs.compile(compiler, Some(rhs_target))?;
		let constant = compiler.get_constant(self.attribute.into());

		compiler.opcode(Opcode::SetAttribute);
		compiler.target(lhs_target);
		compiler.constant(constant);
		compiler.target(rhs_target);

		Ok(())
	}
}

impl Compilable for GetAttr {
	fn compile(self, compiler: &mut Compiler, target: Option<Target>) -> Result<(), CompileError> {
		self.from.compile(compiler, target)?;

		if let Some(target) = target {
			let attribute_index = compiler.get_constant(self.attribute.into());

			compiler.opcode(Opcode::GetAttribute);
			compiler.target(target);
			compiler.constant(attribute_index);
			compiler.target(target);
		}

		Ok(())
	}
}
