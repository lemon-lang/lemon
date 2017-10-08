#ifndef LEMON_LKARG_H
#define LEMON_LKARG_H

#include "lobject.h"

/*
 * keyword argument
 * 'func(foo = 1);'
 */
struct lkarg {
	struct lobject object;

	struct lobject *keyword;
	struct lobject *argument;
};

void *
lkarg_create(struct lemon *lemon,
             struct lobject *keyword,
             struct lobject *argument);

struct ltype *
lkarg_type_create(struct lemon *lemon);

#endif /* LEMON_LKARG_H */
