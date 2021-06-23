mod error;
mod stream;
mod token;
pub mod ast;

pub use error::{Error, ErrorKind, Result};
pub use stream::Stream;
pub use token::{Token, Tokenizer};

#[derive(Debug)]
pub struct Parser<'a, I> {
	tokenizer: &'a mut Tokenizer<'a, I>
}
