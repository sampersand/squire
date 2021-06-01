#include "program.h"
#include "function.h"
#include "shared.h"
#include "parse.h"
#include "class.h"
#include "string.h"
#include <string.h>
#include <errno.h>

struct sq_program *program;

// total temporary hack...
#define free(x) ((void)0)

struct global {
	char *name;
	sq_value value;
};

struct {
	unsigned len, cap;
	struct global *ary;
} globals;

struct sq_code {
	unsigned codecap, codelen;
	union sq_bytecode *bytecode;

	unsigned nlocals;

	struct {
		unsigned cap, len;

		struct local {
			char *name;
			unsigned index;
		} *ary;
	} vars;

	struct {
		unsigned cap, len;
		sq_value *ary;
	} consts;
};

#define RESIZE(cap, len, pos, type) \
	if (code->cap == code->len) \
		code->pos = xrealloc(code->pos, sizeof(type [code->cap*=2]));

#ifdef SQ_LOG
#define LOG(...) printf(__VA_ARGS__)
#else
#define LOG(...) ((void) 0)
#endif

static void set_opcode(struct sq_code *code, enum sq_opcode opcode) {
	LOG("bytecode[%d].opcode=%x\n", code->codelen, opcode);

	RESIZE(codecap, codelen, bytecode, union sq_bytecode);
	code->bytecode[code->codelen++].opcode = opcode;
}

static void set_index(struct sq_code *code, unsigned index) {
	LOG("bytecode[%d].index=%d\n", code->codelen, index);

	RESIZE(codecap, codelen, bytecode, union sq_bytecode);
	code->bytecode[code->codelen++].index = index;
}

static void set_target(struct sq_code *code, unsigned target) {
	LOG("bytecode[%d].index=%d [update]\n", target, code->codelen);

	code->bytecode[target].index = code->codelen;
}

static unsigned next_local(struct sq_code *code) {
	return code->nlocals++;
}

static unsigned declare_constant(struct sq_code *code, sq_value value) {
	if (code->consts.cap == code->consts.len) {
		code->consts.cap *= 2;
		code->consts.ary = xrealloc(code->consts.ary, sizeof(sq_value[code->consts.cap]));
	}

#ifdef SQ_LOG
	printf("consts[%d]=", code->consts.len); 
	sq_value_dump(value);
	putchar('\n');
#endif /* SQ_LOG */

	code->consts.ary[code->consts.len] = value;
	return code->consts.len++;
}

static int lookup_constant(struct sq_code *code, sq_value value) {
	// check to see if we've declared the constant before. if so, reuse that.
	for (unsigned i = 0; i < code->consts.len; ++i) {
		if (sq_value_eql(code->consts.ary[i], value)) {
			// sq_value_free(value); // free it as we're no longer using it.
			return -1; // TODO: THIS does not work because of a bug...
		}
	}

	return -1;
}

static unsigned new_constant(struct sq_code *code, sq_value value) {
	int index = lookup_constant(code, value);

	return (index == -1) ? declare_constant(code, value) : index;
}

static unsigned load_constant(struct sq_code *code, sq_value value) {
	int index = lookup_constant(code, value);

	// if we haven't encountered the value before, load it.
	if (index == -1) {
		set_opcode(code, SQ_OC_CLOAD);
		set_index(code, declare_constant(code, value));
		set_index(code, index = next_local(code));
	}

	return index;
}

static unsigned lookup_global_variable(char *name) {
	// check to see if we've declared the global before
	for (unsigned i = 0; i < globals.len; ++i) {
		if (!strcmp(globals.ary[i].name, name)) {
			free(name); // we have, free the name.
			return i;
		}
	}

	return -1;
}

static unsigned declare_global_variable(char *name, sq_value value) {
	int index = lookup_global_variable(name);

	if (index != -1) {
		if (globals.ary[index].value != SQ_NULL && value != SQ_NULL)
			die("attempted to redefine global variable '%s'", name);
		free(name);
		globals.ary[index].value = value;
		return index;
	}

	// reallocate if necessary
	if (globals.len == globals.cap) {
		globals.cap *= 2;
		globals.ary = xrealloc(globals.ary, sizeof(struct global[globals.cap]));
	}

	LOG("global[%d]: %s\n", globals.len, name);

	// initialize the global
	globals.ary[globals.len].name = name;
	globals.ary[globals.len].value = value;

	// return the index of the global for future use.
	return globals.len++;
}

static int new_global(char *name) {
	int index = lookup_global_variable(name);

	return (index == -1) ? declare_global_variable(name, SQ_NULL) : index;
}

static unsigned declare_local_variable(struct sq_code *code, char *name) {
	// reallocate if necessary
	RESIZE(vars.cap, vars.len, vars.ary, struct local);

	LOG("local[%d]: %s\n", globals.len, name);

	code->vars.ary[code->vars.len].name = name;
	return code->vars.ary[code->vars.len++].index = next_local(code);
}

static int lookup_local_variable(struct sq_code *code, char *name) {
	// check to see if we've declared the local before
	for (unsigned i = 0; i < code->vars.len; ++i) {
		if (!strcmp(name, code->vars.ary[i].name)) {
			free(name);
			return code->vars.ary[i].index;
		}
	}

	return -1;
}

static unsigned new_local_variable(struct sq_code *code, char *name) {
	int index = lookup_local_variable(code, name);

	return (index == -1) ? declare_local_variable(code, name) : index;
}


static unsigned lookup_identifier(struct sq_code *code, char *name) {
	int index;

	if ((index = lookup_local_variable(code, name)) != -1)
		return index;

	if ((index = lookup_global_variable(name)) != -1)
		return ~index;

	return new_local_variable(code, name);
}

static unsigned load_identifier(struct sq_code *code, char *name) {
	int index = lookup_identifier(code, name);

	if (index < 0) {
		set_opcode(code, SQ_OC_GLOAD);
		set_index(code, ~index);
		set_index(code, index = next_local(code));
	}

	return index;
}

static unsigned load_variable_class(struct sq_code *code, struct variable *var, int *parent) {
	int p;
	if (parent == NULL) parent = &p;

	unsigned index = load_identifier(code, var->name);

	if (var->field == NULL) {
		free(var);
		*parent = -1;
		return index;
	}

	set_opcode(code, SQ_OC_MOV);
	set_index(code, *parent = index);
	set_index(code, index = next_local(code));

	while ((var = var->field)) {
		set_opcode(code, SQ_OC_ILOAD);
		set_index(code, *parent = index);
		set_index(code, new_constant(code, sq_value_new_string(sq_string_new(strdup(var->name)))));
		set_index(code, index = next_local(code));
	}

	free(var);

	return index;
}

static unsigned compile_expression(struct sq_code *code, struct expression *expr);
static void compile_statements(struct sq_code *code, struct statements *stmts);
static struct sq_function *compile_function(struct func_declaration *fndecl, bool is_method);

static void compile_class_declaration(struct class_declaration *sdecl) {
	struct sq_class *class = sq_class_new(sdecl->name);

	class->nfields = sdecl->nfields;
	class->fields = sdecl->fields;

	declare_global_variable(strdup(class->name), SQ_NULL);

	if (sdecl->constructor != NULL) {
		class->constructor = compile_function(sdecl->constructor, true);
	} else {
		class->constructor = NULL;
	}

	class->nfuncs = sdecl->nfuncs;
	class->funcs = xmalloc(sizeof(struct sq_function *[class->nfuncs]));
	for (unsigned i = 0; i < class->nfuncs; ++i)
		class->funcs[i] = compile_function(sdecl->funcs[i], false);

	class->nmeths = sdecl->nmeths;
	class->meths = xmalloc(sizeof(struct sq_function *[class->nmeths]));

	for (unsigned i = 0; i < class->nmeths; ++i)
		class->meths[i] = compile_function(sdecl->meths[i], true);

	free(sdecl); // but none of the fields, as they're now owned by `class`.

	declare_global_variable(strdup(class->name), sq_value_new_class(class));
}

static void compile_func_declaration(struct func_declaration *fdecl) {
	assert(fdecl->name != NULL);

	struct sq_function *func = compile_function(fdecl, false);
	free(fdecl); // but none of the fields, as they're now owned by `func`.

	declare_global_variable(strdup(func->name), sq_value_new_function(func));
}

static void compile_if_statement(struct sq_code *code, struct if_statement *ifstmt) {
	unsigned condition_index, iffalse_label, finished_label;

	condition_index = compile_expression(code, ifstmt->cond);
	set_opcode(code, SQ_OC_JMP_FALSE);
	set_index(code, condition_index);
	iffalse_label = code->codelen;
	set_index(code, 0);

	compile_statements(code, ifstmt->iftrue);

	if (ifstmt->iffalse) {
		set_opcode(code, SQ_OC_JMP);
		finished_label = code->codelen;
		set_index(code, 0);

		set_target(code, iffalse_label);
		compile_statements(code, ifstmt->iffalse);
		set_target(code, finished_label);
	} else {
		set_target(code, iffalse_label);
	}

	free(ifstmt);
}

static void compile_while_statement(struct sq_code *code, struct while_statement *wstmt) {
	unsigned condition_index, condition_label, finished_label;

	condition_label = code->codelen;

	condition_index = compile_expression(code, wstmt->cond);
	set_opcode(code, SQ_OC_JMP_FALSE);
	set_index(code, condition_index);
	finished_label = code->codelen;
	set_index(code, 0);

	compile_statements(code, wstmt->body);

	set_opcode(code, SQ_OC_JMP);
	set_index(code, condition_label);
	set_target(code, finished_label);

	free(wstmt);
}

static void compile_return_statement(struct sq_code *code, struct return_statement *rstmt) {
	unsigned index;

	if (rstmt->value == NULL) {
		index = load_constant(code, SQ_NULL);
	} else {
		index = compile_expression(code, rstmt->value);
	}

	set_opcode(code, SQ_OC_RETURN);
	set_index(code, index);
}

static unsigned compile_array(struct sq_code *code, struct array *array) {
	unsigned indices[array->nargs];

	for (unsigned i = 0; i < array->nargs; ++i)
		indices[i] = compile_expression(code, array->args[i]);

	set_opcode(code, SQ_OC_INT);
	set_index(code, SQ_INT_ARRAY_NEW);
	set_index(code, array->nargs);

	for (unsigned i = 0; i < array->nargs; ++i)
		set_index(code, indices[i]);

	unsigned index = next_local(code);
	set_index(code, index);

	return index;
}

static unsigned compile_array_index(struct sq_code *code, struct array_index *array_index) {
	unsigned array = load_variable_class(code, array_index->array, NULL);
	free(array_index->array); // OR SHOULD THIS BE FREED IN `load_variable_class`?
	unsigned index = compile_expression(code, array_index->index);

	set_opcode(code, SQ_OC_INT);
	set_index(code, SQ_INT_ARRAY_INDEX);
	set_index(code, array);
	set_index(code, index);
	set_index(code, index = next_local(code));

	return index;
}

static unsigned compile_primary(struct sq_code *code, struct primary *primary) {
	unsigned result;

	switch (primary->kind) {
	case SQ_PS_PPAREN:
		result = compile_expression(code, primary->expr);
		break;

	case SQ_PS_PARRAY:
		result = compile_array(code, primary->array);
		break;

	case SQ_PS_PARRAY_INDEX:
		result = compile_array_index(code, primary->array_index);
		break;

	case SQ_PS_PLAMBDA: {
		struct sq_function *func = compile_function(primary->lambda, false);
		free(primary->lambda);

		result = load_constant(code, sq_value_new_function(func));
		break;
	}

	case SQ_PS_PNUMBER:
		result = load_constant(code, sq_value_new_number(primary->number));
		break;

	case SQ_PS_PSTRING:
		result = load_constant(code, sq_value_new_string(primary->string));
		break;

	case SQ_PS_PBOOLEAN:
		result = load_constant(code, sq_value_new_boolean(primary->boolean));
		break;

	case SQ_PS_PNULL:
		result = load_constant(code, SQ_NULL);
		break;

	case SQ_PS_PVARIABLE:
		result = load_variable_class(code, primary->variable, NULL);
		break;

	default:
		bug("unknown primary class '%d'", primary->kind);
	}

	free(primary);
	return result;
}

static unsigned compile_unary(struct sq_code *code, struct unary_expression *unary) {
	unsigned rhs, result;

	rhs = compile_primary(code, unary->rhs);

	switch (unary->kind) {
	case SQ_PS_UNEG: set_opcode(code, SQ_OC_NEG); break;
	case SQ_PS_UNOT: set_opcode(code, SQ_OC_NOT); break;
	case SQ_PS_UPRIMARY: result = rhs; goto done;
	default: bug("unknown unary kind '%d'", unary->kind);
	}

	set_index(code, rhs);
	set_index(code, result = next_local(code));

done:

	free(unary);
	return result;
}

static unsigned compile_mul(struct sq_code *code, struct mul_expression *mul) {
	unsigned lhs, rhs, result;

	lhs = compile_unary(code, mul->lhs);

	if (mul->kind != SQ_PS_MUNARY)
		rhs = compile_mul(code, mul->rhs);

	switch (mul->kind) {
	case SQ_PS_MMUL: set_opcode(code, SQ_OC_MUL); break;
	case SQ_PS_MDIV: set_opcode(code, SQ_OC_DIV); break;
	case SQ_PS_MMOD: set_opcode(code, SQ_OC_MOD); break;
	case SQ_PS_MUNARY: result = lhs; goto done;
	default: bug("unknown mul kind '%d'", mul->kind);
	}

	set_index(code, lhs);
	set_index(code, rhs);
	set_index(code, result = next_local(code));

done:

	free(mul);
	return result;
}

static unsigned compile_add(struct sq_code *code, struct add_expression *add) {
	unsigned lhs, rhs, result;

	lhs = compile_mul(code, add->lhs);
	if (add->kind != SQ_PS_AMUL)
		rhs = compile_add(code, add->rhs);

	switch (add->kind) {
	case SQ_PS_AADD: set_opcode(code, SQ_OC_ADD); break;
	case SQ_PS_ASUB: set_opcode(code, SQ_OC_SUB); break;
	case SQ_PS_AMUL: result = lhs; goto done;
	default: bug("unknown add kind '%d'", add->kind);
	}

	set_index(code, lhs);
	set_index(code, rhs);
	set_index(code, result = next_local(code));

done:

	free(add);
	return result;
}

static unsigned compile_cmp(struct sq_code *code, struct cmp_expression *cmp) {
	unsigned lhs, rhs, result;

	lhs = compile_add(code, cmp->lhs);
	if (cmp->kind != SQ_PS_CADD)
		rhs = compile_cmp(code, cmp->rhs);

	switch (cmp->kind) {
	case SQ_PS_CLTH: set_opcode(code, SQ_OC_LTH); break;
	case SQ_PS_CLEQ: set_opcode(code, SQ_OC_LEQ); break;
	case SQ_PS_CGTH: set_opcode(code, SQ_OC_GTH); break;
	case SQ_PS_CGEQ: set_opcode(code, SQ_OC_GEQ); break;
	case SQ_PS_CADD: result = lhs; goto done;
	default: bug("unknown cmp kind '%d'", cmp->kind);
	}

	set_index(code, lhs);
	set_index(code, rhs);
	set_index(code, result = next_local(code));

done:

	free(cmp);
	return result;

}

static unsigned compile_eql(struct sq_code *code, struct eql_expression *eql) {
	unsigned lhs, rhs, result;

	lhs = compile_cmp(code, eql->lhs);
	if (eql->kind != SQ_PS_CADD)
		rhs = compile_eql(code, eql->rhs);

	switch (eql->kind) {
	case SQ_PS_EEQL: set_opcode(code, SQ_OC_EQL); break;
	case SQ_PS_ENEQ: set_opcode(code, SQ_OC_NEQ); break;
	case SQ_PS_ECMP: result = lhs; goto done;
	default: bug("unknown eql kind '%d'", eql->kind);
	}

	set_index(code, lhs);
	set_index(code, rhs);
	set_index(code, result = next_local(code));

done:

	free(eql);
	return result;
}

static unsigned compile_bool(struct sq_code *code, struct bool_expression *bool_) {
	unsigned tmp = compile_eql(code, bool_->lhs);
	unsigned target;
	
	set_opcode(code, SQ_OC_MOV);
	set_opcode(code, tmp);
	set_opcode(code, target = next_local(code));

	if (bool_->kind != SQ_PS_BEQL)
		tmp = compile_bool(code, bool_->rhs);

	switch (bool_->kind) {
	case SQ_PS_BAND: set_opcode(code, SQ_OC_JMP_FALSE); break;
	case SQ_PS_BOR:  set_opcode(code, SQ_OC_JMP_TRUE); break;
	case SQ_PS_BEQL: goto done;
	default: bug("unknown bool kind '%d'", bool_->kind);
	}

	set_index(code, target);
	unsigned dst = code->codelen;
	set_index(code, 0xffff);

	set_opcode(code, SQ_OC_MOV);
	set_index(code, tmp);
	set_index(code, target);
	set_target(code, dst);

done:

	free(bool_);
	return target;
}

static unsigned compile_function_call(struct sq_code *code, struct function_call *fncall) {
	unsigned args[fncall->arglen];

	for (unsigned i = 0; i < fncall->arglen; ++i)
		args[i] = compile_expression(code, fncall->args[i]);

	if (fncall->func->field != NULL) {
		set_opcode(code, SQ_OC_NOOP);
		int dst;
		unsigned var = load_variable_class(code, fncall->func, &dst);
		set_opcode(code, SQ_OC_CALL);
		set_index(code, var);
		set_index(code, fncall->arglen + 1);
		set_index(code, dst);
		goto arguments;
	}

#define BUILTIN_FN(name_, int_, argc_) \
	if (!strcmp(fncall->func->name, name_)) { \
		if (fncall->arglen != argc_) \
			die("exactly %d arg(s) are required for '%s'", argc_, name_); \
		set_opcode(code, SQ_OC_INT); \
		set_index(code, int_); \
		goto arguments; \
	}

	BUILTIN_FN("proclaim", SQ_INT_PRINT, 1);
	BUILTIN_FN("number", SQ_INT_TONUMBER, 1);
	BUILTIN_FN("string", SQ_INT_TOSTRING, 1);
	BUILTIN_FN("boolean", SQ_INT_TOBOOLEAN, 1);
	BUILTIN_FN("dump", SQ_INT_DUMP, 1);
	BUILTIN_FN("length", SQ_INT_LENGTH, 1); // `fathoms` ?
	BUILTIN_FN("substr", SQ_INT_SUBSTR, 3);
	BUILTIN_FN("dismount", SQ_INT_EXIT, 1);
	BUILTIN_FN("kindof", SQ_INT_KINDOF, 1);
	BUILTIN_FN("system", SQ_INT_SYSTEM, 1);
	BUILTIN_FN("inquire", SQ_INT_PROMPT, 0);
	BUILTIN_FN("random", SQ_INT_RANDOM, 0);
	BUILTIN_FN("insert", SQ_INT_ARRAY_INSERT, 3);
	BUILTIN_FN("delete", SQ_INT_ARRAY_DELETE, 2);

	set_opcode(code, SQ_OC_NOOP);
	unsigned var = load_variable_class(code, fncall->func, NULL);
	set_opcode(code, SQ_OC_CALL);
	set_index(code, var);
	set_index(code, fncall->arglen);

arguments:

	for (unsigned i = 0; i < fncall->arglen; ++i)
		set_index(code, args[i]);

	unsigned result;

	set_index(code, result = next_local(code));
	return result;

}

static unsigned compile_expression(struct sq_code *code, struct expression *expr) {
	unsigned index;
	int variable;

	switch (expr->kind) {
	case SQ_PS_EFNCALL:
		return compile_function_call(code, expr->fncall);

	case SQ_PS_EARRAY_ASSIGN: {
		unsigned var = lookup_identifier(code, expr->ary_asgn->var->name);
		index = compile_expression(code, expr->ary_asgn->index);
		unsigned val = compile_expression(code, expr->ary_asgn->value);
		set_opcode(code, SQ_OC_INT);
		set_index(code, SQ_INT_ARRAY_INDEX_ASSIGN);
		set_index(code, var);
		set_index(code, index);
		set_index(code, val);
		set_index(code, index = next_local(code));
		return index;
	}


	case SQ_PS_EASSIGN: {
		index = compile_expression(code, expr->asgn->expr);
		struct variable *var = expr->asgn->var;
		variable = lookup_identifier(code, var->name);

		if (!var->field) {
			if (0 <= variable) {
				set_opcode(code, SQ_OC_MOV);
				set_index(code, index);
				set_index(code, index = variable);
			} else if (!var->field) {
				set_opcode(code, SQ_OC_GSTORE);
				set_index(code, ~variable);
				set_index(code, index);
				set_index(code, index = next_local(code));
			}

			return index;
		} 

		if (0 <= variable) {
			// do nothing, it's already loaded.
		} else {
			set_opcode(code, SQ_OC_GLOAD);
			set_index(code, ~variable);
			set_index(code, variable = next_local(code));
		}

		if (var->field && var->field->field)
			die("only one layer deep for assignment supported rn");

		set_opcode(code, SQ_OC_ISTORE);
		set_opcode(code, variable);
		set_index(code, new_constant(code, sq_value_new_string(sq_string_new(strdup(var->field->name)))));
		set_index(code, index);
		set_index(code, index = next_local(code));

		return index;
	}

	case SQ_PS_EMATH:

		return compile_bool(code, expr->math);

	default:
		bug("unknown expr kind '%d'", expr->kind);
	}
}

static void compile_statements(struct sq_code *code, struct statements *stmts);
static void compile_import(struct sq_code *code, char *import) {
	FILE *stream = fopen(import, "r");
	if (!stream) die("unable to open file: '%s': %s", import, strerror(errno));

	if (fseek(stream, 0, SEEK_END)) die("unable to seek to end: %s", strerror(errno));
	long length = ftell(stream);
	if (fseek(stream, 0, SEEK_SET)) die("unable to seek to start: %s", strerror(errno));

	char contents[length + 1];
	contents[length] = '\0';
	fread(contents, 1, length, stream);

	if (ferror(stream)) die("unable to read contents: %s", strerror(errno));
	if (fclose(stream) == EOF) die("unable to close stream: %s", strerror(errno));

	struct statements *stmts = sq_parse_statements(contents);
	if (!stmts) die("invalid syntax.");

	compile_statements(code, stmts);
}

static unsigned compile_global(struct sq_code *code, struct scope_declaration *gdecl) {
	unsigned index = new_global(gdecl->name);
	if (gdecl->value == NULL) 
		goto done;

	unsigned result = compile_expression(code, gdecl->value);
	set_opcode(code, SQ_OC_GSTORE);
	set_index(code, index);
	set_index(code, result);
	set_index(code, index = next_local(code));

done:

	free(gdecl);
	return index;
}

static unsigned compile_local(struct sq_code *code, struct scope_declaration *ldecl) {
	unsigned index = new_local_variable(code, ldecl->name);

	if (ldecl->value != NULL) {
		unsigned result = compile_expression(code, ldecl->value);
		set_opcode(code, SQ_OC_MOV);
		set_index(code, result);
		set_index(code, index);
	}

	free(ldecl);
	return index;
}

static void compile_statement(struct sq_code *code, struct statement *stmt) {
	switch (stmt->kind) {
	case SQ_PS_SGLOBAL: compile_global(code, stmt->gdecl); break;
	case SQ_PS_SLOCAL: compile_local(code, stmt->ldecl); break;
	case SQ_PS_SIMPORT: compile_import(code, stmt->import); break;
	case SQ_PS_SCLASS: compile_class_declaration(stmt->cdecl); break;
	case SQ_PS_SFUNC: compile_func_declaration(stmt->fdecl); break;
	case SQ_PS_SIF: compile_if_statement(code, stmt->ifstmt); break;
	case SQ_PS_SWHILE: compile_while_statement(code, stmt->wstmt); break;
	case SQ_PS_SRETURN: compile_return_statement(code, stmt->rstmt); break;
	case SQ_PS_SEXPR: compile_expression(code, stmt->expr); break;
	default: bug("unknown statement kind '%d'", stmt->kind);
	}
}

static void compile_statements(struct sq_code *code, struct statements *stmts) {
	for (unsigned i = 0; i < stmts->len; ++i)
		compile_statement(code, stmts->stmts[i]);
}

static struct sq_function *compile_function(struct func_declaration *fndecl, bool is_method) {
	struct sq_code code;
	code.codecap = 2048;
	code.codelen = 0;
	code.bytecode = xmalloc(sizeof(union sq_bytecode[code.codecap]));

	code.nlocals = 0;
	code.consts.cap = 64;
	code.consts.len = 0;
	code.consts.ary = xmalloc(sizeof(sq_value [code.consts.cap]));

	code.vars.len = fndecl->nargs;
	code.vars.cap = 16 + code.vars.len;
	code.vars.ary = xmalloc(sizeof(struct local[code.vars.cap]));

	unsigned offset = 0;

	for (unsigned i = 0; i < fndecl->nargs; ++i) {
		code.vars.ary[i+offset].name = fndecl->args[i];
		code.vars.ary[i+offset].index = next_local(&code);
	}


	if (fndecl->body != NULL)
		compile_statements(&code, fndecl->body);

	struct sq_function *fn = xmalloc(sizeof(struct sq_function));

	fn->refcount = -1; // todo: refcount
	fn->name = fndecl->name;
	fn->argc = fndecl->nargs;
	fn->bytecode = code.bytecode;
	fn->consts = code.consts.ary;
	fn->nconsts = code.consts.len;
	fn->nlocals = code.nlocals;
	fn->program = program;
	fn->codelen = code.codelen;
	fn->is_method = is_method;

	return fn;
}

struct sq_program *sq_program_compile(const char *stream) {
	globals.len = 1;
	globals.ary = xmalloc(sizeof(struct local[globals.cap = 16]));
	globals.ary[0].name = strdup("ARGV");
	globals.ary[0].value = SQ_NULL;

	program = xmalloc(sizeof(struct sq_program));
	program->nglobals = 1;
	program->globals = NULL;

	struct func_declaration maindecl = {
		.name = strdup("main"),
		.nargs = 0, // todo: pass commandline arguments
		.args = NULL,
		.body = sq_parse_statements(stream)
	};

	program->main = compile_function(&maindecl, false);

	program->nglobals = globals.len;
	program->globals = xmalloc(sizeof(sq_value [globals.len]));
	for (unsigned i = 0; i < program->nglobals; ++i)
		program->globals[i] = globals.ary[i].value;

	return program;
}
