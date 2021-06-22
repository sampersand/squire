use crate::{Value, Journey};
use std::hash::{Hash, Hasher};

use std::sync::{Arc, Mutex};
use std::collections::{HashMap, HashSet};

mod imitation;
pub use imitation::*;

type Change = Journey;
type Recollection = Journey;

#[derive(Debug)]
pub struct Form {
	name: String,
	parents: Vec<Arc<Form>>,

	functions: HashMap<String, Recollection>,
	statics: HashMap<String, Mutex<Value>>,

	field_names: Vec<String>,
	methods: HashSet<Change>,
	constructor: Option<Journey>
}

impl Eq for Form {}
impl PartialEq for Form {
	fn eq(&self, rhs: &Self) -> bool {
		(self as *const _) == (rhs as *const _)
	}
}

impl Form {
	pub fn new(name: impl ToString) -> Self {
		Self {
			name: name.to_string(),
			parents: Default::default(),
			functions: Default::default(),
			statics: Default::default(),
			field_names: Default::default(),
			methods: Default::default(),
			constructor: Default::default(),
		}
	}

	pub fn get_recollection(&self, name: &str) -> Option<&Recollection> {
		self.functions.get(name)
	}

	pub fn field_names(&self) -> &[String] {
		&self.field_names
	}

	pub fn methods(&self) -> &HashSet<Change> {
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