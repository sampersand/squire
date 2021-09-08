use super::{Form, FormInner};
use std::sync::Arc;
use crate::value::journey::UserDefined;
use std::fmt::{self, Display, Formatter};

pub struct FormBuilder(pub(super) FormInner);

#[derive(Debug)]
pub enum AlreadyDefinedError {
	Recall(String),
	Essence(String),
	Matter(String),
	Change(String),
	Imitate
}

impl std::error::Error for AlreadyDefinedError {}

impl Display for AlreadyDefinedError {
	fn fmt(&self, f: &mut Formatter) -> fmt::Result {
		write!(f, "The ")?;
		match self {
			Self::Recall(name) => write!(f, "recall '{}'", name)?,
			Self::Essence(name) => write!(f, "essence '{}'", name)?,
			Self::Matter(name) => write!(f, "matter '{}'", name)?,
			Self::Change(name) => write!(f, "change '{}'", name)?,
			Self::Imitate => write!(f, "imitate")?,
		}

		write!(f, " has already been declared")
	}
}


impl FormBuilder {
	fn form_key_exists(&self, key: &str) -> bool {
		self.0.recalls.contains_key(key) || self.0.essences.contains_key(key)
	}

	fn imitation_key_exists(&self, key: &str) -> bool {
		self.0.changes.contains_key(key) || self.0.matter_names.iter().any(|x| x == key)
	}

	pub fn add_parent(&mut self, parent: Form) {
		// todo: do we want to check for the parent already existing?
		self.0.parents.push(parent);
	}

	pub fn add_recall(&mut self, recall: UserDefined) -> Result<(), AlreadyDefinedError> {
		let name = recall.name().to_string();

		if self.form_key_exists(&name) {
			return Err(AlreadyDefinedError::Recall(name));
		}

		self.0.recalls.insert(name.into(), recall.into());

		Ok(())
	}

	pub fn add_essence(&mut self, name: String) -> Result<(), AlreadyDefinedError> {
		if self.form_key_exists(&name) {
			return Err(AlreadyDefinedError::Essence(name));
		}

		self.0.essences.insert(name, Default::default());

		Ok(())
	}

	pub fn add_matter(&mut self, name: String) -> Result<(), AlreadyDefinedError> {
		if self.imitation_key_exists(&name) {
			return Err(AlreadyDefinedError::Matter(name));
		}

		self.0.matter_names.push(name);

		Ok(())
	}

	pub fn add_change(&mut self, change: UserDefined) -> Result<(), AlreadyDefinedError> {
		let name = change.name().to_string();

		if self.imitation_key_exists(&name) {
			return Err(AlreadyDefinedError::Change(name));
		}

		self.0.changes.insert(name.into(), change.into());

		Ok(())
	}

	pub fn add_imitate(&mut self, imitate: UserDefined) -> Result<(), AlreadyDefinedError> {
		if self.0.imitate.is_some() {
			return Err(AlreadyDefinedError::Imitate);
		}

		self.0.imitate = Some(imitate.into());

		Ok(())
	}

	pub fn build(self) -> Form {
		Form(Arc::new(self.0))
	}
}
