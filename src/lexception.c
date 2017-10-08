#include "lemon.h"
#include "lclass.h"
#include "lstring.h"
#include "lexception.h"

#include <stdio.h>
#include <string.h>

static struct lobject *
lexception_addtrace_attr(struct lemon *lemon,
                         struct lobject *self,
                         int argc, struct lobject *argv[])
{
	int i;
	struct lexception *exception;

	exception = (struct lexception *)self;
	for (i = 0; i < argc; i++) {
		exception->frame[exception->nframe++] = argv[i];
	}

	return lemon->l_nil;
}

static struct lobject *
lexception_traceback_attr(struct lemon *lemon,
                          struct lobject *self,
                          int argc, struct lobject *argv[])
{
	int i;
	const char *cstr;
	struct lframe *frame;
	struct lobject *string;
	struct lexception *exception;

	exception = (struct lexception *)self;
	for (i = 0; i < exception->nframe; i++) {
		printf("  at ");
		frame = (struct lframe *)exception->frame[i];
		cstr = "";
		if (frame->callee) {
			string = lobject_string(lemon, frame->callee);
			cstr = lstring_to_cstr(lemon, string);
			printf("%s\n", cstr);
		} else {
			if (frame->self) {
				string = lobject_string(lemon, frame->self);
				cstr = lstring_to_cstr(lemon, string);
			}
			printf("<callback '%s'>\n", cstr);
		}
	}

	return lemon->l_nil;
}

static struct lobject *
lexception_get_attr(struct lemon *lemon,
                    struct lobject *self,
                    struct lobject *name)
{
	const char *cstr;

	cstr = lstring_to_cstr(lemon, name);
	if (strcmp(cstr, "traceback") == 0) {
		return lfunction_create(lemon,
		                        name,
		                        self,
		                        lexception_traceback_attr);
	}
	if (strcmp(cstr, "addtrace") == 0) {
		return lfunction_create(lemon,
		                        name,
		                        self,
		                        lexception_addtrace_attr);
	}

	return NULL;
}

static struct lobject *
lexception_string(struct lemon *lemon, struct lexception *self)
{
	char *buffer;
	unsigned long length;
	const char *name;
	const char *message;

	struct lclass *clazz;
	struct lobject *string;

	length = 5; /* minimal length '<()>\0' */
	if (self->clazz && lobject_is_class(lemon, self->clazz)) {
		clazz = (struct lclass *)self->clazz;
		name = lstring_to_cstr(lemon, clazz->name);
		length += lstring_length(lemon, clazz->name);
	} else {
		name = "Exception";
		length += strlen("Exception");
	}
	message = "";
	if (self->message && lobject_is_string(lemon, self->message)) {
		message = lstring_to_cstr(lemon, self->message);
		length += lstring_length(lemon, self->message);
	}

	buffer = lemon_allocator_alloc(lemon, length);
	if (!buffer) {
		return NULL;
	}
	snprintf(buffer, length, "<%s(%s)>", name, message);
	string = lstring_create(lemon, buffer, length);
	lemon_allocator_free(lemon, buffer);

	return string;
}

static struct lobject *
lexception_mark(struct lemon *lemon, struct lexception *self)
{
	int i;

	if (self->clazz) {
		lobject_mark(lemon, self->clazz);
	}

	if (self->message) {
		lobject_mark(lemon, self->message);
	}

	for (i = 0; i < self->nframe; i++) {
		lobject_mark(lemon, self->frame[i]);
	}

	return NULL;
}

static struct lobject *
lexception_method(struct lemon *lemon,
                  struct lobject *object,
                  int method, int argc, struct lobject *argv[])
{
#define cast(a) ((struct lexception *)(a))

	switch (method) {
	case LOBJECT_METHOD_GET_ATTR:
		return lexception_get_attr(lemon, object, argv[0]);

	case LOBJECT_METHOD_STRING:
		return lexception_string(lemon, cast(object));

	case LOBJECT_METHOD_MARK:
		return lexception_mark(lemon, cast(object));

	default:
		return lobject_default(lemon, object, method, argc, argv);
	}
}

void *
lexception_create(struct lemon *lemon,
                  struct lobject *message)
{
	struct lexception *self;

	self = lobject_create(lemon, sizeof(*self), lexception_method);
	if (self) {
		self->message = message;
	}

	return self;
}

static struct lobject *
lexception_type_method(struct lemon *lemon,
                       struct lobject *self,
                       int method, int argc, struct lobject *argv[])
{
	switch (method) {
	case LOBJECT_METHOD_CALL: {
		struct lobject *message;
		struct lobject *exception;

		message = NULL;
		if (argc) {
			message = argv[0];
		}
		exception = lexception_create(lemon, message);

		return exception;
	}

	case LOBJECT_METHOD_CALLABLE:
		return lemon->l_true;

	default:
		return lobject_default(lemon, self, method, argc, argv);
	}
}

struct ltype *
lexception_type_create(struct lemon *lemon)
{
	struct ltype *type;

	type = ltype_create(lemon,
	                    "Exception",
	                    lexception_method,
	                    lexception_type_method);
	if (type) {
		lemon_add_global(lemon, "Exception", type);
	}

	return type;
}
