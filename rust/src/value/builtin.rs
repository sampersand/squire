use crate::runtime::{Vm, Error as RuntimeError, Args};
use crate::value::Value;
use crate::value::ops::{Dump, IsEqual, Call};
use std::hash::{Hash, Hasher};
use std::sync::Arc;
use std::fmt::{self, Debug, Formatter};

#[derive(Clone)]
pub struct BuiltinJourney(Arc<BuiltinJourneyInner>);

struct BuiltinJourneyInner {
	name: &'static str,
	func: Box<dyn Fn(Args, &mut Vm) -> Result<Value, RuntimeError>>
}

impl Debug for BuiltinJourney {
	fn fmt(&self, f: &mut Formatter<'_>) -> fmt::Result {
		f.debug_tuple("BuiltinJourney")
			.field(&self.name())
			.finish()
	}
}

impl Eq for BuiltinJourney {}
impl PartialEq for BuiltinJourney {
	fn eq(&self, rhs: &Self) -> bool {
		self.name() == rhs.name()
	}
}

impl Hash for BuiltinJourney {
	fn hash<H: Hasher>(&self, h: &mut H) {
		self.name().hash(h);
	}
}

impl BuiltinJourney {
	pub fn new<F: 'static>(name: &'static str, func: F) -> Self
	where
		F: Fn(Args, &mut Vm) -> Result<Value, RuntimeError>
	{
		Self(Arc::new(BuiltinJourneyInner { name, func: Box::new(func) }))
	}

	pub fn name(&self) -> &str {
		self.0.name
	}
}

impl Dump for BuiltinJourney {
	fn dump(&self, to: &mut String, _: &mut Vm) -> Result<(), RuntimeError> {
		to.push_str("<builtin '");
		to.push_str(self.name());
		to.push_str("'>");

		Ok(())
	}
}


impl IsEqual for BuiltinJourney {
	fn is_equal(&self, rhs: &Value, vm: &mut Vm) -> Result<bool, RuntimeError> {
		let _ = (rhs, vm);
		todo!();
	}
}

impl Call for BuiltinJourney {
	fn call(&self, args: Args, vm: &mut Vm) -> Result<Value, RuntimeError> {
		(self.0.func)(args, vm)
	}
}

pub fn defaults() -> Vec<BuiltinJourney> {
	vec![
		BuiltinJourney::new("dump", |args, _| {
			println!("{:?}", args.positional(0).unwrap());
			Ok(args.positional(0).unwrap().clone())
		}),
		BuiltinJourney::new("proclaim", |args, vm| {
			print!("{}", args.positional(0).unwrap().convert_to::<crate::value::Text>(vm)?);
			Ok(Value::Ni)
		}),
		]
}


