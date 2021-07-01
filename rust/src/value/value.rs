use super::{*, ops::*};
use std::sync::Arc;
// use parking_lot::RwLock;
use crate::runtime::{Vm, Result, Error as RuntimeError, Args};
use std::fmt::{self, Debug, Display, Formatter};


#[derive(Clone, PartialEq, Eq, Hash)]
pub enum Value {
	Null,
	Veracity(Veracity),
	Numeral(Numeral),
	// idea: fractions instead of floats.
	Text(Text),

	Array(Array),
	Codex(Codex),

	Form(Arc<Form>),
	Imitation(Arc<Imitation>),
	Journey(Arc<Journey>),
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

impl Debug for Value {
	fn fmt(&self, f: &mut Formatter<'_>) -> fmt::Result {
		match self {
			Self::Null => Debug::fmt(&Null, f),
			Self::Veracity(veracity) => Debug::fmt(&veracity, f),
			Self::Numeral(numeral) => Debug::fmt(&numeral, f),
			Self::Text(text) => Debug::fmt(&text, f),

			Self::Array(array) => Debug::fmt(&array, f),
			Self::Codex(codex) => Debug::fmt(&codex, f),

			Self::Form(form) => Debug::fmt(&form, f),
			Self::Imitation(imitation) => Debug::fmt(&imitation, f),
			Self::Journey(journey) => Debug::fmt(&journey, f),
			Self::BuiltinJourney(builtinjourney) => Debug::fmt(&builtinjourney, f),
		}
	}
}

impl Display for Value {
	fn fmt(&self, f: &mut Formatter<'_>) -> fmt::Result {
		let _ = f;
		todo!("display (note that `{{:#}}` should be `repr()`)")
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
	pub fn call(&self, args: Args, vm: &mut Vm) -> Result<Self> {
		match self {
			Self::Journey(journey) => journey.call(args, vm),
			Self::Form(form) => form.imitate(args, vm),
			Self::Imitation(_) => todo!(),
			Self::BuiltinJourney(builtin) => builtin.run(args, vm),
			_ => Err(RuntimeError::OperationNotSupported { kind: self.kind(), func: "()" })
		}
	}

	pub fn convert_to<T>(&self, vm: &mut Vm) -> Result<T>
	where
		Self: ConvertTo<T>
	{
		self.convert(vm)
	}

	pub fn kind(&self) -> ValueKind {
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

impl ConvertTo<Veracity> for Value {
	fn convert(&self, vm: &mut Vm) -> Result<Veracity> {
		match self {
			Self::Null => Null.convert(vm),
			Self::Veracity(veracity) => veracity.convert(vm),
			Self::Numeral(numeral) => numeral.convert(vm),
			Self::Text(text) => text.convert(vm),
			Self::Imitation(imitation) => imitation.convert(vm),
			_ => Err(RuntimeError::CannotConvert { from: self.kind(), to: ValueKind::Veracity })
		}
	}
}

impl ConvertTo<Numeral> for Value {
	fn convert(&self, vm: &mut Vm) -> Result<Numeral> {
		match self {
			Self::Null => Null.convert(vm),
			Self::Veracity(veracity) => veracity.convert(vm),
			Self::Numeral(numeral) => numeral.convert(vm),
			Self::Text(text) => text.convert(vm),
			Self::Imitation(imitation) => imitation.convert(vm),
			_ => Err(RuntimeError::CannotConvert { from: self.kind(), to: ValueKind::Numeral })
		}
	}
}

impl ConvertTo<Text> for Value {
	fn convert(&self, vm: &mut Vm) -> Result<Text> {
		match self {
			Self::Null => Null.convert(vm),
			Self::Veracity(veracity) => veracity.convert(vm),
			Self::Numeral(numeral) => numeral.convert(vm),
			Self::Text(text) => text.convert(vm),
			Self::Imitation(imitation) => imitation.convert(vm),
			_ => Err(RuntimeError::CannotConvert { from: self.kind(), to: ValueKind::Text })
		}
	}
}

impl Negate for Value {
	fn negate(&self, vm: &mut Vm) -> Result<Self> {
		match self {
			Self::Numeral(numeral) => numeral.negate(vm).map(Self::Numeral),
			Self::Imitation(imitation) => imitation.negate(vm),
			_ => Err(RuntimeError::OperationNotSupported { kind: self.kind(), func: "-@" })
		}
	}
}

impl Add for Value {
	fn add(&self, rhs: &Self, vm: &mut Vm) -> Result<Self> {
		// if the rhs is a string, convert both to a string.
		if let Self::Text(rhs) = rhs {
			return Ok(Self::Text(self.to_text(vm)? + rhs));
		}

		match self {
			Self::Numeral(numeral) => numeral.add(rhs.to_numeral(vm)?, vm).map(Self::Numeral),
			Self::Text(text) => Ok(Self::Text(text.clone() + rhs.to_text(vm)?)),
			Self::Array(array) =>
				if let Self::Array(rhs) = rhs {
					Ok(Self::Array(/*Arc::new*/(/* * */*array).clone() + (/* * */*rhs).clone()))
				} else {
					Err(RuntimeError::InvalidOperand { kind: rhs.kind(), func: "+" })
				},
			Self::Codex(codex) =>
				if let Self::Codex(rhs) = rhs {
					Ok(Self::Codex(/*Arc::new*/(/* * */*codex).clone() + (/* * */*rhs).clone()))
				} else {
					Err(RuntimeError::InvalidOperand { kind: rhs.kind(), func: "+" })
				},

			Self::Imitation(imitation) => imitation.try_add(rhs, vm),
			_ => Err(RuntimeError::OperationNotSupported { kind: self.kind(), func: "+" })
		}
	}
}

impl Subtract for Value {
	fn subtract(&self, rhs: &Self, vm: &mut Vm) -> Result<Self> {
		match self {
			Self::Numeral(numeral) => Ok(Self::Numeral(*numeral - rhs.to_numeral(vm)?)),
			Self::Array(array) =>
				if let Self::Array(rhs) = rhs {
					Ok(Self::Array(/*Arc::new*/(/* * */*array).clone() - rhs.iter()))
				} else {
					Err(RuntimeError::InvalidOperand { kind: rhs.kind(), func: "-" })
				},
			Self::Codex(codex) =>
				if let Self::Codex(rhs) = rhs {
					Ok(Self::Codex(/*Arc::new*/(/* * */*codex).clone() - rhs.iter().map(|(key, _)| key)))
				} else {
					Err(RuntimeError::InvalidOperand { kind: rhs.kind(), func: "-" })
				},

			Self::Imitation(imitation) => imitation.try_sub(rhs, vm),
			_ => Err(RuntimeError::OperationNotSupported { kind: self.kind(), func: "-" })
		}
	}
}

impl Multiply for Value {
	fn multiply(&self, rhs: &Self, vm: &mut Vm) -> Result<Self> {
		match self {
			Self::Numeral(numeral) => Ok(Self::Numeral(*numeral * rhs.to_numeral(vm)?)),
			Self::Text(text) => 
				if let Ok(amount) = <usize as std::convert::TryFrom<i64>>::try_from(rhs.to_numeral(vm)?.get()) {
					Ok(Self::Text(text.clone() * amount))
				} else {
					Err(RuntimeError::ValueError("cannot repeat a text negative times.".to_string()))
				},

			Self::Imitation(imitation) => imitation.try_mul(rhs, vm),
			_ => Err(RuntimeError::OperationNotSupported { kind: self.kind(), func: "*" })
		}
	}
}

impl Divide for Value {
	fn divide(&self, rhs: &Self, vm: &mut Vm) -> Result<Self> {
		match self {
			Self::Numeral(numeral) => 
				match rhs.to_numeral(vm)?.get() {
					0 => Err(RuntimeError::DivisionByZero),
					other => Ok(Self::Numeral(*numeral / other)),
				},
			Self::Imitation(imitation) => imitation.try_div(rhs, vm),
			_ => Err(RuntimeError::OperationNotSupported { kind: self.kind(), func: "/" })
		}
	}
}

impl Modulo for Value {
	fn modulo(self, rhs: &Self, vm: &mut Vm) -> Result<Self> {
		match self {
			Self::Numeral(numeral) => 
				match rhs.to_numeral(vm)?.get() {
					0 => Err(RuntimeError::DivisionByZero),
					other => Ok(Self::Numeral(*numeral % other)),
				},
			Self::Imitation(imitation) => imitation.try_rem(rhs, vm),
			_ => Err(RuntimeError::OperationNotSupported { kind: self.kind(), func: "%" })
		}
	}
}

impl Power for Value {
	fn power(&self, rhs: &Self, vm: &mut Vm) -> Result<Self> {
		const U32MAX_AS_I64: i64 = u32::MAX as i64;

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
			_ => Err(RuntimeError::OperationNotSupported { kind: self.kind(), func: "**" })
		}
	}
}

impl IsEqual for Value {
	fn is_equal(&self, rhs: &Self, vm: &mut Vm) -> Result<bool> {
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
}

impl Compare for Value {
	// pub fn try_neq(&self, rhs: &Self, vm: &mut Vm) -> Result<bool> {
	// 	Ok(!self.try_eql(rhs, vm)?)
	// }

	// pub fn try_lth(&self, rhs: &Self, vm: &mut Vm) -> Result<bool> {
	// 	self.try_cmp(rhs, vm).map(|ord| matches!(ord, Self::Numeral(ord) if ord < 0))
	// }

	// pub fn try_leq(&self, rhs: &Self, vm: &mut Vm) -> Result<bool> {
	// 	self.try_cmp(rhs, vm).map(|ord| matches!(ord, Self::Numeral(ord) if ord <= 0))
	// }

	// pub fn try_gth(&self, rhs: &Self, vm: &mut Vm) -> Result<bool> {
	// 	self.try_cmp(rhs, vm).map(|ord| matches!(ord, Self::Numeral(ord) if ord > 0))
	// }

	// pub fn try_geq(&self, rhs: &Self, vm: &mut Vm) -> Result<bool> {
	// 	self.try_cmp(rhs, vm).map(|ord| matches!(ord, Self::Numeral(ord) if ord >= 0))
	// }

	fn compare(&self, rhs: &Self, vm: &mut Vm) -> Result<Option<std::cmp::Ordering>> {
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
					return Err(RuntimeError::OperationNotSupported { kind: self.kind(), func: "<=>" }),
				_ => return Ok(Self::Null)
			};

		Ok(Numeral::from(ord).into())
	}
}

impl GetIndex for Value {
	fn get_index(&self, by: &Self, vm: &mut Vm) -> Result<Self> {
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
			_ => Err(RuntimeError::OperationNotSupported { kind: self.kind(), func: "[]" })
		}
	}
}

impl SetIndex for Value {
	fn set_index(&self, by: Self, with: Self, vm: &mut Vm) -> Result<()> {
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
			_ => Err(RuntimeError::OperationNotSupported { kind: self.kind(), func: "[]=" })
		}
	}
}

impl GetAttr for Value {
	fn get_attr(&self, attr: &str, vm: &mut Vm) -> Result<Self> {
		match self {
			Self::Null => Null.get_attr(attr, vm),
			Self::Form(form) => form.get_attr(attr, vm),
			Self::Imitation(imitation) => imitation.get_attr(attr, vm),
			_ => todo!()
		}
	}
}

impl SetAttr for Value {
	fn set_attr(&self, attr: &str, value: Value, vm: &mut Vm) -> Result<()> {
		let _ = (attr, value, vm);
		todo!();
	}
}