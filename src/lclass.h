#ifndef LEMON_LCLASS_H
#define LEMON_LCLASS_H

#include "lobject.h"

struct lclass {
	struct lobject object;

	struct lobject *name;
	struct lobject *bases; /* larray of bases */

	struct lobject *attr;
	struct lobject *getter;
	struct lobject *setter;
};

void *
lclass_create(struct lemon *lemon,
              struct lobject *name,
              int nsupers,
              struct lobject *supers[],
              int nattrs,
              struct lobject *attrs[]);

struct ltype *
lclass_type_create(struct lemon *lemon);

#endif /* LEMON_LCLASS_H */
