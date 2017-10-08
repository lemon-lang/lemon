#include "lemon.h"
#include "larray.h"
#include "lsuper.h"
#include "lclass.h"
#include "lstring.h"
#include "linteger.h"
#include "linstance.h"

#include <stdio.h>
#include <string.h>

static struct lobject *
lsuper_call(struct lemon *lemon,
            struct lsuper *self,
            int argc, struct lobject *argv[])
{
	int i;
	struct lclass *clazz;
	struct lobject *base;
	struct lobject *item;
	struct lobject *zuper;

	base = NULL;
	clazz = ((struct linstance *)self->self)->clazz;
	if (argc) {
		zuper = argv[0];

		for (i = 0; i < larray_length(lemon, clazz->bases); i++) {
			item = larray_get_item(lemon, clazz->bases, i);
			if (lobject_is_equal(lemon, zuper, item)) {
				base = item;

				break;
			}
		}
		if (!base) {
			return lobject_error_type(lemon,
			                          "'%@' is not subclass of %@",
			                          self,
			                          zuper);
		}
	}
	self->base = base;
	lemon_machine_push_object(lemon, (struct lobject *)self);

	return lemon->l_nil;
}

static struct lobject *
lsuper_get_attr(struct lemon *lemon,
                struct lsuper *self,
                struct lobject *name)
{
	int i;
	struct lclass *clazz;
	struct lobject *base;
	struct lobject *value;
	struct linstance *instance;

	value = NULL;
	instance = (struct linstance *)self->self;
	if (self->base) {
		base = self->base;
		if (!lobject_is_class(lemon, base)) {
			base = instance->native;
		}
		value = lobject_get_attr(lemon, base, name);
	} else {
		base = NULL;
		clazz = ((struct linstance *)self->self)->clazz;
		for (i = 0; i < larray_length(lemon, clazz->bases); i++) {
			base = larray_get_item(lemon, clazz->bases, i);
			if (!lobject_is_class(lemon, base)) {
				base = instance->native;
			}
			value = lobject_get_attr(lemon, base, name);
			if (value) {
				break;
			}
		}
	}

	if (value && lobject_is_class(lemon, base)) {
		value = lfunction_bind(lemon, value, self->self);
	}

	return value;
}

static struct lobject *
lsuper_string(struct lemon *lemon, struct lsuper *self)
{
	char buffer[9];

	snprintf(buffer, sizeof(buffer), "<super>");

	return lstring_create(lemon, buffer, strnlen(buffer, sizeof(buffer)));
}

static struct lobject *
lsuper_method(struct lemon *lemon,
                   struct lobject *self,
                   int method, int argc, struct lobject *argv[])
{
#define cast(a) ((struct lsuper *)(a))

	switch (method) {
	case LOBJECT_METHOD_GET_ATTR:
		return lsuper_get_attr(lemon, cast(self), argv[0]);

	case LOBJECT_METHOD_CALL:
		if (!cast(self)->base) {
			return lsuper_call(lemon, cast(self), argc, argv);
		}
		return NULL;

	case LOBJECT_METHOD_CALLABLE:
		if (!cast(self)->base) {
			return lemon->l_true;
		}
		return lemon->l_false;

	case LOBJECT_METHOD_STRING:
		return lsuper_string(lemon, cast(self));

	default:
		return lobject_default(lemon, self, method, argc, argv);
	}
}

void *
lsuper_create(struct lemon *lemon, struct lobject *binding)
{
	struct lsuper *self;

	self = lobject_create(lemon, sizeof(*self), lsuper_method);
	if (self) {
		self->self = binding;
	}

	return self;
}

struct ltype *
lsuper_type_create(struct lemon *lemon)
{
	return ltype_create(lemon, "super", lsuper_method, NULL);
}
