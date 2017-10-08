#ifndef LEMON_LACCESSOR_H
#define LEMON_LACCESSOR_H

#include "lobject.h"

struct laccessor {
	struct lobject object;

	struct lobject *self;

	int count;
	struct lobject *items[1];
};

void *
laccessor_create(struct lemon *lemon, int count, struct lobject *items[]);

#endif /* LEMON_LACCESSOR_H */
