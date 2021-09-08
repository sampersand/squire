use std::hash::{Hash, Hasher};

use std::sync::Arc;
use parking_lot::Mutex;
use std::collections::HashMap;
use crate::vm::{Vm, Args, Error as RuntimeError};
use crate::value::{Value, Journey};
use crate::value::ops::{Dump, Matches, IsEqual, Call, GetAttr, SetAttr};
use std::fmt::{self, Debug, Formatter};

mod imitation;
mod builder;
pub use builder::*;
pub use imitation::*;

/// A "Class" within Squire
#[derive(Clone)]
pub struct Form(Arc<FormInner>);

struct FormInner {
	// Name of the class
	name: String,

	// Superclasses
	parents: Vec<Form>,

	// Class functions
	recalls: HashMap<String, Journey>,

	// Static class fields
	essences: HashMap<String, Mutex<Value>>,

	// Instance variable field names
	matter_names: Vec<String>,

	// Instance methods
	changes: HashMap<String, Journey>,

	// Constructor
	imitate: Option<Journey>
}

impl Eq for Form {}
impl PartialEq for Form {
	fn eq(&self, rhs: &Self) -> bool {
		Arc::ptr_eq(&self.0, &rhs.0)
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
			return f.debug_tuple("Form").field(&self.0.name).finish();
		}

		let parents = self.0.parents.iter().map(NonAlternateDebug).collect::<Vec<_>>();
		let recalls = self.0.recalls.iter().map(|(key, val)| (key, NonAlternateDebug(val))).collect::<HashMap<_,_>>();
		let essences = self.0.essences.iter().map(|(key, val)| (key, NonAlternateDebug(val))).collect::<HashMap<_,_>>();
		let changes = self.0.changes.iter().map(|(key, val)| (key, NonAlternateDebug(val))).collect::<HashMap<_,_>>();
		let imitate = self.0.imitate.as_ref().map(NonAlternateDebug);

		f.debug_struct("Form")
			.field("parents", &parents)
			.field("recalls", &recalls)
			.field("essences", &essences)
			.field("matter_names", &self.0.matter_names)
			.field("changes", &changes)
			.field("imitate", &imitate)
			.finish()
	}
}


impl Form {
	pub fn builder(name: impl ToString) -> FormBuilder {
		FormBuilder(FormInner {
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
		&self.0.name
	}

	pub fn get_matter_index(&self, name: &str) -> Option<usize> {
		self.0.matter_names.iter().position(|n| n == name)
			.or_else(|| self.0.parents.iter().find_map(|parent| parent.get_matter_index(name)))
	}

	pub fn get_change(&self, name: &str) -> Option<&Journey> {
		self.0.changes.get(name)
			.or_else(|| self.0.parents.iter().find_map(|parent| parent.get_change(name)))
	}

	pub fn get_essence(&self, name: &str) -> Option<&Mutex<Value>> {
		self.0.essences.get(name)
			.or_else(|| self.0.parents.iter().find_map(|parent| parent.get_essence(name)))
	}

	pub fn get_recall(&self, name: &str) -> Option<&Journey> {
		self.0.recalls.get(name)
			.or_else(|| self.0.parents.iter().find_map(|parent| parent.get_recall(name)))
	}

	pub fn matter_names(&self) -> &[String] {
		&self.0.matter_names
	}

	pub fn imitate(&self, mut args: Args, vm: &mut Vm) -> Result<Value, RuntimeError> {
		if let Some(ref imitate) = self.0.imitate {
			let fields = vec![Default::default(); self.0.matter_names.len()];
			let imitation = Value::Imitation(Imitation::new(self.clone(), fields).into());

			args.add_soul(imitation.clone());
			imitate.call(args, vm)?;

			return Ok(imitation);
		}

		if args._as_slice().len() != self.0.matter_names.len() {
			Err(RuntimeError::ArgumentCountError { given: args._as_slice().len(), expected: self.0.matter_names.len() })
		} else {
			Ok(Value::Imitation(Imitation::new(self.clone(), args._as_slice().to_owned()).into()))
		}
	}

	pub fn is_subform_of(&self, form: &Form) -> bool {
		self == form || self.0.parents.iter().any(|parent| parent.is_subform_of(form))
	}
}

impl Hash for Form {
	fn hash<H: Hasher>(&self, h: &mut H) {
		(Arc::as_ptr(&self.0) as usize).hash(h);
	}
}

impl Dump for Form {
	fn dump(&self, to: &mut String, _: &mut Vm) -> Result<(), RuntimeError> {
		to.push_str(&format!("Form({}:{:p})", self.name(), Arc::as_ptr(&self.0)));

		Ok(())
	}
}


impl Matches for Form {
	fn matches(&self, rhs: &Value, _: &mut Vm) -> Result<bool, RuntimeError> {
		if let Value::Imitation(imitation) = rhs {
			Ok(imitation.form().is_subform_of(self))
		} else {
			Ok(false)
		}
	}
}

impl IsEqual for Form {
	fn is_equal(&self, rhs: &Value, _: &mut Vm) -> Result<bool, RuntimeError> {
		if let Value::Form(_form) = rhs {
			unimplemented!();
		} else {
			Ok(false)
		}
	}
}

impl Call for Form {
	fn call(&self, args: Args, vm: &mut Vm) -> Result<Value, RuntimeError> {
		self.imitate(args, vm)
	}
}

impl GetAttr for Form {
	fn get_attr(&self, attr: &str, _: &mut Vm) -> Result<Value, RuntimeError> {
		if let Some(essence) = self.get_essence(attr) {
			Ok(essence.lock().clone())
		} else if let Some(recall) = self.get_recall(attr) {
			Ok(recall.clone().into())
		} else {
			Err(RuntimeError::UnknownAttribute(attr.to_string()))
		}
	}
}

impl SetAttr for Form {
	fn set_attr(&self, attr: &str, value: Value, _: &mut Vm) -> Result<(), RuntimeError> {
		if let Some(essence) = self.get_essence(attr) {
			*essence.lock() = value;
			Ok(())
		} else {
			Err(RuntimeError::UnknownAttribute(attr.to_string()))
		}
	}
}