#include "lemon.h"
#include "lvkarg.h"

static struct lobject *
lvkarg_eq(struct lemon *lemon, struct lvkarg *a, struct lvkarg *b)
{
	return lobject_eq(lemon, a->arguments, b->arguments);
}

static struct lobject *
lvkarg_map_item(struct lemon *lemon, struct lvkarg *self)
{
	return lobject_map_item(lemon, self->arguments);
}

static struct lobject *
lvkarg_mark(struct lemon *lemon, struct lvkarg *self)
{
	lobject_mark(lemon, self->arguments);

	return NULL;
}

static struct lobject *
lvkarg_string(struct lemon *lemon, struct lvkarg *self)
{
	return lobject_string(lemon, self->arguments);
}

static struct lobject *
lvkarg_method(struct lemon *lemon,
              struct lobject *self,
              int method, int argc, struct lobject *argv[])
{
#define cast(a) ((struct lvkarg *)(a))

	switch (method) {
	case LOBJECT_METHOD_EQ:
		return lvkarg_eq(lemon, cast(self), cast(argv[0]));

	case LOBJECT_METHOD_MAP_ITEM:
		return lvkarg_map_item(lemon, cast(self));

	case LOBJECT_METHOD_MARK:
		return lvkarg_mark(lemon, cast(self));

	case LOBJECT_METHOD_STRING:
		return lvkarg_string(lemon, cast(self));

	default:
		return lobject_default(lemon, self, method, argc, argv);
	}
}

void *
lvkarg_create(struct lemon *lemon, struct lobject *arguments)
{
	struct lvkarg *self;

	self = lobject_create(lemon, sizeof(*self), lvkarg_method);
	if (self) {
		self->arguments = arguments;
	}

	return self;
}

struct ltype *
lvkarg_type_create(struct lemon *lemon)
{
	return ltype_create(lemon, "vkarg", lvkarg_method, NULL);
}
