use super::{Result};
use std::collections::HashMap;
use crate::Value;

#[derive(Debug, Default)]
pub struct Compiler {
	globals: HashMap<Value, usize>,
}

pub trait Compileable {
	fn compile(self, compiler: &mut Compiler) -> Result<()>;
}