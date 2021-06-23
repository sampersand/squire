use std::collections::HashMap;
use crate::Value;
use crate::parse::{Tokenizer, /*Token*/};

#[derive(Debug)]
pub struct Compiler<'a, I> {
	tokenizer: &'a mut Tokenizer<'a, I>,
	globals: HashMap<Value, usize>,
}

impl<'a, I> Compiler<'a, I> {
	pub fn new(tokenizer: &'a mut Tokenizer<'a, I>) -> Self {
		Self { tokenizer, globals: Default::default() }
	}


}