#ifndef LEMON_LSUPER_H
#define LEMON_LSUPER_H

#include "lobject.h"

struct lsuper {
	struct lobject object;

	struct lobject *self;
	struct lobject *base;
};

void *
lsuper_create(struct lemon *lemon, struct lobject *binding);

struct ltype *
lsuper_type_create(struct lemon *lemon);

#endif /* LEMON_LSUPER_H */
