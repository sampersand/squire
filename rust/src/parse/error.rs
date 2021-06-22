#[derive(Debug)]
pub enum ErrorKind {
	MisplacedBackslash
}

pub struct Error {
	pub line: usize,
	pub file: Option<String>,
	pub kind: ErrorKind
}


pub type Result<T> = std::result::Result<T, Error>;
