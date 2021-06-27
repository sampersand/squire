mod bytecode;
mod error;
mod codeblock;
mod vm;
mod args;

pub use bytecode::{Bytecode, Opcode, Interrupt};
pub use codeblock::CodeBlock;
pub use vm::Vm;
pub use error::{Error, Result};
pub use crate::Value;
pub use args::Args;
