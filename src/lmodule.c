#include "lemon.h"
#include "ltable.h"
#include "lmodule.h"
#include "lstring.h"
#include "linteger.h"
#include "laccessor.h"

#include <stdio.h>
#include <string.h>

static struct lobject *
lmodule_get_attr(struct lemon *lemon,
                 struct lmodule *self,
                 struct lobject *name)
{
	int i;
	struct lobject *local;

	local = lobject_get_item(lemon, self->attr, name);
	if (!local) {
		return NULL;
	}

	if (self->frame) {
		i = (int)linteger_to_long(lemon, local);

		return lframe_get_item(lemon, self->frame, i);
	}

	return local;
}

static struct lobject *
lmodule_set_attr(struct lemon *lemon,
                 struct lmodule *self,
                 struct lobject *name,
                 struct lobject *value)
{
	int i;
	struct lobject *local;

	if (self->frame) {
		local = lobject_get_item(lemon, self->attr, name);
		if (!local) {
			const char *fmt;

			fmt = "'%@' has no attribute '%@'";
			return lobject_error_attribute(lemon, fmt, self, name);
		}

		i = (int)linteger_to_long(lemon, local);
		return lframe_set_item(lemon, self->frame, i, value);
	}

	return lobject_set_item(lemon, self->attr, name, value);
}

static struct lobject *
lmodule_del_attr(struct lemon *lemon,
                 struct lmodule *self,
                 struct lobject *name)
{
	return lobject_del_item(lemon, self->attr, name);
}

static struct lobject *
lmodule_get_setter(struct lemon *lemon,
                   struct lmodule *self,
                   struct lobject *name)
{
	return lobject_get_item(lemon, self->setter, name);
}

static struct lobject *
lmodule_set_setter(struct lemon *lemon,
                   struct lmodule *self,
                   int argc, struct lobject *argv[])
{
	struct lobject *setter;

	setter = laccessor_create(lemon, argc - 1, &argv[1]);
	if (!setter) {
		return NULL;
	}

	return lobject_set_item(lemon, self->setter, argv[0], setter);
}

static struct lobject *
lmodule_get_getter(struct lemon *lemon,
                   struct lmodule *self,
                   struct lobject *name)
{
	return lobject_get_item(lemon, self->getter, name);
}

static struct lobject *
lmodule_set_getter(struct lemon *lemon,
                   struct lmodule *self,
                   int argc, struct lobject *argv[])
{
	struct lobject *getter;

	getter = laccessor_create(lemon, argc - 1, &argv[1]);
	if (!getter) {
		return NULL;
	}

	return lobject_set_item(lemon, self->getter, argv[0], getter);
}

static struct lobject *
lmodule_mark(struct lemon *lemon, struct lmodule *self)
{
	lobject_mark(lemon, self->name);
	lobject_mark(lemon, self->attr);
	lobject_mark(lemon, self->getter);
	lobject_mark(lemon, self->setter);

	if (self->frame) {
		lobject_mark(lemon, (struct lobject *)self->frame);
	}

	return NULL;
}

static struct lobject *
lmodule_string(struct lemon *lemon, struct lmodule *self)
{
	char buffer[256];

	snprintf(buffer,
	         sizeof(buffer),
	         "<module '%s'>",
	         lstring_to_cstr(lemon, self->name));

	return lstring_create(lemon, buffer, strnlen(buffer, sizeof(buffer)));
}

static struct lobject *
lmodule_method(struct lemon *lemon,
               struct lobject *self,
               int method, int argc, struct lobject *argv[])
{
#define cast(a) ((struct lmodule *)(a))

	switch (method) {
	case LOBJECT_METHOD_GET_ATTR:
		return lmodule_get_attr(lemon, cast(self), argv[0]);

	case LOBJECT_METHOD_SET_ATTR:
		return lmodule_set_attr(lemon, cast(self), argv[0], argv[1]);

	case LOBJECT_METHOD_DEL_ATTR:
		return lmodule_del_attr(lemon, cast(self), argv[0]);

	case LOBJECT_METHOD_GET_SETTER:
		return lmodule_get_setter(lemon, cast(self), argv[0]);

	case LOBJECT_METHOD_SET_SETTER:
		return lmodule_set_setter(lemon, cast(self), argc, argv);

	case LOBJECT_METHOD_GET_GETTER:
		return lmodule_get_getter(lemon, cast(self), argv[0]);

	case LOBJECT_METHOD_SET_GETTER:
		return lmodule_set_getter(lemon, cast(self), argc, argv);

	case LOBJECT_METHOD_MARK:
		return lmodule_mark(lemon, cast(self));

	case LOBJECT_METHOD_STRING:
		return lmodule_string(lemon, cast(self));

	default:
		return lobject_default(lemon, self, method, argc, argv);
	}
}

void *
lmodule_create(struct lemon *lemon, struct lobject *name)
{
	struct lmodule *self;

	self = lobject_create(lemon, sizeof(*self), lmodule_method);
	if (self) {
		self->name = name;
		self->attr = ltable_create(lemon);
		if (!self->attr) {
			return NULL;
		}
		self->getter = ltable_create(lemon);
		if (!self->getter) {
			return NULL;
		}
		self->setter = ltable_create(lemon);
		if (!self->setter) {
			return NULL;
		}
	}

	return self;
}

struct ltype *
lmodule_type_create(struct lemon *lemon)
{
	return ltype_create(lemon, "module", lmodule_method, NULL);
}
