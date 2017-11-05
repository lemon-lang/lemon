#include "lemon.h"
#include "table.h"
#include "lstring.h"

#include <stdio.h>
#include <string.h>

static struct lobject *
ltype_string(struct lemon *lemon, struct ltype *self)
{
	char buffer[64];

	if (self->name) {
		snprintf(buffer, sizeof(buffer), "<type %s>", self->name);
	} else {
		snprintf(buffer,
		         sizeof(buffer),
		         "<type %p>",
		         (void *)(uintptr_t)self->method);
	}
	buffer[sizeof(buffer) - 1] = '\0';

	return lstring_create(lemon, buffer, strlen(buffer));
}

static struct lobject *
ltype_method(struct lemon *lemon,
             struct lobject *self,
             int method, int argc, struct lobject *argv[])
{
#define cast(a) ((struct ltype *)(a))

	struct ltype *type;

	type = (struct ltype *)self;
	switch (method) {
	case LOBJECT_METHOD_EQ:
		if (!lobject_is_type(lemon, argv[0])) {
			return lemon->l_false;
		}
		if (type->method == cast(argv[0])->method) {
			return lemon->l_true;
		}
		return lemon->l_false;

	case LOBJECT_METHOD_CALL: {
		struct lframe *frame;

		if (!type->type_method) {
			return lobject_error_not_callable(lemon, self);
		}
		frame = lemon_machine_push_new_frame(lemon,
		                                     NULL,
		                                     self,
		                                     lframe_default_callback,
		                                     0);
		if (!frame) {
			return NULL;
		}
		break;
	}

	case LOBJECT_METHOD_CALLABLE:
		if (!type->type_method) {
			return lemon->l_false;
		}
		break;

	case LOBJECT_METHOD_STRING:
		return ltype_string(lemon, type);

	case LOBJECT_METHOD_DESTROY:
		lemon_del_type(lemon, type);
		break;

	default:
		break;
	}

	if (!type->type_method) {
		return lobject_default(lemon, self, method, argc, argv);
	}
	return type->type_method(lemon, self, method, argc, argv);
}

void *
ltype_create(struct lemon *lemon,
             const char *name,
             lobject_method_t method,
             lobject_method_t type_method)
{
	struct ltype *self;

	self = lobject_create(lemon, sizeof(*self), ltype_method);
	if (self) {
		self->name = name;
		self->method = method;
		self->type_method = type_method;
		if (!lemon_add_type(lemon, self)) {
			return NULL;
		}
	}

	return self;
}

static struct lobject *
ltype_type_method(struct lemon *lemon,
                  struct lobject *self,
                  int method, int argc, struct lobject *argv[])
{
	switch (method) {
	case LOBJECT_METHOD_CALL: {
		if (!argc) {
			return lemon->l_nil;
		}

		if (lobject_is_integer(lemon, argv[0])) {
			return (struct lobject *)lemon->l_integer_type;
		}

		return lemon_get_type(lemon, argv[0]->l_method);
	}

	case LOBJECT_METHOD_CALLABLE:
		return lemon->l_true;

	default:
		return lobject_default(lemon, self, method, argc, argv);
	}
}

struct ltype *
ltype_type_create(struct lemon *lemon)
{
	struct ltype *type;

	type = ltype_create(lemon, "type", ltype_method, ltype_type_method);
	if (type) {
		lemon_add_global(lemon, "type", type);
	}

	return type;
}
