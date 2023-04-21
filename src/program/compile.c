#include <squire/program.h>
#include <squire/journey.h>
#include <squire/shared.h>
#include <squire/parse.h>
#include <squire/form.h>
#include <squire/text.h>

#include <string.h>
#include <errno.h>

static struct sq_program *program;

#define CURRENT_INDEX_PTR(code) (&(code)->bytecode[(code)->codelen].index)
static struct {
	unsigned len, cap;
	struct global {
		char *name;
		sq_value value;
	} *ary;
} globals;

#ifndef MAX_COMEFROMS
# define MAX_COMEFROMS 16
#endif
#ifndef MAX_THENCES
# define MAX_THENCES 16
#endif 

struct sq_code {
	unsigned codecap, codelen;
	union sq_bytecode *bytecode;

	unsigned nlocals;

	struct {
		unsigned cap, len;
		struct label {
			char *name;
			int start, *length, indices[MAX_COMEFROMS];
			int *thence_length, thences[MAX_THENCES];
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
	if (code->cap == code->len) code->pos = sq_realloc_vec(type, code->pos, code->cap*=2);

static void extend_bytecode_cap(struct sq_code *code) {
	RESIZE(codecap, codelen, bytecode, union sq_bytecode);
}

static void set_opcode(struct sq_code *code, enum sq_opcode opcode) {
	sq_log_old("bytecode[%d].opcode=%s\n", code->codelen, sq_opcode_repr(opcode));
	extend_bytecode_cap(code);
	code->bytecode[code->codelen++].opcode = opcode;
}

static void set_index(struct sq_code *code, unsigned index) {
	sq_log_old("bytecode[%d].index=%d\n", code->codelen, index);
	extend_bytecode_cap(code);
	code->bytecode[code->codelen++].index = index;
}

static void set_interrupt(struct sq_code *code, enum sq_interrupt interrupt) {
	sq_log_old("bytecode[%d].interrupt=%s\n", code->codelen, sq_interrupt_repr(interrupt));
	extend_bytecode_cap(code);
	code->bytecode[code->codelen++].interrupt = interrupt;
}

static void set_count(struct sq_code *code, unsigned count) {
	sq_log_old("bytecode[%d].count=%d\n", code->codelen, count);

	RESIZE(codecap, codelen, bytecode, union sq_bytecode);
	code->bytecode[code->codelen++].count = count;
}

static void set_target_to_codelen(struct sq_code *code, unsigned target) {
	sq_log_old("bytecode[%d].index=%d [update]\n", target, code->codelen);

	code->bytecode[target].index = code->codelen;
}

static unsigned next_local(struct sq_code *code) {
	return code->nlocals++;
}

static unsigned declare_constant(struct sq_code *code, sq_value value) {
	if (code->consts.cap == code->consts.len) {
		code->consts.cap *= 2;
		code->consts.ary = sq_realloc_vec(sq_value, code->consts.ary, code->consts.cap);
	}

#ifdef SQ_LOG
	printf("consts[%d]=", code->consts.len); 
	sq_value_dump(stdout, value);
	putchar('\n');
#endif /* sq_log_old */

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
			sq_throw("attempted to redefine global variable '%s'", name);
		globals.ary[index].value = value;
		return index;
	}

	// reallocate if necessary
	if (globals.len == globals.cap) {
		globals.cap *= 2;
		globals.ary = sq_realloc_vec(struct global, globals.ary, globals.cap);
	}

	sq_log_old("global[%d]: %s\n", globals.len, name);

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

	sq_log_old("local[%d]: %s\n", globals.len, name);

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

static unsigned load_variable_class(struct sq_code *code, struct variable_old *var, int *parent) {
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
		set_index(code, new_constant(code, sq_value_new_text(sq_text_new(strdup(var->name)))));
		set_index(code, index = next_local(code));
	}

	free(var);

	return index;
}

static unsigned compile_expression(struct sq_code *code, struct expression *expr);
static unsigned compile_primary(struct sq_code *code, struct primary *primary);
static void compile_statements(struct sq_code *code, struct statements *stmts);
static struct sq_journey *compile_journey(struct journey_declaration *jd, bool is_method);

static void compile_form_declaration(struct sq_code *code, struct form_declaration *fdecl) {
	struct sq_form *form = sq_form_new(fdecl->name);
	declare_global_variable(form->vt->name, sq_value_new_form(form));

	form->vt->nmatter = fdecl->nmatter;
	form->vt->matter = sq_malloc_vec(struct sq_form_matter, form->vt->nmatter);

	int global = -1;

	for (unsigned i = 0; i < fdecl->nmatter; ++i) {
		form->vt->matter[i].name = fdecl->matter[i].name;
		form->vt->matter[i].genus = SQ_UNDEFINED;

		if (fdecl->matter[i].genus) {
			if (global == -1) {
				global = lookup_global_variable(form->vt->name);
				set_opcode(code, SQ_OC_GLOAD);
				set_index(code, global);
				set_index(code, global = next_local(code));
			}

//			unsigned const_index = new_constant(code, sq_value_new_text(sq_text_new(strdup(fdecl->matter[i].name))));

			unsigned kind = compile_expression(code, fdecl->matter[i].genus);
			set_opcode(code, SQ_OC_FMGENUS_STORE);
			set_index(code, global);
			set_index(code, kind);
			set_index(code, i);
		}
	}

	form->vt->imitate = fdecl->constructor ? compile_journey(fdecl->constructor, true) : NULL;

	form->vt->nrecollections = fdecl->nfuncs;
	form->vt->recollections = sq_malloc_vec(struct sq_journey *, form->vt->nrecollections);
	for (unsigned i = 0; i < form->vt->nrecollections; ++i)
		form->vt->recollections[i] = compile_journey(fdecl->funcs[i], false);

	form->vt->nchanges = fdecl->nmeths;
	form->vt->changes = sq_malloc_vec(struct sq_journey *, form->vt->nchanges);
	for (unsigned i = 0; i < form->vt->nchanges; ++i)
		form->vt->changes[i] = compile_journey(fdecl->meths[i], true);

	form->vt->nessences = fdecl->nessences;
	form->vt->essences = sq_malloc_vec(struct sq_essence, form->vt->nessences);
	for (unsigned i = 0; i < fdecl->nessences; ++i) {
		form->vt->essences[i].name = fdecl->essences[i].name;
		form->vt->essences[i].value = SQ_NI;
		form->vt->essences[i].genus = SQ_UNDEFINED;
	}

	// no idea why this is a separate block. i think it's a holdover of older code and can be merged in
	// with the `for` loop above.
	if (fdecl->nessences) {
		if (global < 0) {
			global = lookup_global_variable(form->vt->name);
			set_opcode(code, SQ_OC_GLOAD);
			set_index(code, global);
			set_index(code, global = next_local(code));
		}

		for (unsigned i = 0; i < fdecl->nessences; ++i) {
			// note: this is technically extraneous if we never access the essence
			unsigned index;
			unsigned const_index = new_constant(code, sq_value_new_text(sq_text_new(strdup(fdecl->essences[i].name))));

			if (fdecl->essences[i].genus != NULL) {
				index = compile_expression(code, fdecl->essences[i].genus);
				set_opcode(code, SQ_OC_FEGENUS_STORE);
				set_index(code, global);
				set_index(code, index);
				set_index(code, i);
			}

			if (!fdecl->essences[i].value) {
				form->vt->essences[i].value = SQ_NI;
				continue;
			}

			index = compile_expression(code, fdecl->essences[i].value);
			set_opcode(code, SQ_OC_ISTORE);
			set_index(code, global);
			set_index(code, index);
			set_index(code, const_index);
		}
	}

	form->vt->parents = sq_malloc_vec(struct sq_form *, fdecl->nparents);
	form->vt->nparents = fdecl->nparents;

	for (unsigned i = 0; i < fdecl->nparents; ++i) {
		int index = lookup_global_variable(fdecl->parents[i]);

		if (index < 0)
			sq_throw("undeclared form '%s' set as parent", fdecl->parents[i]);
		else
			free(fdecl->parents[i]);

		if (!sq_value_is_form(globals.ary[index].value))
			sq_throw("can only set forms as parents, not %s", sq_value_typename(globals.ary[index].value));
		form->vt->parents[i] = sq_value_as_form(globals.ary[index].value);
	}

	free(fdecl); // but none of the fields, as they're now owned by `form`.
}

static void compile_kingdom_declaration(struct sq_code *code, struct kingdom_declaration *kdecl) {
// struct kingdom_declaration {
// 	char *name;
// 	unsigned nsubjects;
// 	struct {
// 		enum {
// 			SQ_PS_K_FORM,
// 			SQ_PS_K_STATEMENT,
// 			SQ_PS_K_JOURNEY,
// 			SQ_PS_K_RENOWNED,
// 			SQ_PS_K_KINGDOM,
// 		} kind;

// 		union {
// 			struct form_declaration *form_decl;	
// 			struct statement *statement;
// 			struct journey *journey;
// 			struct scope_declaration *renowned;
// 			struct kingdom_declaration *kingdom;
// 		};
// 	} *subjects;
// };

// // lol, todo better hashmaps than linked lists
// struct sq_kingdom {
// 	char *name;
// 	unsigned nsubjects, subject_cap;

// 	struct sq_kingdom_subject {
// 		char *name;
// 		sq_value person;
// 	} *subjects;
// };

	// struct sq_kingdom *kingdom = sq_malloc_heap(sizeof(struct sq_kingdom));
	// kingdom->name = kdecl->name;
	// kingdom->nsubjects = kingdom->subject_cap = kdecl->nsubjects;
	// kingdom->subjects = sq_malloc_vec(struct sq_kingdom_subject, kingdom->nsubjects);

	// declare_global_variable(strdup(kingdom->name), sq_value_new(kingdom));

	// compile_statements(code, kdecl->statements);

	(void) code;
	free(kdecl->name);
	free(kdecl);
}

static void compile_journey_declaration(struct journey_declaration *jd) {
	sq_assert_nn(jd->name);

	declare_global_variable(strdup(jd->name), SQ_NI);

	struct sq_journey *func = compile_journey(jd, false);
	free(jd); // but none of the fields, as they're now owned by `func`.

	declare_global_variable(func->name, sq_value_new_journey(func));
}

static void compile_if_statement(struct sq_code *code, struct if_statement *ifstmt) {
	unsigned condition_index, iffalse_label, finished_label;

	condition_index = compile_expression(code, ifstmt->cond);
#ifdef SQ_NMOON_JOKE
	set_opcode(code, SQ_OC_JMP_FALSE);
#else
	set_opcode(code, SQ_OC_WERE_JMP);
#endif /* SQ_NMOON_JOKE */
	set_index(code, condition_index);
	iffalse_label = code->codelen;
	set_index(code, 0);

	compile_statements(code, ifstmt->iftrue);

	if (ifstmt->iffalse) {
		set_opcode(code, SQ_OC_JMP);
		finished_label = code->codelen;
		set_index(code, 0);

		set_target_to_codelen(code, iffalse_label);
		compile_statements(code, ifstmt->iffalse);
		set_target_to_codelen(code, finished_label);
	} else {
		set_target_to_codelen(code, iffalse_label);
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
	set_target_to_codelen(code, finished_label);

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
#define ALLOCA_MAX_LEN 16

static void compile_switch_statement(struct sq_code *code, struct switch_statement *sw) {
	unsigned condition_index = compile_expression(code, sw->cond);
	SQ_ALLOCA(unsigned, jump_to_body_indices, sw->ncases, ALLOCA_MAX_LEN);
	SQ_ALLOCA(int, jump_to_end_indices, sw->ncases + 1, ALLOCA_MAX_LEN);

	for (unsigned i = 0; i < sw->ncases; ++i) {
		unsigned case_index = compile_expression(code, sw->cases[i].expr);
		set_opcode(code, SQ_OC_MATCHES);
		set_index(code, case_index);
		set_index(code, condition_index);
		set_index(code, case_index); // overwrite the case with the destination

		set_opcode(code, SQ_OC_JMP_TRUE);
		set_index(code, case_index);
		jump_to_body_indices[i] = code->codelen;
		set_index(code, 65530);
	}

	if (sw->alas)
		compile_statements(code, sw->alas);

	set_opcode(code, SQ_OC_JMP);
	jump_to_end_indices[sw->ncases] = code->codelen;
	set_index(code, 65531);

	unsigned amnt_of_blank = 0;
	for (unsigned i = 0; i < sw->ncases; ++i) {
		// `i != sw->ncases - 1` is in case the last statement does nothing.
		if (sw->cases[i].body == NULL && i != sw->ncases - 1) {
			amnt_of_blank++;
			jump_to_end_indices[i] = -1;
			continue;
		}

		for (unsigned j = 0; j < amnt_of_blank; ++j)
			set_target_to_codelen(code, jump_to_body_indices[i - j - 1]);

		amnt_of_blank = 0;
		set_target_to_codelen(code, jump_to_body_indices[i]);
		jump_to_body_indices[i] = code->codelen;

		if (sw->cases[i].body)
			compile_statements(code, sw->cases[i].body);

		if (sw->cases[i].fallthru) {
			jump_to_end_indices[i] = -1;
		} else {
			set_opcode(code, SQ_OC_JMP);
			jump_to_end_indices[i] = code->codelen;
			set_index(code, -1);
		}
	}

	for (unsigned j = 0; j < amnt_of_blank; ++j)
		jump_to_body_indices[sw->ncases - j - 1] = code->codelen;

	for (unsigned i = 0; i <= sw->ncases; ++i) {
		if (0 <= jump_to_end_indices[i])
			set_target_to_codelen(code, jump_to_end_indices[i]);
	}

	free(sw->cases);
	free(sw);
	SQ_ALLOCA_FREE(jump_to_body_indices, sw->ncases, ALLOCA_MAX_LEN);
	SQ_ALLOCA_FREE(jump_to_end_indices, sw->ncases + 1, ALLOCA_MAX_LEN);
}

static unsigned compile_book(struct sq_code *code, struct book *book) {
	SQ_ALLOCA(unsigned, indices, book->npages, ALLOCA_MAX_LEN);

	for (unsigned i = 0; i < book->npages; ++i)
		indices[i] = compile_expression(code, book->pages[i]);

	set_opcode(code, SQ_OC_INT);
	set_interrupt(code, SQ_INT_BOOK_NEW);
	set_count(code, book->npages);

	for (unsigned i = 0; i < book->npages; ++i)
		set_index(code, indices[i]);

	unsigned index = next_local(code);
	set_index(code, index);

	SQ_ALLOCA_FREE(indices, book->npages, ALLOCA_MAX_LEN);
	return index;
}

static unsigned compile_codex(struct sq_code *code, struct dict *dict) {
	SQ_ALLOCA(unsigned, keys, dict->neles, ALLOCA_MAX_LEN);
	SQ_ALLOCA(unsigned, vals, dict->neles, ALLOCA_MAX_LEN);

	for (unsigned i = 0; i < dict->neles; ++i) {
		keys[i] = compile_expression(code, dict->keys[i]);
		vals[i] = compile_expression(code, dict->vals[i]);
	}

	set_opcode(code, SQ_OC_INT);
	set_interrupt(code, SQ_INT_CODEX_NEW);
	set_count(code, dict->neles);

	for (unsigned i = 0; i < dict->neles; ++i) {
		set_index(code, keys[i]);
		set_index(code, vals[i]);
	}

	unsigned index = next_local(code);
	set_index(code, index);

	SQ_ALLOCA_FREE(keys, dict->neles, ALLOCA_MAX_LEN);
	SQ_ALLOCA_FREE(vals, dict->neles, ALLOCA_MAX_LEN);

	return index;
}

static unsigned compile_function_call(struct sq_code *code, struct function_call *fncall) {
	unsigned soul;
	enum sq_interrupt interrupt = SQ_INT_UNDEFINED;

#define CHECK_FOR_BUILTIN(name_, interrupt_, argc_) \
		if (!strcmp(name_, fncall->soul->variable)) { \
			if (argc_ != fncall->argc) \
				sq_throw("argc mismatch for '%s' (expected %d, got %d)", name_, argc_, fncall->argc); \
			interrupt = interrupt_; \
			goto compile_arguments; \
		}

	if (!fncall->field && fncall->soul->kind == SQ_PS_PVARIABLE) {
		CHECK_FOR_BUILTIN("proclaim",  SQ_INT_PRINTLN, 1);
		CHECK_FOR_BUILTIN("proclaimn", SQ_INT_PRINT, 1);
		CHECK_FOR_BUILTIN("dump",      SQ_INT_DUMP, 1); // not changing this, it's used for internal debugging.
		CHECK_FOR_BUILTIN("inquire",   SQ_INT_PROMPT, 0);
		CHECK_FOR_BUILTIN("dismount",  SQ_INT_EXIT, 1);
		CHECK_FOR_BUILTIN("hex",       SQ_INT_SYSTEM, 1); // this doesn't feel right... `pray`? but that's too strong.

		CHECK_FOR_BUILTIN("tally",     SQ_INT_TONUMERAL, 1);
		CHECK_FOR_BUILTIN("numeral",   SQ_INT_TONUMERAL, 1);
		CHECK_FOR_BUILTIN("text",      SQ_INT_TOTEXT, 1); // `prose` ?
		CHECK_FOR_BUILTIN("veracity",  SQ_INT_TOVERACITY, 1);
		CHECK_FOR_BUILTIN("book",      SQ_INT_TOBOOK, 1);
		CHECK_FOR_BUILTIN("codex",     SQ_INT_TOCODEX, 1);
		CHECK_FOR_BUILTIN("genus",     SQ_INT_KINDOF, 1);

		CHECK_FOR_BUILTIN("length",    SQ_INT_LENGTH, 1); // `fathoms` ? furlong
		CHECK_FOR_BUILTIN("substr",    SQ_INT_SUBSTR, 3);
		CHECK_FOR_BUILTIN("slice",     SQ_INT_SUBSTR, 3);
		CHECK_FOR_BUILTIN("insert",    SQ_INT_ARRAY_INSERT, 3);
		CHECK_FOR_BUILTIN("delete",    SQ_INT_ARRAY_DELETE, 2); // `slay`?

		CHECK_FOR_BUILTIN("gamble",    SQ_INT_RANDOM, 0);
		CHECK_FOR_BUILTIN("roman",     SQ_INT_ROMAN, 1);
		CHECK_FOR_BUILTIN("arabic",    SQ_INT_ARABIC, 1);
		CHECK_FOR_BUILTIN("ascii",     SQ_INT_ASCII, 1);
		CHECK_FOR_BUILTIN("read",      SQ_INT_PTR_GET, 1);
		CHECK_FOR_BUILTIN("addend",    SQ_INT_PTR_SET, 2); // technically not a word; derived from `addenda`

		// TODO: remove scroll as a builtin
		CHECK_FOR_BUILTIN("Scroll", SQ_INT_FOPEN, 2); // just so you can do `Scroll(...)`
	}

	soul = compile_primary(code, fncall->soul);

compile_arguments:;
	SQ_ALLOCA(unsigned, args, fncall->argc, ALLOCA_MAX_LEN);

	for (unsigned i = 0; i < fncall->argc; ++i)
		args[i] = compile_expression(code, fncall->args[i]);

	if (interrupt != SQ_INT_UNDEFINED) {
		set_opcode(code, SQ_OC_INT);
		set_interrupt(code, interrupt);
		goto assign_arguments;
	}

	if (fncall->field != NULL) {
		unsigned target;
		set_opcode(code, SQ_OC_NOOP);

		set_opcode(code, SQ_OC_ILOAD);
		set_index(code, soul);
		set_index(code, new_constant(code, sq_value_new_text(sq_text_new(strdup(fncall->field)))));
		set_index(code, target = next_local(code));

		set_opcode(code, SQ_OC_CALL);
		set_index(code, target);
		set_count(code, fncall->argc + 1);
		set_index(code, soul);
	} else {
		set_opcode(code, SQ_OC_CALL);
		set_index(code, soul);
		set_index(code, fncall->argc);
	}

assign_arguments:

	for (unsigned i = 0; i < fncall->argc; ++i)
		set_index(code, args[i]);

	unsigned result;
	set_index(code, result = next_local(code));

	SQ_ALLOCA_FREE(args, fncall->argc, ALLOCA_MAX_LEN);
	return result;
}

static unsigned compile_field_access(struct sq_code *code, struct field_access *faccess) {
	unsigned soul = compile_primary(code, faccess->soul), target;

	set_opcode(code, SQ_OC_ILOAD);
	set_index(code, soul);
	set_index(code, new_constant(code, sq_value_new_text(sq_text_new(strdup(faccess->field)))));
	set_index(code, target = next_local(code));

	return target;
}

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

	case SQ_PS_PLAMBDA: {
		struct sq_journey *func = compile_journey(primary->lambda, false);
		free(primary->lambda);

		result = load_constant(code, sq_value_new_journey(func));
		break;
	}

	case SQ_PS_PNUMERAL:
		result = load_constant(code, sq_value_new_numeral(primary->numeral));
		break;

	case SQ_PS_PTEXT:
		result = load_constant(code, sq_value_new_text(primary->text));
		break;

	case SQ_PS_PVERACITY:
		result = load_constant(code, sq_value_new_veracity(primary->veracity));
		break;

	case SQ_PS_PNI:
		result = load_constant(code, SQ_NI);
		break;

	case SQ_PS_PVARIABLE_OLD:
		result = load_variable_class(code, primary->variable_old, NULL);
		break;

	case SQ_PS_PVARIABLE:
		result = load_identifier(code, primary->variable);
		break;

	case SQ_PS_PFNCALL:
		result = compile_function_call(code, &primary->fncall);
		break;

	case SQ_PS_PFACCESS:
		result = compile_field_access(code, &primary->faccess);
		break;

	case SQ_PS_PINDEX:
		result = compile_index(code, &primary->index);
		break;

	case SQ_PS_PCITE:
		result = compile_expression(code, primary->expr);
		set_opcode(code, SQ_OC_CITE);
		set_index(code, result);
		set_index(code, result = next_local(code));
		break;

	case SQ_PS_PBABEL: {
		unsigned executable = compile_primary(code, primary->babel.executable);
		unsigned args[SQ_BABEL_MAX_ARGC];
		for (unsigned i = 0; i < primary->babel.nargs; ++i)
			args[i] = compile_expression(code, primary->babel.args[i]);
		unsigned stdin = compile_primary(code, primary->babel.stdin);

		set_opcode(code, SQ_OC_INT);
		set_interrupt(code, SQ_INT_BABEL);
		set_index(code, executable);
		set_index(code, stdin);
		set_count(code, primary->babel.nargs);
		for (unsigned i = 0; i < primary->babel.nargs; ++i)
			set_index(code, args[i]);
		set_index(code, result = next_local(code));
		break;
	}

	default:
		sq_bug("unknown primary class '%d'", primary->kind);
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
	case SQ_PS_UPAT_NOT: set_opcode(code, SQ_OC_PAT_NOT); break;

	case SQ_PS_UPRIMARY: result = rhs; goto done;
	default: sq_bug("unknown unary kind '%d'", unary->kind);
	}

	set_index(code, rhs);
	set_index(code, result = next_local(code));

done:

	free(unary);
	return result;
}

static unsigned compile_pow(struct sq_code *code, struct pow_expression *pow) {
	unsigned lhs, rhs, result;

	lhs = compile_unary(code, pow->lhs);

	if (pow->kind != SQ_PS_PUNARY)
		rhs = compile_pow(code, pow->rhs);

	switch (pow->kind) {
	case SQ_PS_PPOW: set_opcode(code, SQ_OC_POW); break;
	case SQ_PS_PUNARY: result = lhs; goto done;
	default: sq_bug("unknown pow kind '%d'", pow->kind);
	}

	set_index(code, lhs);
	set_index(code, rhs);
	set_index(code, result = next_local(code));

done:

	free(pow);
	return result;
}

static unsigned compile_mul(struct sq_code *code, struct mul_expression *mul) {
	unsigned lhs, rhs, result;

	lhs = compile_pow(code, mul->lhs);

	if (mul->kind != SQ_PS_MPOW)
		rhs = compile_mul(code, mul->rhs);

	switch (mul->kind) {
	case SQ_PS_MMUL: set_opcode(code, SQ_OC_MUL); break;
	case SQ_PS_MDIV: set_opcode(code, SQ_OC_DIV); break;
	case SQ_PS_MMOD: set_opcode(code, SQ_OC_MOD); break;
	case SQ_PS_MPOW: result = lhs; goto done;
	default: sq_bug("unknown mul kind '%d'", mul->kind);
	}

	set_index(code, lhs);
	set_index(code, rhs);
	set_index(code, result = next_local(code));

done:

	free(mul);
	return result;
}

// static unsigned compile_mul(struct sq_code *code, struct mul_expression *mul) {
// 	unsigned lhs, rhs, result;

// 	lhs = compile_pow(code, mul->lhs);

// 	if (mul->kind != SQ_PS_MPOW)
// 		rhs = compile_mul(code, mul->rhs);

// 	switch (mul->kind) {
// 	case SQ_PS_MMUL: set_opcode(code, SQ_OC_MUL); break;
// 	case SQ_PS_MDIV: set_opcode(code, SQ_OC_DIV); break;
// 	case SQ_PS_MMOD: set_opcode(code, SQ_OC_MOD); break;
// 	case SQ_PS_MPOW: result = lhs; goto done;
// 	default: sq_bug("unknown mul kind '%d'", mul->kind);
// 	}

// 	set_index(code, lhs);
// 	set_index(code, rhs);
// 	set_index(code, result = next_local(code));

// done:

// 	free(mul);
// 	return result;
// }

static unsigned compile_add(struct sq_code *code, struct add_expression *add) {
	unsigned lhs, rhs, result;

	lhs = compile_mul(code, add->lhs);
	if (add->kind != SQ_PS_AMUL)
		rhs = compile_add(code, add->rhs);

	switch (add->kind) {
	case SQ_PS_AADD: set_opcode(code, SQ_OC_ADD); break;
	case SQ_PS_ASUB: set_opcode(code, SQ_OC_SUB); break;
	case SQ_PS_AMUL: result = lhs; goto done;
	default: sq_bug("unknown add kind '%d'", add->kind);
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
	case SQ_PS_CCMP: set_opcode(code, SQ_OC_CMP); break;
	case SQ_PS_CADD: result = lhs; goto done;
	default: sq_bug("unknown cmp kind '%d'", cmp->kind);
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
	case SQ_PS_EMATCHES: set_opcode(code, SQ_OC_MATCHES); break;
	case SQ_PS_EAND_PAT: set_opcode(code, SQ_OC_PAT_AND); break;
	case SQ_PS_EOR_PAT: set_opcode(code, SQ_OC_PAT_OR); break;
	case SQ_PS_ECMP: result = lhs; goto done;
	default: sq_bug("unknown eql kind '%d'", eql->kind);
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

	if (bool_->kind == SQ_PS_BEQL) {
		target = tmp;
		goto done;
	}
	
	set_opcode(code, SQ_OC_MOV);
	set_index(code, tmp);
	set_index(code, target = next_local(code));

	switch (bool_->kind) {
	case SQ_PS_BAND: set_opcode(code, SQ_OC_JMP_FALSE); break;
	case SQ_PS_BOR:  set_opcode(code, SQ_OC_JMP_TRUE); break;
	default: sq_bug("unknown bool kind '%d'", bool_->kind);
	}

	set_index(code, tmp);
	unsigned dst_label = code->codelen;
	set_index(code, 0);

	tmp = compile_bool(code, bool_->rhs);
	set_opcode(code, SQ_OC_MOV);
	set_index(code, tmp);
	set_index(code, target);
	set_target_to_codelen(code, dst_label);

done:

	free(bool_);
	return target;
}

static unsigned compile_function_call_old(struct sq_code *code, struct function_call_old *fncall) {
	SQ_ALLOCA(unsigned, args, fncall->arglen, ALLOCA_MAX_LEN);

	for (unsigned i = 0; i < fncall->arglen; ++i)
		args[i] = compile_expression(code, fncall->args[i]);

	if (fncall->func->field != NULL) {
		set_opcode(code, SQ_OC_NOOP);
		int dst;
		unsigned var = load_variable_class(code, fncall->func, &dst);
		set_opcode(code, SQ_OC_CALL);
		set_index(code, var);
		set_count(code, fncall->arglen + 1);
		set_index(code, dst);
		goto arguments;
	}

#define BUILTIN_FN(name_, int_, argc_) \
	if (!strcmp(fncall->func->name, name_)) { \
		if (fncall->arglen != argc_) \
			sq_throw("exactly %d arg(s) are required for '%s'", argc_, name_); \
		set_opcode(code, SQ_OC_INT); \
		set_interrupt(code, int_); \
		goto arguments; \
	}

	BUILTIN_FN("proclaim",  SQ_INT_PRINTLN, 1);
	BUILTIN_FN("proclaimn", SQ_INT_PRINT, 1);
	BUILTIN_FN("dump",      SQ_INT_DUMP, 1); // not changing this, it's used for internal debugging.
	BUILTIN_FN("inquire",   SQ_INT_PROMPT, 0);
	BUILTIN_FN("dismount",  SQ_INT_EXIT, 1);
	BUILTIN_FN("hex",       SQ_INT_SYSTEM, 1); // this doesn't feel right... `pray`? but that's too strong.

	BUILTIN_FN("tally",     SQ_INT_TONUMERAL, 1);
	BUILTIN_FN("numeral",   SQ_INT_TONUMERAL, 1);
	BUILTIN_FN("text",      SQ_INT_TOTEXT, 1); // `prose` ?
	BUILTIN_FN("veracity",  SQ_INT_TOVERACITY, 1);
	BUILTIN_FN("book",      SQ_INT_TOBOOK, 1);
	BUILTIN_FN("codex",     SQ_INT_TOCODEX, 1);
	BUILTIN_FN("genus",     SQ_INT_KINDOF, 1);

	BUILTIN_FN("length",    SQ_INT_LENGTH, 1); // `fathoms` ? furlong
	BUILTIN_FN("substr",    SQ_INT_SUBSTR, 3);
	BUILTIN_FN("slice",     SQ_INT_SUBSTR, 3);
	BUILTIN_FN("insert",    SQ_INT_ARRAY_INSERT, 3);
	BUILTIN_FN("delete",    SQ_INT_ARRAY_DELETE, 2); // `slay`?

	BUILTIN_FN("read",      SQ_INT_PTR_GET, 1);
	BUILTIN_FN("addend",    SQ_INT_PTR_SET, 2);
	BUILTIN_FN("gamble",    SQ_INT_RANDOM, 0);
	BUILTIN_FN("roman",     SQ_INT_ROMAN, 1);
	BUILTIN_FN("arabic",    SQ_INT_ARABIC, 1);

	BUILTIN_FN("Scroll", SQ_INT_FOPEN, 2); // just so you can do `Scroll(...)`
	BUILTIN_FN("ascii", SQ_INT_ASCII, 1);

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

	SQ_ALLOCA_FREE(args, fncall->arglen, ALLOCA_MAX_LEN);
	return result;

}

static unsigned compile_expression(struct sq_code *code, struct expression *expr) {
	unsigned index;
	int variable;

	switch (expr->kind) {
	case SQ_PS_EFNCALL:
		return compile_function_call_old(code, expr->fncall);

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
		struct variable_old *var = expr->asgn->var;
		variable = lookup_identifier(code, var->name);

		if (!var->field) {
			if (0 <= variable) {
				set_opcode(code, SQ_OC_MOV);
				set_index(code, index);
				set_index(code, index = variable);
			} else if (!var->field) {
				set_opcode(code, SQ_OC_GSTORE);
				set_index(code, index);
				set_index(code, ~variable);
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
			sq_throw("only one layer deep for assignment supported rn");

		set_opcode(code, SQ_OC_ISTORE);
		set_opcode(code, variable);
		set_index(code, index);
		set_index(code, new_constant(code, sq_value_new_text(sq_text_new(strdup(var->field->name)))));

		return index;
	}

	case SQ_PS_EMATH:
		return compile_bool(code, expr->math);

	default:
		sq_bug("unknown expr kind '%d'", expr->kind);
	}
}

static void compile_statements(struct sq_code *code, struct statements *stmts);

static unsigned compile_global(struct sq_code *code, struct scope_declaration *gdecl) {
	unsigned index = new_global(gdecl->name);
	if (gdecl->value == NULL) 
		goto done;

	unsigned result = compile_expression(code, gdecl->value);
	set_opcode(code, SQ_OC_GSTORE);
	set_index(code, result);
	set_index(code, index);

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
		code->labels.ary = sq_realloc_vec(struct label, code->labels.ary, code->labels.cap *= 2);

	code->labels.ary[len].start = code->codelen;
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
				sq_throw("cannot redefine '%s'", label);
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

			sq_throw("max amount of 'whence's encountered.");
		}
	}

	j = 0;

	// it doesn't, create it.
	create_label_statement(code, label, false);

set_existing:

	code->labels.ary[i].indices[j] = code->codelen;
	return;

already_exists:

	if (*lbl->length == MAX_COMEFROMS) sq_throw("max amount of 'whence's encountered.");

	unsigned length = ++*lbl->length;
	lbl->length[length] = code->codelen;
}

// TODO: this doesnt actually work
static void compile_thence_statement(struct sq_code *code, char *label) {
	unsigned i, j;
	struct label *lbl;

	// check to see if the label statement exists.
	for (i = 0; i < code->labels.len; ++i) {
		lbl = &code->labels.ary[i];
		if (!strcmp(lbl->name, label)) {
			// if the label already exists, make it go here.
			free(label);
			if (lbl->thence_length != NULL) goto already_exists;

			for (j = 0; j < MAX_THENCES; ++j)
				if (lbl->thences[j] == -1)
					goto set_existing;

			sq_throw("max amount of 'whence's encountered.");
		}
	}

	j = 0;

	// it doesn't, create it.
	create_label_statement(code, label, false);

set_existing:

	code->labels.ary[i].thences[j] = code->codelen;
	return;

already_exists:

	if (*lbl->length == MAX_THENCES) sq_throw("max amount of 'thences's encountered.");
// 	set_opcode(code, SQ_OC_JMP);
// 	set_index(code, code->labels.ary[i].start + MAX_THENCES + 2);

	unsigned length = ++*lbl->thence_length;
	lbl->thence_length[length] = code->codelen;

// 	unsigned i;

// 	// check to see if the label statement exists.
// 	for (i = 0; i < code->labels.len; ++i)
// 		if (!strcmp(code->labels.ary[i].name, label)) {
// 			free(label); 
// 			goto set_existing;
// 		}

// 	// it doesn't, create it.
// 	create_label_statement(code, label, false);
// 	start = 2;

// set_existing:
	
// 	set_opcode(code, SQ_OC_JMP);
// 	set_index(code, code->labels.ary[i].start + MAX_COMEFROMS + 2);
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
	case SQ_PS_SKINGDOM: compile_kingdom_declaration(code, stmt->kdecl); break;
	case SQ_PS_SJOURNEY: compile_journey_declaration(stmt->jdecl); break;
	case SQ_PS_SIF: compile_if_statement(code, stmt->ifstmt); break;
	case SQ_PS_SWHILE: compile_while_statement(code, stmt->wstmt); break;
	case SQ_PS_SLABEL: compile_label_statement(code, stmt->label); break;
	case SQ_PS_SCOMEFROM: compile_comefrom_statement(code, stmt->comefrom); break;
	case SQ_PS_STHENCE: compile_thence_statement(code, stmt->thence); break;
	case SQ_PS_SRETURN: compile_return_statement(code, stmt->rstmt); break;
	case SQ_PS_STRYCATCH: compile_trycatch_statement(code, stmt->tcstmt); break;
	case SQ_PS_STHROW: compile_throw_statement(code, stmt->throwstmt); break;
	case SQ_PS_SSWITCH: compile_switch_statement(code, stmt->sw_stmt); break;
	case SQ_PS_SEXPR: compile_expression(code, stmt->expr); break;
	}
}

static void compile_statements(struct sq_code *code, struct statements *stmts) {
	for (unsigned i = 0; i < stmts->len; ++i)
		compile_statement(code, stmts->stmts[i]);
}

static void compile_journey_pattern(
	struct sq_journey_pattern *pattern,
	struct journey_pattern *jp,
	bool is_method
) {
	(void) is_method;
	pattern->pargc = jp->pargc;
	pattern->kwargc = jp->kwargc;
	pattern->splat = jp->splat != NULL;
	pattern->splatsplat = jp->splatsplat != NULL;
	pattern->pargv = sq_malloc_vec(struct sq_journey_argument, pattern->pargc);
	pattern->kwargv = sq_malloc_vec(struct sq_journey_argument, pattern->kwargc);

	struct sq_code code;
	code.codecap = 2048;
	code.codelen = 0;
	code.bytecode = sq_malloc_vec(union sq_bytecode, code.codecap);
	code.bytecode = sq_malloc_vec(union sq_bytecode, code.codecap);

	code.nlocals = pattern->pargc + pattern->kwargc + (pattern->splat ? 1 : 0) + (pattern->splatsplat ? 1 : 0);
	code.consts.cap = 64;
	code.consts.len = 0;
	code.consts.ary = sq_malloc_vec(sq_value, code.consts.cap);
	code.consts.ary = sq_malloc_vec(sq_value, code.consts.cap);

	code.vars.len = 0;
	code.vars.cap = SQ_JOURNEY_MAX_ARGC * 2 + 2; // *2 for both positional and kw, then +2 for splat and splatsplat
	code.vars.ary = sq_malloc_vec(struct local, code.vars.cap);
	code.vars.ary = sq_malloc_vec(struct local, code.vars.cap);

	code.labels.len = 0;
	code.labels.cap = 4;
	code.labels.ary = sq_malloc_vec(struct label, code.labels.cap);
	code.labels.ary = sq_malloc_vec(struct label, code.labels.cap);

	unsigned local_index = 0;

	for (unsigned i = 0; i < pattern->pargc; ++i, ++code.vars.len) {
		pattern->pargv[i].name = jp->pargv[i].name;

		code.vars.ary[code.vars.len].name = strdup(jp->pargv[i].name);
		code.vars.ary[code.vars.len].index = local_index++;

		if (jp->pargv[i].default_ == NULL) { 
			pattern->pargv[i].default_start = -1;
		} else {
			pattern->pargv[i].default_start = code.codelen;
			unsigned dst = compile_expression(&code, jp->pargv[i].default_);
			set_opcode(&code, SQ_OC_RETURN);
			set_index(&code, dst);
		}

		if (jp->pargv[i].genus == NULL) {
			pattern->pargv[i].genus_start = -1;
		} else {
			pattern->pargv[i].genus_start = code.codelen;
			unsigned dst = compile_expression(&code, jp->pargv[i].genus);
			set_opcode(&code, SQ_OC_RETURN);
			set_index(&code, dst);
		}
	}

	if (jp->splat) {
		code.vars.ary[code.vars.len].name = strdup(jp->splat);
		code.vars.ary[code.vars.len++].index = local_index++;
	}

	for (unsigned i = 0; i < pattern->kwargc; ++i, ++code.vars.len) {
		pattern->kwargv[i].name = jp->kwargv[i].name;

		code.vars.ary[code.vars.len].name = strdup(jp->pargv[i].name);
		code.vars.ary[code.vars.len].index = local_index++;

		sq_assert_n(jp->kwargv[i].genus); // todo
		sq_assert_n(jp->kwargv[i].default_); // todo
		// todo: use `i` when compiling default.
	}

	if (jp->splatsplat) {
		code.vars.ary[code.vars.len].name = strdup(jp->splatsplat);
		code.vars.ary[code.vars.len++].index = local_index++;
	}

	if (jp->condition) {
		pattern->condition_start = code.codelen;
		unsigned dst = compile_expression(&code, jp->condition);
		set_opcode(&code, SQ_OC_RETURN);
		set_index(&code, dst);
	} else {
		pattern->condition_start = -1;
	}

	sq_assert_nn(jp->body);

	pattern->start_index = code.codelen;
	compile_statements(&code, jp->body);

	pattern->code.nlocals = code.nlocals;
	pattern->code.nconsts = code.consts.len;
	pattern->code.codelen = code.codelen;
	pattern->code.consts = code.consts.ary;
	pattern->code.bytecode = code.bytecode;

	// todo: free everything made by `code`.

	return;
}

static struct sq_journey *compile_journey(struct journey_declaration *jd, bool is_method) {
	struct sq_journey *journey = sq_mallocv(struct sq_journey);

	journey->name = jd->name;
	journey->npatterns = jd->npatterns;
	journey->program = program;
	journey->sq_journey_is_method = is_method;
	journey->patterns = sq_malloc_vec(struct sq_journey_pattern, jd->npatterns);

	for (unsigned i = 0; i < jd->npatterns; ++i)
		compile_journey_pattern(&journey->patterns[i], &jd->patterns[i], is_method);

	return journey;
}

static void setup_globals(void) {
	globals.len = 0;
	globals.ary = sq_malloc_vec(struct global, globals.cap = 16);

	globals.ary[globals.len  ].name = strdup("ARGV");
	globals.ary[globals.len++].value = SQ_NI;

	globals.ary[globals.len  ].name = strdup("Numeral");
	globals.ary[globals.len++].value = sq_value_new_text(sq_text_new(strdup("Numeral")));

	globals.ary[globals.len  ].name = strdup("Text");
	globals.ary[globals.len++].value = sq_value_new_text(sq_text_new(strdup("Text")));

	globals.ary[globals.len  ].name = strdup("Veracity");
	globals.ary[globals.len++].value = sq_value_new_text(sq_text_new(strdup("Veracity")));

	globals.ary[globals.len  ].name = strdup("Ni");
	globals.ary[globals.len++].value = sq_value_new_text(sq_text_new(strdup("Ni")));

	globals.ary[globals.len  ].name = strdup("Journey");
	globals.ary[globals.len++].value = sq_value_new_text(sq_text_new(strdup("Journey")));

	globals.ary[globals.len  ].name = strdup("Imitation");
	globals.ary[globals.len++].value = sq_value_new_text(sq_text_new(strdup("Imitation")));

	globals.ary[globals.len  ].name = strdup("Form");
	globals.ary[globals.len++].value = sq_value_new_text(sq_text_new(strdup("Form")));

	globals.ary[globals.len  ].name = strdup("Book");
	globals.ary[globals.len++].value = sq_value_new_text(sq_text_new(strdup("Book")));

	globals.ary[globals.len  ].name = strdup("Codex");
	globals.ary[globals.len++].value = sq_value_new_text(sq_text_new(strdup("Codex")));
}

void sq_program_compile(struct sq_program *program_, const char *stream) {
	setup_globals();

	program = program_;
	program->nglobals = 0;
	program->globals = NULL;

	struct journey_declaration maindecl = {
		.name = strdup("main"),
		.npatterns = 1,
		.patterns = {
			{
				.pargc = 0,
				.kwargc = 0,
				.splat = NULL,
				.splatsplat = NULL,
				.return_genus = NULL,
				.body = sq_parse_statements(stream)
			}
		}
	};

	program->main = compile_journey(&maindecl, false);

	program->nglobals = globals.len;
	program->globals = sq_malloc_vec(sq_value, globals.len);

	for (unsigned i = 0; i < program->nglobals; ++i)
		program->globals[i] = globals.ary[i].value;
	
}
