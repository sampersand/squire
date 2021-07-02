use super::Form;
use std::sync::Arc;
use parking_lot::Mutex;
use std::hash::{Hash, Hasher};
use crate::runtime::{Result, Error as RuntimeError, Args, Vm};
use std::fmt::{self, Debug, Formatter};
use crate::value::{Value, ValueKind, Text, Numeral, Veracity, Journey, Book, Codex, BoundJourney};
use crate::value::ops::{
	ConvertTo, Dump,
	Negate, Add, Subtract, Multiply, Divide, Modulo, Power,
	IsEqual, Compare, Call,
	GetAttr, SetAttr, GetIndex, SetIndex
};

#[derive(Clone)]
pub struct Imitation(Arc<ImitationInner>);

struct ImitationInner {
	form: Form,
	fields: Box<[Mutex<Value>]>
}

impl Debug for Imitation {
	fn fmt(&self, f: &mut Formatter<'_>) -> fmt::Result {
		struct FieldMapper<'a>(&'a Form, &'a [Mutex<Value>]);

		impl Debug for FieldMapper<'_> {
			fn fmt(&self, f: &mut Formatter<'_>) -> fmt::Result {
				let mut map = f.debug_map();

				for (index, name) in self.0.matter_names().iter().enumerate() {
					map.entry(name, &self.1[index].lock());
				}

				map.finish()
			}
		}

		f.debug_tuple("Imitation")
			.field(&self.form())
			.field(&FieldMapper(&self.form(), &self.0.fields))
			.finish()
	}
}

impl Imitation {
	pub fn new(form: Form, fields: Vec<Value>) -> Self {
		assert_eq!(form.matter_names().len(), fields.len());

		Self(Arc::new(ImitationInner{
			form,
			fields: fields.into_iter().map(Mutex::new).collect()
		}))
	}

	pub fn form(&self) -> &Form {
		&self.0.form
	}

	// what should the return value be?
	// `&Value` doens't work. Should it be `impl Deref<Target=Value>`?
	// Or should i take a lambda? Or should I expose the `RwLock` directly? Something else?
	pub fn get_matter(&self, key: &str) -> Option<Value> {
		self.with_field(key, Clone::clone)
	}

	pub fn set_field(&self, key: &str, value: Value) -> Option<Value> {
		self.with_field_mut(key, move |old| std::mem::replace(old, value))
	}

	pub fn with_field<F: FnOnce(&Value) -> T, T>(&self, key: &str, func: F) -> Option<T> {
		self.form()
			.matter_names()
			.iter()
			.position(|name| name == key)
			.map(|index| func(&self.0.fields[index].lock()))
	}

	pub fn with_field_mut<F: FnOnce(&mut Value) -> T, T>(&self, key: &str, func: F) -> Option<T> {
		self.form().matter_names()
			.iter()
			.position(|name| name == key)
			.map(|index| func(&mut self.0.fields[index].lock()))
	}

	pub fn get_change(&self, key: &str) -> Option<&Journey> {
		self.form().changes().get(key)
			// .get(key)
			// .map(|journey| Change { imitation: self.clone(), journey: journey.clone() })
	}
}

impl Eq for Imitation {}
impl PartialEq for Imitation {
	/// Imitations are only `eq` to  identical ones.
	///
	/// To check for equality within Squire, use [`try_eq`].
	fn eq(&self, rhs: &Self) -> bool {
		Arc::ptr_eq(&self.0, &rhs.0)
	}
}

impl Hash for Imitation {
	fn hash<H: Hasher>(&self, h: &mut H) {
		(Arc::as_ptr(&self.0) as usize).hash(h);
	}
}

impl From<Imitation> for Value {
	#[inline]
	fn from(imitation: Imitation) -> Self {
		Self::Imitation(imitation)
	}
}

impl Imitation {
	pub fn call_method(&self, func: &'static str, mut args: Args, vm: &mut Vm) -> Result<Value> {
		args.add_soul(self.clone().into());

		self.form()
			.changes()
			.get(func)
			.ok_or_else(|| RuntimeError::OperationNotSupported { kind: self.kind(), func })?
			.call(args, vm)
	}

	pub fn kind(&self) -> ValueKind {
		ValueKind::Imitation(self.form().clone())
	}
}

impl Call for Imitation {
	fn call(&self, args: Args, vm: &mut Vm) -> Result<Value> {
		self.call_method("()", args, vm)
	}
}

macro_rules! expect_a {
	($method:expr, $kind:ident, $func:expr) => {
		match $method {
			Value::$kind(value) => Ok(value),
			other =>
				Err(RuntimeError::InvalidReturnType {
					expected: ValueKind::$kind,
					given: other.kind(),
					func: $func
				})
		}
	};
}

impl Dump for Imitation {
	fn dump(&self, to: &mut String, vm: &mut Vm) -> Result<()> {
		// todo: allow for builtin dumps
		to.push_str(&expect_a!(self.call_method("dump", Args::default(), vm)?, Text, "dump")?.as_str());
		Ok(())
	}
}

impl ConvertTo<Veracity> for Imitation {
	fn convert(&self, vm: &mut Vm) -> Result<Veracity> {
		const NAME: &str = "to_veracity";

		expect_a!(self.call_method(NAME, Args::default(), vm)?, Veracity, NAME)
	}
}

impl ConvertTo<Text> for Imitation {
	fn convert(&self, vm: &mut Vm) -> Result<Text> {
		const NAME: &str = "to_text";

		expect_a!(self.call_method(NAME, Args::default(), vm)?, Text, NAME)
	}
}

impl ConvertTo<Numeral> for Imitation {
	fn convert(&self, vm: &mut Vm) -> Result<Numeral> {
		const NAME: &str = "to_numeral";

		expect_a!(self.call_method(NAME, Args::default(), vm)?, Numeral, NAME)
	}
}

impl ConvertTo<Book> for Imitation {
	fn convert(&self, vm: &mut Vm) -> Result<Book> {
		const NAME: &str = "to_text";

		expect_a!(self.call_method(NAME, Args::default(), vm)?, Book, NAME)
	}
}

impl ConvertTo<Codex> for Imitation {
	fn convert(&self, vm: &mut Vm) -> Result<Codex> {
		const NAME: &str = "to_codex";

		expect_a!(self.call_method(NAME, Args::default(), vm)?, Codex, NAME)
	}
}

impl Negate for Imitation {
	fn negate(&self, vm: &mut Vm) -> Result<Value> {
		self.call_method("-@", Args::default(), vm)
	}
}

impl Add for Imitation {
	fn add(&self, rhs: &Value, vm: &mut Vm) -> Result<Value> {
		self.call_method("+", Args::new(&[rhs.clone()]), vm)
	}
}

impl Subtract for Imitation {
	fn subtract(&self, rhs: &Value, vm: &mut Vm) -> Result<Value> {
		self.call_method("-", Args::new(&[rhs.clone()]), vm)
	}
}

impl Multiply for Imitation {
	fn multiply(&self, rhs: &Value, vm: &mut Vm) -> Result<Value> {
		self.call_method("*", Args::new(&[rhs.clone()]), vm)
	}
}

impl Divide for Imitation {
	fn divide(&self, rhs: &Value, vm: &mut Vm) -> Result<Value> {
		self.call_method("/", Args::new(&[rhs.clone()]), vm)
	}
}

impl Modulo for Imitation {
	fn modulo(&self, rhs: &Value, vm: &mut Vm) -> Result<Value> {
		self.call_method("%", Args::new(&[rhs.clone()]), vm)
	}
}

impl Power for Imitation {
	fn power(&self, rhs: &Value, vm: &mut Vm) -> Result<Value> {
		self.call_method("**", Args::new(&[rhs.clone()]), vm)
	}
}

impl IsEqual for Imitation {
	fn is_equal(&self, rhs: &Value, vm: &mut Vm) -> Result<bool> {
		const NAME: &str = "==";

		// unimplemented: if `==` method doesn't exist, use the default one.
		expect_a!(self.call_method(NAME, Args::new(&[rhs.clone()]), vm)?, Veracity, NAME)
	}
}

impl Compare for Imitation {
	fn compare(&self, rhs: &Value, vm: &mut Vm) -> Result<Option<std::cmp::Ordering>> {
		let _ = (rhs, vm);
		unimplemented!();
		// expect_a!(self.call_method(NAME, Args::default(), vm)?, Veracity, NAME)

		// self.call_method("?", Args::new(&[rhs.clone()]), vm)
	}
}

impl GetIndex for Imitation {
	fn get_index(&self, index: &Value, vm: &mut Vm) -> Result<Value> {
		self.call_method("[]", Args::new(&[index.clone()]), vm)
	}
}

impl SetIndex for Imitation {
	fn set_index(&self, index: Value, value: Value, vm: &mut Vm) -> Result<()> {
		self.call_method("[]=", Args::new(&[index, value]), vm)?;

		Ok(())
	}
}

impl GetAttr for Imitation {
	fn get_attr(&self, attr: &str, _: &mut Vm) -> Result<Value> {
		// in the future, we might have getters/setters
		if let Some(matter) = self.get_matter(attr) {
			Ok(matter)
		} else if let Some(change) = self.get_change(attr) {
			Ok(BoundJourney::new(self.clone().into(), change.clone()).into())
		} else {
			Err(RuntimeError::UnknownAttribute(attr.to_string()))
		}
	}
}

impl SetAttr for Imitation {
	fn set_attr(&self, attr: &str, value: Value, _: &mut Vm) -> Result<()> {
		let _ = (attr, value);
		unimplemented!();
		// // in the future, we might want to have getters/setters
		// self.get_matter(attr)
		// 	.or_else(|| self.get_change(attr).map(|method| Value::from(Value::Journey(method.clone()))))
		// 	.ok_or_else(|| RuntimeError::UnknownAttribute(attr.to_string()))
	}
}
