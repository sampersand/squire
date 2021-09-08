use super::{*, ops::*};
use crate::vm::{Vm, Result, Error as RuntimeError, Args};
use std::fmt::{self, Debug, Formatter};

#[derive(Clone, PartialEq, Eq, Hash)]
#[non_exhaustive]
pub enum Value {
	Ni,
	Veracity(Veracity),
	Numeral(Numeral),
	// idea: fractions instead of floats.
	Text(Text),

	Book(Book),
	Codex(Codex),

	Form(Form),
	Imitation(Imitation),
	Journey(Journey),
}

#[derive(Debug, Clone, PartialEq, Eq, Hash)]
#[non_exhaustive]
pub enum Genus {
	Ni,
	Veracity,
	Numeral,
	Text,
	Book,
	Form,
	Codex,
	Imitation(Form),
	Journey,
}

impl Default for Value {
	fn default() -> Self {
		Self::Ni
	}
}

impl Debug for Value {
	fn fmt(&self, f: &mut Formatter<'_>) -> fmt::Result {
		match self {
			Self::Ni => Debug::fmt(&Ni, f),
			Self::Veracity(veracity) => Debug::fmt(&veracity, f),
			Self::Numeral(numeral) => Debug::fmt(&numeral, f),
			Self::Text(text) => Debug::fmt(&text, f),

			Self::Book(array) => Debug::fmt(&array, f),
			Self::Codex(codex) => Debug::fmt(&codex, f),

			Self::Form(form) => Debug::fmt(&form, f),
			Self::Imitation(imitation) => Debug::fmt(&imitation, f),
			Self::Journey(journey) => Debug::fmt(&journey, f),
		}
	}
}

impl From<Veracity> for Value {
	#[inline]
	fn from(boolean: Veracity) -> Self {
		Self::Veracity(boolean)
	}
}

impl From<String> for Value {
	#[inline]
	fn from(text: String) -> Self {
		Self::from(Text::new(text))
	}
}

impl From<Text> for Value {
	#[inline]
	fn from(text: Text) -> Self {
		Self::Text(text)
	}
}

impl From<Numeral> for Value {
	#[inline]
	fn from(numeral: Numeral) -> Self {
		Self::Numeral(numeral)
	}
}

impl Value {
	pub fn convert_to<T>(&self, vm: &mut Vm) -> Result<T>
	where
		Self: ConvertTo<T>
	{
		self.convert(vm)
	}

	pub fn genus(&self) -> Genus {
		match self {
			Self::Ni => Genus::Ni,
			Self::Veracity(_) => Genus::Veracity,
			Self::Numeral(_) => Genus::Numeral,
			Self::Text(_) => Genus::Text,
			Self::Book(_) => Genus::Book,
			Self::Codex(_) => Genus::Codex,
			Self::Form(_) => Genus::Form,
			Self::Imitation(imitation) => Genus::Imitation(imitation.form().clone()),
			Self::Journey(_) => Genus::Journey,
		}
	}
}

impl Matches for Value {
	fn matches(&self, rhs: &Value, vm: &mut Vm) -> Result<bool> {
		match self {
			Self::Ni => Ni.matches(rhs, vm),
			Self::Veracity(veracity) => veracity.matches(rhs, vm),
			Self::Numeral(numeral) => numeral.matches(rhs, vm),
			Self::Text(text) => text.matches(rhs, vm),
			Self::Book(book) => dbg!(book).matches(rhs, vm),
			Self::Codex(codex) => codex.matches(rhs, vm),
			Self::Form(form) => form.matches(rhs, vm),
			Self::Imitation(imitation) => imitation.matches(rhs, vm),
			Self::Journey(journey) => journey.matches(rhs, vm),
		}
	}
}

impl Dump for Value {
	fn dump(&self, to: &mut String, vm: &mut Vm) -> Result<()> {
		match self {
			Self::Ni => Ni.dump(to, vm),
			Self::Veracity(veracity) => veracity.dump(to, vm),
			Self::Numeral(numeral) => numeral.dump(to, vm),
			Self::Text(text) => text.dump(to, vm),
			Self::Book(book) => book.dump(to, vm),
			Self::Codex(codex) => codex.dump(to, vm),
			Self::Form(form) => form.dump(to, vm),
			Self::Imitation(imitation) => imitation.dump(to, vm),
			Self::Journey(journey) => journey.dump(to, vm),
		}
	}
}
impl ConvertTo<Veracity> for Value {
	fn convert(&self, vm: &mut Vm) -> Result<Veracity> {
		match self {
			Self::Ni => Ni.convert(vm),
			Self::Veracity(veracity) => veracity.convert(vm),
			Self::Numeral(numeral) => numeral.convert(vm),
			Self::Text(text) => text.convert(vm),
			Self::Book(book) => book.convert(vm),
			Self::Codex(codex) => codex.convert(vm),
			Self::Imitation(imitation) => <Imitation as ConvertTo<Veracity>>::convert(imitation, vm),
			_ => Err(RuntimeError::CannotConvert { from: self.genus(), to: Genus::Veracity })
		}
	}
}

impl ConvertTo<Numeral> for Value {
	fn convert(&self, vm: &mut Vm) -> Result<Numeral> {
		match self {
			Self::Ni => Ni.convert(vm),
			Self::Veracity(veracity) => veracity.convert(vm),
			Self::Numeral(numeral) => numeral.convert(vm),
			Self::Text(text) => text.convert(vm),
			Self::Imitation(imitation) => <Imitation as ConvertTo<Numeral>>::convert(imitation, vm),
			_ => Err(RuntimeError::CannotConvert { from: self.genus(), to: Genus::Numeral })
		}
	}
}

impl ConvertTo<Text> for Value {
	fn convert(&self, vm: &mut Vm) -> Result<Text> {
		match self {
			Self::Ni => Ni.convert(vm),
			Self::Veracity(veracity) => veracity.convert(vm),
			Self::Numeral(numeral) => numeral.convert(vm),
			Self::Text(text) => text.convert(vm),
			Self::Book(book) => book.convert(vm),
			Self::Codex(codex) => codex.convert(vm),
			Self::Imitation(imitation) => <Imitation as ConvertTo<Text>>::convert(imitation, vm),
			_ => Err(RuntimeError::CannotConvert { from: self.genus(), to: Genus::Text })
		}
	}
}

impl ConvertTo<Book> for Value {
	fn convert(&self, vm: &mut Vm) -> Result<Book> {
		match self {
			Self::Ni => Ni.convert(vm),
			Self::Text(text) => text.convert(vm),
			Self::Book(book) => book.convert(vm),
			Self::Codex(codex) => codex.convert(vm),
			Self::Imitation(imitation) => <Imitation as ConvertTo<Book>>::convert(imitation, vm),
			_ => Err(RuntimeError::CannotConvert { from: self.genus(), to: Genus::Book })
		}
	}
}

impl ConvertTo<Codex> for Value {
	fn convert(&self, vm: &mut Vm) -> Result<Codex> {
		match self {
			Self::Ni => Ni.convert(vm),
			Self::Book(book) => book.convert(vm),
			Self::Codex(codex) => codex.convert(vm),
			Self::Imitation(imitation) => <Imitation as ConvertTo<Codex>>::convert(imitation, vm),
			_ => Err(RuntimeError::CannotConvert { from: self.genus(), to: Genus::Codex })
		}
	}
}

impl Negate for Value {
	fn negate(&self, vm: &mut Vm) -> Result<Value> {
		match self {
			Self::Numeral(numeral) => numeral.negate(vm),
			Self::Imitation(imitation) => imitation.negate(vm),
			_ => Err(RuntimeError::OperationNotSupported { kind: self.genus(), func: "-@" })
		}
	}
}

impl Add for Value {
	fn add(&self, rhs: &Value, vm: &mut Vm) -> Result<Value> {
		// if rhs is a string, convert us to a string and concat them.
		if matches!(rhs, Self::Text(_)) && !matches!(self, Self::Text(_)) {
			return Self::Text(self.convert_to::<Text>(vm)?).add(rhs, vm);
		}

		match self {
			Self::Numeral(numeral) => numeral.add(rhs, vm),
			Self::Text(text) => text.add(rhs, vm),
			Self::Book(book) => book.add(rhs, vm),
			Self::Codex(codex) => codex.add(rhs, vm),
			Self::Imitation(imitation) => imitation.add(rhs, vm),
			_ => Err(RuntimeError::OperationNotSupported { kind: self.genus(), func: "+" })
		}
	}
}

impl Subtract for Value {
	fn subtract(&self, rhs: &Value, vm: &mut Vm) -> Result<Value> {
		match self {
			Self::Numeral(numeral) => numeral.subtract(rhs, vm),
			Self::Book(book) => book.subtract(rhs, vm),
			Self::Codex(codex) => codex.subtract(rhs, vm),
			Self::Imitation(imitation) => imitation.subtract(rhs, vm),
			_ => Err(RuntimeError::OperationNotSupported { kind: self.genus(), func: "-" })
		}
	}
}

impl Multiply for Value {
	fn multiply(&self, rhs: &Value, vm: &mut Vm) -> Result<Value> {
		match self {
			Self::Numeral(numeral) => numeral.multiply(rhs, vm),
			Self::Text(text) => text.multiply(rhs, vm),
			Self::Book(book) => book.multiply(rhs, vm),
			Self::Imitation(imitation) => imitation.multiply(rhs, vm),
			_ => Err(RuntimeError::OperationNotSupported { kind: self.genus(), func: "*" })
		}
	}
}

impl Divide for Value {
	fn divide(&self, rhs: &Value, vm: &mut Vm) -> Result<Value> {
		match self {
			Self::Numeral(numeral) => numeral.divide(rhs, vm),
			Self::Imitation(imitation) => imitation.divide(rhs, vm),
			_ => Err(RuntimeError::OperationNotSupported { kind: self.genus(), func: "/" })
		}
	}
}

impl Modulo for Value {
	fn modulo(&self, rhs: &Value, vm: &mut Vm) -> Result<Value> {
		match self {
			Self::Numeral(numeral) => numeral.modulo(rhs, vm),
			Self::Text(text) => text.modulo(rhs, vm),
			Self::Imitation(imitation) => imitation.modulo(rhs, vm),
			_ => Err(RuntimeError::OperationNotSupported { kind: self.genus(), func: "%" })
		}
	}
}

impl Power for Value {
	fn power(&self, rhs: &Value, vm: &mut Vm) -> Result<Value> {
		match self {
			Self::Numeral(numeral) => numeral.power(rhs, vm),
			Self::Imitation(imitation) => imitation.power(rhs, vm),
			_ => Err(RuntimeError::OperationNotSupported { kind: self.genus(), func: "**" })
		}

/*		const U32MAX_AS_I64: i64 = u32::MAX as i64;

		match self {
			Self::Numeral(numeral) => 
				match rhs.to_numeral(vm)?.get() {
					exp @ (i64::MIN..=-1) =>
						match numeral.get() {
							0 => Err(RuntimeError::DivisionByZero),
							-1 if exp.abs() % 2 == 0 => Ok(Self::Numeral(Numeral::new(1))),
							1 | -1 => Ok(Self::Numeral(*numeral)),
							_ => Ok(Self::Numeral(Numeral::new(0)))
						},
					exp @ (0..=U32MAX_AS_I64) => Ok(Self::Numeral(numeral.pow(exp as u32))),
					_ => Err(RuntimeError::OutOfBounds) // or should it be infinity or somethin? idk.
				},
			Self::Imitation(imitation) => imitation.try_pow(rhs, vm),
			_ => Err(RuntimeError::OperationNotSupported { kind: self.genus(), func: "**" })
		}*/
	}
}

impl IsEqual for Value {
	fn is_equal(&self, rhs: &Value, vm: &mut Vm) -> Result<bool> {
		match self {
			Self::Ni => Ni.is_equal(rhs, vm),
			Self::Veracity(veracity) => veracity.is_equal(rhs, vm),
			Self::Numeral(numeral) => numeral.is_equal(rhs, vm),
			Self::Text(text) => text.is_equal(rhs, vm),

			Self::Book(book) => book.is_equal(rhs, vm),
			Self::Codex(codex) => codex.is_equal(rhs, vm),

			Self::Form(form) => form.is_equal(rhs, vm),
			Self::Imitation(imitation) => imitation.is_equal(rhs, vm),
			Self::Journey(journey) => journey.is_equal(rhs, vm),
		}
	}
}

impl Compare for Value {
	fn compare(&self, rhs: &Value, vm: &mut Vm) -> Result<Option<std::cmp::Ordering>> {
		match self {
			Self::Veracity(veracity) => veracity.compare(rhs, vm),
			Self::Numeral(numeral) => numeral.compare(rhs, vm),
			Self::Text(text) => text.compare(rhs, vm),
			Self::Book(book) => book.compare(rhs, vm),
			Self::Codex(codex) => codex.compare(rhs, vm),
			Self::Imitation(imitation) => imitation.compare(rhs, vm),
			_ => Err(RuntimeError::OperationNotSupported { kind: self.genus(), func: "<=>" })
		}
	}
}

impl Call for Value {
	fn call(&self, args: Args, vm: &mut Vm) -> Result<Value> {
		match self {
			Self::Journey(journey) => journey.call(args, vm),
			Self::Form(form) => form.call(args, vm),
			Self::Imitation(imitation) => imitation.call(args, vm),
			_ => Err(RuntimeError::OperationNotSupported { kind: self.genus(), func: "()" })
		}
	}
}


impl GetIndex for Value {
	fn get_index(&self, key: &Value, vm: &mut Vm) -> Result<Value> {
		match self {
			Self::Text(text) => text.get_index(key, vm),
			Self::Book(book) => book.get_index(key, vm),
			Self::Codex(codex) => codex.get_index(key, vm),
			Self::Imitation(imitation) => imitation.get_index(key, vm),
			_ => Err(RuntimeError::OperationNotSupported { kind: self.genus(), func: "[]" })
		}
	}
}

impl SetIndex for Value {
	fn set_index(&self, key: Value, value: Value, vm: &mut Vm) -> Result<()> {
		match self {
			Self::Book(book) => book.set_index(key, value, vm),
			Self::Codex(codex) => codex.set_index(key, value, vm),
			Self::Imitation(imitation) => imitation.set_index(key, value, vm),
			_ => Err(RuntimeError::OperationNotSupported { kind: self.genus(), func: "[]=" })
		}
	}
}

impl GetAttr for Value {
	fn get_attr(&self, attr: &str, vm: &mut Vm) -> Result<Value> {
		match attr {
			"-@" => Ok(journey::Bound::new(self.clone(), "-@").into()),
			"+" => Ok(journey::Bound::new(self.clone(), "+").into()),
			"-" => Ok(journey::Bound::new(self.clone(), "-").into()),
			"*" => Ok(journey::Bound::new(self.clone(), "*").into()),
			"/" => Ok(journey::Bound::new(self.clone(), "/").into()),
			"%" => Ok(journey::Bound::new(self.clone(), "%").into()),
			"**" => Ok(journey::Bound::new(self.clone(), "**").into()),
			"!" => Ok(journey::Bound::new(self.clone(), "!").into()),
			"==" => Ok(journey::Bound::new(self.clone(), "==").into()),
			"!=" => Ok(journey::Bound::new(self.clone(), "!=").into()),
			"<" => Ok(journey::Bound::new(self.clone(), "<").into()),
			"<=" => Ok(journey::Bound::new(self.clone(), "<=").into()),
			">" => Ok(journey::Bound::new(self.clone(), ">").into()),
			">=" => Ok(journey::Bound::new(self.clone(), ">=").into()),
			"<=>" => Ok(journey::Bound::new(self.clone(), "<=>").into()),
			"[]" => Ok(journey::Bound::new(self.clone(), "[]").into()),
			"[]=" => Ok(journey::Bound::new(self.clone(), "[]=").into()),
			"to_veracity" => Ok(journey::Bound::new(self.clone(), "to_veracity").into()),
			"to_numeral" => Ok(journey::Bound::new(self.clone(), "to_numeral").into()),
			"to_text" => Ok(journey::Bound::new(self.clone(), "to_text").into()),
			"to_book" => Ok(journey::Bound::new(self.clone(), "to_book").into()),
			"to_codex" => Ok(journey::Bound::new(self.clone(), "to_codex").into()),
			_ => match self {
				Self::Numeral(numeral) => numeral.get_attr(attr, vm),
				Self::Text(text) => text.get_attr(attr, vm),

				Self::Book(book) => book.get_attr(attr, vm),
				Self::Codex(codex) => codex.get_attr(attr, vm),

				Self::Form(form) => form.get_attr(attr, vm),
				Self::Imitation(imitation) => imitation.get_attr(attr, vm),
				Self::Journey(journey) => journey.get_attr(attr, vm),
				_ => Err(RuntimeError::UnknownAttribute(attr.to_string())),
			},
		}
	}
}

impl SetAttr for Value {
	fn set_attr(&self, attr: &str, value: Value, vm: &mut Vm) -> Result<()> {
		match self {
			Self::Form(form) => form.set_attr(attr, value, vm),
			Self::Imitation(imitation) => imitation.set_attr(attr, value, vm),
			_ => Err(RuntimeError::OperationNotSupported { kind: self.genus(), func: ".=" })
		}
	}
}