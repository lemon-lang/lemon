#include "lemon.h"
#include "input.h"
#include "lexer.h"
#include "scope.h"
#include "symbol.h"
#include "syntax.h"
#include "parser.h"
#include "compiler.h"
#include "machine.h"
#include "generator.h"
#include "ltable.h"

#include <stdlib.h>
#include <string.h>

#include <readline/readline.h>
#include <readline/history.h>

int
check_paren_is_closed(char *code, int len)
{
	int i;
	int a;
	int b;
	int c;
	int ss;
	int ds;

	a = 0;
	b = 0;
	c = 0;
	ss = 0;
	ds = 0;
	for (i = 0; i < len; i++) {
		if (ss) {
			switch (code[i]) {
			case '\\':
				i += 1;
				break;

			case '\'':
				ss = 0;
				break;
			}

			continue;
		}
		if (ds) {
			switch (code[i]) {
			case '\\':
				i += 1;
				break;
			case '"':
				ds = 0;
				break;
			}
			continue;
		}
		switch (code[i]) {
		case '\'':
			ss = 1;
			break;
		case '"':
			ds = 1;
			break;
		case '(':
			a += 1;
			break;
		case ')':
			a -= 1;
			break;
		case '[':
			b += 1;
			break;
		case ']':
			b -= 1;
			break;
		case '{':
			c += 1;
			break;
		case '}':
			c -= 1;
			break;
		}
	}

	return (a == 0 && b == 0 && c == 0);
}

int
check_stmt_is_closed(char *code, int len)
{
	int i;
	int brace;
	int semicon;

	if (!len) {
		return 1;
	}

	if (!check_paren_is_closed(code, len)) {
		return 0;
	}

	brace = 0;
	semicon = 0;
	for (i = 0; i < len; i++) {
		if (semicon) {
			switch (code[i]) {
			case ' ':
			case '\n':
			case '\t':
				break;
			default:
				semicon = 0;
			}
		} else {
			switch (code[i]) {
			case '}':
				brace = 1;
				break;
			case ';':
				semicon = 1;
				break;
			default:
				break;
			}
		}
	}

	if (semicon) {
		return 1;
	}

	if (brace) {
		return 1;
	}

	return 0;
}

void
shell_store_frame(struct lemon *lemon,
                  struct lframe *frame,
                  struct lobject *locals[])
{
	int i;

	for (i = 0; i < frame->nlocals; i++) {
		locals[i] = lframe_get_item(lemon, frame, i);
	}
}

void
shell_restore_frame(struct lemon *lemon,
                    struct lframe *frame,
                    int nlocals,
                    struct lobject *locals[])
{
	int i;

	for (i = 0; i < nlocals; i++) {
		lframe_set_item(lemon, frame, i, locals[i]);
	}
}

struct lframe *
shell_extend_frame(struct lemon *lemon, struct lframe *frame, int nlocals)
{
	int i;
	struct lframe *newframe;

	newframe = frame;
	if (frame->nlocals < nlocals) {
		newframe = lframe_create(lemon, NULL, NULL, NULL, nlocals);

		lobject_copy(lemon,
		             (struct lobject *)frame,
		             (struct lobject *)newframe,
		             sizeof(struct lframe));

		for (i = frame->nlocals; i < nlocals; i++) {
			lframe_set_item(lemon, newframe, i, lemon->l_nil);
		}
	}

	return newframe;
}

void
shell(struct lemon *lemon)
{
	int pc;

	int stmtlen;
	int codelen;
	char stmt[4096];
	char code[40960];
	char *buffer;

	struct syntax *node;
	struct lframe *frame;

	int nlocals;
	struct lobject *locals[256];

	puts("Lemon Version 0.0.1");
	puts("Copyright 2017 Zhicheng Wei");
	puts("Type '\\help' for more information, '\\exit' or ^D exit\n");

	pc = 0;
	codelen = 0;
	stmtlen = 0;
	nlocals = 0;
	lemon_machine_reset(lemon);
	memset(code, 0, sizeof(code));
	while (1) {
		if (check_stmt_is_closed(code, codelen)) {
			buffer = readline(">>> ");
		} else {
			buffer = readline("... ");
		}
        add_history(buffer);

		if (!buffer) {
			break;
		}

		if (strcmp(buffer, "\\help\n") == 0) {
			printf("'\\dis'  print bytecode\n"
			       "'\\list' print source code\n"
			       "'\\exit' exit from shell\n");
			continue;
		}
		if (strcmp(buffer, "\\dis\n") == 0) {
			machine_disassemble(lemon);
			continue;
		}
		if (strcmp(buffer, "\\list\n") == 0) {
			printf("%.*s", codelen, code);
			continue;
		}
		if (strcmp(buffer, "\\exit\n") == 0) {
			break;
		}

		/* copy buffer to stmt for error recovery */
		memcpy(stmt + stmtlen,
		       buffer,
		       strlen(buffer));
		stmtlen += strlen(buffer);

		/* copy buffer to code */
		memcpy(code + codelen,
		       buffer,
		       strlen(buffer));
		codelen += strlen(buffer);

        free(buffer);

		if (!check_stmt_is_closed(code, codelen)) {
			continue;
		}
		input_set_buffer(lemon, "main", code, codelen);
		lexer_next_token(lemon);

		node = parser_parse(lemon);
		if (!node) {
			codelen -= stmtlen;
			stmtlen = 0;
			fprintf(stderr, "lemon: syntax error\n");

			continue;
		}

		if (!compiler_compile(lemon, node)) {
			codelen -= stmtlen;
			stmtlen = 0;
			lemon->l_generator = generator_create(lemon);
			fprintf(stderr, "generator: syntax error\n");

			continue;
		}

		stmtlen = 0;
		lemon_machine_reset(lemon);
		generator_emit(lemon);
		lemon->l_generator = generator_create(lemon);
		lemon_machine_set_pc(lemon, pc);
		if (lemon_machine_get_fp(lemon) >= 0) {
			frame = lemon_machine_get_frame(lemon, 0);
			frame = shell_extend_frame(lemon,
			                           frame,
			                           node->nlocals);
			shell_restore_frame(lemon,
			                    frame,
			                    nlocals,
			                    locals);
			lemon_machine_set_frame(lemon, 0, frame);
		}

		lemon_machine_execute_loop(lemon);
		pc = ((struct machine *)lemon->l_machine)->maxpc;
		if (lemon_machine_get_fp(lemon) >= 0) {
			frame = lemon_machine_get_frame(lemon, 0);
			shell_store_frame(lemon, frame, locals);
			nlocals = frame->nlocals;
		} else {
			break;
		}

		if (feof(stdin)) {
			printf("\n");

			break;
		}
	}
}
