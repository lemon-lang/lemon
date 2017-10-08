#ifndef LEMON_LVKARG_H
#define LEMON_LVKARG_H

#include "lobject.h"

/*
 * variable keword argument
 * 'func(*foo);'
 * foo is iterable object for element like (keyword, argument)
 */
struct lvkarg {
	struct lobject object;

	struct lobject *arguments;
};

void *
lvkarg_create(struct lemon *lemon, struct lobject *arguments);

struct ltype *
lvkarg_type_create(struct lemon *lemon);

#endif /* LEMON_LVKARG_H */
