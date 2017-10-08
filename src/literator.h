#ifndef LEMON_LITERATOR_H
#define LEMON_LITERATOR_H

#include "lobject.h"

typedef struct lobject *(*literator_next_t)(struct lemon *,
                                            struct lobject *,
                                            struct lobject **);

struct literator {
	struct lobject object;

	struct lobject *iterable;
	struct lobject *context;

	long max;
	literator_next_t next;
};

struct lobject *
literator_to_array(struct lemon *lemon, struct lobject *iterator, long max);

void *
literator_create(struct lemon *lemon,
                 struct lobject *iterable,
                 struct lobject *context,
                 literator_next_t next);

struct ltype *
literator_type_create(struct lemon *lemon);

#endif /* LEMON_LITERATOR_H */
