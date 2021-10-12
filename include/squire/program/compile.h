#ifndef SQ_COMPILE_H
#define SQ_COMPILE_H

#include <squire/program/compiler.h>
#include <squire/program/parse.h>

#define SQ_SCRATCH_TARGET 0

void sq_compile_statements(struct sq_compiler *, struct sq_ps_statements *, sq_target);
void sq_compile_if(struct sq_compiler *, struct sq_ps_if *, sq_target);
void sq_compile_whilst(struct sq_compiler *, struct sq_ps_whilst *, sq_target);
void sq_compile_return(struct sq_compiler *, struct sq_ps_expression *); // expr can be null. notably no target.
void sq_compile_fork(struct sq_compiler *, struct sq_ps_fork *, sq_target);

void sq_compile_assign(struct sq_compiler *, struct sq_ps_assign *, sq_target);
void sq_compile_variable_decl(struct sq_compiler *, struct sq_ps_variable_decl *, sq_target);
void sq_compile_besiege(struct sq_compiler *, struct sq_ps_besiege *, sq_target);
void sq_compile_expression(struct sq_compiler *, struct sq_ps_expression *, sq_target);
void sq_compile_fn_call(struct sq_compiler *, struct sq_ps_fn_call *, sq_target);
void sq_compile_compound(struct sq_compiler *, struct sq_ps_compound *, sq_target);
void sq_compile_book(struct sq_compiler *, struct sq_ps_book *, sq_target);
void sq_compile_codex(struct sq_compiler *, struct sq_ps_codex *, sq_target);
void sq_compile_get_attr(struct sq_compiler *, struct sq_ps_get_attr *, sq_target);
void sq_compile_primary(struct sq_compiler *, struct sq_ps_primary *, sq_target);

#endif /* !SQ_COMPILE_H */
