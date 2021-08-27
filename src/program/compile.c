struct sq_compiler *compiler;

#define set_opcode(opcode) sq_compiler_set_opcode(compiler, set_opcode)
#define set_index(index) sq_compiler_set_index(compiler, set_index)
#define set_interrupt(interrupt) sq_compiler_set_interrupt(compiler, set_interrupt)
#define set_count(count) sq_compiler_set_count(compiler, set_count)
#define next_local() sq_compiler_next_local(compiler)

// static unsigned compile_primary(struct primary *primary) {
	// unsigned result;

	// switch (primary->kind) {
	// case SQ_PS_
	// }
// static unsigned compile_primary(struct sq_code *code, struct primary *primary) {
// 	unsigned result;

// 	switch (primary->kind) {
// 	case SQ_PS_PPAREN:
// 		result = compile_expression(code, primary->expr);
// 		break;

// 	case SQ_PS_PBOOK:
// 		result = compile_book(code, primary->book);
// 		break;

// 	case SQ_PS_PCODEX:
// 		result = compile_codex(code, primary->dict);
// 		break;

// 	case SQ_PS_PINDEX:
// 		result = compile_index(code, primary->index);
// 		break;

// 	case SQ_PS_PLAMBDA: {
// 		struct sq_journey *func = compile_journey(primary->lambda, false);
// 		free(primary->lambda);

// 		result = load_constant(code, sq_value_new(func));
// 		break;
// 	}

// 	case SQ_PS_PNUMERAL:
// 		result = load_constant(code, sq_value_new(primary->numeral));
// 		break;

// 	case SQ_PS_PTEXT:
// 		result = load_constant(code, sq_value_new(primary->text));
// 		break;

// 	case SQ_PS_PVERACITY:
// 		result = load_constant(code, sq_value_new(primary->veracity));
// 		break;

// 	case SQ_PS_PNI:
// 		result = load_constant(code, SQ_NI);
// 		break;

// 	case SQ_PS_PVARIABLE:
// 		result = load_variable_class(code, primary->variable, NULL);
// 		break;

// 	default:
// 		bug("unknown primary class '%d'", primary->kind);
// 	}

// 	free(primary);
// 	return result;
// }

// }

// int sq_compiler_constant_lookup(struct sq_compiler *compiler, sq_value constant);
// unsigned sq_compiler_constant_declare(struct sq_compiler *compiler, sq_value constant);
// unsigned sq_compiler_constant_new(struct sq_compiler *compiler, sq_value constant);
// unsigned sq_compiler_constant_load(struct sq_compiler *compiler, sq_value constant);

// int sq_compiler_global_lookup(struct sq_globals *globals, const char *name);
// unsigned sq_compiler_global_declare(struct sq_globals *globals, char *name, sq_value value);
// unsigned sq_compiler_global_new(struct sq_globals *globals, char *name, sq_value value);

// int sq_compiler_variable_lookup(struct sq_compiler *compiler, const char *name);
// unsigned sq_compiler_variable_declare(struct sq_compiler *compiler, char *name);
// unsigned sq_compiler_variable_new(struct sq_compiler *compiler, char *name);

// #define SQ_COMPILER_IS_GLOBAL(x) ((x) < 0)
// #define SQ_COMPILER_GLOBAL_INDEX(x) (~(x))

// int sq_compiler_identifier_lookup(struct sq_compiler *compiler, char *name);
// unsigned sq_compiler_identifier_load(struct sq_compiler *compiler, char *name);

// static unsigned genus_load(struct sq_code *code, unsigned local) {
// 	set_opcode(code, SQ_OC_MOV)
// }

// static unsigned load_variable_class(struct sq_code *code, struct variable *var, int *parent) {
// 	int p;
// 	if (parent == NULL) parent = &p;

// 	unsigned index = load_identifier(code, var->name);

// 	if (var->field == NULL) {
// 		free(var);
// 		*parent = -1;
// 		return index;
// 	}

// 	set_opcode(code, SQ_OC_MOV);
// 	set_index(code, *parent = index);
// 	set_index(code, index = next_local(code));

// 	while ((var = var->field)) {
// 		set_opcode(code, SQ_OC_ILOAD);
// 		set_index(code, *parent = index);
// 		set_index(code, new_constant(code, sq_value_new(sq_text_new(strdup(var->name)))));
// 		set_index(code, index = next_local(code));
// 	}

// 	free(var);

// 	return index;
// }
