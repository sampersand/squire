use crate::compile::Error as CompilerError;
use super::{Form, Journey};

#[derive(Debug)]
pub struct FormBuilder(pub(super) Form);

impl FormBuilder {
	pub fn add_recall(&mut self, recall: Journey) -> Result<(), CompilerError> {
		if self.0.functions.contains_key(recall.name()) {
			Err(CompilerError::FormValueAlreadyDefined { name: recall.name().to_string(), kind: "recall" })
		} else {
			self.0.functions.insert(recall.name().to_string(), recall.into());
			Ok(())
		}
	}

	pub fn build(self) -> Form {
		self.0
	}
}
// 	name: String,
// 	parents: Vec<Arc<Form>>,

// 	functions: HashMap<String, Recollection>,
// 	statics: HashMap<String, Mutex<Value>>,

// 	field_names: Vec<String>,
// 	methods: HashSet<Change>,
// 	constructor: Option<Journey>
// }
