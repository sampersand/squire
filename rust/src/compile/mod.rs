mod compiler;
mod error;

pub use compiler::*;
pub use error::{Error, Result};

pub trait Compilable {
	fn compile(self, compiler: &mut Compiler, target: Option<Target>) -> Result<()>;
}
