#ifndef LEMON_LINSTANCE_H
#define LEMON_LINSTANCE_H

#include "lobject.h"

struct linstance {
	struct lobject object;

	struct lclass *clazz;
	struct lobject *attr;
	struct lobject *native;
};

void *
linstance_create(struct lemon *lemon, struct lclass *clazz);

struct ltype *
linstance_type_create(struct lemon *lemon);

#endif /* LEMON_LINSTANCE_H */
