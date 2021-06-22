use std::str::Chars;
use super::{Error, ErrorKind};

#[derive(Debug, Clone)]
pub struct Stream<'a, I> {
	source: I,
	file: Option<&'a str>,
	lineno: usize,
	put_back: Vec<char>
}

impl Default for Stream<'_, Chars<'static>> {
	fn default() -> Self {
		Self::new("".chars())
	}
}

impl<'a> Stream<'a, Chars<'a>> {
	pub fn from_str(source: &'a str) -> Self {
		Self::new(source.chars())
	}
}

impl<'a, I> Stream<'a, I> {
	pub const fn new(source: I) -> Self {
		Self::_new(source, None)
	}

	pub const fn with_file(source: I, file: &'a str) -> Self {
		Self::_new(source, Some(file))
	}

	const fn _new(source: I, file: Option<&'a str>) -> Self {
		Self { source, file, lineno: 1, put_back: Vec::new() }
	}

	pub const fn lineno(&self) -> usize {
		self.lineno
	}

	pub const fn file(&self) -> Option<&'a str> {
		self.file
	}
}

impl<'a, I: Iterator<Item=char>> Stream<'a, I> {
	pub fn peek(&mut self) -> Option<char> {
		if self.put_back.is_empty() {
			if let Some(last) = self.next() {
				self.put_back([last]);
			}
		}

		self.put_back.last().cloned()
	}

	pub fn put_back(&mut self, iter: impl IntoIterator<Item=char>) {
		let len = self.put_back.len();

		for chr in iter {
			self.put_back.insert(len, chr);
		}
	}

	pub fn error(&self, error: impl Into<ErrorKind>) -> Error {
		Error {
			lineno: self.lineno,
			file: self.file.map(ToString::to_string),
			error: error.into()
		}
	}

	pub fn strip_comment(&mut self) {
		assert_eq!(self.next(), Some('#'), "called `strip_comment` without a leading `#`");

		self.take_while(|chr| chr != '\n');
	}

	pub fn strip_whitespace(&mut self) {
		self.take_while(char::is_whitespace);
	}

	pub fn take_while(&mut self, mut condition: impl FnMut(char) -> bool) -> Option<String> {
		let mut acc = String::new();

		for chr in self.by_ref() {
			if condition(chr) {
				acc.push(chr)
			} else {
				self.put_back([chr]);
				break;
			}
		}

		if acc.is_empty() {
			None
		} else {
			Some(acc)
		}
	}

	pub fn take_prefix(&mut self, prefix: &str) -> bool {
		let mut prefix_chars = prefix.chars();

		for (prefix_chr, our_chr) in prefix_chars.by_ref().zip(self.by_ref()) {
			if prefix_chr == our_chr {
				continue;
			}

			// uh oh, they dont match, we have to put everything back and return false.
			let mut prefix_chars2 = prefix.chars();
			let len = self.put_back.len();
			while let Some(chr) = prefix_chars2.next() {
				if prefix_chars2.as_str().len() == prefix_chars.as_str().len() {
					break;
				} else {
					self.put_back.insert(len, chr);
				}
			}
			self.put_back.insert(len, our_chr);
			return false;
		}

		true
	}

	pub fn take_identifier(&mut self, identifier: &str) -> bool {
		if !self.take_prefix(identifier) {
			return false;
		}

		if self.peek().map_or(false, char::is_alphanumeric) {
			self.put_back(identifier.chars());
			false
		} else {
			true
		}
	}
}

impl<I: Iterator<Item=char>> Iterator for Stream<'_, I> {
	type Item = char;

	fn next(&mut self) -> Option<Self::Item> {
		if let Some(put_back) = self.put_back.pop() {
			return Some(put_back);
		}

		let chr = self.source.next();
		if chr == Some('\n') {
			self.lineno += 1;
		}

		chr
	}
}