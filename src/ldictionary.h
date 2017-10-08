#ifndef LEMON_LDICTIONARY_H
#define LEMON_LDICTIONARY_H

#include "lobject.h"

struct ldictionary {
	struct lobject object;

	struct lobject *table;
};

/*
 * count is items's count not pair count
 * items is [key0, value0, key1, value1 ... keyn, valuen] array
 */
void *
ldictionary_create(struct lemon *lemon, int count, struct lobject *items[]);

struct ltype *
ldictionary_type_create(struct lemon *lemon);

#endif /* LEMON_LDICTIONARY_H */
