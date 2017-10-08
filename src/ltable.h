#ifndef LEMON_LTABLE_H
#define LEMON_LTABLE_H

struct lobject;

struct ltable {
	struct lobject object;

	int count;
	int length;

	void *items;
};

void *
ltable_create(struct lemon *lemon);

struct ltype *
ltable_type_create(struct lemon *lemon);

#endif /* LEMON_LTABLE_H */
