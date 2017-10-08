#include "lemon.h"
#include "ltable.h"
#include "larray.h"
#include "lclass.h"
#include "lstring.h"
#include "linteger.h"
#include "linstance.h"
#include "laccessor.h"

#include <stdio.h>
#include <string.h>

static struct lobject *
lclass_get_attr(struct lemon *lemon, struct lclass *self, struct lobject *name)
{
	return lobject_get_item(lemon, self->attr, name);
}

static struct lobject *
lclass_set_attr(struct lemon *lemon,
                struct lclass *self,
                struct lobject *name,
                struct lobject *value)
{
	return lobject_set_item(lemon, self->attr, name, value);
}

static struct lobject *
lclass_del_attr(struct lemon *lemon, struct lclass *self, struct lobject *name)
{
	return lobject_del_item(lemon, self->attr, name);
}

static struct lobject *
lclass_get_setter(struct lemon *lemon,
                  struct lclass *self,
                  struct lobject *name)
{
	return lobject_get_item(lemon, self->setter, name);
}

static struct lobject *
lclass_set_setter(struct lemon *lemon,
                  struct lclass *self,
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
lclass_get_getter(struct lemon *lemon,
                  struct lclass *self,
                  struct lobject *name)
{
	return lobject_get_item(lemon, self->getter, name);
}

static struct lobject *
lclass_set_getter(struct lemon *lemon,
                  struct lclass *self,
                  int argc, struct lobject *argv[])
{
	struct lobject *getter;

	getter = laccessor_create(lemon, argc - 1, &argv[1]);
	if (!getter) {
		return NULL;
	}

	return lobject_set_item(lemon, self->getter, argv[0], getter);
}

int
lclass_check_slot(struct lemon *lemon,
                  struct lobject *base,
                  int nslots, struct lobject **slots)
{
	int i;
	int j;
	int candidate;
	struct lobject *item;

	candidate = 1;
	for (i = 0; i < nslots; i++) {
		for (j = 1; j < larray_length(lemon, slots[i]); j++) {
			item = larray_get_item(lemon, slots[i], j);
			if (lobject_is_equal(lemon, base, item)) {
				candidate = 0;
			}
		}
	}

	return candidate;
}

void
lclass_del_slot(struct lemon *lemon,
                struct lobject *base,
                int nslots, struct lobject **slots)
{
	int i;
	int j;
	struct lobject *item;

	for (i = 0; i < nslots; i++) {
		for (j = 0; j < larray_length(lemon, slots[i]); j++) {
			item = larray_get_item(lemon, slots[i], j);
			if (lobject_is_equal(lemon, base, item)) {
				larray_del_item(lemon, slots[i], j);
				j -= 1;
			}
		}
	}
}

static struct lobject *
lclass_set_supers(struct lemon *lemon,
                  struct lclass *self,
                  int nsupers, struct lobject *supers[])
{
	int i;
	int finished;
	struct lobject *zuper;
	struct lobject *bases;

	int n;
	int nonclass;
	struct lobject **slots;

	/* prepare merge slot */
	n = nsupers;
	nonclass = 0;
	slots = lemon_allocator_alloc(lemon,
	                              sizeof(struct lobject *) * nsupers);
	if (!slots) {
		return NULL;
	}
	for (i = 0; i < nsupers; i++) {
		zuper = supers[nsupers - i - 1];
		slots[i] = larray_create(lemon, 1, &zuper);
		if (!slots[i]) {
			return NULL;
		}

		if (lobject_is_class(lemon, supers[nsupers - i - 1])) {
			bases = ((struct lclass *)zuper)->bases;
			slots[i] = lobject_binop(lemon,
			                         LOBJECT_METHOD_ADD,
			                         slots[i],
			                         bases);
		} else if (nonclass) {
			lemon_allocator_free(lemon, slots);

			return lobject_error_type(lemon,
			                          "'%@' inherit conflict",
			                          self);
		} else {
			nonclass = 1;
		}
	}

	/* algorithm from https://www.python.org/download/releases/2.3/mro/ */
	for (;;) {
		struct lobject *cand;

		/* check finished */
		finished = 1;
		for (i = 0; i < n; i++) {
			if (larray_length(lemon, slots[i]) > 0) {
				finished = 0;
				break;
			}
		}

		if (finished) {
			break;
		}

		/* merge */
		cand = NULL;
		for (i = 0; i < n; i++) {
			if (larray_length(lemon, slots[i])) {
				cand = larray_get_item(lemon, slots[i], 0);
				if (lclass_check_slot(lemon, cand, n, slots)) {
					break;
				}

				cand = NULL;
			}
		}

		if (!cand) {
			lemon_allocator_free(lemon, slots);

			return lobject_error_type(lemon,
			                          "'%@' inconsistent hierarchy",
			                          self);
		}

		if (!larray_append(lemon, self->bases, 1, &cand)) {
			return NULL;
		}
		lclass_del_slot(lemon, cand, n, slots);
	}
	lemon_allocator_free(lemon, slots);

	return lemon->l_nil;
}

static struct lobject *
lclass_frame_callback(struct lemon *lemon,
                      struct lframe *frame,
                      struct lobject *retval)
{
	if (lobject_is_error(lemon, retval)) {
		return retval;
	}
	return frame->self;
}

static struct lobject *
lclass_call(struct lemon *lemon,
            struct lobject *self,
            int argc, struct lobject *argv[])
{
	int i;
	struct lframe *frame;
	struct lclass *clazz;
	struct lobject *base;
	struct lobject *native;
	struct lobject *function;
	struct linstance *instance;

	clazz = (struct lclass *)self;
	instance = linstance_create(lemon, clazz);
	if (!instance) {
		return NULL;
	}

	for (i = 0; i < larray_length(lemon, clazz->bases); i++) {
		base = larray_get_item(lemon, clazz->bases, i);
		if (!lobject_is_class(lemon, base)) {
			native = lobject_call(lemon, base, argc, argv);
			if (lobject_is_error(lemon, native)) {
				return instance->native;
			}
			instance->native = native;

			break;
		}
	}

	frame = lemon_machine_push_new_frame(lemon,
	                                     (struct lobject *)instance,
	                                     NULL,
	                                     lclass_frame_callback,
	                                     0);
	if (!frame) {
		return NULL;
	}

	function = lobject_get_attr(lemon,
	                            (struct lobject *)instance,
	                            lemon->l_init_string);
	if (function && !lobject_is_error(lemon, function)) {
		return lobject_call(lemon, function, argc, argv);
	}

	return (struct lobject *)instance;
}

static struct lobject *
lclass_mark(struct lemon *lemon, struct lclass *self)
{
	lobject_mark(lemon, self->name);
	lobject_mark(lemon, self->bases);
	lobject_mark(lemon, self->attr);
	lobject_mark(lemon, self->getter);
	lobject_mark(lemon, self->setter);

	return NULL;
}

static struct lobject *
lclass_string(struct lemon *lemon, struct lclass *self)
{
	char buffer[32];

	snprintf(buffer,
	         sizeof(buffer),
	         "<class '%s'>",
	         lstring_to_cstr(lemon, self->name));

	return lstring_create(lemon, buffer, strnlen(buffer, sizeof(buffer)));
}

static struct lobject *
lclass_method(struct lemon *lemon,
              struct lobject *self,
              int method, int argc, struct lobject *argv[])
{
#define cast(a) ((struct lclass *)(a))

	switch (method) {
	case LOBJECT_METHOD_GET_ATTR:
		return lclass_get_attr(lemon, cast(self), argv[0]);

	case LOBJECT_METHOD_SET_ATTR:
		return lclass_set_attr(lemon, cast(self), argv[0], argv[1]);

	case LOBJECT_METHOD_DEL_ATTR:
		return lclass_del_attr(lemon, cast(self), argv[0]);

	case LOBJECT_METHOD_GET_SETTER:
		return lclass_get_setter(lemon, cast(self), argv[0]);

	case LOBJECT_METHOD_SET_SETTER:
		return lclass_set_setter(lemon, cast(self), argc, argv);

	case LOBJECT_METHOD_GET_GETTER:
		return lclass_get_getter(lemon, cast(self), argv[0]);

	case LOBJECT_METHOD_SET_GETTER:
		return lclass_set_getter(lemon, cast(self), argc, argv);

	case LOBJECT_METHOD_CALL:
		return lclass_call(lemon, self, argc, argv);

	case LOBJECT_METHOD_CALLABLE:
		return lemon->l_true;

	case LOBJECT_METHOD_MARK:
		return lclass_mark(lemon, cast(self));

	case LOBJECT_METHOD_STRING:
		return lclass_string(lemon, cast(self));

	default:
		return lobject_default(lemon, self, method, argc, argv);
	}
}

void *
lclass_create(struct lemon *lemon,
              struct lobject *name,
              int nsupers,
              struct lobject *supers[],
              int nattrs,
              struct lobject *attrs[])
{
	int i;
	struct lclass *self;
	struct lobject *error;

	self = lobject_create(lemon, sizeof(*self), lclass_method);
	if (self) {
		self->name = name;
		self->bases = larray_create(lemon, 0, NULL);
		if (!self->bases) {
			return NULL;
		}
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

		error = lclass_set_supers(lemon, self, nsupers, supers);
		if (!error || lobject_is_error(lemon, error)) {
			return error;
		}

		for (i = 0; i < nattrs; i += 2) {
			if (!lobject_is_string(lemon, attrs[i])) {
				return NULL;
			}

			if (!lobject_set_item(lemon,
			                      self->attr,
			                      attrs[i],
			                      attrs[i + 1]))
			{
				return NULL;
			}
		}
	}

	return self;
}

struct ltype *
lclass_type_create(struct lemon *lemon)
{
	return ltype_create(lemon, "class", lclass_method, NULL);
}
