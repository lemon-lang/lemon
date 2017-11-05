#include "lemon.h"
#include "lstring.h"
#include "linteger.h"

#include <stdio.h>
#include <string.h>

static struct lobject *
lfunction_string(struct lemon *lemon, struct lfunction *self)
{
	char buffer[64];

	snprintf(buffer,
	         sizeof(buffer),
	         "<function '%s'>",
	         lstring_to_cstr(lemon, self->name));
	buffer[sizeof(buffer) - 1] = '\0';

	return lstring_create(lemon, buffer, strlen(buffer));
}

static struct lobject *
lfunction_mark(struct lemon *lemon, struct lfunction *self)
{
	int i;

	lobject_mark(lemon, self->name);

	if (self->self) {
		lobject_mark(lemon, self->self);
	}

	/* C function doesn't have frame */
	if (self->frame) {
		lobject_mark(lemon, (struct lobject *)self->frame);
	}

	for (i = 0; i < self->nparams; i++) {
		lobject_mark(lemon, self->params[i]);
	}

	return NULL;
}

static struct lobject *
lfunction_call(struct lemon *lemon,
               struct lfunction *self,
               int argc, struct lobject *argv[])
{
	struct lframe *frame;
	struct lobject *retval;
	struct lobject *exception;

	retval = lemon->l_nil;
	if (self->address > 0) {
		frame = lemon_machine_push_new_frame(lemon,
		                                     self->self,
		                                     (struct lobject *)self,
		                                     NULL,
		                                     self->nlocals);
		if (!frame) {
			return NULL;
		}
		frame->upframe = self->frame;

		exception = lemon_machine_parse_args(lemon,
		                                     (struct lobject *)self,
		                                     frame,
		                                     self->define,
		                                     self->nvalues,
		                                     self->nparams,
		                                     self->params,
		                                     argc,
		                                     argv);

		if (exception) {
			return exception;
		}
		lemon_machine_set_pc(lemon, self->address);
	} else {
		frame = lemon_machine_push_new_frame(lemon,
		                                     NULL,
		                                     (struct lobject *)self,
		                                     lframe_default_callback,
		                                     0);
		if (!frame) {
			return NULL;
		}
		retval = self->callback(lemon, self->self, argc, argv);
	}

	return retval;
}

static struct lobject *
lfunction_method(struct lemon *lemon,
                 struct lobject *self,
                 int method, int argc, struct lobject *argv[])
{
#define cast(a) ((struct lfunction *)(a))

	switch (method) {
	case LOBJECT_METHOD_CALL:
		return lfunction_call(lemon, cast(self), argc, argv);

	case LOBJECT_METHOD_CALLABLE:
		return lemon->l_true;

	case LOBJECT_METHOD_STRING:
		return lfunction_string(lemon, cast(self));

	case LOBJECT_METHOD_MARK:
		return lfunction_mark(lemon, cast(self));

	case LOBJECT_METHOD_DESTROY:
		return NULL;

	default:
		return lobject_default(lemon, self, method, argc, argv);
	}
}

void *
lfunction_create(struct lemon *lemon,
                 struct lobject *name,
                 struct lobject *self,
                 lfunction_call_t callback)
{
	struct lfunction *function;

	function = lobject_create(lemon, sizeof(*function), lfunction_method);
	if (function) {
		function->name = name;
		function->self = self;
		function->callback = callback;
	}

	return function;
}

void *
lfunction_create_with_address(struct lemon *lemon,
                              struct lobject *name,
                              int define,
                              int nlocals,
                              int nparams,
                              int nvalues,
                              int address,
                              struct lobject *params[])
{
	size_t size;
	struct lfunction *self;

	size = 0;
	if (nparams > 0) {
		size = sizeof(struct lobject *) * (nparams - 1);
	}
	self = lobject_create(lemon, sizeof(*self) + size, lfunction_method);
	if (self) {
		self->name = name;
		self->define = define;
		self->nlocals = nlocals;
		self->nparams = nparams;
		self->nvalues = nvalues;
		self->address = address;
		size = sizeof(struct lobject *) * nparams;
		memcpy(self->params, params, size);
	}

	return self;
}

void *
lfunction_bind(struct lemon *lemon,
               struct lobject *function,
               struct lobject *self)
{
	struct lfunction *oldfunction;
	struct lfunction *newfunction;

	oldfunction = (struct lfunction *)function;
	if (oldfunction->callback) {
		newfunction = lfunction_create(lemon,
		                               oldfunction->name,
		                               self,
		                               oldfunction->callback);
		return newfunction;
	}

	newfunction = lfunction_create_with_address(lemon,
						    oldfunction->name,
						    oldfunction->define,
						    oldfunction->nlocals,
						    oldfunction->nparams,
						    oldfunction->nvalues,
						    oldfunction->address,
						    oldfunction->params);
	if (newfunction) {
		newfunction->self = self;
		newfunction->frame = oldfunction->frame;
	}

	return newfunction;
}

struct ltype *
lfunction_type_create(struct lemon *lemon)
{
	struct ltype *type;

	type = ltype_create(lemon, "function", lfunction_method, NULL);
	lemon_add_global(lemon, "func", type);

	return type;
}
