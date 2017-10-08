#ifndef LEMON_LVARG_H
#define LEMON_LVARG_H

#include "lobject.h"

/*
 * variable argument
 * 'func(*foo);'
 * foo is iterable object
 */
struct lvarg {
	struct lobject object;

	struct lobject *arguments;
};

void *
lvarg_create(struct lemon *lemon, struct lobject *arguments);

struct ltype *
lvarg_type_create(struct lemon *lemon);

#endif /* LEMON_LVARG_H */
