#ifndef LEMON_LFRAME_H
#define LEMON_LFRAME_H

#include "lobject.h"

struct lframe;

typedef struct lobject *(*lframe_call_t)(struct lemon *,
                                         struct lframe *,
                                         struct lobject *); /* retval */

struct lobject *
lframe_default_callback(struct lemon *lemon,
                        struct lframe *frame,
                        struct lobject *retval);

struct lframe {
	struct lobject object;

	int ra; /* return address            */
	int sp; /* previous operand sp       */
	int ea; /* exception handler address */
	int nlocals;

	struct lobject *self;
	struct lobject *callee;
	lframe_call_t callback; /* call this function after frame is poped */
	                        /* also auto return if callback is not NULL */

	struct lframe *upframe; /* up level frame for closure function */

	/* lframe is dynamic size */
	struct lobject *locals[1];
};

struct lobject *
lframe_get_item(struct lemon *lemon,
                struct lframe *frame,
                int local);

struct lobject *
lframe_set_item(struct lemon *lemon,
                struct lframe *frame,
                int local,
                struct lobject *value);

void *
lframe_create(struct lemon *lemon,
              struct lobject *self,
              struct lobject *callee,
              lframe_call_t callback,
              int nlocals);

struct ltype *
lframe_type_create(struct lemon *lemon);

#endif /* LEMON_LFRAME_H */
