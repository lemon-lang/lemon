#ifndef LEMON_LCOROUTINE_H
#define LEMON_LCOROUTINE_H

#include "lobject.h"

struct lframe;

struct lcoroutine {
	struct lobject object;

	struct lframe *frame;

	int address;
	int stacklen;
	int finished;

	struct lobject *current;
	struct lobject **stack;
};

void *
lcoroutine_create(struct lemon *lemon, struct lframe *frame);

struct ltype *
lcoroutine_type_create(struct lemon *lemon);

#endif /* LEMON_LCOROUTINE_H */
