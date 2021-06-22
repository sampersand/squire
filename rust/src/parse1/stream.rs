use super::{Token, Result, Error, ErrorKind};

#[derive(Debug, Clone, PartialEq, Eq, Hash, Default)]
pub struct Stream<'a> {
	source: &'a str,
	file: Option<&'a str>,
	put_back
	line: usize
}

impl<'a> Stream<'a> {
	pub const fn new(source: &'a str, file: Option<&'a str>) -> Self {
		Self { source, file, line: 1 }
	}

	pub const fn line(&self) -> usize {
		self.line
	}

	pub const fn file(&self) -> Option<&'a str> {
		self.file
	}

	pub fn peek(&self) -> Option<char> {
		self.source.chars().next()
	}

	fn strip_keyword_prefix(&mut self, prefix: &str) -> bool {
		if let Some(rest) = self.source.strip_prefix(prefix) {
			if rest.chars().next().map_or(true, |c| !c.is_alphanumeric()) {
				self.source = rest;
				true
			} else {
				false
			}
		} else {
			false
		}
	}

	fn strip_prefix(&mut self, prefix: &str) -> bool {
		if let Some(rest) = self.source.strip_prefix(prefix) {
			self.source = rest;
			true
		} else {
			false
		}
	}

	fn next_char(&mut self) -> Option<char> {
		let mut chars = self.source.chars();
		let chr = chars.next();

		if chr == Some('\n') {
			self.line += 1;
		}

		self.source = chars.as_str();
		chr
	}

	fn error(&self, kind: ErrorKind) -> Result<()> {
		Err(Error { line: self.line(), file: self.file().map(str::to_string), kind })
	}

	fn strip_comment(&mut self) {
		assert_eq!(self.next(), Some('#'), "called 'strip_comment' without a leading '#'");

		while let S
	}

	fn strip_whitespace(&mut self) -> Result<()> {
		while let Some(chr) = self.peek() {
			match chr {
				'#' =>
					while let Some(chr) = self.next_char() {
						if chr == '\n' {
							break;
						}
					},

				'\\' => {
					self.next_char();

					if self.next_char() != Some('\n')  {
						return self.error(ErrorKind::MisplacedBackslash)
					}
				},

				_ if chr.is_whitespace() => { self.next_char(); },
				_ => break
			};
		}

		Ok(())
	}

	const fn is_empty(&self) -> bool {
		self.source.is_empty()
	}
}


impl<'a> Stream<'a> {
	fn next_macro_token(&mut self) -> Result<Option<Token<'a>>> {
		Ok(None)
	}

	fn next_normal_token(&mut self) -> Result<Option<Token<'a>>> {
		self.strip_whitespace()?;

		todo!();
		// if self.is_empty() || self.strip_keyword_prefix("__END__") {
		// 	self.stream = ""; // in case it's `__END__`
		// 	return None;
		// }

		// match self.peek().unwrap() {
		// 	'0'..='9' => self.parse_arabic_number(),
		// 	chr if Tally::is_roman_numeral(chr) => self.parse_roman_number(),
		// 	'\'' | '\"' => self.parse_string(),
		// 	'@' => self.parse_macro_statement(),
		// 	'$' => self.parse_macro_identifier(),
			
		// 	_ if self.str
		// }

/*
static struct sq_token next_normal_token(void) {
	CHECK_FOR_START_KW("form",         SQ_TK_CLASS);
	CHECK_FOR_START_KW("matter",       SQ_TK_FIELD);
	CHECK_FOR_START_KW("action",       SQ_TK_METHOD);
	CHECK_FOR_START_KW("describe",     SQ_TK_CLASSFN);
	CHECK_FOR_START_KW("substantiate", SQ_TK_CONSTRUCTOR);

	CHECK_FOR_START_KW("journey",      SQ_TK_FUNC);
	CHECK_FOR_START_KW("renowned",     SQ_TK_GLOBAL);
	CHECK_FOR_START_KW("nigh",         SQ_TK_LOCAL);
	CHECK_FOR_START_KW("import",       SQ_TK_IMPORT); // `befriend`? `beseech`?

	CHECK_FOR_START_KW("if",           SQ_TK_IF); // _should_ we have a better one?
	CHECK_FOR_START_KW("alas",         SQ_TK_ELSE);
	CHECK_FOR_START_KW("whence",       SQ_TK_COMEFROM);
	CHECK_FOR_START_KW("whilst",       SQ_TK_WHILE);
	CHECK_FOR_START_KW("reward",       SQ_TK_RETURN);
	CHECK_FOR_START_KW("quest",        SQ_TK_TRY);
	CHECK_FOR_START_KW("abandon",      SQ_TK_THROW);
	CHECK_FOR_START_KW("feint",        SQ_TK_UNDO);

	CHECK_FOR_START_KW("yay",          SQ_TK_TRUE);
	CHECK_FOR_START_KW("nay",          SQ_TK_FALSE);
	CHECK_FOR_START_KW("ni",           SQ_TK_NULL);

	if (isalpha(*sq_stream) || *sq_stream == '_')
		return parse_identifier();

	CHECK_FOR_START("[]", SQ_TK_INDEX);
	CHECK_FOR_START("[]=", SQ_TK_INDEX_ASSIGN);
	CHECK_FOR_START("{", SQ_TK_LBRACE);
	CHECK_FOR_START("}", SQ_TK_RBRACE);
	CHECK_FOR_START("}", SQ_TK_RBRACE);
	CHECK_FOR_START("(", SQ_TK_LPAREN);
	CHECK_FOR_START(")", SQ_TK_RPAREN);
	CHECK_FOR_START("[", SQ_TK_LBRACKET);
	CHECK_FOR_START("]", SQ_TK_RBRACKET);
	CHECK_FOR_START(";", SQ_TK_ENDL);
	CHECK_FOR_START("\n", SQ_TK_SOFT_ENDL);
	CHECK_FOR_START(",", SQ_TK_COMMA);
	CHECK_FOR_START(".", SQ_TK_DOT);
	CHECK_FOR_START(":", SQ_TK_COLON);

	CHECK_FOR_START("==", SQ_TK_EQL);
	CHECK_FOR_START("!=", SQ_TK_NEQ);
	CHECK_FOR_START("<=", SQ_TK_LEQ);
	CHECK_FOR_START(">=", SQ_TK_GEQ);
	CHECK_FOR_START("<", SQ_TK_LTH);
	CHECK_FOR_START(">", SQ_TK_GTH);
	CHECK_FOR_START("+", SQ_TK_ADD);
	CHECK_FOR_START("-@", SQ_TK_NEG);
	CHECK_FOR_START("-", SQ_TK_SUB);
	CHECK_FOR_START("*", SQ_TK_MUL);
	CHECK_FOR_START("/", SQ_TK_DIV);
	CHECK_FOR_START("%", SQ_TK_MOD);
	CHECK_FOR_START("!", SQ_TK_NOT);
	CHECK_FOR_START("&&", SQ_TK_AND);
	CHECK_FOR_START("||", SQ_TK_OR);
	CHECK_FOR_START("=", SQ_TK_ASSIGN);

	die("unknown token start '%c'", *sq_stream);
}*/
	}
}

// 		char c;

// 		// strip whitespace
// 		while ((c = *sq_stream)) {
// 			if (c == '#') {
// 				do {
// 					c = *++sq_stream;
// 				} while (c && c != '\n');
// 				if (c == '\n') ++sq_stream;
// 				continue;
// 			}

// 			if (*sq_stream == '\\') {
// 				++sq_stream;

// 				if (*sq_stream && *sq_stream++ != '\n') 
// 					die("unexpected '\\' on its own.");
// 				continue;
// 			}

// 			if (!isspace(c) || (!strip_newline && c == '\n'))
// 				break;

// 			while (isspace(c) && c != '\n')
// 				c = *++sq_stream;
// 		}
// 	}
// 	}
// }

impl<'a> Iterator for Stream<'a> {
	type Item = Result<Token<'a>>;

	fn next(&mut self) -> Option<Self::Item> {
		self.next_macro_token().transpose().or_else(|| self.next_normal_token().transpose())
	}
}