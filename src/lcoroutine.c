#include "lemon.h"
#include "lstring.h"
#include "lcoroutine.h"

#include <stdio.h>
#include <string.h>

static struct lobject *
lcoroutine_current(struct lemon *lemon,
                   struct lobject *self,
                   int argc, struct lobject *argv[])
{
	struct lcoroutine *coroutine;

	coroutine = (struct lcoroutine *)self;

	return coroutine->current;
}

static struct lobject *
lcoroutine_frame_callback(struct lemon *lemon,
                          struct lframe *frame,
                          struct lobject *retval)
{
	struct lcoroutine *coroutine;

	coroutine = (struct lcoroutine *)frame->self;
	if (coroutine->finished) {
		coroutine->current = retval;
	}

	return retval;
}

static struct lobject *
lcoroutine_resume(struct lemon *lemon,
                  struct lobject *self,
                  int argc, struct lobject *argv[])
{
	int i;
	struct lframe *frame;
	struct lcoroutine *coroutine;

	coroutine = (struct lcoroutine *)self;
	if (coroutine->finished) {
		return lobject_error_type(lemon,
		                          "resume finished coroutine '%@'",
		                          self);
	}
	lemon_machine_pop_frame(lemon);

	lemon_machine_store_frame(lemon, coroutine->frame);
	for (i = coroutine->stacklen; i > 0; i--) {
		lemon_machine_push_object(lemon, coroutine->stack[i-1]);
	}

	frame = lemon_machine_push_new_frame(lemon,
	                                     self,
	                                     NULL,
	                                     lcoroutine_frame_callback,
	                                     0);
	if (!frame) {
		return NULL;
	}
	lemon_machine_push_frame(lemon, coroutine->frame);
	if (argc) {
		lemon_machine_push_object(lemon, argv[0]);
	} else {
		lemon_machine_push_object(lemon, lemon->l_nil);
	}
	lemon_machine_set_pc(lemon, coroutine->address);
	coroutine->finished = 1;

	return lemon->l_nil;
}

static struct lobject *
lcoroutine_transfer(struct lemon *lemon,
                    struct lobject *self,
                    int argc, struct lobject *argv[])
{
	int i;
	size_t size;
	struct lframe *frame;
	struct lcoroutine *coroutine;

	/* old's coroutine */
	lemon_machine_pop_frame(lemon); /* pop transfer's frame */
	frame = lemon_machine_pop_frame(lemon); /* pop caller's frame */

	if (!lobject_is_coroutine(lemon, frame->callee)) {
		const char *cstr = "can't call transfer from %@";

		return lobject_error_type(lemon, cstr, frame->callee);
	}

	/* save old coroutine */
	coroutine = (struct lcoroutine *)frame->callee;
	coroutine->frame = frame;
	coroutine->address = lemon_machine_get_pc(lemon);

	if (lemon_machine_get_sp(lemon) - frame->sp > 0) {
		coroutine->stacklen = lemon_machine_get_sp(lemon) - frame->sp;
		if (coroutine->stack) {
			lemon_allocator_free(lemon, coroutine->stack);
		}
		size = sizeof(struct lobject *) * coroutine->stacklen;
		coroutine->stack = lemon_allocator_alloc(lemon, size);
		if (!coroutine->stack) {
			return lemon->l_out_of_memory;
		}
		memset(coroutine->stack, 0, size);
		for (i = 0; i < coroutine->stacklen; i++) {
			coroutine->stack[i] = lemon_machine_pop_object(lemon);
		}
	}
	lemon_machine_restore_frame(lemon, frame);

	/* restore new coroutine */
	coroutine = (struct lcoroutine *)self;
	for (i = coroutine->stacklen; i > 0; i--) {
		lemon_machine_push_object(lemon, coroutine->stack[i-1]);
	}

	lemon_machine_push_frame(lemon, coroutine->frame);
	if (argc) {
		lemon_machine_push_object(lemon, argv[0]);
	} else {
		lemon_machine_push_object(lemon, lemon->l_nil);
	}
	lemon_machine_set_pc(lemon, coroutine->address);

	return lemon->l_nil;
}

static struct lobject *
lcoroutine_get_attr(struct lemon *lemon,
                    struct lobject *self,
                    struct lobject *name)
{
	const char *cstr;

	cstr = lstring_to_cstr(lemon, name);
	if (strcmp(cstr, "resume") == 0) {
		return lfunction_create(lemon, name, self, lcoroutine_resume);
	}

	if (strcmp(cstr, "transfer") == 0) {
		return lfunction_create(lemon, name, self, lcoroutine_transfer);
	}

	if (strcmp(cstr, "current") == 0) {
		return lfunction_create(lemon, name, self, lcoroutine_current);
	}

	return lemon->l_nil;
}

static struct lobject *
lcoroutine_mark(struct lemon *lemon, struct lcoroutine *self)
{
	int i;

	lobject_mark(lemon, (struct lobject *)self->frame);
	for (i = 0; i < self->stacklen; i++) {
		lobject_mark(lemon, self->stack[i]);
	}

	return NULL;
}

static struct lobject *
lcoroutine_string(struct lemon *lemon, struct lobject *self)
{
	char buffer[256];

	snprintf(buffer, sizeof(buffer), "<coroutine %p>", (void *)self);
	buffer[sizeof(buffer) - 1] = '\0';

	return lstring_create(lemon, buffer, strlen(buffer));
}

static struct lobject *
lcoroutine_destroy(struct lemon *lemon, struct lcoroutine *self)
{
	lemon_allocator_free(lemon, self->stack);

	return NULL;
}

static struct lobject *
lcoroutine_method(struct lemon *lemon,
                  struct lobject *self,
                  int method, int argc, struct lobject *argv[])
{
	switch (method) {
	case LOBJECT_METHOD_GET_ATTR:
		return lcoroutine_get_attr(lemon, self, argv[0]);

	case LOBJECT_METHOD_MARK:
		return lcoroutine_mark(lemon, (struct lcoroutine *)self);

	case LOBJECT_METHOD_STRING:
		return lcoroutine_string(lemon, self);

	case LOBJECT_METHOD_DESTROY:
		return lcoroutine_destroy(lemon, (struct lcoroutine *)self);

	default:
		return lobject_default(lemon, self, method, argc, argv);
	}
}

void *
lcoroutine_create(struct lemon *lemon, struct lframe *frame)
{
	struct lcoroutine *self;

	self = lobject_create(lemon, sizeof(*self), lcoroutine_method);
	if (self) {
		self->frame = frame;
	}

	return self;
}

static struct lobject *
lcoroutine_yield(struct lemon *lemon,
                 struct lobject *self,
                 int argc, struct lobject *argv[])
{
	int i;
	size_t size;
	struct lframe *frame;
	struct lcoroutine *coroutine;

	/* get caller's frame */
	frame = lemon_machine_get_frame(lemon, lemon_machine_get_fp(lemon) - 1);

	if (lobject_is_coroutine(lemon, frame->callee)) {
		coroutine = (struct lcoroutine *)frame->callee;
	} else {
		coroutine = lcoroutine_create(lemon, frame);
		frame->callee = (struct lobject *)coroutine;
	}
	coroutine->address = lemon_machine_get_pc(lemon);
	coroutine->finished = 0;

	if (lemon_machine_get_sp(lemon) - frame->sp > 0) {
		coroutine->stacklen = lemon_machine_get_sp(lemon) - frame->sp;
		if (coroutine->stack) {
			lemon_allocator_free(lemon, coroutine->stack);
		}
		size = sizeof(struct lobject *) * coroutine->stacklen;
		coroutine->stack = lemon_allocator_alloc(lemon, size);
		if (!coroutine->stack) {
			return NULL;
		}
		memset(coroutine->stack, 0, size);
		for (i = 0; i < coroutine->stacklen; i++) {
			coroutine->stack[i] = lemon_machine_pop_object(lemon);
		}
	}

	if (argc) {
		coroutine->current = argv[0];
	} else {
		coroutine->current = lemon->l_nil;
	}

	lemon_machine_pop_frame(lemon); /* pop yield's frame */
	lemon_machine_pop_frame(lemon); /* pop caller's frame */
	lemon_machine_restore_frame(lemon, frame);

	/* we're already pop out yield's frame, manual push return value */
	lemon_machine_push_object(lemon, (struct lobject *)coroutine);

	return lemon->l_nil;
}

struct ltype *
lcoroutine_type_create(struct lemon *lemon)
{
	char *cstr;
	struct ltype *type;
	struct lobject *name;
	struct lobject *function;

	type = ltype_create(lemon, "coroutine", lcoroutine_method, NULL);
	if (type) {
		lemon_add_global(lemon, "coroutine", type);
	}

	cstr = "yield";
	name = lstring_create(lemon, cstr, strlen(cstr));
	function = lfunction_create(lemon, name, NULL, lcoroutine_yield);
	lemon_add_global(lemon, cstr, function);

	return type;
}
