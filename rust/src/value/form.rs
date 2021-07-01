use std::hash::{Hash, Hasher};

use std::sync::{Arc, Mutex};
use std::collections::HashMap;
use crate::runtime::{Vm, Args, Error as RuntimeError};
use crate::value::{Value, Journey};
use crate::value::ops::{IsEqual, Call, GetAttr, SetAttr};
use std::fmt::{self, Debug, Formatter};

mod imitation;
mod builder;
pub use builder::*;
pub use imitation::*;

/// A "Class" within Squire
pub struct Form {
	// Name of the class
	name: String,

	// Superclasses
	parents: Vec<Arc<Form>>,

	// Class functions
	recalls: HashMap<String, Arc<Journey>>,

	// Static class fields
	essences: HashMap<String, Mutex<Value>>,

	// Instance variable field names
	matter_names: Vec<String>,

	// Instance methods
	changes: HashMap<String, Arc<Journey>>,

	// Constructor
	imitate: Option<Journey>
}

impl Eq for Form {}
impl PartialEq for Form {
	fn eq(&self, rhs: &Self) -> bool {
		(self as *const _) == (rhs as *const _)
	}
}

struct NonAlternateDebug<'a, I: Debug>(&'a I);

impl<I: Debug> Debug for NonAlternateDebug<'_, I> {
	fn fmt(&self, f: &mut Formatter<'_>) -> fmt::Result {
		write!(f, "{:?}", self.0)
	}
}


impl Debug for Form {
	fn fmt(&self, f: &mut Formatter<'_>) -> fmt::Result {
		if !f.alternate() {
			return f.debug_tuple("Form").field(&self.name).finish();
		}

		let parents = self.parents.iter().map(NonAlternateDebug).collect::<Vec<_>>();
		let recalls = self.recalls.iter().map(|(key, val)| (key, NonAlternateDebug(val))).collect::<HashMap<_,_>>();
		let essences = self.essences.iter().map(|(key, val)| (key, NonAlternateDebug(val))).collect::<HashMap<_,_>>();
		let changes = self.changes.iter().map(|(key, val)| (key, NonAlternateDebug(val))).collect::<HashMap<_,_>>();
		let imitate = self.imitate.as_ref().map(NonAlternateDebug);

		f.debug_struct("Form")
			.field("parents", &parents)
			.field("recalls", &recalls)
			.field("essences", &essences)
			.field("matter_names", &self.matter_names)
			.field("changes", &changes)
			.field("imitate", &imitate)
			.finish()
	}
}


impl Form {
	pub fn builder(name: impl ToString) -> FormBuilder {
		FormBuilder(Self {
			name: name.to_string(),
			parents: Default::default(),
			recalls: Default::default(),
			essences: Default::default(),
			matter_names: Default::default(),
			changes: Default::default(),
			imitate: Default::default(),
		})
	}

	pub fn name(&self) -> &str {
		&self.name
	}

	pub fn get_essence(&self, name: &str) -> Option<Value> {
		self.essences.get(name).map(|x| x.lock().unwrap().clone())
	}

	pub fn get_recall(&self, name: &str) -> Option<&Arc<Journey>> {
		self.recalls.get(name)
	}

	pub fn matter_names(&self) -> &[String] {
		&self.matter_names
	}

	pub fn changes(&self) -> &HashMap<String, Arc<Journey>> {
		&self.changes
	}

	pub fn imitate(self: &Arc<Self>, mut args: Args, vm: &mut Vm) -> Result<Value, RuntimeError> {
		if let Some(ref imitate) = self.imitate {
			let fields = vec![Default::default(); self.matter_names.len()];
			let imitation = Value::Imitation(Imitation::new(self.clone(), fields).into());

			args.add_me(imitation.clone());
			imitate.call(args, vm)?;

			return Ok(imitation);
		}

		if args._as_slice().len() != self.matter_names.len() {
			Err(RuntimeError::ArgumentError { given: args._as_slice().len(), expected: self.matter_names.len() })
		} else {
			Ok(Value::Imitation(Imitation::new(self.clone(), args._as_slice().to_owned()).into()))
		}
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

impl IsEqual for Form {
	fn is_equal(&self, rhs: &Value, _: &mut Vm) -> Result<bool, RuntimeError> {
		if let Value::Form(_form) = rhs {
			todo!();
		} else {
			Ok(false)
		}
	}
}

impl Call for Form {
	fn call(&self, args: Args, vm: &mut Vm) -> Result<Value, RuntimeError> {
		let _ = (args, vm);
		todo!();
	}
}

impl GetAttr for Form {
	fn get_attr(&self, attr: &str, _vm: &mut Vm) -> Result<Value, RuntimeError> {
		self.get_essence(attr)
			.or_else(|| self.get_recall(attr).map(|recall| Value::Journey(recall.clone())))
			.ok_or_else(|| RuntimeError::UnknownAttribute(attr.to_string()))
	}
}

impl SetAttr for Form {
	fn set_attr(&self, attr: &str, value: Value, _: &mut Vm) -> Result<(), RuntimeError> {
		let _ = (attr, value);
		todo!();
	}
}