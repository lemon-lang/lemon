#include "lemon.h"
#include "lvarg.h"

static struct lobject *
lvarg_eq(struct lemon *lemon, struct lvarg *a, struct lvarg *b)
{
	return lobject_eq(lemon, a->arguments, b->arguments);
}

static struct lobject *
lvarg_mark(struct lemon *lemon, struct lvarg *self)
{
	lobject_mark(lemon, self->arguments);

	return NULL;
}

static struct lobject *
lvarg_string(struct lemon *lemon, struct lvarg *self)
{
	return lobject_string(lemon, self->arguments);
}

static struct lobject *
lvarg_method(struct lemon *lemon,
             struct lobject *self,
             int method, int argc, struct lobject *argv[])
{
#define cast(a) ((struct lvarg *)(a))

	switch (method) {
	case LOBJECT_METHOD_EQ:
		return lvarg_eq(lemon, cast(self), cast(argv[0]));

	case LOBJECT_METHOD_MARK:
		return lvarg_mark(lemon, cast(self));

	case LOBJECT_METHOD_STRING:
		return lvarg_string(lemon, cast(self));

	default:
		return lobject_default(lemon, self, method, argc, argv);
	}
}

void *
lvarg_create(struct lemon *lemon, struct lobject *arguments)
{
	struct lvarg *self;

	self = lobject_create(lemon, sizeof(*self), lvarg_method);
	if (self) {
		self->arguments = arguments;
	}

	return self;
}

struct ltype *
lvarg_type_create(struct lemon *lemon)
{
	return ltype_create(lemon, "varg", lvarg_method, NULL);
}
