use crate::value::Value;

#[derive(Debug)]
pub struct Vm {
	globals: Vec<Value>,
}


impl Vm {
	pub fn new(globals: Vec<Value>) -> Self {
		Self { globals }
	}
}

impl Vm {
	pub fn get_global(&self, index: usize) -> Value {
		self.globals[index].clone()
	}

	pub fn set_global(&mut self, index: usize, global: Value) {
		self.globals[index] = global;
	}
}
