use crate::vm::{Vm, Args, Error as RuntimeError};
use crate::value::{Value, Journey};
use crate::value::ops::{self, Dump, IsEqual, Call, GetAttr};
use std::sync::Arc;

#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub enum BoundKind {
	Journey(Journey),
	Attr(&'static str)
}

#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub struct Bound(Arc<BoundInner>);

#[derive(Debug, PartialEq, Eq, Hash)]
struct BoundInner {
	soul: Value,
	journey: BoundKind
}

impl Bound {
	pub fn new(soul: Value, journey: impl Into<BoundKind>) -> Self {
		Self(Arc::new(BoundInner { soul, journey: journey.into() }))
	}

	pub fn soul(&self) -> &Value {
		&self.0.soul
	}

	pub fn journey(&self) -> &BoundKind {
		&self.0.journey
	}
}

impl From<Journey> for BoundKind {
	#[inline]
	fn from(journey: Journey) -> Self {
		Self::Journey(journey)
	}
}

impl From<&'static str> for BoundKind {
	#[inline]
	fn from(attr: &'static str) -> Self {
		Self::Attr(attr)
	}
}

impl From<Bound> for Value {
	#[inline]
	fn from(bound: Bound) -> Self {
		Self::Journey(bound.into())
	}
}

impl Dump for Bound {
	fn dump(&self, to: &mut String, vm: &mut Vm) -> Result<(), RuntimeError> {
		to.push_str("Bound(");
		self.soul().dump(to, vm)?;

		to.push_str(", ");
		match self.journey() {
			BoundKind::Journey(user) => user.dump(to, vm)?,
			BoundKind::Attr(attr) => {
				to.push('"');
				to.push_str(attr);
				to.push('"');
			}
		}

		to.push(')');
		Ok(())
	}
}

impl IsEqual for Bound {
	fn is_equal(&self, rhs: &Value, _: &mut Vm) -> Result<bool, RuntimeError> {
		if let Value::Journey(Journey::Bound(rhs)) = rhs {
			Ok(self == rhs)
		} else {
			Ok(false)
		}
	}
}

impl Call for Bound {
	fn call(&self, mut args: Args, vm: &mut Vm) -> Result<Value, RuntimeError> {
		let soul = self.soul();

		match self.journey() {
			BoundKind::Journey(user) => {
				args.add_soul(soul.clone());
				user.call(args, vm)
			},
			BoundKind::Attr("to_") => ops::Negate::negate(soul, vm),
			BoundKind::Attr("to_veracity") => soul.convert_to::<crate::value::Veracity>(vm).map(From::from),
			BoundKind::Attr("to_numeral") => soul.convert_to::<crate::value::Numeral>(vm).map(From::from),
			BoundKind::Attr("to_text") => soul.convert_to::<crate::value::Text>(vm).map(From::from),
			BoundKind::Attr("to_book") => soul.convert_to::<crate::value::Book>(vm).map(From::from),
			BoundKind::Attr("to_codex") => soul.convert_to::<crate::value::Codex>(vm).map(From::from),
			BoundKind::Attr("-@") => ops::Negate::negate(soul, vm),
			BoundKind::Attr("+") => ops::Add::add(soul, { args.guard_required_positional(1)?; &args[0] }, vm),
			BoundKind::Attr("-") => ops::Subtract::subtract(soul, { args.guard_required_positional(1)?; &args[0] }, vm),
			BoundKind::Attr("*") => ops::Multiply::multiply(soul, { args.guard_required_positional(1)?; &args[0] }, vm),
			BoundKind::Attr("/") => ops::Divide::divide(soul, { args.guard_required_positional(1)?; &args[0] }, vm),
			BoundKind::Attr("%") => ops::Modulo::modulo(soul, { args.guard_required_positional(1)?; &args[0] }, vm),
			BoundKind::Attr("**") => ops::Power::power(soul, { args.guard_required_positional(1)?; &args[0] }, vm),

			BoundKind::Attr("!") => todo!(),//ops::Add::add(soul, { args.guard_required_positional(1)?; &args[0] }, vm),
			BoundKind::Attr("==") => ops::IsEqual::is_equal(soul, { args.guard_required_positional(1)?; &args[0] }, vm).map(From::from),
			BoundKind::Attr("!=") => todo!(),//ops::Add::add(soul, { args.guard_required_positional(1)?; &args[0] }, vm),
			BoundKind::Attr("<") => todo!(),//ops::Add::add(soul, { args.guard_required_positional(1)?; &args[0] }, vm),
			BoundKind::Attr("<=") => todo!(),//ops::Add::add(soul, { args.guard_required_positional(1)?; &args[0] }, vm),
			BoundKind::Attr(">") => todo!(),//ops::Add::add(soul, { args.guard_required_positional(1)?; &args[0] }, vm),
			BoundKind::Attr(">=") => todo!(),//ops::Add::add(soul, { args.guard_required_positional(1)?; &args[0] }, vm),
			BoundKind::Attr("<=>") => ops::Compare::compare(soul, { args.guard_required_positional(1)?; &args[0] }, vm)
				.map(|x| x.map_or(Value::Ni, |x| crate::value::Numeral::from(x).into())),

			BoundKind::Attr("[]") => ops::GetIndex::get_index(soul, { args.guard_required_positional(1)?; &args[0] }, vm),
			BoundKind::Attr("[]=") => {
				ops::SetIndex::set_index(soul, { args.guard_required_positional(2)?; args[0].clone() }, args[1].clone(), vm).and_then(|_| Ok(args[1].clone()))
			},
			BoundKind::Attr(other) => unreachable!("unknown builtin '{}'?", other)

		}
	}
}

impl GetAttr for Bound {
	fn get_attr(&self, attr: &str, _: &mut Vm) -> Result<Value, RuntimeError> {
		match attr {
			"soul" => Ok(self.soul().clone()),
			"journey" => 
				match self.journey() {
					BoundKind::Journey(user) => Ok(user.clone().into()),
					BoundKind::Attr(_attr) => todo!(),
				},
			_ => Err(RuntimeError::UnknownAttribute(attr.to_string()))
		}
	}
}
