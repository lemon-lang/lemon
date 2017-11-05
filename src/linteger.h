#ifndef LEMON_LINTEGER_H
#define LEMON_LINTEGER_H

#include "extend.h"

struct linteger {
	struct lobject object;

	int sign; /* 1 postive, -1 negative */
	int length; /* count of digits[] */
	int ndigits; /* used digits in digits[] */
	extend_t digits[1];
};

struct lobject *
linteger_method(struct lemon *lemon,
                struct lobject *self,
                int method, int argc, struct lobject *argv[]);

long
linteger_to_long(struct lemon *lemon, struct lobject *self);

void *
linteger_create_from_long(struct lemon *lemon, long value);

void *
linteger_create_from_cstr(struct lemon *lemon, const char *cstr);

struct ltype *
linteger_type_create(struct lemon *lemon);

#endif /* LEMON_LINTEGER_H */
