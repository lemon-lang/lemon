#ifndef LEMON_LARRAY_H
#define LEMON_LARRAY_H

#include "lobject.h"

struct larray {
	struct lobject object;

	int alloc;
	int count;
	struct lobject **items;
};

struct lobject *
larray_append(struct lemon *lemon,
              struct lobject *self,
              int argc, struct lobject *argv[]);

struct lobject *
larray_get_item(struct lemon *lemon,
                struct lobject *self,
                long i);

struct lobject *
larray_set_item(struct lemon *lemon,
                struct lobject *self,
                long i,
                struct lobject *value);

struct lobject *
larray_del_item(struct lemon *lemon,
                struct lobject *self,
                long i);

long
larray_length(struct lemon *lemon, struct lobject *self);

void *
larray_create(struct lemon *lemon, int count, struct lobject *items[]);

struct ltype *
larray_type_create(struct lemon *lemon);

#endif /* LEMON_LARRAY_H */
