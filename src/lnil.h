#ifndef LEMON_LNIL_H
#define LEMON_LNIL_H

#include "lobject.h"

struct lnil {
	struct lobject object;
};

void *
lnil_create(struct lemon *);

#endif /* LEMON_LNIL_H */
