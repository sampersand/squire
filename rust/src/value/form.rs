use std::hash::{Hash, Hasher};

use std::sync::{Arc, Mutex};
use std::collections::HashMap;
use crate::runtime::{Vm, Error as RuntimeError};
use crate::value::{Value, Journey};

mod imitation;
mod builder;
pub use builder::*;
pub use imitation::*;

#[derive(Debug)]
pub struct Form {
	name: String,
	parents: Vec<Arc<Form>>,

	functions: HashMap<String, Arc<Journey>>,
	essences: HashMap<String, Mutex<Value>>,

	field_names: Vec<String>,
	methods: HashMap<String, Arc<Journey>>,
	constructor: Option<Journey>
}

impl Eq for Form {}
impl PartialEq for Form {
	fn eq(&self, rhs: &Self) -> bool {
		(self as *const _) == (rhs as *const _)
	}
}

impl Form {
	pub fn builder(name: impl ToString) -> FormBuilder {
		FormBuilder(Self {
			name: name.to_string(),
			parents: Default::default(),
			functions: Default::default(),
			essences: Default::default(),
			field_names: Default::default(),
			methods: Default::default(),
			constructor: Default::default(),
		})
	}

	pub fn name(&self) -> &str {
		&self.name
	}

	pub fn get_essence(&self, name: &str) -> Option<Value> {
		self.essences.get(name).map(|x| x.lock().unwrap().clone())
	}

	pub fn get_recall(&self, name: &str) -> Option<&Arc<Journey>> {
		self.functions.get(name)
	}

	pub fn field_names(&self) -> &[String] {
		&self.field_names
	}

	pub fn methods(&self) -> &HashMap<String, Arc<Journey>> {
		&self.methods
	}

	// pub fn get_essence(&self, name: &str) -> Option<&Value> {

	// }

	// pub fn get_
}

impl Hash for Form {
	fn hash<H: Hasher>(&self, h: &mut H) {
		(self as *const _ as usize).hash(h)
	}
}

impl crate::value::GetAttr for Form {
	fn get_attr(&self, attr: &str, _vm: &mut Vm) -> Result<Value, RuntimeError> {
		self.get_essence(attr)
			.or_else(|| self.get_recall(attr).map(|recall| Value::Journey(recall.clone())))
			.ok_or_else(|| RuntimeError::UnknownAttribute(attr.to_string()))
	}
}