use crate::runtime::{Vm, Args, Error as RuntimeError};
use crate::value::{Value, Journey, BuiltinJourney};
use crate::value::ops::{self, Dump, IsEqual, Call, GetAttr};
use std::sync::Arc;

#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub enum BoundJourneyKind {
   User(Journey),
   Builtin(BuiltinJourney),
   Attr(&'static str)
}

#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub struct BoundJourney(Arc<BoundJourneyInner>);

#[derive(Debug, PartialEq, Eq, Hash)]
struct BoundJourneyInner {
   soul: Value,
   journey: BoundJourneyKind
}

impl BoundJourney {
   pub fn new(soul: Value, journey: impl Into<BoundJourneyKind>) -> Self {
      Self(Arc::new(BoundJourneyInner { soul, journey: journey.into() }))
   }

   pub fn soul(&self) -> &Value {
      &self.0.soul
   }

   pub fn journey(&self) -> &BoundJourneyKind {
      &self.0.journey
   }
}

impl From<Journey> for BoundJourneyKind {
   #[inline]
   fn from(journey: Journey) -> Self {
      Self::User(journey)
   }
}

impl From<BuiltinJourney> for BoundJourneyKind {
   #[inline]
   fn from(builtin: BuiltinJourney) -> Self {
      Self::Builtin(builtin)
   }
}

impl From<&'static str> for BoundJourneyKind {
   #[inline]
   fn from(attr: &'static str) -> Self {
      Self::Attr(attr)
   }
}

impl From<BoundJourney> for Value {
   #[inline]
   fn from(bound: BoundJourney) -> Self {
      Self::BoundJourney(bound)
   }
}

impl Dump for BoundJourney {
   fn dump(&self, to: &mut String, vm: &mut Vm) -> Result<(), RuntimeError> {
      to.push_str("BoundJourney(");
      self.soul().dump(to, vm)?;

      to.push_str(", ");
      match self.journey() {
         BoundJourneyKind::User(user) => user.dump(to, vm)?,
         BoundJourneyKind::Builtin(builtin) => builtin.dump(to, vm)?,
         BoundJourneyKind::Attr(attr) => {
            to.push('"');
            to.push_str(attr);
            to.push('"');
         }
      }

      to.push(')');
      Ok(())
   }
}

impl IsEqual for BoundJourney {
   fn is_equal(&self, rhs: &Value, _: &mut Vm) -> Result<bool, RuntimeError> {
      if let Value::BoundJourney(rhs) = rhs {
         Ok(self == rhs)
      } else {
         Ok(false)
      }
   }
}

impl Call for BoundJourney {
   fn call(&self, mut args: Args, vm: &mut Vm) -> Result<Value, RuntimeError> {
      let soul = self.soul();

      match self.journey() {
         BoundJourneyKind::User(user) => {
            args.add_soul(soul.clone());
            user.call(args, vm)
         },
         BoundJourneyKind::Builtin(builtin) => {
            args.add_soul(soul.clone());
            builtin.call(args, vm)
         },

         BoundJourneyKind::Attr("to_") => ops::Negate::negate(soul, vm),
         BoundJourneyKind::Attr("to_veracity") => soul.convert_to::<crate::value::Veracity>(vm).map(From::from),
         BoundJourneyKind::Attr("to_numeral") => soul.convert_to::<crate::value::Numeral>(vm).map(From::from),
         BoundJourneyKind::Attr("to_text") => soul.convert_to::<crate::value::Text>(vm).map(From::from),
         BoundJourneyKind::Attr("to_book") => soul.convert_to::<crate::value::Book>(vm).map(From::from),
         BoundJourneyKind::Attr("to_codex") => soul.convert_to::<crate::value::Codex>(vm).map(From::from),
         BoundJourneyKind::Attr("-@") => ops::Negate::negate(soul, vm),
         BoundJourneyKind::Attr("+") => ops::Add::add(soul, { args.guard_required_positional(1)?; &args[0] }, vm),
         BoundJourneyKind::Attr("-") => ops::Subtract::subtract(soul, { args.guard_required_positional(1)?; &args[0] }, vm),
         BoundJourneyKind::Attr("*") => ops::Multiply::multiply(soul, { args.guard_required_positional(1)?; &args[0] }, vm),
         BoundJourneyKind::Attr("/") => ops::Divide::divide(soul, { args.guard_required_positional(1)?; &args[0] }, vm),
         BoundJourneyKind::Attr("%") => ops::Modulo::modulo(soul, { args.guard_required_positional(1)?; &args[0] }, vm),
         BoundJourneyKind::Attr("**") => ops::Power::power(soul, { args.guard_required_positional(1)?; &args[0] }, vm),

         BoundJourneyKind::Attr("!") => todo!(),//ops::Add::add(soul, { args.guard_required_positional(1)?; &args[0] }, vm),
         BoundJourneyKind::Attr("==") => ops::IsEqual::is_equal(soul, { args.guard_required_positional(1)?; &args[0] }, vm).map(From::from),
         BoundJourneyKind::Attr("!=") => todo!(),//ops::Add::add(soul, { args.guard_required_positional(1)?; &args[0] }, vm),
         BoundJourneyKind::Attr("<") => todo!(),//ops::Add::add(soul, { args.guard_required_positional(1)?; &args[0] }, vm),
         BoundJourneyKind::Attr("<=") => todo!(),//ops::Add::add(soul, { args.guard_required_positional(1)?; &args[0] }, vm),
         BoundJourneyKind::Attr(">") => todo!(),//ops::Add::add(soul, { args.guard_required_positional(1)?; &args[0] }, vm),
         BoundJourneyKind::Attr(">=") => todo!(),//ops::Add::add(soul, { args.guard_required_positional(1)?; &args[0] }, vm),
         BoundJourneyKind::Attr("<=>") => ops::Compare::compare(soul, { args.guard_required_positional(1)?; &args[0] }, vm)
            .map(|x| x.map_or(Value::Ni, |x| crate::value::Numeral::from(x).into())),

         BoundJourneyKind::Attr("[]") => ops::GetIndex::get_index(soul, { args.guard_required_positional(1)?; &args[0] }, vm),
         BoundJourneyKind::Attr("[]=") => {
            ops::SetIndex::set_index(soul, { args.guard_required_positional(2)?; args[0].clone() }, args[1].clone(), vm).and_then(|_| Ok(args[1].clone()))
         },
         BoundJourneyKind::Attr(other) => unreachable!("unknown builtin '{}'?", other)

      }
   }
}

impl GetAttr for BoundJourney {
   fn get_attr(&self, attr: &str, _: &mut Vm) -> Result<Value, RuntimeError> {
      match attr {
         "soul" => Ok(self.soul().clone()),
         "journey" => 
            match self.journey() {
               BoundJourneyKind::User(user) => Ok(user.clone().into()),
               BoundJourneyKind::Builtin(builtin) => Ok(builtin.clone().into()),
               BoundJourneyKind::Attr(_attr) => todo!(),
            },
         _ => Err(RuntimeError::UnknownAttribute(attr.to_string()))
      }
   }
}
