#ifndef LEMON_LBOOLEAN_H
#define LEMON_LBOOLEAN_H

#include "lobject.h"

struct lboolean {
	struct lobject object;
};

void *
lboolean_create(struct lemon *lemon, int value);

struct ltype *
lboolean_type_create(struct lemon *lemon);

#endif /* LEMON_LBOOLEAN_H */
