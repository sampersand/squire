use crate::runtime::{Vm, Error as RuntimeError, Args};
use crate::value::Value;
use std::hash::{Hash, Hasher};
use std::sync::Arc;
use std::fmt::{self, Debug, Formatter};

pub struct BuiltinJourney {
	name: &'static str,
	func: Box<dyn Fn(Args, &mut Vm) -> Result<Value, RuntimeError>>
}

impl Debug for BuiltinJourney {
	fn fmt(&self, f: &mut Formatter<'_>) -> fmt::Result {
		f.debug_tuple("BuiltinJourney")
			.field(&self.name)
			.finish()
	}
}

impl Eq for BuiltinJourney {}
impl PartialEq for BuiltinJourney {
	fn eq(&self, rhs: &Self) -> bool {
		self.name == rhs.name
	}
}

impl Hash for BuiltinJourney {
	fn hash<H: Hasher>(&self, h: &mut H) {
		self.name.hash(h);
	}
}

impl BuiltinJourney {
	pub fn new<F: 'static>(name: &'static str, func: F) -> Self
	where
		F: Fn(Args, &mut Vm) -> Result<Value, RuntimeError>
	{
		Self { name, func: Box::new(func) }
	}

	pub fn name(&self) -> &str {
		self.name
	}

	pub fn run(&self, args: Args, vm: &mut Vm) -> Result<Value, RuntimeError> {
		(self.func)(args, vm)
	}
}

pub fn defaults() -> Vec<Arc<BuiltinJourney>> {
	vec![
		Arc::new(BuiltinJourney::new("dump", |args, _| {
			println!("{:?}", args.positional(0).unwrap());
			Ok(args.positional(0).unwrap().clone())
		})),
		Arc::new(BuiltinJourney::new("proclaim", |args, vm| {
			print!("{:?}", args.positional(0).unwrap().convert_to::<crate::value::Text>(vm)?);
			Ok(Value::Null)
		})),
		]
}
