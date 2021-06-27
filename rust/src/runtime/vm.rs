use crate::Value;
use crate::value::builtin::defaults as default_globals;

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Vm {
	globals: Vec<Value>
}

impl Default for Vm {
	fn default() -> Self {
		let globals = default_globals().into_iter().map(Value::BuiltinJourney).collect();

		Self { globals }
	}
}

impl Vm {
	pub fn get_global(&self, index: usize) -> Value {
		self.globals[index].clone()
	}
}