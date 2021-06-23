#![allow(unused)]
use std::collections::HashMap;
use super::{Token, Tokenizer, ErrorKind, Result, Symbol, ParenKind};

#[derive(Debug)]
enum MacroToken {
	Token(Token),
	MacroIdent(String)
}

#[derive(Debug)]
enum MacroTokenVerbatim {
	MacroToken(MacroToken),
	Verbatim { start: bool },
}

#[derive(Debug)]
enum Replacement {
	Simple(Vec<MacroToken>),
	Functional(Vec<String>, Vec<MacroToken>)
}

#[derive(Debug, Default)]
pub struct Macros {
	replacements: HashMap<String, Replacement>
}

struct MacroTokenizer<'a, 'b, I>(&'a mut Tokenizer<'b, I>);

impl<I: Iterator<Item=char>> Iterator for MacroTokenizer<'_, '_, I> {
	type Item = Result<MacroTokenVerbatim>;

	fn next(&mut self) -> Option<Self::Item> {
		self.0.stream.strip_whitespace_and_comments();

		if self.0.stream.take_prefix("<<") {
			Some(Ok(MacroTokenVerbatim::Verbatim { start: true }))
		} else if self.0.stream.take_prefix(">>") {
			Some(Ok(MacroTokenVerbatim::Verbatim { start: false }))
		} else {
			self.next_macro_variable()
				.map(MacroToken::MacroIdent)
				.map(MacroTokenVerbatim::MacroToken)
				.map(Ok)
				.or_else(|| self.0.next().map(|res| res.map(MacroToken::Token).map(MacroTokenVerbatim::MacroToken)))
		}
	}
}

impl<I: Iterator<Item=char>> MacroTokenizer<'_, '_, I> {
	fn next_macro_variable(&mut self) -> Option<String> {
		self.0.next_macro_variable()
	}

	fn parse_henceforth_body(&mut self) -> Result<Vec<MacroToken>> {
		let mut token_or_macros = vec![];
		let mut is_verbatim = false;

		loop {
			match self.next().unwrap_or_else(|| Err(self.0.error("unexpected end of macro declaration")))? {
				MacroTokenVerbatim::Verbatim { start: true } if !is_verbatim => is_verbatim = true,
				MacroTokenVerbatim::Verbatim { start: true } => return Err(self.0.error("cannot nest verbatim modes")),
				MacroTokenVerbatim::Verbatim { start: false } if is_verbatim => is_verbatim = false,
				MacroTokenVerbatim::Verbatim { start: false }  => return Err(self.0.error("unexpected verbatim mode stop")),
				MacroTokenVerbatim::MacroToken(MacroToken::Token(Token::Symbol(Symbol::Endline))) if !is_verbatim => break,
				MacroTokenVerbatim::MacroToken(token_or_macro) => token_or_macros.push(token_or_macro),
			}
		}

		debug_assert!(!is_verbatim);
		Ok(token_or_macros)
	}

	// fn parse_macro_henceforth_function(&mut self) -> Result<(Vec<String>, Vec<TokenOrMacro>)> {
	// 	let mut args = vec![];

	// 	while let Some(arg) = self.

	// 	todo!();
	// }

	fn parse_henceforth(&mut self) -> Result<()> {
		todo!()
		// let variable =
		// 	self.next_macro_variable()
		// 		.ok_or_else(|| self.0.error("missing macro variable after 'henceforth'"))?;

		// let replacement = 
		// 	match self.next().transpose()? {
		// 		Some(Token::Symbol(Symbol::Equal)) => Replacement::Simple(self.parse_macro_henceforth_body()?),
		// 		Some(Token::LeftParen(ParenKind::Round)) => {
		// 			let (args, body) = self.parse_macro_henceforth_function()?;
		// 			Replacement::Functional(args, body)
		// 		},
		// 		_ => return Err(self.stream.error("expected a token after 'henceforth': '=' or '('")),
		// 	};

		// self.macros.replacements.insert(variable, replacement);
		// Ok(())
	}

	// fn parse_macro_nevermore(&mut self) -> Result<()> {
	// 	if let Some(variable) = self.next_macro_variable() {
	// 		self.macros.replacements.remove(&variable);
	// 		Ok(())
	// 	} else {
	// 		Err(self.stream.error("missing macro variable after 'nevermore'"))
	// 	}
	// }

	// pub(super) fn parse_macro_invocation_for(&mut self, ident: &str) -> Result<()> {
	// 	match ident {
	// 		"henceforth" => self.parse_macro_henceforth(),
	// 		"nevermore" => self.parse_macro_nevermore(),
	// 		other => Err(self.stream.error(ErrorKind::UnknownMacroInvocation(other.to_string())))
	// 	}
	// }

	// pub fn parse(&mut self, ident: String, tokenizer: &mut super::Tokenizer<I>) {

	// }
}