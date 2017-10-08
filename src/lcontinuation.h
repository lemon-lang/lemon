#ifndef LEMON_LCONTINUATION_H
#define LEMON_LCONTINUATION_H

#include "lobject.h"

struct lframe;

struct lcontinuation {
	struct lobject object;

	int address;
	int framelen;
	int stacklen;

	struct lframe **frame;
	struct lobject **stack;
	struct lobject *value;
};

void *
lcontinuation_create(struct lemon *lemon);

struct ltype *
lcontinuation_type_create(struct lemon *lemon);

#endif /* LEMON_LCONTINUATION_H */
