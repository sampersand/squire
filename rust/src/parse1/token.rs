use crate::Value;

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum Keyword {
	// form methods
	Form, // `class`
	Matter, // `field`
	Action, // for instance methods
	Describe, // for class methods
	Substantiate, // constructor

	// functions
	Journey, // function
	Reward, // return

	// Scoping
	Renowned, // global
	Nigh, // explicit local

	// Control flow
	If,
	Alas, // else
	Whence, // comefrom
	Whilst, // while

	Attempt, // try
	Catapult, // throw
	Retreat, // catch
	Regardless, // finally; TODO better name

	// punctuation
	LBrace,
	RBrace,
	LParen,
	RParen,
	LBracket,
	RBracket,
	Endl { soft: bool },
	Comma,
	Colon,
	Dot,

	// comparisons
	Equal,
	NotEqual,
	LessThan,
	LessThanOrEqual,
	GreaterThan,
	GreaterThanOrEqual,
	Compare,
	ExclamationPoint,

	// math
	Add,
	Minus,
	Asterisk,
	Slash,
	PercentSign,

	// short circuit
	AndAnd,
	OrOr,

	// Misc
	Equals,
	Index,
	IndexAssign
}

#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub enum Token<'a> {
	Literal(Value),
	InterpolatedString {
		begin: &'a str,
		val_and_suffix: Vec<(Vec<Token<'a>>, &'a str)>
	},
	Keyword(Keyword),
	Identifier(&'a str),
	Label(&'a str)
}


pub struct Tokenizer<'a> {
	text: &'a str
}