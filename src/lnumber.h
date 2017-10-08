#ifndef LEMON_LNUMBER_H
#define LEMON_LNUMBER_H

#include "lobject.h"

struct lnumber {
	struct lobject object;

	double value;
};

double
lnumber_to_double(struct lemon *lemon, struct lobject *self);

void *
lnumber_create_from_long(struct lemon *lemon, long value);

void *
lnumber_create_from_cstr(struct lemon *lemon, const char *value);

struct ltype *
lnumber_type_create(struct lemon *lemon);

#endif /* LEMON_LNUMBER_H */
