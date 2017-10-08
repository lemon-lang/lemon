#include "lemon.h"
#include "lstring.h"
#include "linteger.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>

struct lobject *
lframe_default_callback(struct lemon *lemon,
                        struct lframe *frame,
                        struct lobject *retval)
{
	return retval;
}

struct lobject *
lframe_get_item(struct lemon *lemon,
                struct lframe *frame,
                int local)
{
	assert(local < frame->nlocals);

	return frame->locals[local];
}

struct lobject *
lframe_set_item(struct lemon *lemon,
                struct lframe *frame,
                int local,
                struct lobject *value)
{
	assert(local < frame->nlocals);

	frame->locals[local] = value;
	lemon_collector_barrier(lemon, (struct lobject *)frame, value);

	return lemon->l_nil;
}

static struct lobject *
lframe_string(struct lemon *lemon, struct lframe *self)
{
	char buffer[256];
	unsigned long maxlen;
	const char *callee;
	struct lobject *string;

	callee = "callback";
	if (self->callee) {
		string = lobject_string(lemon, self->callee);
		callee = lstring_to_cstr(lemon, string);
	}
	maxlen = sizeof(buffer);
	snprintf(buffer, maxlen, "<frame '%s': %p>", callee, (void *)self);

	return lstring_create(lemon, buffer, strnlen(buffer, maxlen));
}

static struct lobject *
lframe_mark(struct lemon *lemon, struct lframe *self)
{
	int i;

	if (self->self) {
		lobject_mark(lemon, self->self);
	}

	if (self->callee) {
		lobject_mark(lemon, self->callee);
	}

	if (self->upframe) {
		lobject_mark(lemon, (struct lobject *)self->upframe);
	}

	for (i = 0; i < self->nlocals; i++) {
		if (self->locals[i]) {
			lobject_mark(lemon, self->locals[i]);
		}
	}

	return NULL;
}

static struct lobject *
lframe_method(struct lemon *lemon,
              struct lobject *self,
              int method, int argc, struct lobject *argv[])
{
#define cast(a) ((struct lframe *)(a))

	switch (method) {
	case LOBJECT_METHOD_GET_ITEM:
		if (lobject_is_integer(lemon, argv[0])) {
			long i;

			i = linteger_to_long(lemon, argv[0]);
			return lframe_get_item(lemon, cast(self), i);
		}
		return NULL;

	case LOBJECT_METHOD_SET_ITEM:
		if (lobject_is_integer(lemon, argv[0])) {
			long i;

			i = linteger_to_long(lemon, argv[0]);
			return lframe_set_item(lemon, cast(self), i, argv[1]);
		}
		return NULL;

	case LOBJECT_METHOD_STRING:
		return lframe_string(lemon, cast(self));

	case LOBJECT_METHOD_MARK:
		return lframe_mark(lemon, cast(self));

	case LOBJECT_METHOD_DESTROY:
		return NULL;

	default:
		return lobject_default(lemon, self, method, argc, argv);
	}
}

void *
lframe_create(struct lemon *lemon,
              struct lobject *self,
              struct lobject *callee,
              lframe_call_t callback,
              int nlocals)
{
	size_t size;
	struct lframe *frame;

	size = 0;
	if (nlocals > 1) {
		size = sizeof(struct lobject *) * (nlocals - 1);
	}
	frame = lobject_create(lemon, sizeof(*frame) + size, lframe_method);
	if (frame) {
		frame->self = self;
		frame->callee = callee;
		frame->callback = callback;
		frame->nlocals = nlocals;
	}

	return frame;
}

struct ltype *
lframe_type_create(struct lemon *lemon)
{
	return ltype_create(lemon, "frame", lframe_method, NULL);
}
