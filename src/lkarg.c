#include "lemon.h"
#include "lkarg.h"
#include "lstring.h"

#include <stdio.h>
#include <string.h>

static struct lobject *
lkarg_eq(struct lemon *lemon, struct lkarg *a, struct lkarg *b)
{
	if (lobject_is_equal(lemon, a->keyword, b->keyword) &&
	    lobject_is_equal(lemon, a->keyword, b->keyword))
	{
		return lemon->l_true;
	}
	return lemon->l_false;
}

static struct lobject *
lkarg_string(struct lemon *lemon, struct lkarg *self)
{
	char *buffer;
	unsigned long length;

	struct lobject *keyword;
	struct lobject *argument;

	struct lobject *string;

	keyword = lobject_string(lemon, self->keyword);
	argument = lobject_string(lemon, self->argument);

	length = 4; /* minimal length '(, )' */
	length += lstring_length(lemon, keyword);
	length += lstring_length(lemon, argument);

	buffer = lemon_allocator_alloc(lemon, length);
	if (!buffer) {
		return NULL;
	}
	snprintf(buffer,
	         length,
	         "(%s, %s)",
	         lstring_to_cstr(lemon, keyword),
	         lstring_to_cstr(lemon, argument));

	string = lstring_create(lemon, buffer, length);
	lemon_allocator_free(lemon, buffer);

	return string;
}

static struct lobject *
lkarg_mark(struct lemon *lemon, struct lkarg *self)
{
	lobject_mark(lemon, self->keyword);
	lobject_mark(lemon, self->argument);

	return NULL;
}

static struct lobject *
lkarg_method(struct lemon *lemon,
             struct lobject *self,
             int method, int argc, struct lobject *argv[])
{
#define cast(a) ((struct lkarg *)(a))

	switch (method) {
	case LOBJECT_METHOD_EQ:
		return lkarg_eq(lemon, cast(self), cast(argv[0]));

	case LOBJECT_METHOD_MARK:
		return lkarg_mark(lemon, cast(self));

	case LOBJECT_METHOD_STRING:
		return lkarg_string(lemon, cast(self));

	default:
		return lobject_default(lemon, self, method, argc, argv);
	}
}

void *
lkarg_create(struct lemon *lemon,
             struct lobject *keyword,
             struct lobject *argument)
{
	struct lkarg *self;

	self = lobject_create(lemon, sizeof(*self), lkarg_method);
	if (self) {
		self->keyword = keyword;
		self->argument = argument;
	}

	return self;
}

struct ltype *
lkarg_type_create(struct lemon *lemon)
{
	return ltype_create(lemon, "karg", lkarg_method, NULL);
}
