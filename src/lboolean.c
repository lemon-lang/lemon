#include "lemon.h"
#include "lstring.h"
#include "lboolean.h"

static struct lobject *
lboolean_eq(struct lemon *lemon, struct lobject *a, struct lobject *b)
{
	if (a == b) {
		return lemon->l_true;
	}
	return lemon->l_false;
}

static struct lobject *
lboolean_string(struct lemon *lemon, struct lobject *boolean)
{
	if (boolean == lemon->l_true) {
		return lstring_create(lemon, "true", 4);
	}

	return lstring_create(lemon, "false", 5);
}

static struct lobject *
lboolean_method(struct lemon *lemon,
                struct lobject *self,
                int method, int argc, struct lobject *argv[])
{
	switch (method) {
	case LOBJECT_METHOD_EQ:
		return lboolean_eq(lemon, self, argv[0]);

	case LOBJECT_METHOD_STRING:
		return lboolean_string(lemon, self);

	default:
		return lobject_default(lemon, self, method, argc, argv);
	}
}

void *
lboolean_create(struct lemon *lemon, int value)
{
	struct lboolean *self;

	self = lobject_create(lemon, sizeof(*self), lboolean_method);

	return self;
}

static struct lobject *
lboolean_type_method(struct lemon *lemon,
                     struct lobject *self,
                     int method, int argc, struct lobject *argv[])
{
	switch (method) {
	case LOBJECT_METHOD_CALL:
		if (argv) {
			return lobject_boolean(lemon, argv[0]);
		}

	case LOBJECT_METHOD_CALLABLE:
		return lemon->l_true;

	default:
		return lobject_default(lemon, self, method, argc, argv);
	}
}

struct ltype *
lboolean_type_create(struct lemon *lemon)
{
	struct ltype *type;

	type = ltype_create(lemon,
	                    "boolean",
	                    lboolean_method,
	                    lboolean_type_method);
	if (type) {
		lemon_add_global(lemon, "boolean", type);
	}

	return type;
}
