use super::{Form, Change};
use crate::Value;
use std::sync::Arc;
use parking_lot::RwLock;
use std::hash::{Hash, Hasher};

#[derive(Debug)]
pub struct Imitation {
	form: Arc<Form>,
	fields: Box<[RwLock<Value>]>
}

impl Imitation {
	pub fn new(form: Arc<Form>, fields: Vec<Value>) -> Self {
		assert_eq!(form.field_names().len(), fields.len());

		Self { form, fields: fields.into_iter().map(RwLock::new).collect() }
	}

	pub fn form(&self) -> &Arc<Form> {
		&self.form
	}

	// what should the return value be?
	// `&Value` doens't work. Should it be `impl Deref<Target=Value>`?
	// Or should i take a lambda? Or should I expose the `RwLock` directly? Something else?
	pub fn get_field(&self, key: &str) -> Option<Value> {
		self.with_field(key, Clone::clone)
	}

	pub fn set_field(&self, key: &str, value: Value) -> Option<Value> {
		self.with_field_mut(key, move |old| std::mem::replace(old, value))
	}

	pub fn with_field<F: FnOnce(&Value) -> T, T>(&self, key: &str, func: F) -> Option<T> {
		self.form
			.field_names()
			.iter()
			.position(|name| name == key)
			.map(|index| func(&self.fields[index].read()))
	}

	pub fn with_field_mut<F: FnOnce(&mut Value) -> T, T>(&self, key: &str, func: F) -> Option<T> {
		self.form.field_names()
			.iter()
			.position(|name| name == key)
			.map(|index| func(&mut self.fields[index].write()))
	}

	pub fn get_method(&self, key: &str) -> Option<&Change> {
		self.form.methods().get(key)
	}

	pub fn get(&self, key: &str) -> Option<Value> {
		self.get_field(key)
			.or_else(|| self.get_method(key).map(|method| Value::from(Value::Journey(method.clone()))))
	}
}

impl Hash for Imitation {
	fn hash<H: Hasher>(&self, h: &mut H) {
		let _ = h;
		todo!()
	}
}

impl Eq for Imitation {}
impl PartialEq for Imitation {
	fn eq(&self, _rhs: &Self) -> bool {
		todo!()
	}
}