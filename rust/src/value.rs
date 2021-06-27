use std::sync::Arc;
// use parking_lot::RwLock;
pub use crate::{Journey, Form, Imitation};
use crate::runtime::{Vm, Result, Error, Args};
use std::fmt::{self, Display, Formatter};

mod array;
mod codex;
pub mod builtin;
pub mod numeral;
pub mod text;

pub use text::Text;
pub use array::Array;
pub use codex::Codex;
pub use numeral::Numeral;
pub use builtin::BuiltinJourney;

#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub enum Value {
	Null,
	Veracity(bool),
	Numeral(Numeral),
	// idea: fractions instead of floats.
	Text(Text),

	Array(Array),
	Codex(Codex),

	Form(Arc<Form>),
	Imitation(Arc<Imitation>),
	Journey(Journey),
	BuiltinJourney(Arc<BuiltinJourney>),
}

#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub enum ValueKind {
	Null,
	Veracity,
	Numeral,
	Text,
	Array,
	Form,
	Codex,
	Imitation(Arc<Form>),
	Journey,
	BuiltinJourney
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
	pub fn call(&self, args: Args, vm: &mut Vm) -> Result<Self> {
		match self {
			Self::Journey(journey) => journey.call(args, vm),
			Self::Form(_) => todo!(),
			Self::Imitation(_) => todo!(),
			Self::BuiltinJourney(builtin) => builtin.run(args, vm),
			_ => Err(Error::OperationNotSupported { kind: self.classify(), func: "()" })
		}
	}

	pub fn classify(&self) -> ValueKind {
		match self {
			Self::Null => ValueKind::Null,
			Self::Veracity(_) => ValueKind::Veracity,
			Self::Numeral(_) => ValueKind::Numeral,
			Self::Text(_) => ValueKind::Text,
			Self::Array(_) => ValueKind::Array,
			Self::Codex(_) => ValueKind::Codex,
			Self::Form(_) => ValueKind::Form,
			Self::Imitation(imitation) => ValueKind::Imitation(imitation.form().clone()),
			Self::Journey(_) => ValueKind::Journey,
			Self::BuiltinJourney(_) => ValueKind::BuiltinJourney,
		}
	}
}

impl Value {
	pub fn to_veracity(&self, vm: &mut Vm) -> Result<bool> {
		match self {
			Self::Null => Ok(false),
			Self::Veracity(veracity) => Ok(*veracity),
			Self::Numeral(numeral) => Ok(*numeral != 0),
			Self::Text(text) => Ok(!text.is_empty()),
			Self::Imitation(imitation) => imitation.to_veracity(vm),
			_ => Err(Error::OperationNotSupported { kind: self.classify(), func: "to_veracity" })
		}
	}

	pub fn to_text(&self, vm: &mut Vm) -> Result<Text> {
		match self {
			Self::Null => Ok(Text::new("null")),
			Self::Veracity(veracity) => Ok(Text::new(veracity)),
			Self::Numeral(numeral) => Ok(Text::new(numeral)),
			Self::Text(text) => Ok(text.clone()),
			Self::Imitation(imitation) => imitation.to_text(vm),
			_ => Err(Error::OperationNotSupported { kind: self.classify(), func: "to_text" })
		}
	}

	pub fn to_numeral(&self, vm: &mut Vm) -> Result<Numeral> {
		match self {
			Self::Null => Ok(Numeral::new(0)),
			Self::Veracity(veracity) => Ok(Numeral::new(*veracity as i64)),
			Self::Numeral(numeral) => Ok(*numeral),
			Self::Text(text) => Ok(text.as_str().parse()?),
			Self::Imitation(imitation) => imitation.to_numeral(vm),
			_ => Err(Error::OperationNotSupported { kind: self.classify(), func: "to_numeral" })
		}
	}

	pub fn try_neg(&self, vm: &mut Vm) -> Result<Self> {
		match self {
			Self::Numeral(numeral) => Ok(Self::Numeral(-*numeral)),
			Self::Imitation(imitation) => imitation.try_neg(vm),
			_ => Err(Error::OperationNotSupported { kind: self.classify(), func: "-@" })
		}
	}

	pub fn try_add(&self, rhs: &Self, vm: &mut Vm) -> Result<Self> {
		// if the rhs is a string, convert both to a string.
		if let Self::Text(rhs) = rhs {
			return Ok(Self::Text(self.to_text(vm)? + rhs));
		}

		match self {
			Self::Numeral(numeral) => Ok(Self::Numeral(*numeral + rhs.to_numeral(vm)?)),
			Self::Text(text) => Ok(Self::Text(text.clone() + rhs.to_text(vm)?)),
			Self::Array(array) =>
				if let Self::Array(rhs) = rhs {
					Ok(Self::Array(array.clone() + rhs.clone()))
				} else {
					Err(Error::InvalidOperand { kind: rhs.classify(), func: "+" })
				},
			Self::Codex(codex) =>
				if let Self::Codex(rhs) = rhs {
					Ok(Self::Codex(codex.clone() + rhs.clone()))
				} else {
					Err(Error::InvalidOperand { kind: rhs.classify(), func: "+" })
				},

			Self::Imitation(imitation) => imitation.try_add(rhs, vm),
			_ => Err(Error::OperationNotSupported { kind: self.classify(), func: "+" })
		}
	}

	pub fn try_sub(&self, rhs: &Self, vm: &mut Vm) -> Result<Self> {
		match self {
			Self::Numeral(numeral) => Ok(Self::Numeral(*numeral - rhs.to_numeral(vm)?)),
			Self::Array(array) =>
				if let Self::Array(rhs) = rhs {
					Ok(Self::Array(array.clone() - rhs.iter()))
				} else {
					Err(Error::InvalidOperand { kind: rhs.classify(), func: "-" })
				},
			Self::Codex(codex) =>
				if let Self::Codex(rhs) = rhs {
					Ok(Self::Codex(codex.clone() - rhs.iter().map(|(key, _)| key)))
				} else {
					Err(Error::InvalidOperand { kind: rhs.classify(), func: "-" })
				},

			Self::Imitation(imitation) => imitation.try_sub(rhs, vm),
			_ => Err(Error::OperationNotSupported { kind: self.classify(), func: "-" })
		}
	}

	pub fn try_mul(&self, rhs: &Self, vm: &mut Vm) -> Result<Self> {
		match self {
			Self::Numeral(numeral) => Ok(Self::Numeral(*numeral * rhs.to_numeral(vm)?)),
			Self::Text(text) => 
				if let Ok(amount) = <usize as std::convert::TryFrom<i64>>::try_from(rhs.to_numeral(vm)?.get()) {
					Ok(Self::Text(text.clone() * amount))
				} else {
					Err(Error::ValueError("cannot repeat a text negative times.".to_string()))
				},

			Self::Imitation(imitation) => imitation.try_mul(rhs, vm),
			_ => Err(Error::OperationNotSupported { kind: self.classify(), func: "*" })
		}
	}

	pub fn try_div(&self, rhs: &Self, vm: &mut Vm) -> Result<Self> {
		match self {
			Self::Numeral(numeral) => 
				match rhs.to_numeral(vm)?.get() {
					0 => Err(Error::DivisionByZero),
					other => Ok(Self::Numeral(*numeral / other)),
				},
			Self::Imitation(imitation) => imitation.try_div(rhs, vm),
			_ => Err(Error::OperationNotSupported { kind: self.classify(), func: "/" })
		}
	}

	pub fn try_rem(&self, rhs: &Self, vm: &mut Vm) -> Result<Self> {
		match self {
			Self::Numeral(numeral) => 
				match rhs.to_numeral(vm)?.get() {
					0 => Err(Error::DivisionByZero),
					other => Ok(Self::Numeral(*numeral % other)),
				},
			Self::Imitation(imitation) => imitation.try_rem(rhs, vm),
			_ => Err(Error::OperationNotSupported { kind: self.classify(), func: "%" })
		}
	}

	pub fn try_pow(&self, rhs: &Self, vm: &mut Vm) -> Result<Self> {
		const U32MAX_AS_I64: i64 = u32::MAX as i64;

		match self {
			Self::Numeral(numeral) => 
				match rhs.to_numeral(vm)?.get() {
					exp @ (i64::MIN..=-1) =>
						match numeral.get() {
							0 => Err(Error::DivisionByZero),
							-1 if exp.abs() % 2 == 0 => Ok(Self::Numeral(Numeral::new(1))),
							1 | -1 => Ok(Self::Numeral(*numeral)),
							_ => Ok(Self::Numeral(Numeral::new(0)))
						},
					exp @ (0..=U32MAX_AS_I64) => Ok(Self::Numeral(numeral.pow(exp as u32))),
					_ => Err(Error::OutOfBounds) // or should it be infinity or somethin? idk.
				},
			Self::Imitation(imitation) => imitation.try_pow(rhs, vm),
			_ => Err(Error::OperationNotSupported { kind: self.classify(), func: "**" })
		}
	}

	pub fn try_not(&self, vm: &mut Vm) -> Result<bool> {
		Ok(!self.to_veracity(vm)?)
	}

	pub fn try_eql(&self, rhs: &Self, vm: &mut Vm) -> Result<bool> {
		match (self, rhs) {
			(Self::Null, Self::Null) => Ok(true),
			(Self::Veracity(lhs), Self::Veracity(rhs)) => Ok(lhs == rhs),
			(Self::Numeral(lhs), Self::Numeral(rhs)) => Ok(lhs == rhs),
			(Self::Text(lhs), Self::Text(rhs)) => Ok(lhs == rhs),
			(Self::Array(lhs), Self::Array(rhs)) => lhs.try_eql(rhs, vm),
			(Self::Codex(lhs), Self::Codex(rhs)) => lhs.try_eql(rhs, vm),
			(Self::Form(lhs), Self::Form(rhs)) => Ok(lhs == rhs),
			(Self::Imitation(lhs), _) => lhs.try_eql(rhs, vm),
			(Self::Journey(lhs), Self::Journey(rhs)) => Ok(lhs == rhs),
			(Self::BuiltinJourney(lhs), Self::BuiltinJourney(rhs)) => Ok(lhs == rhs),
			_ => Ok(false)
		}
	}

	pub fn try_neq(&self, rhs: &Self, vm: &mut Vm) -> Result<bool> {
		Ok(!self.try_eql(rhs, vm)?)
	}

	pub fn try_lth(&self, rhs: &Self, vm: &mut Vm) -> Result<bool> {
		self.try_cmp(rhs, vm).map(|ord| matches!(ord, Self::Numeral(ord) if ord < 0))
	}

	pub fn try_leq(&self, rhs: &Self, vm: &mut Vm) -> Result<bool> {
		self.try_cmp(rhs, vm).map(|ord| matches!(ord, Self::Numeral(ord) if ord <= 0))
	}

	pub fn try_gth(&self, rhs: &Self, vm: &mut Vm) -> Result<bool> {
		self.try_cmp(rhs, vm).map(|ord| matches!(ord, Self::Numeral(ord) if ord > 0))
	}

	pub fn try_geq(&self, rhs: &Self, vm: &mut Vm) -> Result<bool> {
		self.try_cmp(rhs, vm).map(|ord| matches!(ord, Self::Numeral(ord) if ord >= 0))
	}

	pub fn try_cmp(&self, rhs: &Self, vm: &mut Vm) -> Result<Self> {
		let ord =
			match (self, rhs) {
				(Self::Null, Self::Null) => std::cmp::Ordering::Equal,
				(Self::Veracity(lhs), Self::Veracity(rhs)) => lhs.cmp(rhs),
				(Self::Numeral(lhs), Self::Numeral(rhs)) => lhs.cmp(rhs),
				(Self::Text(lhs), Self::Text(rhs)) => lhs.cmp(rhs),
				(Self::Array(lhs), Self::Array(rhs)) => lhs.try_cmp(rhs, vm)?,
				(Self::Codex(lhs), Self::Codex(rhs)) =>
					if let Some(ord) = lhs.try_partial_cmp(rhs, vm)? {
						ord
					} else {
						return Ok(Self::Null);
					},
				(Self::Imitation(imitation), _) => return imitation.try_cmp(rhs, vm),
				(Self::Form(_) | Self::Journey(_) | Self::BuiltinJourney(_), _) => 
					return Err(Error::OperationNotSupported { kind: self.classify(), func: "<=>" }),
				_ => return Ok(Self::Null)
			};

		Ok(Numeral::from(ord).into())
	}

	pub fn try_index(&self, by: &Self, vm: &mut Vm) -> Result<Self> {
		match self {
			Self::Text(text) => 
				Ok(text.char_at(by.to_numeral(vm)?.get() as isize)
					.map(|chr| chr.to_string().into())
					.unwrap_or_default()),

			Self::Array(array) =>
				Ok(array
						.get2_maybe_a_better_name(by.to_numeral(vm)?.get() as isize)
						.cloned()
						.unwrap_or_default()),
			Self::Codex(codex) => Ok(codex.get(by).cloned().unwrap_or_default()),
			Self::Imitation(imitation) => imitation.try_index(by, vm),
			_ => Err(Error::OperationNotSupported { kind: self.classify(), func: "[]" })
		}
	}

	pub fn try_index_assign(&mut self, by: Self, with: Self, vm: &mut Vm) -> Result<()> {
		match self {
			Self::Array(array) => {
				array.set2_maybe_a_better_name(by.to_numeral(vm)?.get() as isize, with);
				Ok(())
			},
			Self::Codex(codex) => {
				codex.insert(by, with);
				Ok(())
			},
			Self::Imitation(imitation) => imitation.try_index_assign(by, with, vm),
			_ => Err(Error::OperationNotSupported { kind: self.classify(), func: "[]=" })
		}
	}

	pub fn try_get_attr(&self, attr: &str, vm: &mut Vm) -> Result<Self> {
		let _ = (attr, vm);
		todo!();
	}

	pub fn try_set_attr(&mut self, attr: &str, value: Value, vm: &mut Vm) -> Result<()> {
		let _ = (attr, value, vm);
		todo!();
	}
}