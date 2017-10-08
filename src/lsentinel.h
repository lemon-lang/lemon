#ifndef LEMON_LSENTINEL_H
#define LEMON_LSENTINEL_H

#include "lobject.h"

struct lsentinel {
	struct lobject object;
};

void *
lsentinel_create(struct lemon *);

#endif /* LEMON_LSENTINEL_H */
