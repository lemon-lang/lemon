#ifndef LEMON_VM_H
#define LEMON_VM_H

#include <stdint.h>

#include "lemon.h"
#include "lframe.h"

struct machine {
	int pc; /* program counter */
	int fp; /* frame pointer */
	int sp; /* stack pointer */

	int halt;
	int maxpc;

	int codelen;

	int framelen;
	int stacklen;
	int cpoollen;

	unsigned char *code;

	struct lframe **frame;
	struct lobject **stack;
	struct lobject **cpool;

	struct lobject *exception;
};

struct machine *
machine_create(struct lemon *lemon);

void
machine_destroy(struct lemon *lemon, struct machine *machine);

void
machine_reset(struct lemon *lemon);

int
machine_add_const(struct lemon *lemon, struct lobject *object);

struct lobject *
machine_get_const(struct lemon *lemon, int pool);

int
machine_add_code1(struct lemon *lemon, int value);

int
machine_set_code1(struct lemon *lemon, int location, int value);

int
machine_add_code4(struct lemon *lemon, int value);

int
machine_set_code4(struct lemon *lemon, int location, int value);

struct lobject *
machine_pop_object(struct lemon *lemon);

void
machine_push_object(struct lemon *lemon, struct lobject *object);

struct lframe *
lemon_machine_push_new_frame(struct lemon *lemon,
                             struct lobject *self,
                             struct lobject *callee,
                             lframe_call_t callback,
                             int nlocals);

/*
 * pop out all top frame with callback
 */
struct lobject *
machine_return_frame(struct lemon *lemon,
                     struct lobject *retval);

void
machine_push_frame(struct lemon *lemon,
                   struct lframe *frame);

struct lframe *
machine_peek_frame(struct lemon *lemon);

struct lframe *
machine_pop_frame(struct lemon *lemon);

void
machine_store_frame(struct lemon *lemon,
                    struct lframe *frame);

void
machine_restore_frame(struct lemon *lemon,
                      struct lframe *frame);

int
machine_throw(struct lemon *lemon);

void
machine_disassemble(struct lemon *lemon);

#endif /* LEMON_VM_H */
