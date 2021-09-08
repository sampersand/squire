use crate::vm::{Vm, Error as RuntimeError, Args};
use crate::value::{Value, Text, Numeral, Journey};
use crate::value::ops::{Dump, IsEqual, Call, GetAttr};
use std::hash::{Hash, Hasher};
use std::sync::Arc;
use std::fmt::{self, Debug, Formatter};

#[derive(Clone)]
pub struct Builtin(Arc<BuiltinInner>);

struct BuiltinInner {
	name: &'static str,
	func: Box<dyn Fn(Args, &mut Vm) -> Result<Value, RuntimeError>>
}

impl Debug for Builtin {
	fn fmt(&self, f: &mut Formatter<'_>) -> fmt::Result {
		f.debug_tuple("Builtin")
			.field(&self.name())
			.finish()
	}
}

impl Eq for Builtin {}
impl PartialEq for Builtin {
	fn eq(&self, rhs: &Self) -> bool {
		self.name() == rhs.name()
	}
}

impl Hash for Builtin {
	fn hash<H: Hasher>(&self, h: &mut H) {
		self.name().hash(h);
	}
}

impl Builtin {
	pub fn new<F: 'static>(name: &'static str, func: F) -> Self
	where
		F: Fn(Args, &mut Vm) -> Result<Value, RuntimeError>
	{
		Self(Arc::new(BuiltinInner { name, func: Box::new(func) }))
	}

	pub fn name(&self) -> &str {
		self.0.name
	}
}

impl From<Builtin> for Value {
	#[inline]
	fn from(builtin: Builtin) -> Self {
		Self::Journey(builtin.into())
	}
}

impl Dump for Builtin {
	fn dump(&self, to: &mut String, _: &mut Vm) -> Result<(), RuntimeError> {
		to.push_str("<builtin '");
		to.push_str(self.name());
		to.push_str("'>");

		Ok(())
	}
}


impl IsEqual for Builtin {
	fn is_equal(&self, rhs: &Value, _: &mut Vm) -> Result<bool, RuntimeError> {
		if let Value::Journey(Journey::Builtin(rhs)) = rhs {
			Ok(Arc::ptr_eq(&self.0, &rhs.0))
		} else {
			Ok(false)
		}
	}
}

impl Call for Builtin {
	fn call(&self, args: Args, vm: &mut Vm) -> Result<Value, RuntimeError> {
		(self.0.func)(args, vm)
	}
}

impl GetAttr for Builtin {
	fn get_attr(&self, attr: &str, _: &mut Vm) -> Result<Value, RuntimeError> {
		match attr {
			"name" => Ok(Value::from(self.name().to_string())),
			_ => Err(RuntimeError::UnknownAttribute(attr.to_string()))
		}
	}
}

pub fn dump(args: Args, _: &mut Vm) -> Result<Value, RuntimeError> {
	args.guard_required_positional(1)?;

	println!("{:?}", args[0]);
	Ok(args[0].clone())
}

pub fn proclaim(args: Args, vm: &mut Vm) -> Result<Value, RuntimeError> {
	use std::borrow::Cow;
	let sep =
		if let Some(sep) = args.kwarg("sep") {
			Cow::Owned(sep.convert_to::<Text>(vm)?.to_string())
		} else {
			Cow::Borrowed("")
		};

	let end =
		if let Some(end) = args.kwarg("end") {
			Cow::Owned(end.convert_to::<Text>(vm)?.to_string())
		} else {
			Cow::Borrowed("\n")
		};

	let mut is_first = true;
	for arg in args.positional() {
		if is_first {
			is_first = false;
		} else {
			print!("{}", sep);
		}

		print!("{}", arg.convert_to::<Text>(vm)?);
	}

	print!("{}", end);

	Ok(Value::Ni)
}

pub fn inquire(args: Args, vm: &mut Vm) -> Result<Value, RuntimeError> {
	if args.positional().len() == 1 {
		print!("{}", args[0].convert_to::<Text>(vm)?);
	} else {
		args.guard_required_positional(0)?; // todo: variable argc in guard clause.
	}

	let mut line = String::new();
	std::io::stdin().read_line(&mut line)?;

	Ok(line.into())
}

pub fn dismount(args: Args, vm: &mut Vm) -> Result<Value, RuntimeError> {
	args.guard_required_positional(1)?;

	let status = args[0].convert_to::<Numeral>(vm)?;
	std::process::exit(status.get() as i32);
}

pub fn gamble(args: Args, _: &mut Vm) -> Result<Value, RuntimeError> {
	args.guard_required_positional(0)?;
	todo!();
}

pub fn hex(args: Args, vm: &mut Vm) -> Result<Value, RuntimeError> {
	let _ = (args, vm);
	todo!();
}


pub fn defaults() -> Vec<Builtin> {
	vec![
		Builtin::new("dump", dump),
		Builtin::new("proclaim", proclaim),
		Builtin::new("inquire", inquire),
		Builtin::new("dismount", dismount),
		Builtin::new("gamble", gamble),
		Builtin::new("hex", hex),
	]
}

