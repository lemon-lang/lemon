#ifndef LEMON_GC_H
#define LEMON_GC_H

struct collector {
	int phase;
	int enabled;

	long live; /* number of live objects */

	long step_ratio; /* ratio to perform action */
	long step_threshold;

	long full_ratio; /* ratio to compute threshold */
	long full_threshold;

	int stacklen;
	int stacktop;
	struct lobject **stack;

	struct lobject *object_list;
	struct lobject *sweeping_prev;
	struct lobject *sweeping;
};

void *
collector_create(struct lemon *lemon);

void
collector_destroy(struct lemon *lemon, struct collector *collector);

void
collector_step(struct lemon *lemon, long step_max);

void
collector_full(struct lemon *lemon);

void
collector_collect(struct lemon *lemon);

#endif /* LEMON_GC_H */
