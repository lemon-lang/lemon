#include "lemon.h"
#include "larray.h"
#include "ltable.h"
#include "lclass.h"
#include "lstring.h"
#include "linteger.h"
#include "linstance.h"
#include "laccessor.h"

#include <stdio.h>
#include <string.h>

static struct lobject *
linstance_call(struct lemon *lemon,
               struct lobject *self,
               int argc, struct lobject *argv[])
{
	struct lobject *call;

	call = lobject_get_attr(lemon, self, lemon->l_call_string);
	if (call) {
		if (!lobject_is_exception(lemon, call)) {
			return lobject_call(lemon, call, argc, argv);
		}

		return call;
	}

	return lobject_error_not_callable(lemon, self);
}

static struct lobject *
linstance_get_callable(struct lemon *lemon,
                       struct lobject *self,
                       int argc, struct lobject *argv[])
{
	struct lobject *call;

	call = lobject_get_attr(lemon, self, lemon->l_call_string);
	if (call && !lobject_is_exception(lemon, call)) {
		return lemon->l_true;
	}

	return lemon->l_false;
}

static struct lobject *
linstance_get_attr(struct lemon *lemon,
                   struct linstance *self,
                   struct lobject *name)
{
	int i;
	long length;
	const char *cstr;
	struct lobject *base;
	struct lobject *value;

	cstr = lstring_to_cstr(lemon, name);
	if (strcmp(cstr, "__callable__") == 0) {
		return lfunction_create(lemon,
		                        name,
		                        (struct lobject *)self,
		                        linstance_get_callable);
	}

	/* search self */
	value = lobject_get_item(lemon, self->attr, name);
	if (value) {
		return value;
	}

	/* search class */
	base = (struct lobject *)self->clazz;
	value = lobject_get_attr(lemon, base, name);
	if (!value) {
		/* search class->bases */
		length = larray_length(lemon, self->clazz->bases);
		for (i = 0; i < length; i++) {
			base = larray_get_item(lemon, self->clazz->bases, i);
			if (lobject_is_class(lemon, base)) {
				value = lobject_get_attr(lemon,
				                         base,
				                         name);
			} else {
				value = lobject_get_attr(lemon,
				                         self->native,
				                         name);
			}

			if (value) {
				break;
			}
		}
	}

	if (value) {
		if (lobject_is_function(lemon, value)) {
			struct lobject *binding;
			if (base && lobject_is_class(lemon, base)) {
				/* make a self binding function */
				binding = (struct lobject *) self;
			} else {
				binding = self->native;
			}

			value = lfunction_bind(lemon, value, binding);
		}

		return value;
	}

	return NULL;
}

static struct lobject *
linstance_call_get_attr_callback(struct lemon *lemon,
                                 struct lframe *frame,
                                 struct lobject *retval)
{
	struct lobject *self;
	struct lobject *name;

	if (retval == lemon->l_sentinel) {
		self = frame->self;
		name = lframe_get_item(lemon, frame, 0);

		return lobject_error_attribute(lemon,
		                               "'%@' has no attribute '%@'",
		                               self,
		                               name);
	}

	return retval;
}

static struct lobject *
linstance_call_get_attr(struct lemon *lemon,
                        struct linstance *self,
                        struct lobject *name)
{
	const char *cstr;
	struct lframe *frame;
	struct lobject *retval;
	struct lobject *function;

	cstr = lstring_to_cstr(lemon, name);
	/*
	 * ignore name start with '__', '__xxx__' is internal use
	 * dynamic replace internal method is too complex
	 */
	if (strncmp(cstr, "__", 2) == 0) {
		return NULL;
	}

	function = linstance_get_attr(lemon, self, lemon->l_get_attr_string);
	if (!function) {
		return NULL;
	}

	frame = lemon_machine_push_new_frame(lemon,
	                                     (struct lobject *)self,
	                                     NULL,
	                                     linstance_call_get_attr_callback,
	                                     1);
	if (!frame) {
		return NULL;
	}
	lframe_set_item(lemon, frame, 0, name);

	retval = lobject_call(lemon, function, 1, &name);

	return lemon_machine_return_frame(lemon, retval);
}

static struct lobject *
linstance_set_attr(struct lemon *lemon,
                   struct linstance *self,
                   struct lobject *name,
                   struct lobject *value)
{
	return lobject_set_item(lemon, self->attr, name, value);
}

static struct lobject *
linstance_call_set_attr(struct lemon *lemon,
                        struct linstance *self,
                        struct lobject *name,
                        struct lobject *value)
{
	const char *cstr;
	struct lobject *argv[2];
	struct lobject *function;

	cstr = lstring_to_cstr(lemon, name);
	if (strncmp(cstr, "__", 2) == 0) {
		return NULL;
	}

	function = linstance_get_attr(lemon, self, lemon->l_set_attr_string);
	if (!function) {
		return NULL;
	}
	argv[0] = name;
	argv[1] = value;

	return lobject_call(lemon, function, 2, argv);
}

static struct lobject *
linstance_del_attr(struct lemon *lemon,
                   struct linstance *self,
                   struct lobject *name)
{
	return lobject_del_item(lemon, self->attr, name);
}

static struct lobject *
linstance_call_del_attr(struct lemon *lemon,
                        struct linstance *self,
                        struct lobject *name)
{
	const char *cstr;
	struct lobject *function;

	cstr = lstring_to_cstr(lemon, name);
	if (strncmp(cstr, "__", 2) == 0) {
		return NULL;
	}

	function = linstance_get_attr(lemon, self, lemon->l_del_attr_string);
	if (function) {
		return NULL;
	}
	return lobject_call(lemon, function, 1, &name);
}

static struct lobject *
linstance_has_attr_callback(struct lemon *lemon,
                            struct lframe *frame,
                            struct lobject *retval)
{
	if (retval == lemon->l_sentinel) {
		return lemon->l_false;
	}

	if (lobject_is_error(lemon, retval)) {
		return retval;
	}

	if (retval) {
		return lemon->l_true;
	}

	return lemon->l_false;
}

static struct lobject *
linstance_has_attr(struct lemon *lemon,
                   struct linstance *self,
                   struct lobject *name)
{
	int i;
	long length;
	struct lframe *frame;
	struct lobject *base;
	struct lobject *value;
	struct lobject *function;

	/* search self */
	if (lobject_get_item(lemon, self->attr, name)) {
		return lemon->l_true;
	}

	/* search class */
	base = (struct lobject *)self->clazz;
	if (lobject_get_attr(lemon, base, name)) {
		return lemon->l_true;
	}

	/* search class->bases */
	length = larray_length(lemon, self->clazz->bases);
	for (i = 0; i < length; i++) {
		base = larray_get_item(lemon, self->clazz->bases, i);
		if (lobject_is_class(lemon, base)) {
			value = lobject_get_attr(lemon, base, name);
		} else {
			value = lobject_get_attr(lemon,
			                         self->native,
			                         name);
		}

		if (value) {
			return lemon->l_true;
		}
	}

	if (self->native &&
	    lobject_has_attr(lemon, self->native, name) == lemon->l_true)
	{
		return lemon->l_true;
	}

	function = linstance_get_attr(lemon, self, lemon->l_get_attr_string);
	if (!function) {
		return lemon->l_false;
	}

	frame = lemon_machine_push_new_frame(lemon,
	                                     NULL,
	                                     NULL,
	                                     linstance_has_attr_callback,
	                                     0);
	if (!frame) {
		return NULL;
	}
	lobject_call(lemon, function, 1, &name);

	return lemon->l_nil; /* make a junk value avoid exception */
}

static struct lobject *
linstance_get_getter(struct lemon *lemon,
                     struct linstance *self,
                     struct lobject *name)
{
	struct lobject *getter;

	getter = lobject_get_getter(lemon,
	                            (struct lobject *)self->clazz,
	                            name);
	if (getter) {
		getter = laccessor_create(lemon,
		                          ((struct laccessor *)getter)->count,
		                          ((struct laccessor *)getter)->items);
		((struct laccessor *)getter)->self = (struct lobject *)self;

		return getter;
	}

	return NULL;
}

static struct lobject *
linstance_get_setter(struct lemon *lemon,
                     struct linstance *self,
                     struct lobject *name)
{
	struct lobject *setter;

	setter = lobject_get_setter(lemon,
	                            (struct lobject *)self->clazz,
	                            name);
	if (setter) {
		setter = laccessor_create(lemon,
		                          ((struct laccessor *)setter)->count,
		                          ((struct laccessor *)setter)->items);
		((struct laccessor *)setter)->self = (struct lobject *)self;

		return setter;
	}

	return NULL;
}

static struct lobject *
linstance_set_item(struct lemon *lemon,
                   struct linstance *self,
                   struct lobject *name,
                   struct lobject *value)
{
	struct lobject *argv[2];
	struct lobject *function;

	argv[0] = name;
	argv[1] = value;
	function = linstance_get_attr(lemon, self, lemon->l_set_item_string);
	if (function) {
		return lobject_call(lemon, function, 2, argv);
	}

	if (self->native) {
		return lobject_set_item(lemon, self->native, name, value);
	}

	return lobject_error_item(lemon,
	                          "'%@' unsupport set item",
	                          self);
}

static struct lobject *
linstance_get_item(struct lemon *lemon,
                   struct linstance *self,
                   struct lobject *name)
{
	struct lobject *argv[1];
	struct lobject *function;

	argv[0] = name;
	function = linstance_get_attr(lemon, self, lemon->l_get_item_string);
	if (function) {
		lobject_call(lemon, function, 1, argv);

		return lemon->l_nil; /* make a junk value avoid exception */
	}

	if (self->native) {
		return lobject_get_item(lemon, self->native, name);
	}

	return NULL;
}

static struct lobject *
linstance_string(struct lemon *lemon, struct linstance *self)
{
	char buffer[256];
	if (self->native) {
		return lobject_string(lemon, self->native);
	}

	snprintf(buffer,
	         sizeof(buffer),
	         "<instance of %s %p>",
	         lstring_to_cstr(lemon, self->clazz->name),
	         (void *)self);

	return lstring_create(lemon, buffer, strnlen(buffer, sizeof(buffer)));
}

static struct lobject *
linstance_mark(struct lemon *lemon, struct linstance *self)
{
	lobject_mark(lemon, self->attr);
	lobject_mark(lemon, (struct lobject *)self->clazz);

	if (self->native) {
		lobject_mark(lemon, self->native);
	}

	return NULL;
}

static struct lobject *
linstance_method(struct lemon *lemon,
                 struct lobject *self,
                 int method, int argc, struct lobject *argv[])
{
	struct lobject *attr;

#define cast(a) ((struct linstance *)(a))

	switch (method) {
	case LOBJECT_METHOD_ADD:
		attr = linstance_get_attr(lemon,
		                          cast(self),
		                          lemon->l_add_string);
		if (attr) {
			return lobject_call(lemon, attr, argc, argv);
		}
		return lobject_default(lemon, self, method, argc, argv);

	case LOBJECT_METHOD_SUB:
		attr = linstance_get_attr(lemon,
		                          cast(self),
		                          lemon->l_sub_string);
		if (attr) {
			return lobject_call(lemon, attr, argc, argv);
		}
		return lobject_default(lemon, self, method, argc, argv);

	case LOBJECT_METHOD_MUL:
		attr = linstance_get_attr(lemon,
		                          cast(self),
		                          lemon->l_mul_string);
		if (attr) {
			return lobject_call(lemon, attr, argc, argv);
		}
		return lobject_default(lemon, self, method, argc, argv);

	case LOBJECT_METHOD_DIV:
		attr = linstance_get_attr(lemon,
		                          cast(self),
		                          lemon->l_div_string);
		if (attr) {
			return lobject_call(lemon, attr, argc, argv);
		}
		return lobject_default(lemon, self, method, argc, argv);

	case LOBJECT_METHOD_MOD:
		attr = linstance_get_attr(lemon,
		                          cast(self),
		                          lemon->l_mod_string);
		if (attr) {
			return lobject_call(lemon, attr, argc, argv);
		}
		return lobject_default(lemon, self, method, argc, argv);

	case LOBJECT_METHOD_SET_ITEM:
		return linstance_set_item(lemon, cast(self), argv[0], argv[1]);

	case LOBJECT_METHOD_GET_ITEM:
		return linstance_get_item(lemon, cast(self), argv[0]);

	case LOBJECT_METHOD_GET_ATTR: {
		struct lobject *value;

		value = linstance_get_attr(lemon, cast(self), argv[0]);
		if (value) {
			return value;
		}
		return linstance_call_get_attr(lemon, cast(self), argv[0]);
	}

	case LOBJECT_METHOD_SET_ATTR: {
		struct lobject *value;

		value = linstance_set_attr(lemon, cast(self), argv[0], argv[1]);
		if (value) {
			return value;
		}
		return linstance_call_set_attr(lemon,
		                               cast(self),
		                               argv[0],
		                               argv[1]);
	}

	case LOBJECT_METHOD_DEL_ATTR: {
		struct lobject *value;

		value = linstance_del_attr(lemon, cast(self), argv[0]);
		if (value) {
			return value;
		}
		return linstance_call_del_attr(lemon, cast(self), argv[0]);
	}

	case LOBJECT_METHOD_HAS_ATTR:
		return linstance_has_attr(lemon, cast(self), argv[0]);

	case LOBJECT_METHOD_GET_GETTER:
		return linstance_get_getter(lemon, cast(self), argv[0]);

	case LOBJECT_METHOD_GET_SETTER:
		return linstance_get_setter(lemon, cast(self), argv[0]);

	case LOBJECT_METHOD_STRING:
		return linstance_string(lemon, cast(self));

	case LOBJECT_METHOD_CALL:
		return linstance_call(lemon, self, argc, argv);

	case LOBJECT_METHOD_MARK:
		return linstance_mark(lemon, cast(self));

	default:
		return lobject_default(lemon, self, method, argc, argv);
	}
}

void *
linstance_create(struct lemon *lemon, struct lclass *clazz)
{
	struct linstance *self;

	self = lobject_create(lemon, sizeof(*self), linstance_method);
	if (self) {
		self->attr = ltable_create(lemon);
		self->clazz = clazz;
	}

	return self;
}

struct ltype *
linstance_type_create(struct lemon *lemon)
{
	return ltype_create(lemon, "instance", linstance_method, NULL);
}
