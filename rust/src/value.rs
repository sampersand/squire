use std::sync::Arc;
use crate::{Journey, Form, Imitation};
use std::fmt::{self, Display, Formatter};

mod array;
mod codex;
pub mod numeral;
pub mod text;

pub use text::Text;
pub use array::Array;
pub use codex::Codex;
pub use numeral::Numeral;

#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub enum Value {
	Null,
	Veracity(bool),
	Numeral(Numeral),
	Text(Text),
	Journey(Journey),
	Form(Arc<Form>),
	Imitation(Arc<Imitation>)
}

impl Default for Value {
	fn default() -> Self {
		Self::Null
	}
}

impl PartialOrd for Value {
	fn partial_cmp(&self, _rhs: &Self) -> Option<std::cmp::Ordering> {
		todo!()
	}
}

impl Display for Value {
	fn fmt(&self, f: &mut Formatter<'_>) -> fmt::Result {
		let _ = f;
		todo!("display (note that `{{:#}}` should be `repr()`)")
	}
}

impl From<bool> for Value {
	#[inline]
	fn from(boolean: bool) -> Self {
		Self::Veracity(boolean)
	}
}