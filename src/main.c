// #include "token.h"
// #include "function.h"
// #include <string.h>
// #include "program.h"
// #include "parse.h"
// #include "struct.h"
// #include "value.h"
// #include <stdio.h>

#include "program.h"

int main(int a, char **v) {
	struct sq_program *program = sq_program_compile(v[1]);
	sq_program_run(program);
	sq_program_free(program);

	return 0;
}


// sq_value consts[100];
// union sq_bytecode code[100];
// struct sq_function *funcs[100];
// struct sq_variable globals[100];

// int main4() {
// 	struct sq_program program;
// 	program.nglobals = 1;
// 	program.globals = globals;
// 	program.globals[0].value = sq_value_new_number(99);
// 	program.globals[0].name = strdup("foo");
// 	program.nfuncs = 1;
// 	program.funcs = funcs;

// 	struct sq_function main;
// 	printf("123\n");
// 	main.name = "main";
// 	main.refcount = -1;
// 	main.argc = 0;
// 	main.nlocals = 1;
// 	main.nconsts = 3;
// 	main.consts = consts;
// 	main.code = code;
// 	main.program = &program;

// 	char *fieldnames[2];
// 	fieldnames[0] = strdup("name");
// 	fieldnames[1] = strdup("age");
// 	struct sq_struct *person = sq_struct_new("person", 2, fieldnames);

// 	sq_value fields[2];
// 	fields[0] = sq_value_new_string(sq_string_new("sam"));
// 	fields[1] = sq_value_new_number(23);
// 	struct sq_instance *me = sq_instance_new(person, fields);

// 	main.consts[0] = sq_value_new_number(12);
// 	main.consts[1] = sq_value_new_instance(me);
// 	main.consts[2] = sq_value_new_string(sq_string_new("age"));
// 	main.code[0].opcode = SQ_OC_CLOAD;
// 	main.code[1].index = 1;
// 	main.code[2].index = 0;
// 	main.code[3].opcode = SQ_OC_ILOAD;
// 	main.code[4].index = 0;
// 	main.code[5].index = 2;
// 	main.code[6].index = 0;
// 	main.code[7].index = SQ_OC_RETURN;
// 	main.code[8].index = 0;

//   	program.funcs[0] = &main;

//   	printf("%ld", sq_value_as_number(sq_function_run(&main, NULL)));
//   	return 0;
// }

// struct sq_token sq_next_token(const char **stream);

// int main1() {
// 	const char*str = "\n\
// struct person { name, favcolor }	\n\
// 	\n\
// func greet(person) {	\n\
// 	print('Hello, ' + person.name + '!\\n')	\n\
// 	\n\
// 	if person.favcolor == 'green' {	\n\
// 		print('I like green too!\\n')	\n\
// 	}	\n\
// }	\n\
// 	\n\
// samp = person('samp', 'green')	\n\
// greet(samp)	\n\
// 	\n\
// 	\n\
// func fizzBuzz (max) {	\n\
// 	i = 1;	\n\
// 	\n\
// 	while i < max {	\n\
// 		if (i % 3) == 0 {	\n\
// 			print('Fizz')	\n\
// 		}	\n\
// 	\n\
// 		if (i % 5) == 0 {	\n\
// 			print('Buzz')	\n\
// 		}	\n\
// 	\n\
// 		if ((i % 3) * (i % 5)) != 0 {	\n\
// 			print(i)	\n\
// 		}	\n\
// 	\n\
// 		i += 1	\n\
// 	\n\
// 		print('\\n')	\n\
// 	}	\n\
// }	\n\
// 	\n\
// fizzBuzz(100)	\n\
// ";
// 	struct sq_token token;


// 	while ((token = sq_next_token(&str)).kind != SQ_TK_UNDEFINED) {
// 		sq_token_dump(&token);
// 		puts("");
// 	}
// 	return 0;
// }

