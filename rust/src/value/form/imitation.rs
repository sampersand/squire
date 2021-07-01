use super::Form;
use std::sync::Arc;
use parking_lot::RwLock;
use std::hash::{Hash, Hasher};
use crate::runtime::{Result, Error as RuntimeError, Args, Vm};
use std::fmt::{self, Debug, Formatter};
use crate::value::{Value, ValueKind, Text, Numeral, Veracity, Journey};
use crate::value::ops::{
	ConvertTo,
	Negate, Add, Subtract, Multiply, Divide, Modulo, Power,
	IsEqual, Compare, Call,
	GetAttr, SetAttr, GetIndex, SetIndex
};

pub struct Imitation {
	form: Arc<Form>,
	fields: Box<[RwLock<Value>]>
}

#[derive(Debug, Clone)]
pub struct Change {
	imitation: Arc<Imitation>,
	journey: Arc<Journey>
}

impl Debug for Imitation {
	fn fmt(&self, f: &mut Formatter<'_>) -> fmt::Result {
		struct FieldMapper<'a>(&'a Form, &'a [RwLock<Value>]);

		impl Debug for FieldMapper<'_> {
			fn fmt(&self, f: &mut Formatter<'_>) -> fmt::Result {
				let mut map = f.debug_map();

				for (index, name) in self.0.matter_names().iter().enumerate() {
					map.entry(name, &*self.1[index].read());
				}

				map.finish()
			}
		}

		f.debug_tuple("Imitation")
			.field(&self.form)
			.field(&FieldMapper(&self.form, &self.fields))
			.finish()
	}
}

impl Imitation {
	pub fn new(form: Arc<Form>, fields: Vec<Value>) -> Self {
		assert_eq!(form.matter_names().len(), fields.len());

		Self {
			form,
			fields: fields.into_iter().map(RwLock::new).collect()
		}
	}

	pub fn form(&self) -> &Arc<Form> {
		&self.form
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
		self.form
			.matter_names()
			.iter()
			.position(|name| name == key)
			.map(|index| func(&self.fields[index].read()))
	}

	pub fn with_field_mut<F: FnOnce(&mut Value) -> T, T>(&self, key: &str, func: F) -> Option<T> {
		self.form.matter_names()
			.iter()
			.position(|name| name == key)
			.map(|index| func(&mut self.fields[index].write()))
	}

	pub fn get_change(&self, key: &str) -> Option<&Arc<Journey>> {
		self.form.changes().get(key)
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
		(self as *const _) == (rhs as *const _)
	}
}

impl Hash for Imitation {
	fn hash<H: Hasher>(&self, h: &mut H) {
		(self as *const _  as usize).hash(h);
	}
}


impl Imitation {
	pub fn call_method(&self, func: &'static str, args: Args, vm: &mut Vm) -> Result<Value> {
		let _ = (func, args, vm);
		todo!()
		// match self.get_change(func) {
		// 	Some(change) => change.call(args, vm),
		// 	None => Err(RuntimeError::OperationNotSupported { kind: ValueKind::Imitation(self.form().clone()), func })
		// }
	}

	fn kind(&self) -> ValueKind {
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

		expect_a!(self.call_method(NAME, Args::default(), vm)?, Veracity, NAME)
	}
}

impl Compare for Imitation {
	fn compare(&self, rhs: &Value, vm: &mut Vm) -> Result<Option<std::cmp::Ordering>> {
		let _ = (rhs, vm);
		todo!();
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
		// in the future, we might want to have getters/setters
		self.get_matter(attr)
			.or_else(|| self.get_change(attr).map(|method| Value::from(Value::Journey(method.clone()))))
			.ok_or_else(|| RuntimeError::UnknownAttribute(attr.to_string()))
	}
}

impl SetAttr for Imitation {
	fn set_attr(&self, attr: &str, value: Value, _: &mut Vm) -> Result<()> {
		let _ = (attr, value);
		todo!();
		// // in the future, we might want to have getters/setters
		// self.get_matter(attr)
		// 	.or_else(|| self.get_change(attr).map(|method| Value::from(Value::Journey(method.clone()))))
		// 	.ok_or_else(|| RuntimeError::UnknownAttribute(attr.to_string()))
	}
}
