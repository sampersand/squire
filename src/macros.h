#ifndef SQ_MACROS_H
#define SQ_MACROS_H

#include <stdbool.h>
#include "token.h"

struct sq_token next_macro_token(void);
void parse_macro_statement(char *);
void parse_macro_identifier(char *);


/*
# Macros in Squire. the `$` sigil is used to denote macro parameters, and
# the `@` sigil is used to denote the start of a preprocessor directive.
# Note that because im lazy, they'll be parsed at the same time as normal code, and
# so you can actually reference variables in the surrounding scope.

# Simple declarations and functional macros. They simply take the next line, like c.
@henceforth $ab = "ab" # note `$` is required
@henceforth $add(a, b) $a + $bs
proclaim($add(2, 3)); # => same as `proclaim(2 + 3)`.

ab = "ab"
# Conditional compilation
@if $ab == ab # you can just use arbitrary expressions here, including referencing surroudning scope
    @henceforth $life = 42
@alasif $ab == "bc"
    @henceforth $life = 43
@alas
    @henceforth $foo = "bar"
@end

# Looping. requires the use of arrays.
@foreach i in [$ab, "cd", "ef"]
    proclaim($i + "\n");
@end

# Interpret a value as a token stream
@henceforth $x = "proclaim("
@henceforth $y = "'hello, world!\n'"
@henceforth $z = ")"
@explicate $x + $y + $z # => proclaim('hello, world!\n')]
# ^-- maybe fix?
*/
#endif /* !SQ_MACROS_H */
