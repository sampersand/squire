use std::str::Chars;

#[derive(Debug, Clone)]
pub struct Stream<'a> {
	source: Chars<'a>,
	file: Option<&'a str>,
	line: usize,
	put_back: Option<char>
}

impl Default for Stream<'_> {
	fn default() -> Self {
		Self::new("", None)
	}
}

impl<'a> Stream<'a> {
	pub fn new(source: &'a str, file: Option<&'a str>) -> Self {
		Self {
			source: source.chars(),
			file,
			line: 1,
			put_back: None
		}
	}

	pub fn source(&self) -> &'a str {
		self.source.as_str()
	}

	pub fn line(&self) -> usize {
		self.line
	}

	pub fn file(&self) -> Option<&'a str> {
		self.file
	}

	pub fn peek(&mut self) -> Option<char> {
		if self.put_back.is_none() {
			self.put_back = self.next();
		}

		self.put_back
	}
}

impl AsRef<str> for Stream<'_> {
	fn as_ref(&self) -> &str {
		self.source()
	}
}

impl Iterator for Stream<'_> {
	type Item = char;

	fn next(&mut self) -> Option<Self::Item> {
		self.put_back.take().or_else(|| self.source.next())
	}
}