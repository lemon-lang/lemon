#ifndef LEMON_LMODULE_H
#define LEMON_LMODULE_H

#include "lobject.h"

struct lmodule {
	struct lobject object;

	int nlocals;
	struct lobject *name;
	struct lframe *frame;

	struct lobject *attr;
	struct lobject *getter;
	struct lobject *setter;
};

void *
lmodule_create(struct lemon *lemon, struct lobject *name);

struct ltype *
lmodule_type_create(struct lemon *lemon);

#endif /* LEMON_LMODULE_H */
