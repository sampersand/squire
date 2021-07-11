#include "program.h"
#include "journey.h"
#include "shared.h"
#include "parse.h"
#include "form.h"
#include "text.h"
#include <string.h>
#include <errno.h>

struct sq_program *program;

#define CURRENT_INDEX_PTR(code) (&(code)->bytecode[(code)->codelen].index)
struct {
	unsigned len, cap;
	struct global {
		char *name;
		sq_value value;
	} *ary;
} globals;

#define MAX_COMEFROMS 16

struct sq_code {
	unsigned codecap, codelen;
	union sq_bytecode *bytecode;

	unsigned nlocals;

	struct {
		unsigned cap, len;
		struct label {
			char *name;
			int *length, indices[MAX_COMEFROMS];
		} *ary;
	} labels;

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
		code->pos = xrealloc(code->pos, sizeof_array(type , code->cap*=2));

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
		code->consts.ary = xrealloc(code->consts.ary, sizeof_array(sq_value, code->consts.cap));
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

static int lookup_global_variable(const char *name) {
	// check to see if we've declared the global before
	for (unsigned i = 0; i < globals.len; ++i) {
		if (!strcmp(globals.ary[i].name, name)) {
			return i;
		}
	}

	return -1;
}

static unsigned declare_global_variable(const char *name, sq_value value) {
	int index = lookup_global_variable(name);

	if (index != -1) {
		if (globals.ary[index].value != SQ_NI && value != SQ_NI)
			die("attempted to redefine global variable '%s'", name);
		globals.ary[index].value = value;
		return index;
	}

	// reallocate if necessary
	if (globals.len == globals.cap) {
		globals.cap *= 2;
		globals.ary = xrealloc(globals.ary, sizeof_array(struct global, globals.cap));
	}

	LOG("global[%d]: %s\n", globals.len, name);

	// initialize the global
	globals.ary[globals.len].name = strdup(name);
	globals.ary[globals.len].value = value;

	// return the index of the global for future use.
	return globals.len++;
}

static int new_global(const char *name) {
	int index = lookup_global_variable(name);

	return (index == -1) ? declare_global_variable(name, SQ_NI) : index;
}

static unsigned declare_local_variable(struct sq_code *code, const char *name) {
	// reallocate if necessary
	RESIZE(vars.cap, vars.len, vars.ary, struct local);

	LOG("local[%d]: %s\n", globals.len, name);

	code->vars.ary[code->vars.len].name = strdup(name);
	return code->vars.ary[code->vars.len++].index = next_local(code);
}

static int lookup_local_variable(struct sq_code *code, const char *name) {
	// check to see if we've declared the local before
	for (unsigned i = 0; i < code->vars.len; ++i) {
		if (!strcmp(name, code->vars.ary[i].name)) {
			return code->vars.ary[i].index;
		}
	}

	return -1;
}

static unsigned new_local_variable(struct sq_code *code, const char *name) {
	int index = lookup_local_variable(code, name);

	return (index == -1) ? declare_local_variable(code, name) : index;
}


static int lookup_identifier(struct sq_code *code, const char *name) {
	int index;
	if ((index = lookup_local_variable(code, name)) != -1)
		return index;

	if ((index = lookup_global_variable(name)) != -1)
		return ~index;

	return new_local_variable(code, name);
}

static unsigned load_identifier(struct sq_code *code, const char *name) {
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
		set_index(code, new_constant(code, sq_value_new(sq_text_new(strdup(var->name)))));
		set_index(code, index = next_local(code));
	}

	free(var);

	return index;
}

static unsigned compile_expression(struct sq_code *code, struct expression *expr);
static void compile_statements(struct sq_code *code, struct statements *stmts);
static struct sq_journey *compile_function(struct func_declaration *fndecl, bool is_method);

static void compile_form_declaration(struct sq_code *code, struct class_declaration *fdecl) {
	struct sq_form *form = sq_form_new(fdecl->name);
	declare_global_variable(form->name, sq_value_new(form));

	form->nmatter = fdecl->nmatter;
	form->matter = xmalloc(sizeof(struct sq_form_matter));
	printf("nmatter=%d\n", fdecl->nmatter);
	for (unsigned i = 0; i < fdecl->nmatter; ++i) {
		form->matter[i].name = fdecl->matter[i].name;
		form->matter[i].type = SQ_UNDEFINED;
		assert(fdecl->matter[i].type == NULL); // todo
	}

	declare_global_variable(form->name, SQ_NI);

	form->imitate = fdecl->constructor ? compile_function(fdecl->constructor, true) : NULL;

	form->nrecollections = fdecl->nfuncs;
	form->recollections = xmalloc(sizeof_array(struct sq_journey *, form->nrecollections));
	for (unsigned i = 0; i < form->nrecollections; ++i)
		form->recollections[i] = compile_function(fdecl->funcs[i], false);

	form->nchanges = fdecl->nmeths;
	form->changes = xmalloc(sizeof_array(struct sq_journey *, form->nchanges));
	for (unsigned i = 0; i < form->nchanges; ++i)
		form->changes[i] = compile_function(fdecl->meths[i], true);

	form->nessences = fdecl->nessences;
	form->essences = xmalloc(sizeof_array(struct sq_form_essence, form->nessences));
	for (unsigned i = 0; i < fdecl->nessences; ++i) {
		form->essences[i].name = fdecl->essences[i].name;
		form->essences[i].value = SQ_NI;
	}

	if (fdecl->nessences) {
		unsigned global = lookup_global_variable(form->name);
		set_opcode(code, SQ_OC_GLOAD);
		set_index(code, global);
		set_index(code, global = next_local(code));

		for (unsigned i = 0; i < fdecl->nessences; ++i) {
			if (!fdecl->essences[i].value) {
				form->essences[i].value = SQ_NI;
				continue;
			}
			unsigned index = compile_expression(code, fdecl->essences[i].value);

			set_opcode(code, SQ_OC_ISTORE);
			set_opcode(code, global);
			set_index(code, new_constant(code, sq_value_new(sq_text_new(strdup(fdecl->essences[i].name)))));
			set_index(code, index);
			set_index(code, index);
		}
	}

	form->parents = xmalloc(sizeof_array(struct sq_form *, fdecl->nparents));
	form->nparents = fdecl->nparents;

	for (unsigned i = 0; i < fdecl->nparents; ++i) {
		int index = lookup_global_variable(fdecl->parents[i]);

		if (index < 0)
			die("undeclared form '%s' set as parent", fdecl->parents[i]);
		else
			free(fdecl->parents[i]);

		if (!sq_value_is_form(globals.ary[index].value))
			die("can only set forms as parents, not %s", sq_value_typename(globals.ary[index].value));
		form->parents[i] = sq_value_as_form(sq_value_clone(globals.ary[index].value));
	}

	free(fdecl); // but none of the fields, as they're now owned by `form`.
}

static void compile_func_declaration(struct func_declaration *fdecl) {
	assert(fdecl->name != NULL);

	declare_global_variable(strdup(fdecl->name), SQ_NI);

	struct sq_journey *func = compile_function(fdecl, false);
	free(fdecl); // but none of the fields, as they're now owned by `func`.

	declare_global_variable(func->name, sq_value_new(func));
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
		index = load_constant(code, SQ_NI);
	} else {
		index = compile_expression(code, rstmt->value);
	}

	set_opcode(code, SQ_OC_RETURN);
	set_index(code, index);
}

/* layout:
<compile condition>
for each case:
	<compile case>
	<compare case to condition>
	<jeq bodies[case index]>
if alas:
	<compile alas>
jump to end

for each body:
	if body isnt null:
		<compile body>
		<jump to end>
	if body is null:
		have the next non-nulll body set us.
alas:

*/
static void compile_switch_statement(struct sq_code *code, struct switch_statement *sw) {
	unsigned condition_index = compile_expression(code, sw->cond);
	unsigned *jump_to_body_indices[sw->ncases];

	for (unsigned i = 0; i < sw->ncases; ++i) {
		unsigned case_index = compile_expression(code, sw->cases[i].expr);
		set_opcode(code, SQ_OC_EQL);
		set_index(code, condition_index);
		set_index(code, case_index);
		set_index(code, case_index); // overwrite the case with the destination

		set_opcode(code, SQ_OC_JMP_TRUE);
		set_index(code, case_index);
		jump_to_body_indices[i] = CURRENT_INDEX_PTR(code);
		set_index(code, 0xffff);
	}

	unsigned *jump_to_end_indices[sw->ncases + 1];

	if (sw->alas)
		compile_statements(code, sw->alas);

	set_opcode(code, SQ_OC_JMP);
	jump_to_end_indices[sw->ncases] = CURRENT_INDEX_PTR(code);
	set_index(code, 0xffff);

	unsigned amnt_of_blank = 0;
	for (unsigned i = 0; i < sw->ncases; ++i) {
		if (sw->cases[i].body == NULL) {
			amnt_of_blank++;
			jump_to_end_indices[i] = NULL;
			continue;
		}

		for (unsigned j = 0; j < amnt_of_blank; ++j)
			*jump_to_body_indices[i - j - 1] = code->codelen;
		amnt_of_blank = 0;
		*jump_to_body_indices[i] = code->codelen;
		compile_statements(code, sw->cases[i].body);
		set_opcode(code, SQ_OC_JMP);
		jump_to_end_indices[i] = CURRENT_INDEX_PTR(code);
		set_index(code, 0xffff);
	}

	for (unsigned j = 0; j < amnt_of_blank; ++j)
		*jump_to_body_indices[sw->ncases - j - 1] = code->codelen;

	for (unsigned i = 0; i < sw->ncases + 1; ++i) {
		if (jump_to_end_indices[i])
			*jump_to_end_indices[i] = code->codelen;
	}

	free(sw->cases);
	free(sw);
}

static unsigned compile_book(struct sq_code *code, struct book *book) {
	unsigned indices[book->npages];

	for (unsigned i = 0; i < book->npages; ++i)
		indices[i] = compile_expression(code, book->pages[i]);

	set_opcode(code, SQ_OC_INT);
	set_index(code, SQ_INT_BOOK_NEW);
	set_index(code, book->npages);

	for (unsigned i = 0; i < book->npages; ++i)
		set_index(code, indices[i]);

	unsigned index = next_local(code);
	set_index(code, index);

	return index;
}

static unsigned compile_codex(struct sq_code *code, struct dict *dict) {
	unsigned keys[dict->neles], vals[dict->neles];

	for (unsigned i = 0; i < dict->neles; ++i) {
		keys[i] = compile_expression(code, dict->keys[i]);
		vals[i] = compile_expression(code, dict->vals[i]);
	}

	set_opcode(code, SQ_OC_INT);
	set_index(code, SQ_INT_CODEX_NEW);
	set_index(code, dict->neles);

	for (unsigned i = 0; i < dict->neles; ++i) {
		set_index(code, keys[i]);
		set_index(code, vals[i]);
	}

	unsigned index = next_local(code);
	set_index(code, index);

	return index;
}

static unsigned compile_primary(struct sq_code *code, struct primary *primary);

static unsigned compile_index(struct sq_code *code, struct index *index) {
	unsigned into = compile_primary(code, index->into);
	unsigned idx = compile_expression(code, index->index);

	set_opcode(code, SQ_OC_INDEX);
	set_index(code, into);
	set_index(code, idx);
	set_index(code, idx = next_local(code));

	return idx;
}

static unsigned compile_primary(struct sq_code *code, struct primary *primary) {
	unsigned result;

	switch (primary->kind) {
	case SQ_PS_PPAREN:
		result = compile_expression(code, primary->expr);
		break;

	case SQ_PS_PBOOK:
		result = compile_book(code, primary->book);
		break;

	case SQ_PS_PCODEX:
		result = compile_codex(code, primary->dict);
		break;

	case SQ_PS_PINDEX:
		result = compile_index(code, primary->index);
		break;

	case SQ_PS_PLAMBDA: {
		struct sq_journey *func = compile_function(primary->lambda, false);
		free(primary->lambda);

		result = load_constant(code, sq_value_new(func));
		break;
	}

	case SQ_PS_PNUMERAL:
		result = load_constant(code, sq_value_new(primary->numeral));
		break;

	case SQ_PS_PTEXT:
		result = load_constant(code, sq_value_new(primary->text));
		break;

	case SQ_PS_PVERACITY:
		result = load_constant(code, sq_value_new(primary->veracity));
		break;

	case SQ_PS_PNI:
		result = load_constant(code, SQ_NI);
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

	BUILTIN_FN("proclaim",  SQ_INT_PRINTLN, 1);
	BUILTIN_FN("proclaimn", SQ_INT_PRINT, 1);
	BUILTIN_FN("tally",     SQ_INT_TONUMERAL, 1);
	BUILTIN_FN("numeral",   SQ_INT_TONUMERAL, 1);
	BUILTIN_FN("text",      SQ_INT_TOTEXT, 1); // `prose` ?
	BUILTIN_FN("veracity",  SQ_INT_TOVERACITY, 1); // `veracity`
	BUILTIN_FN("dump",      SQ_INT_DUMP, 1); // not changing this, it's used for internal debugging.
	BUILTIN_FN("length",	SQ_INT_LENGTH, 1); // `fathoms` ? furlong
	BUILTIN_FN("substr",	SQ_INT_SUBSTR, 3);
	BUILTIN_FN("dismount",  SQ_INT_EXIT, 1);
	BUILTIN_FN("genus",     SQ_INT_KINDOF, 1);
	BUILTIN_FN("hex",       SQ_INT_SYSTEM, 1); // this doesn't feel right... `pray`? but that's too strong.
	BUILTIN_FN("inquire",   SQ_INT_PROMPT, 0);
	BUILTIN_FN("gamble",	SQ_INT_RANDOM, 0);
	BUILTIN_FN("insert",	SQ_INT_ARRAY_INSERT, 3);
	BUILTIN_FN("delete",	SQ_INT_ARRAY_DELETE, 2); // `slay`?
	BUILTIN_FN("roman",     SQ_INT_ROMAN, 1);
	BUILTIN_FN("arabic",	SQ_INT_ARABIC, 1);

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
		
		int into = compile_primary(code, expr->ary_asgn->into);
		index = compile_expression(code, expr->ary_asgn->index);
		unsigned val = compile_expression(code, expr->ary_asgn->value);

		set_opcode(code, SQ_OC_INDEX_ASSIGN);
		set_index(code, into);
		set_index(code, index);
		set_index(code, val);

		return val;
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
		set_index(code, new_constant(code, sq_value_new(sq_text_new(strdup(var->field->name)))));
		set_index(code, index);
		set_index(code, index = next_local(code));

		return index;
	}

	case SQ_PS_EMATH:
		return compile_bool(code, expr->math);

	case SQ_PS_EINDEX:
		return compile_index(code, expr->index);

	default:
		bug("unknown expr kind '%d'", expr->kind);
	}
}

static void compile_statements(struct sq_code *code, struct statements *stmts);

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

static void create_label_statement(struct sq_code *code, char *label, bool in_mem) {
	unsigned len = code->labels.len++;
	unsigned i;

	// havent found the label, add it. (note we increase len here)
	if (len == code->labels.cap)
		code->labels.ary = xrealloc(code->labels.ary, sizeof_array(struct label, code->labels.cap *= 2));

	code->labels.ary[len].name = label;
	if (in_mem) {
		set_opcode(code, SQ_OC_COMEFROM);
		code->labels.ary[len].length = (int *) &code->bytecode[code->codelen];
#ifdef SQ_WHENCE_CONTINUES_ON // ie continues onwards in addition to jumping
		set_index(code, i = 1);
		set_index(code, code->codelen + MAX_COMEFROMS); 
#else
		set_index(code, i = 0);
#endif
	} else {
		code->labels.ary[len].length = NULL;
		i = 0;
	}

	for (; i < MAX_COMEFROMS; ++i) {
		code->labels.ary[len].indices[i] = -1;

		if (in_mem) set_index(code, -1);
	}
}

static void compile_label_statement(struct sq_code *code, char *label) {
	struct label *lbl;
	unsigned j;
	for (unsigned i = 0; i < code->labels.len; ++i) {
		// we've found a destination, assign to that.
		if (!strcmp((lbl=&code->labels.ary[i])->name, label)) {
			if (lbl->length != NULL)
				die("cannot redefine '%s'", label);
			free(label);

			set_opcode(code, SQ_OC_COMEFROM);
			lbl->length = (int *) &code->bytecode[code->codelen];

#ifdef SQ_WHENCE_CONTINUES_ON // ie continues onwards in addition to jumping
			set_index(code, j = 1);
			set_index(code, code->codelen + MAX_COMEFROMS); 
#else
			set_index(code, j = 0);
#endif

			for (; j < MAX_COMEFROMS; ++j) {
				if (lbl->indices[j] != -1) ++*lbl->length;
				set_index(code, lbl->indices[j]);
			}

			return;
		}
	}

	// haven't found it, need to set it.
	create_label_statement(code, label, true);
}

static void compile_comefrom_statement(struct sq_code *code, char *label) {
	unsigned i, j;
	struct label *lbl;

	// check to see if the label statement exists.
	for (i = 0; i < code->labels.len; ++i) {
		if (!strcmp(code->labels.ary[i].name, label)) {
			lbl = &code->labels.ary[i];

			// if the label already exists, make it go here.
			free(label);
			if (lbl->length != NULL) goto already_exists;

			for (j = 0; j < MAX_COMEFROMS; ++j)
				if (lbl->indices[j] == -1) {
					goto set_existing;
				}

			die("max amount of 'whence's encountered.");
		}
	}

	j = 0;

	// it doesn't, create it.
	create_label_statement(code, label, false);

set_existing:

	code->labels.ary[i].indices[j] = code->codelen;
	return;

already_exists:

	if (*lbl->length == MAX_COMEFROMS) die("max amount of 'whence's encountered.");

	unsigned length = ++*lbl->length;
	lbl->length[length] = code->codelen;
}

static void compile_trycatch_statement(struct sq_code *code, struct trycatch_statement *tc) {
	unsigned *catchblock, *noerror, exception;

	set_opcode(code, SQ_OC_TRYCATCH);
	catchblock = CURRENT_INDEX_PTR(code);
	set_index(code, -1);
	set_index(code, exception = new_local_variable(code, tc->exception));

	compile_statements(code, tc->try);
	set_opcode(code, SQ_OC_POPTRYCATCH);
	set_opcode(code, SQ_OC_JMP);
	noerror = CURRENT_INDEX_PTR(code);
	set_index(code, -1);

	*catchblock = code->codelen;
	compile_statements(code, tc->catch);
	*noerror = code->codelen;

	// free(tc->exception);
	free(tc);
}

static void compile_throw_statement(struct sq_code *code, struct expression *throw) {
	unsigned dst = compile_expression(code, throw);
	set_opcode(code, SQ_OC_THROW);
	set_index(code, dst);
}

static void compile_statement(struct sq_code *code, struct statement *stmt) {
	switch (stmt->kind) {
	case SQ_PS_SGLOBAL: compile_global(code, stmt->gdecl); break;
	case SQ_PS_SLOCAL: compile_local(code, stmt->ldecl); break;
	case SQ_PS_SCLASS: compile_form_declaration(code, stmt->cdecl); break;
	case SQ_PS_SFUNC: compile_func_declaration(stmt->fdecl); break;
	case SQ_PS_SIF: compile_if_statement(code, stmt->ifstmt); break;
	case SQ_PS_SWHILE: compile_while_statement(code, stmt->wstmt); break;
	case SQ_PS_SLABEL: compile_label_statement(code, stmt->label); break;
	case SQ_PS_SCOMEFROM: compile_comefrom_statement(code, stmt->comefrom); break;
	case SQ_PS_SRETURN: compile_return_statement(code, stmt->rstmt); break;
	case SQ_PS_STRYCATCH: compile_trycatch_statement(code, stmt->tcstmt); break;
	case SQ_PS_STHROW: compile_throw_statement(code, stmt->throwstmt); break;
	case SQ_PS_SSWITCH: compile_switch_statement(code, stmt->sw_stmt); break;
	case SQ_PS_SEXPR: compile_expression(code, stmt->expr); break;
	default: bug("unknown statement kind '%d'", stmt->kind);
	}
}

static void compile_statements(struct sq_code *code, struct statements *stmts) {
	for (unsigned i = 0; i < stmts->len; ++i)
		compile_statement(code, stmts->stmts[i]);
}

static struct sq_journey *compile_function(struct func_declaration *fndecl, bool is_method) {
	struct sq_code code;
	code.codecap = 2048;
	code.codelen = 0;
	code.bytecode = xmalloc(sizeof_array(union sq_bytecode, code.codecap));

	code.nlocals = 0;
	code.consts.cap = 64;
	code.consts.len = 0;
	code.consts.ary = xmalloc(sizeof_array(sq_value , code.consts.cap));

	code.vars.len = fndecl->nargs;
	code.vars.cap = 16 + code.vars.len;
	code.vars.ary = xmalloc(sizeof_array(struct local, code.vars.cap));

	code.labels.len = 0;
	code.labels.cap = 4;
	code.labels.ary = xmalloc(sizeof_array(struct label, code.labels.cap));

	unsigned offset = 0;

	for (unsigned i = 0; i < fndecl->nargs; ++i) {
		code.vars.ary[i+offset].name = fndecl->args[i];
		code.vars.ary[i+offset].index = next_local(&code);
	}

	if (fndecl->body != NULL)
		compile_statements(&code, fndecl->body);

	struct sq_journey *fn = xmalloc(sizeof(struct sq_journey));

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
	globals.ary = xmalloc(sizeof_array(struct local, globals.cap = 16));
	globals.ary[0].name = strdup("ARGV");
	globals.ary[0].value = SQ_NI;

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
	program->globals = xmalloc(sizeof_array(sq_value , globals.len));
	for (unsigned i = 0; i < program->nglobals; ++i)
		program->globals[i] = globals.ary[i].value;

	return program;
}
