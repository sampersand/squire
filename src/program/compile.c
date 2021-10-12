#include <squire/program/parse.h>
#include <squire/program/compiler.h>
#include <squire/program/compile.h>
#include <squire/shared.h>

#define SET_OPCODE(opcode) sq_compiler_set_opcode(compiler, opcode)
#define SET_TARGET(index) sq_compiler_set_index(compiler, index)
#define SET_CODEPOS(index) sq_compiler_set_index(compiler, index)
#define SET_INTERRUPT(interrupt) sq_compiler_set_interrupt(compiler, interrupt)
#define SET_COUNT(count) sq_compiler_set_count(compiler, count)
#define NEXT_TARGET() sq_compiler_next_local(compiler)
#define CODE_POS() sq_compiler_codepos(compiler)

#define DEFER_JUMP() sq_compiler_defer_jump(compiler)
#define SET_JUMP_DST(lbl, dst) sq_compiler_set_jmp_dst(compiler, lbl, dst)

void sq_compile_book(struct sq_compiler *compiler, struct sq_ps_book *book, sq_target target) {
	sq_target page_targets[book->len];

	for (unsigned i = 0; i < book->len; ++i)
		sq_compile_expression(compiler, book->pages[i], page_targets[i] = NEXT_TARGET());

	SET_OPCODE(SQ_OC_INT);
	SET_INTERRUPT(SQ_INT_BOOK_NEW);
	SET_COUNT(book->len);

	for (unsigned i = 0; i < book->len; ++i)
		SET_TARGET(page_targets[i]);

	SET_TARGET(target);
}

void sq_compile_codex(struct sq_compiler *compiler, struct sq_ps_codex *codex, sq_target target) {
	sq_target key_targets[codex->len];
	sq_target val_targets[codex->len];

	for (unsigned i = 0; i < codex->len; ++i) {
		sq_compile_expression(compiler, codex->keys[i], key_targets[i] = NEXT_TARGET());
		sq_compile_expression(compiler, codex->vals[i], val_targets[i] = NEXT_TARGET());
	}

	SET_OPCODE(SQ_OC_INT);
	SET_INTERRUPT(SQ_INT_CODEX_NEW);
	SET_COUNT(codex->len);

	for (unsigned i = 0; i < codex->len; ++i) {
		SET_TARGET(key_targets[i]);
		SET_TARGET(val_targets[i]);
	}

	SET_TARGET(target);
}

void sq_compile_if(struct sq_compiler *, struct sq_ps_if *, sq_target);

/*
loop_begin:
	target = <whilst->condition>
	if (!target) goto end;
	<whilst->body>
	goto loop_begin;
end:
*/
void sq_compile_whilst(struct sq_compiler *compiler, struct sq_ps_whilst *whilst, sq_target target) {
	unsigned begin_label, jump_to_end;

	if (whilst->alas)
		todo("alas in whilst");

	begin_label = CODE_POS();
	sq_compile_expression(compiler, whilst->condition, target);
	SET_OPCODE(SQ_OC_JMP_FALSE);
	SET_TARGET(target);
	jump_to_end = DEFER_JUMP();

	sq_compile_statements(compiler, whilst->body, target);
	SET_OPCODE(SQ_OC_JMP);
	SET_CODEPOS(begin_label);

	SET_JUMP_DST(jump_to_end, CODE_POS());
	// do i free `whilst` here?
}

void sq_compile_return(struct sq_compiler *compiler, struct sq_ps_expression *expr) {
	sq_target return_target;

	if (!expr) {
		 return_target = sq_compiler_constant_load(compiler, SQ_NI);
	} else {
		sq_compile_expression(compiler, expr, return_target = SQ_SCRATCH_TARGET);
	}

	SET_OPCODE(SQ_OC_RETURN);
	SET_TARGET(return_target);
}

void sq_compile_expression(struct sq_compiler *compiler, struct sq_ps_expression *expr, sq_target target) {
	(void) compiler;
	(void) expr;
	(void) target;
}

void sq_compile_statements(struct sq_compiler *compiler, struct sq_ps_statements *stmts, sq_target target) {
	(void) compiler;
	(void) stmts;
	(void) target;
}
/*
void sq_compile_fork(struct sq_compiler *compiler, struct sq_ps_fork *fork, sq_target target) {
	sq_target condition_target = NEXT_TARGET();
	sq_compile_expression(compiler, fork->condition, condition_target);

	unsigned jump_to_body_indices[fork->count];

	for (unsigned i = 0; i < fork->len; ++i) {
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
}

static void compile_switch_statement(struct sq_code *code, struct switch_statement *sw) {
	unsigned condition_index = compile_expression(code, sw->cond);
	unsigned jump_to_body_indices[sw->ncases];

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

	int jump_to_end_indices[sw->ncases + 1];

	if (sw->alas)
		compile_statements(code, sw->alas);

	set_opcode(code, SQ_OC_JMP);
	jump_to_end_indices[sw->ncases] = code->codelen;
	set_index(code, 65531);

	unsigned amnt_of_blank = 0;
	for (unsigned i = 0; i < sw->ncases; ++i) {
		if (sw->cases[i].body == NULL) {
			amnt_of_blank++;
			jump_to_end_indices[i] = -1;
			continue;
		}

		for (unsigned j = 0; j < amnt_of_blank; ++j)
			jump_to_body_indices[i - j - 1] = code->codelen;
		amnt_of_blank = 0;
		set_target_to_codelen(code, jump_to_body_indices[i]);
		jump_to_body_indices[i] = code->codelen;
		compile_statements(code, sw->cases[i].body);
		set_opcode(code, SQ_OC_JMP);
		jump_to_end_indices[i] = code->codelen;
		set_index(code, 65532);
	}

	for (unsigned j = 0; j < amnt_of_blank; ++j)
		jump_to_body_indices[sw->ncases - j - 1] = code->codelen;

	for (unsigned i = 0; i <= sw->ncases; ++i) {
		if (0 <= jump_to_end_indices[i])
			set_target_to_codelen(code, jump_to_end_indices[i]);
	}

	free(sw->cases);
	free(sw);
}
*/
