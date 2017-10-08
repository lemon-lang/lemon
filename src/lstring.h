#ifndef LEMON_LSTRING_H
#define LEMON_LSTRING_H

#include "lobject.h"

struct lstring {
	struct lobject object;

	long length;

	/* lstring is dynamic size */
	char buffer[1];
};

const char *
lstring_to_cstr(struct lemon *lemon, struct lobject *object);

char *
lstring_buffer(struct lemon *lemon, struct lobject *object);

long
lstring_length(struct lemon *lemon, struct lobject *object);

void *
lstring_create(struct lemon *lemon, const char *buffer, long length);

struct ltype *
lstring_type_create(struct lemon *);

#endif /* LEMON_LSTRING_H */
