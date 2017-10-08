#include "lemon.h"
#include "lstring.h"
#include "lcontinuation.h"

#include <stdio.h>
#include <string.h>

static struct lobject *
lcontinuation_call(struct lemon *lemon,
                   struct lcontinuation *self,
                   int argc, struct lobject *argv[])
{
	int i;
	int ra;
	struct lframe *frame;

	lemon_machine_set_sp(lemon, -1);
	for (i = 0; i < self->stacklen; i++) {
		lemon_machine_push_object(lemon, self->stack[i]);
	}

	ra = lemon_machine_get_ra(lemon);
	lemon_machine_set_fp(lemon, -1);
	for (i = 0; i < self->framelen; i++) {
		frame = self->frame[i];
		lemon_machine_push_frame(lemon, frame);
	}
	lemon_machine_set_ra(lemon, ra);

	frame = lemon_machine_pop_frame(lemon);
	lemon_machine_restore_frame(lemon, frame);
	if (argc) {
		lemon_machine_push_object(lemon, argv[0]);
	} else {
		lemon_machine_push_object(lemon, lemon->l_nil);
	}
	lemon_machine_set_pc(lemon, self->address);

	return lemon->l_nil;
}

static struct lobject *
lcontinuation_mark(struct lemon *lemon, struct lcontinuation *self)
{
	int i;

	if (self->value) {
		lobject_mark(lemon, self->value);
	}

	for (i = 0; i < self->framelen; i++) {
		lobject_mark(lemon, (struct lobject *)self->frame[i]);
	}

	for (i = 0; i < self->stacklen; i++) {
		lobject_mark(lemon, self->stack[i]);
	}

	return 0;
}

static struct lobject *
lcontinuation_string(struct lemon *lemon, struct lcontinuation *self)
{
	char buffer[64];

	snprintf(buffer, sizeof(buffer), "<continuation %p>", (void *)self);

	return lstring_create(lemon, buffer, strnlen(buffer, sizeof(buffer)));
}

static struct lobject *
lcontinuation_destroy(struct lemon *lemon, struct lcontinuation *self)
{
	lemon_allocator_free(lemon, self->stack);
	lemon_allocator_free(lemon, self->frame);

	return NULL;
}

static struct lobject *
lcontinuation_method(struct lemon *lemon,
                     struct lobject *self,
                     int method, int argc, struct lobject *argv[])
{
#define cast(a) ((struct lcontinuation *)(a))

	switch (method) {
	case LOBJECT_METHOD_CALL:
		return lcontinuation_call(lemon, cast(self), argc, argv);

	case LOBJECT_METHOD_CALLABLE:
		return lemon->l_true;

	case LOBJECT_METHOD_STRING:
		return lcontinuation_string(lemon, cast(self));

	case LOBJECT_METHOD_MARK:
		return lcontinuation_mark(lemon, cast(self));

	case LOBJECT_METHOD_DESTROY:
		return lcontinuation_destroy(lemon, cast(self));

	default:
		return lobject_default(lemon, self, method, argc, argv);
	}
}

void *
lcontinuation_create(struct lemon *lemon)
{
	struct lcontinuation *self;

	self = lobject_create(lemon, sizeof(*self), lcontinuation_method);

	return self;
}

static struct lobject *
lcontinuation_callcc(struct lemon *lemon,
                     struct lobject *self,
                     int argc, struct lobject *argv[])
{
	int i;
	int framelen;
	int stacklen;
	size_t size;
	struct lobject *function;
	struct lcontinuation *continuation;

	continuation = lcontinuation_create(lemon);
	if (!continuation) {
		return NULL;
	}
	continuation->address = lemon_machine_get_pc(lemon);

	framelen = lemon_machine_get_fp(lemon) + 1;

	size = sizeof(struct lframe *) * framelen;
	continuation->frame = lemon_allocator_alloc(lemon, size);
	if (!continuation->frame) {
		return NULL;
	}
	for (i = 0; i < framelen; i++) {
		continuation->frame[i] = lemon_machine_get_frame(lemon, i);
	}
	continuation->framelen = framelen;

	stacklen = lemon_machine_get_sp(lemon) + 1;
	size = sizeof(struct lobject *) * stacklen;
	continuation->stack = lemon_allocator_alloc(lemon, size);
	if (!continuation->stack) {
		return NULL;
	}
	for (i = 0; i < stacklen; i++) {
		continuation->stack[i] = lemon_machine_get_stack(lemon, i);
	}
	continuation->stacklen = stacklen;

	if (argc) {
		struct lobject *value[1];

		function = argv[0];
		value[0] = (struct lobject *)continuation;
		lobject_call(lemon, function, 1, value);
	}

	return lemon->l_nil;
}

struct ltype *
lcontinuation_type_create(struct lemon *lemon)
{
	char *cstr;
	struct ltype *type;
	struct lobject *name;
	struct lobject *function;

	type = ltype_create(lemon, "continuation", lcontinuation_method, NULL);
	if (type) {
		lemon_add_global(lemon, "continuation", type);
	}

	cstr = "calcc";
	name = lstring_create(lemon, cstr, strlen(cstr));
	function = lfunction_create(lemon, name, NULL, lcontinuation_callcc);
	lemon_add_global(lemon, cstr, function);

	return type;
}
