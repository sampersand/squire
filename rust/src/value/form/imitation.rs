use super::Form;
use std::sync::Arc;
use parking_lot::RwLock;
use std::hash::{Hash, Hasher};
use crate::runtime::{Result, Error as RuntimeError, Vm};
use crate::value::{Value, ValueKind, Text, Numeral, Journey, GetAttr};
use std::fmt::{self, Debug, Formatter};

pub struct Imitation {
	form: Arc<Form>,
	fields: Box<[RwLock<Value>]>
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
	// pub fn call_method(&self, method: &str, args: Args, vm: &mut Vm) -> Result<Value> {
	// 	self.get_change(method)
	// }
	pub fn to_veracity(&self, vm: &mut Vm) -> Result<bool> {
		// if let Some(to_veracity) = self.get_change("to_Veracity") {
			// to_veracity.call()
	// _ => Err(RuntimeError::OperationNotSupported { kind: self.form().clone(), func: "-" })

		let _ = vm; todo!()
	}

	pub fn to_text(&self, vm: &mut Vm) -> Result<Text> {
		let _ = vm; todo!();
	}

	pub fn to_numeral(&self, vm: &mut Vm) -> Result<Numeral> {
		let _ = vm; todo!();
	}

	fn valuekind(&self) -> ValueKind {
		ValueKind::Imitation(self.form().clone())
	}

	pub fn try_neg(&self, vm: &mut Vm) -> Result<Value> {
		let _ = vm;
		Err(RuntimeError::OperationNotSupported { kind: self.valuekind(), func: "-@" })
	}

	pub fn try_add(&self, rhs: &Value, vm: &mut Vm) -> Result<Value> { let _ = (rhs, vm); todo!(); }
	pub fn try_sub(&self, rhs: &Value, vm: &mut Vm) -> Result<Value> { let _ = (rhs, vm); todo!(); }
	pub fn try_mul(&self, rhs: &Value, vm: &mut Vm) -> Result<Value> { let _ = (rhs, vm); todo!(); }
	pub fn try_div(&self, rhs: &Value, vm: &mut Vm) -> Result<Value> { let _ = (rhs, vm); todo!(); }
	pub fn try_rem(&self, rhs: &Value, vm: &mut Vm) -> Result<Value> { let _ = (rhs, vm); todo!(); }
	pub fn try_pow(&self, rhs: &Value, vm: &mut Vm) -> Result<Value> { let _ = (rhs, vm); todo!(); }

	pub fn try_not(&self, vm: &mut Vm) -> Result<bool> { let _ = vm; todo!(); }
	pub fn try_eql(&self, rhs: &Value, vm: &mut Vm) -> Result<bool> { let _ = (rhs, vm); todo!(); }
	pub fn try_neq(&self, rhs: &Value, vm: &mut Vm) -> Result<bool> { let _ = (rhs, vm); todo!(); }
	pub fn try_lth(&self, rhs: &Value, vm: &mut Vm) -> Result<bool> { let _ = (rhs, vm); todo!(); }
	pub fn try_leq(&self, rhs: &Value, vm: &mut Vm) -> Result<bool> { let _ = (rhs, vm); todo!(); }
	pub fn try_gth(&self, rhs: &Value, vm: &mut Vm) -> Result<bool> { let _ = (rhs, vm); todo!(); }
	pub fn try_geq(&self, rhs: &Value, vm: &mut Vm) -> Result<bool> { let _ = (rhs, vm); todo!(); }
	pub fn try_cmp(&self, rhs: &Value, vm: &mut Vm) -> Result<Value> { let _ = (rhs, vm); todo!(); }

	pub fn try_index(&self, by: &Value, vm: &mut Vm) -> Result<Value> { let _ = (by, vm); todo!(); }
	pub fn try_index_assign(&self, by: Value, with: Value, vm: &mut Vm) -> Result<()> { let _ = (by, with, vm); todo!(); }
}

impl GetAttr for Imitation {
	fn get_attr(&self, attr: &str, _: &mut Vm) -> Result<Value> {
		// in the future, we might want to have getters/setters
		self.get_matter(attr)
			.or_else(|| self.get_change(attr).map(|method| Value::from(Value::Journey(method.clone()))))
			.ok_or_else(|| RuntimeError::UnknownAttribute(attr.to_string()))
	}
}
