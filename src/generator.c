#include "lemon.h"
#include "arena.h"
#include "opcode.h"
#include "machine.h"
#include "generator.h"

#include <stdio.h>
#include <string.h>

struct generator *
generator_create(struct lemon *lemon)
{
	struct generator *generator;

	generator = arena_alloc(lemon, lemon->l_arena, sizeof(*generator));
	if (generator) {
		memset(generator, 0, sizeof(*generator));
	}

	return generator;
}

struct generator_arg *
generator_make_arg(struct lemon *lemon,
                   int type,
                   int size,
                   int value)
{
	struct generator_arg *arg;

	arg = arena_alloc(lemon, lemon->l_arena, sizeof(*arg));
	if (arg) {
		memset(arg, 0, sizeof(*arg));
		arg->type = type;
		arg->size = size;
		arg->value = value;
	}

	return arg;
}

struct generator_arg *
generator_make_arg_label(struct lemon *lemon,
                         struct generator_label *label)
{
	struct generator_arg *arg;

	arg = generator_make_arg(lemon, 1, 4, 0);
	if (arg) {
		arg->label = label;
	}

	return arg;
}

struct generator_code *
generator_make_code(struct lemon *lemon,
                    int opcode,
                    struct generator_arg *arg1,
                    struct generator_arg *arg2,
                    struct generator_arg *arg3,
                    struct generator_arg *arg4,
                    struct generator_arg *arg5)
{
	struct generator_code *code;

	code = arena_alloc(lemon, lemon->l_arena, sizeof(*code));
	if (code) {
		memset(code, 0, sizeof(*code));
		code->opcode = opcode;
		code->arg[0] = arg1;
		code->arg[1] = arg2;
		code->arg[2] = arg3;
		code->arg[3] = arg4;
		code->arg[4] = arg5;
	}

	return code;
}

struct generator_label *
generator_make_label(struct lemon *lemon)
{
	struct generator_label *label;

	label = arena_alloc(lemon, lemon->l_arena, sizeof(*label));
	if (label) {
		memset(label, 0, sizeof(*label));
	}

	return label;
}

struct generator_code *
generator_emit_code(struct lemon *lemon,
                    struct generator_code *code)
{
	int i;
	struct generator *gen;

	gen = lemon->l_generator;
	gen->address += 1;
	code->prev = gen->tail;
	if (gen->tail) {
		gen->tail->next = code;
	}
	gen->tail = code;
	if (!gen->head) {
		gen->head = code;
	}
	code->address = gen->address;

	for (i = 0; i < MAX_ARG; i++) {
		if (code->arg[i] &&
		    code->arg[i]->type == 1 &&
		    code->arg[i]->label)
		{
			code->arg[i]->label->count += 1;
		}
	}

	return code;
}

void
generator_insert_code(struct lemon *lemon,
                      struct generator_code *code,
                      struct generator_code *newcode)
{
	newcode->prev = code->prev;
	newcode->next = code;
	if (code->prev) {
		code->prev->next = newcode;
	}
}

void
generator_delete_code(struct lemon *lemon,
                      struct generator_code *code)
{
	int i;
	struct generator *gen;

	if (code->dead) {
		return;
	}

	gen = lemon->l_generator;
	if (code == gen->head) {
		gen->head = code->next;
	}

	if (code == gen->tail) {
		gen->tail = code->prev;
	}

	if (code->prev) {
		code->prev->next = code->next;
	}

	if (code->next) {
		code->next->prev = code->prev;
	}

	code->dead = 1;
	code->prev = NULL;
	code->next = NULL;

	for (i = 0; i < MAX_ARG; i++) {
		if (code->arg[i] &&
		    code->arg[i]->type == 1 &&
		    code->arg[i]->label)
		{
			code->arg[i]->label->count -= 1;
		}
	}
}

void
generator_emit_label(struct lemon *lemon,
                     struct generator_label *label)
{
	struct generator_code *code;

	code = generator_make_code(lemon, -1, NULL, NULL, NULL, NULL, NULL);
	code->label = label;
	label->code = code;
	generator_emit_code(lemon, code);
}

void
generator_patch_arg_label(struct lemon *lemon,
                          struct generator_arg *arg,
                          struct generator_label *label)
{
	int address;

	address = arg->address;
	if (arg->type == 1) {
		machine_set_code4(lemon, address, label->address);
	} else {
		printf("patch wrong arg\n");
	}
}

struct generator_code *
generator_patch_code(struct lemon *lemon,
                     struct generator_code *oldcode,
                     struct generator_code *newcode)
{
	int i;
	struct generator_code *prev;
	struct generator_code *next;

	prev = oldcode->prev;
	next = oldcode->next;

	newcode->prev = prev;
	if (prev) {
		prev->next = newcode;
	}
	newcode->next = next;
	if (next) {
		next->prev = newcode;
	}
	oldcode->dead = 1;
	oldcode->next = NULL;
	oldcode->prev = NULL;

	for (i = 0; i < MAX_ARG; i++) {
		if (oldcode->arg[i] &&
		    oldcode->arg[i]->type == 1 &&
		    oldcode->arg[i]->label)
		{
			oldcode->arg[i]->label->count -= 1;
		}
		if (newcode->arg[i] &&
		    newcode->arg[i]->type == 1 &&
		    newcode->arg[i]->label)
		{
			newcode->arg[i]->label->count -= 1;
		}
	}

	return newcode;
}

void
generator_generator_label(struct lemon *lemon,
                          struct generator_label *label)
{
	struct generator_arg *arg;

	label->address = lemon_machine_get_pc(lemon);
	for (arg = label->prevlabel;
	     arg;
	     arg = arg->prevlabel)
	{
		generator_patch_arg_label(lemon, arg, label);
	}
}

void
generator_generator_code(struct lemon *lemon,
                         struct generator_code *code)
{
	int i;
	struct generator_arg *arg;
	struct generator_label *label;

	machine_add_code1(lemon, code->opcode);
	for (i = 0; i < MAX_ARG; i++) {
		arg = code->arg[i];
		if (!arg) {
			break;
		}
		arg->address = lemon_machine_get_pc(lemon);
		if (arg->type == 1) {
			label = arg->label;
			machine_add_code4(lemon, label->address);

			if (label->address == 0) {
				arg->prevlabel = label->prevlabel;
				label->prevlabel = arg;
			}
		} else {
			/* current only 1 byte code and 4 byte code */
			if (arg->size == 1) {
				machine_add_code1(lemon, arg->value);
			} else if (arg->size == 4) {
				machine_add_code4(lemon, arg->value);
			}
		}
	}
}

void
generator_emit(struct lemon *lemon)
{
	struct machine *machine;
	struct generator *gen;
	struct generator_code *code;

	gen = lemon->l_generator;
	for (code = gen->head; code; code = code->next) {
		if (code->opcode == -1) {
			generator_generator_label(lemon, code->label);
		} else if (code->opcode != OPCODE_NOP) {
			generator_generator_code(lemon, code);
		}
	}
	machine = lemon->l_machine;
	machine->maxpc = lemon_machine_get_pc(lemon);
}

struct generator_code *
generator_emit_opcode(struct lemon *lemon,
                      int opcode)
{
	struct generator_code *code;

	code = generator_make_code(lemon,
	                           opcode,
	                           NULL,
	                           NULL,
	                           NULL,
	                           NULL,
	                           NULL);

	return generator_emit_code(lemon, code);
}

struct generator_code *
generator_emit_const(struct lemon *lemon,
                     int pool)
{
	struct generator_code *code;

	code = generator_make_code(lemon,
	                           OPCODE_CONST,
	                           generator_make_arg(lemon, 0, 4, pool),
	                           NULL,
	                           NULL,
	                           NULL,
	                           NULL);

	return generator_emit_code(lemon, code);
}

struct generator_code *
generator_emit_unpack(struct lemon *lemon,
                      int count)
{
	struct generator_code *code;

	code = generator_make_code(lemon,
	                           OPCODE_UNPACK,
	                           generator_make_arg(lemon, 0, 1, count),
	                           NULL,
	                           NULL,
	                           NULL,
	                           NULL);

	return generator_emit_code(lemon, code);
}

struct generator_code *
generator_emit_load(struct lemon *lemon,
                    int level,
                    int local)
{
	struct generator_code *code;

	code = generator_make_code(lemon,
	                           OPCODE_LOAD,
	                           generator_make_arg(lemon, 0, 1, level),
	                           generator_make_arg(lemon, 0, 1, local),
	                           NULL,
	                           NULL,
	                           NULL);

	return generator_emit_code(lemon, code);
}

struct generator_code *
generator_emit_store(struct lemon *lemon,
                     int level,
                     int local)
{
	struct generator_code *code;

	code = generator_make_code(lemon,
	                           OPCODE_STORE,
	                           generator_make_arg(lemon, 0, 1, level),
	                           generator_make_arg(lemon, 0, 1, local),
	                           NULL,
	                           NULL,
	                           NULL);

	return generator_emit_code(lemon, code);
}

struct generator_code *
generator_emit_array(struct lemon *lemon,
                     int length)
{
	struct generator_code *code;

	code = generator_make_code(lemon,
	                           OPCODE_ARRAY,
	                           generator_make_arg(lemon, 0, 4, length),
	                           NULL,
	                           NULL,
	                           NULL,
	                           NULL);

	return generator_emit_code(lemon, code);
}

struct generator_code *
generator_emit_dictionary(struct lemon *lemon,
                          int length)
{
	struct generator_code *code;

	code = generator_make_code(lemon,
	                           OPCODE_DICTIONARY,
	                           generator_make_arg(lemon, 0, 4, length),
	                           NULL,
	                           NULL,
	                           NULL,
	                           NULL);

	return generator_emit_code(lemon, code);
}

struct generator_code *
generator_emit_jmp(struct lemon *lemon,
                   struct generator_label *label)
{
	struct generator_code *code;

	code = generator_make_code(lemon,
	                           OPCODE_JMP,
	                           generator_make_arg_label(lemon, label),
	                           NULL,
	                           NULL,
	                           NULL,
	                           NULL);

	return generator_emit_code(lemon, code);
}

struct generator_code *
generator_emit_jz(struct lemon *lemon,
                  struct generator_label *label)
{
	struct generator_code *code;

	code = generator_make_code(lemon,
	                           OPCODE_JZ,
	                           generator_make_arg_label(lemon, label),
	                           NULL,
	                           NULL,
	                           NULL,
	                           NULL);

	return generator_emit_code(lemon, code);
}

struct generator_code *
generator_emit_jnz(struct lemon *lemon,
                   struct generator_label *label)
{
	struct generator_code *code;

	code = generator_make_code(lemon,
	                           OPCODE_JNZ,
	                           generator_make_arg_label(lemon, label),
	                           NULL,
	                           NULL,
	                           NULL,
	                           NULL);

	return generator_emit_code(lemon, code);
}

struct generator_code *
generator_emit_call(struct lemon *lemon,
                    int argc)
{
	struct generator_code *code;

	code = generator_make_code(lemon,
	                           OPCODE_CALL,
	                           generator_make_arg(lemon, 0, 1, argc),
	                           NULL,
	                           NULL,
	                           NULL,
	                           NULL);

	return generator_emit_code(lemon, code);
}

struct generator_code *
generator_emit_tailcall(struct lemon *lemon,
                        int argc)
{
	struct generator_code *code;

	code = generator_make_code(lemon,
	                           OPCODE_TAILCALL,
	                           generator_make_arg(lemon, 0, 1, argc),
	                           NULL,
	                           NULL,
	                           NULL,
	                           NULL);

	return generator_emit_code(lemon, code);
}

struct generator_code *
generator_emit_module(struct lemon *lemon,
                      int nlocals,
                      struct generator_label *label)
{
	struct generator_code *code;

	code = generator_make_code(lemon,
	                           OPCODE_MODULE,
	                           generator_make_arg(lemon, 0, 1, nlocals),
	                           generator_make_arg_label(lemon, label),
	                           NULL,
	                           NULL,
	                           NULL);

	return generator_emit_code(lemon, code);
}

struct generator_code *
generator_patch_module(struct lemon *lemon,
                       struct generator_code *code,
                       int nlocals,
                       struct generator_label *label)
{
	struct generator_code *newcode;

	newcode = generator_make_code(lemon,
	                              OPCODE_MODULE,
	                              generator_make_arg(lemon, 0, 1, nlocals),
	                              generator_make_arg_label(lemon, label),
	                              NULL,
	                              NULL,
	                              NULL);

	return generator_patch_code(lemon, code, newcode);
}

struct generator_code *
generator_emit_define(struct lemon *lemon,
                      int define,
                      int nvalues,
                      int nparams,
                      int nlocals,
                      struct generator_label *label)
{
	struct generator_code *code;

	code = generator_make_code(lemon,
	                           OPCODE_DEFINE,
	                           generator_make_arg(lemon, 0, 1, define),
	                           generator_make_arg(lemon, 0, 1, nvalues),
	                           generator_make_arg(lemon, 0, 1, nparams),
	                           generator_make_arg(lemon, 0, 1, nlocals),
	                           generator_make_arg_label(lemon, label));

	return generator_emit_code(lemon, code);
}

struct generator_code *
generator_patch_define(struct lemon *lemon,
                       struct generator_code *code,
                       int define,
                       int nvalues,
                       int nparams,
                       int nlocals,
                       struct generator_label *label)
{
	struct generator_code *newcode;

	newcode = generator_make_code(lemon,
	                              OPCODE_DEFINE,
	                              generator_make_arg(lemon, 0, 1, define),
	                              generator_make_arg(lemon, 0, 1, nvalues),
	                              generator_make_arg(lemon, 0, 1, nparams),
	                              generator_make_arg(lemon, 0, 1, nlocals),
	                              generator_make_arg_label(lemon, label));

	return generator_patch_code(lemon, code, newcode);
}

struct generator_code *
generator_emit_try(struct lemon *lemon,
                   struct generator_label *label)
{
	struct generator_code *code;

	code = generator_make_code(lemon,
	                           OPCODE_TRY,
	                           generator_make_arg_label(lemon, label),
	                           NULL,
	                           NULL,
	                           NULL,
	                           NULL);

	return generator_emit_code(lemon, code);
}

struct generator_code *
generator_emit_class(struct lemon *lemon,
                     int nsupers,
                     int nattrs)
{
	struct generator_code *code;

	code = generator_make_code(lemon,
	                           OPCODE_CLASS,
	                           generator_make_arg(lemon, 0, 1, nsupers),
	                           generator_make_arg(lemon, 0, 1, nattrs),
	                           NULL,
	                           NULL,
	                           NULL);

	return generator_emit_code(lemon, code);
}

struct generator_code *
generator_emit_setgetter(struct lemon *lemon,
                         int ngetters)
{
	struct generator_code *code;

	code = generator_make_code(lemon,
	                           OPCODE_SETGETTER,
	                           generator_make_arg(lemon,0, 1, ngetters),
	                           NULL,
	                           NULL,
	                           NULL,
	                           NULL);

	return generator_emit_code(lemon, code);
}

struct generator_code *
generator_emit_setsetter(struct lemon *lemon,
                         int nsetters)
{
	struct generator_code *code;

	code = generator_make_code(lemon,
	                           OPCODE_SETSETTER,
	                           generator_make_arg(lemon, 0, 1, nsetters),
	                           NULL,
	                           NULL,
	                           NULL,
	                           NULL);

	return generator_emit_code(lemon, code);
}
