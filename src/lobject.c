#include "lemon.h"
#include "larray.h"
#include "lclass.h"
#include "lstring.h"
#include "linteger.h"
#include "linstance.h"
#include "lexception.h"

#include <stdio.h>
#include <assert.h>
#include <stdarg.h>
#include <string.h>

void *
lobject_create(struct lemon *lemon, size_t size, lobject_method_t method)
{
	struct lobject *self;

	self = lemon_allocator_alloc(lemon, size);
	if (self) {
		assert(((uintptr_t)self & 0x7) == 0);
		memset(self, 0, size);

		self->l_method = method;
		lemon_collector_trace(lemon, self);
	}

	return self;
}

int
lobject_destroy(struct lemon *lemon, struct lobject *self)
{
	if (self && lobject_is_pointer(lemon, self)) {
		lobject_method_call(lemon,
		                    self,
		                    LOBJECT_METHOD_DESTROY, 0, NULL);
		lemon_allocator_free(lemon, self);
	}

	return 0;
}

void
lobject_copy(struct lemon *lemon,
             struct lobject *newobject,
             struct lobject *oldobject,
             size_t size)
{
	if (size > sizeof(struct lobject)) {
		memcpy((char *)newobject + sizeof(struct lobject),
		       (char *)oldobject + sizeof(struct lobject),
		       size - sizeof(struct lobject));
	}
}

struct lobject *
lobject_eq(struct lemon *lemon, struct lobject *a, struct lobject *b)
{
	return lobject_binop(lemon, LOBJECT_METHOD_EQ, a, b);
}

struct lobject *
lobject_unop(struct lemon *lemon, int method, struct lobject *a)
{
	return lobject_method_call(lemon, a, method, 0, NULL);
}

struct lobject *
lobject_binop(struct lemon *lemon,
              int method, struct lobject *a, struct lobject *b)
{
	struct lobject *argv[1];

	argv[0] = b;
	return lobject_method_call(lemon, a, method, 1, argv);
}

struct lobject *
lobject_mark(struct lemon *lemon, struct lobject *self)
{
	if (lobject_is_pointer(lemon, self)) {
		lemon_collector_mark(lemon, self);
	}

	return NULL;
}

struct lobject *
lobject_length(struct lemon *lemon, struct lobject *self)
{
	return lobject_method_call(lemon,
	                           self,
	                           LOBJECT_METHOD_LENGTH, 0, NULL);
}

struct lobject *
lobject_string(struct lemon *lemon, struct lobject *self)
{
	return lobject_method_call(lemon,
	                           self,
	                           LOBJECT_METHOD_STRING, 0, NULL);
}

struct lobject *
lobject_boolean(struct lemon *lemon, struct lobject *self)
{
	if (lobject_is_integer(lemon, self)) {
		return linteger_method(lemon,
	                               self,
	                               LOBJECT_METHOD_BOOLEAN, 0, NULL);
	}
	if (self->l_method == lemon->l_boolean_type->method) {
		return self;
	}

	return lobject_method_call(lemon,
	                           self,
	                           LOBJECT_METHOD_BOOLEAN, 0, NULL);
}

struct lobject *
lobject_call(struct lemon *lemon,
             struct lobject *self,
             int argc, struct lobject *argv[])
{
	return lobject_method_call(lemon,
	                           self,
	                           LOBJECT_METHOD_CALL, argc, argv);
}

struct lobject *
lobject_all_item(struct lemon *lemon, struct lobject *self)
{
	struct lobject *value;
	value = lobject_method_call(lemon,
	                            self,
	                            LOBJECT_METHOD_ALL_ITEM, 0, NULL);

	return value;
}

struct lobject *
lobject_map_item(struct lemon *lemon, struct lobject *self)
{
	struct lobject *value;
	value = lobject_method_call(lemon,
	                            self,
	                            LOBJECT_METHOD_MAP_ITEM, 0, NULL);

	return value;
}

struct lobject *
lobject_get_item(struct lemon *lemon,
                 struct lobject *self,
                 struct lobject *name)
{
	struct lobject *value;
	struct lobject *argv[1];

	argv[0] = name;
	value = lobject_method_call(lemon,
	                            self,
	                            LOBJECT_METHOD_GET_ITEM, 1, argv);

	return value;
}

struct lobject *
lobject_has_item(struct lemon *lemon,
                 struct lobject *self,
                 struct lobject *name)
{
	struct lobject *value;
	struct lobject *argv[1];

	argv[0] = name;
	value = lobject_method_call(lemon,
	                            self,
	                            LOBJECT_METHOD_HAS_ITEM, 1, argv);

	return value;
}

struct lobject *
lobject_set_item(struct lemon *lemon,
                 struct lobject *self,
                 struct lobject *name,
                 struct lobject *value)
{
	struct lobject *argv[2];

	argv[0] = name;
	argv[1] = value;
	lemon_collector_barrierback(lemon, self, name);
	lemon_collector_barrierback(lemon, self, value);

	return lobject_method_call(lemon,
	                           self,
	                           LOBJECT_METHOD_SET_ITEM, 2, argv);
}

struct lobject *
lobject_del_item(struct lemon *lemon,
                 struct lobject *self,
                 struct lobject *name)
{
	struct lobject *argv[1];

	argv[0] = name;
	return lobject_method_call(lemon,
	                           self,
	                           LOBJECT_METHOD_DEL_ITEM, 1, argv);
}

struct lobject *
lobject_add_attr(struct lemon *lemon,
                 struct lobject *self,
                 struct lobject *name,
                 struct lobject *value)
{
	struct lobject *argv[2];

	argv[0] = name;
	argv[1] = value;
	lemon_collector_barrierback(lemon, self, name);
	lemon_collector_barrierback(lemon, self, value);
	return lobject_method_call(lemon,
	                           self,
	                           LOBJECT_METHOD_ADD_ATTR, 2, argv);
}

struct lobject *
lobject_get_attr(struct lemon *lemon,
                 struct lobject *self,
                 struct lobject *name)
{
	struct lobject *value;
	struct lobject *argv[1];

	argv[0] = name;
	value = lobject_method_call(lemon,
	                            self,
	                            LOBJECT_METHOD_GET_ATTR, 1, argv);

	return value;
}

struct lobject *
lobject_set_attr(struct lemon *lemon,
                 struct lobject *self,
                 struct lobject *name,
                 struct lobject *value)
{
	struct lobject *argv[2];

	argv[0] = name;
	argv[1] = value;
	lemon_collector_barrierback(lemon, self, name);
	lemon_collector_barrierback(lemon, self, value);
	return lobject_method_call(lemon,
	                           self,
	                           LOBJECT_METHOD_SET_ATTR, 2, argv);
}

struct lobject *
lobject_del_attr(struct lemon *lemon,
                 struct lobject *self,
                 struct lobject *name)
{
	struct lobject *value;
	struct lobject *argv[1];

	argv[0] = name;
	value = lobject_method_call(lemon,
	                            self,
	                            LOBJECT_METHOD_DEL_ATTR, 1, argv);

	return value;
}

struct lobject *
lobject_has_attr(struct lemon *lemon,
                 struct lobject *self,
                 struct lobject *name)
{
	struct lobject *value;
	struct lobject *argv[1];

	argv[0] = name;
	value = lobject_method_call(lemon,
	                            self,
	                            LOBJECT_METHOD_HAS_ATTR, 1, argv);

	return value;
}

struct lobject *
lobject_get_slice(struct lemon *lemon,
                  struct lobject *self,
                  struct lobject *start,
                  struct lobject *stop,
                  struct lobject *step)
{
	struct lobject *argv[3];

	argv[0] = start;
	argv[1] = stop;
	argv[2] = step;
	return lobject_method_call(lemon,
	                           self,
	                           LOBJECT_METHOD_GET_SLICE, 3, argv);
}

struct lobject *
lobject_set_slice(struct lemon *lemon,
                  struct lobject *self,
                  struct lobject *start,
                  struct lobject *stop,
                  struct lobject *step,
                  struct lobject *value)
{
	struct lobject *argv[4];

	argv[0] = start;
	argv[1] = stop;
	argv[2] = step;
	argv[3] = value;
	return lobject_method_call(lemon,
	                           self,
	                           LOBJECT_METHOD_SET_SLICE, 4, argv);
}

struct lobject *
lobject_del_slice(struct lemon *lemon,
                  struct lobject *self,
                  struct lobject *start,
                  struct lobject *stop,
                  struct lobject *step)
{
	struct lobject *argv[3];

	argv[0] = start;
	argv[1] = stop;
	argv[2] = step;
	return lobject_method_call(lemon,
	                           self,
	                           LOBJECT_METHOD_DEL_SLICE, 3, argv);
}

struct lobject *
lobject_get_getter(struct lemon *lemon,
                   struct lobject *self,
                   struct lobject *name)
{
	struct lobject *value;
	struct lobject *argv[1];

	argv[0] = name;
	value = lobject_method_call(lemon,
	                            self,
	                            LOBJECT_METHOD_GET_GETTER, 1, argv);

	return value;
}

struct lobject *
lobject_get_setter(struct lemon *lemon,
                   struct lobject *self,
                   struct lobject *name)
{
	struct lobject *value;
	struct lobject *argv[1];

	argv[0] = name;
	value = lobject_method_call(lemon,
	                            self,
	                            LOBJECT_METHOD_GET_SETTER, 1, argv);

	return value;
}

struct lobject *
lobject_call_attr(struct lemon *lemon,
                  struct lobject *self,
                  struct lobject *name,
                  int argc, struct lobject *argv[])
{
	struct lobject *value;

	value = lobject_get_attr(lemon, self, name);
	if (!value) {
		return lobject_error_attribute(lemon,
		                               "'%@' has no attribute %@",
		                               self,
		                               name);
	}

	return lobject_call(lemon, value, argc, argv);
}

void
lobject_print(struct lemon *lemon, ...)
{
	va_list ap;

	struct lobject *object;
	struct lobject *string;

	va_start(ap, lemon);
	for (;;) {
		object = va_arg(ap, struct lobject *);
		if (!object) {
			break;
		}

		string = lobject_string(lemon, object);
		if (string && lobject_is_string(lemon, string)) {
			printf("%s ", lstring_to_cstr(lemon, string));
		}
	}
	va_end(ap);

	printf("\n");
}

int
lobject_is_integer(struct lemon *lemon, struct lobject *object)
{
	if ((int)((intptr_t)(object)) & 0x1) {
		return 1;
	}

	return object->l_method == lemon->l_integer_type->method;
}

int
lobject_is_pointer(struct lemon *lemon, struct lobject *object)
{
	return ((int)((intptr_t)(object)) & 0x1) == 0;
}

int
lobject_is_type(struct lemon *lemon, struct lobject *object)
{
	if (lobject_is_pointer(lemon, object)) {
		return object->l_method == lemon->l_type_type->method;
	}

	return 0;
}

int
lobject_is_karg(struct lemon *lemon, struct lobject *object)
{
	if (lobject_is_pointer(lemon, object)) {
		return object->l_method == lemon->l_karg_type->method;
	}

	return 0;
}

int
lobject_is_varg(struct lemon *lemon, struct lobject *object)
{
	if (lobject_is_pointer(lemon, object)) {
		return object->l_method == lemon->l_varg_type->method;
	}

	return 0;
}

int
lobject_is_vkarg(struct lemon *lemon, struct lobject *object)
{
	if (lobject_is_pointer(lemon, object)) {
		return object->l_method == lemon->l_vkarg_type->method;
	}

	return 0;
}

int
lobject_is_class(struct lemon *lemon, struct lobject *object)
{
	if (lobject_is_pointer(lemon, object)) {
		return object->l_method == lemon->l_class_type->method;
	}

	return 0;
}

int
lobject_is_array(struct lemon *lemon, struct lobject *object)
{
	if (lobject_is_pointer(lemon, object)) {
		return object->l_method == lemon->l_array_type->method;
	}

	return 0;
}

int
lobject_is_number(struct lemon *lemon, struct lobject *object)
{
	if (lobject_is_pointer(lemon, object)) {
		return object->l_method == lemon->l_number_type->method;
	}

	return 0;
}

int
lobject_is_string(struct lemon *lemon, struct lobject *object)
{
	if (lobject_is_pointer(lemon, object)) {
		return object->l_method == lemon->l_string_type->method;
	}

	return 0;
}

int
lobject_is_iterator(struct lemon *lemon, struct lobject *object)
{
	if (lobject_is_pointer(lemon, object)) {
		return object->l_method == lemon->l_iterator_type->method;
	}

	return 0;
}

int
lobject_is_instance(struct lemon *lemon, struct lobject *object)
{
	if (lobject_is_pointer(lemon, object)) {
		return object->l_method == lemon->l_instance_type->method;
	}

	return 0;
}

int
lobject_is_function(struct lemon *lemon, struct lobject *object)
{
	if (lobject_is_pointer(lemon, object)) {
		return object->l_method == lemon->l_function_type->method;
	}

	return 0;
}

int
lobject_is_coroutine(struct lemon *lemon, struct lobject *object)
{
	if (lobject_is_pointer(lemon, object)) {
		return object->l_method == lemon->l_coroutine_type->method;
	}

	return 0;
}

int
lobject_is_exception(struct lemon *lemon, struct lobject *object)
{
	if (lobject_is_pointer(lemon, object)) {
		return object->l_method == lemon->l_exception_type->method;
	}

	return 0;
}

int
lobject_is_dictionary(struct lemon *lemon, struct lobject *object)
{
	if (lobject_is_pointer(lemon, object)) {
		return object->l_method == lemon->l_dictionary_type->method;
	}

	return 0;
}

int
lobject_is_error(struct lemon *lemon, struct lobject *object)
{
	if (object) {
		if (lobject_is_instance(lemon, object)) {
			struct linstance *instance;

			instance = (struct linstance *)object;
			object = instance->native;
			if (!object) {
				return 0;
			}
		}
		if (lobject_is_exception(lemon, object)) {
			return ((struct lexception *)object)->throwed;
		}

	}

	return 0;
}

int
lobject_is_equal(struct lemon *lemon, struct lobject *a, struct lobject *b)
{
	if (a == b) {
		return 1;
	}

	return lobject_eq(lemon, a, b) == lemon->l_true;
}

struct lobject *
lobject_error_base(struct lemon *lemon,
                   struct lobject *base,
                   const char *fmt,
                   va_list ap)
{
	const char *p;

	char buffer[4096];
	size_t length;
	struct lobject *value;
	struct lobject *string;
	struct lobject *message;
	struct lobject *exception;

	length = 0;
	memset(buffer, 0, sizeof(buffer));
	for (p = fmt; *p; p++) {
		if (*p == '%') {
			p++;
			switch (*p) {
			case 'd':
				snprintf(buffer + length,
				         sizeof(buffer) - length,
				         "%d",
				         va_arg(ap, int));
				length = strnlen(buffer, sizeof(buffer));
				if (length >= sizeof(buffer)) {
					goto out;
				}
				break;
			case '@':
				value = va_arg(ap, struct lobject *);
				string = lobject_string(lemon, value);
				snprintf(buffer + length,
				         sizeof(buffer) - length,
				         "%s",
				         lstring_to_cstr(lemon, string));
				length = strnlen(buffer, sizeof(buffer));
				if (length >= sizeof(buffer)) {
					goto out;
				}
				break;
			case 's':
				snprintf(buffer + length,
				         sizeof(buffer) - length,
				         "%s",
				         va_arg(ap, char *));
				length = strnlen(buffer, sizeof(buffer));
				if (length >= sizeof(buffer)) {
					goto out;
				}
				break;
			default:
				buffer[length++] = '%';
				break;
			}
		} else {
			buffer[length++] = *p;
			if (length >= sizeof(buffer)) {
				goto out;
			}
		}
	}

out:
	message = lstring_create(lemon,
	                         buffer,
	                         strnlen(buffer, sizeof(buffer)));
	exception = lobject_call(lemon, base, 1, &message);

	/* return from class's call */
	exception = lemon_machine_return_frame(lemon, exception);
	exception = lobject_throw(lemon, exception);

	return exception;
}

struct lobject *
lobject_error(struct lemon *lemon,
              struct lobject *base,
              const char *fmt, ...)
{
	va_list ap;
	struct lobject *e;

	va_start(ap, fmt);
	e = lobject_error_base(lemon, base, fmt, ap);
	va_end(ap);

	return e;
}

struct lobject *
lobject_error_type(struct lemon *lemon, const char *fmt, ...)
{
	va_list ap;
	struct lobject *e;

	va_start(ap, fmt);
	e = lobject_error_base(lemon, lemon->l_type_error, fmt, ap);
	va_end(ap);

	return e;
}

struct lobject *
lobject_error_item(struct lemon *lemon, const char *fmt, ...)
{
	va_list ap;
	struct lobject *e;

	va_start(ap, fmt);
	e = lobject_error_base(lemon, lemon->l_item_error, fmt, ap);
	va_end(ap);

	return e;
}

struct lobject *
lobject_error_memory(struct lemon *lemon, const char *fmt, ...)
{
	va_list ap;
	struct lobject *e;

	va_start(ap, fmt);
	e = lobject_error_base(lemon, lemon->l_memory_error, fmt, ap);
	va_end(ap);

	return e;
}

struct lobject *
lobject_error_runtime(struct lemon *lemon, const char *fmt, ...)
{
	va_list ap;
	struct lobject *e;

	va_start(ap, fmt);
	e = lobject_error_base(lemon, lemon->l_runtime_error, fmt, ap);
	va_end(ap);

	return e;
}

struct lobject *
lobject_error_argument(struct lemon *lemon, const char *fmt, ...)
{
	va_list ap;
	struct lobject *e;

	va_start(ap, fmt);
	e = lobject_error_base(lemon, lemon->l_argument_error, fmt, ap);
	va_end(ap);

	return e;
}

struct lobject *
lobject_error_attribute(struct lemon *lemon, const char *fmt, ...)
{
	va_list ap;
	struct lobject *e;

	va_start(ap, fmt);
	e = lobject_error_base(lemon, lemon->l_attribute_error, fmt, ap);
	va_end(ap);

	return e;
}

struct lobject *
lobject_error_arithmetic(struct lemon *lemon, const char *fmt, ...)
{
	va_list ap;
	struct lobject *e;

	va_start(ap, fmt);
	e = lobject_error_base(lemon, lemon->l_arithmetic_error, fmt, ap);
	va_end(ap);

	return e;
}

void *
lobject_error_not_callable(struct lemon *lemon, struct lobject *object)
{
	return lobject_error(lemon,
	                     lemon->l_not_callable_error,
	                     "'%@' is not callable",
	                     object);
}

void *
lobject_error_not_iterable(struct lemon *lemon, struct lobject *object)
{
	return lobject_error(lemon,
	                     lemon->l_not_callable_error,
	                     "'%@' is not iterable",
	                     object);
}

void *
lobject_error_not_implemented(struct lemon *lemon)
{
	return lobject_error(lemon, lemon->l_not_implemented_error, "");
}

struct lobject *
lobject_throw(struct lemon *lemon, struct lobject *error)
{
	struct lobject *clazz;
	struct lobject *message;
	struct linstance *instance;
	struct lexception *exception;

	clazz = NULL;
	exception = NULL;
	if (lobject_is_exception(lemon, error)) {
		exception = (struct lexception *)error;
	} else if (lobject_is_instance(lemon, error)) {
		instance = (struct linstance *)error;

		if (instance->native &&
		    lobject_is_exception(lemon, instance->native))
		{
			clazz = (struct lobject *)instance->clazz;
			exception = (struct lexception *)instance->native;
		}
	}

	if (!exception) {
		message = lstring_create(lemon, "throw non-Exception", 19);
		error = lobject_call(lemon, lemon->l_type_error, 1, &message);
		instance = (struct linstance *)error;
		clazz = (struct lobject *)instance->clazz;
		exception = (struct lexception *)instance->native;
	}

	assert(exception);

	exception->clazz = clazz;
	exception->throwed = 1;

	return error;
}

struct lobject *
lobject_method_call(struct lemon *lemon,
                    struct lobject *self,
                    int method, int argc, struct lobject *argv[])
{
	if (lobject_is_pointer(lemon, self) && self->l_method) {
		return self->l_method(lemon, self, method, argc, argv);
	}

	if (lobject_is_integer(lemon, self)) {
		return linteger_method(lemon, self, method, argc, argv);
	}

	return lobject_default(lemon, self, method, argc, argv);
}

struct lobject *
lobject_default(struct lemon *lemon,
                struct lobject *self,
                int method, int argc, struct lobject *argv[])
{
	switch (method) {
	case LOBJECT_METHOD_EQ:
		if (self == argv[0]) {
			return lemon->l_true;
		}
		return lemon->l_false;

	case LOBJECT_METHOD_NE:
		if (!lobject_is_equal(lemon, self, argv[0])) {
			return lemon->l_true;
		}
		return lemon->l_false;

	case LOBJECT_METHOD_LE:
		return lobject_eq(lemon, self, argv[0]);

	case LOBJECT_METHOD_GE:
		return lobject_eq(lemon, self, argv[0]);

	case LOBJECT_METHOD_BOOLEAN:
		return lemon->l_true;

	case LOBJECT_METHOD_CALL:
		return lobject_error_not_callable(lemon, self);

	case LOBJECT_METHOD_CALLABLE:
		return lemon->l_false;

	case LOBJECT_METHOD_GET_ITEM:
		return lobject_error_not_implemented(lemon);

	case LOBJECT_METHOD_SET_ITEM:
		return lobject_error_not_implemented(lemon);

	case LOBJECT_METHOD_GET_ATTR:
		return NULL;

	case LOBJECT_METHOD_SET_ATTR:
		return lobject_error_not_implemented(lemon);

	case LOBJECT_METHOD_GET_GETTER:
		return NULL;

	case LOBJECT_METHOD_GET_SETTER:
		return NULL;

	case LOBJECT_METHOD_SUPER:
		return NULL;

	case LOBJECT_METHOD_SUBCLASS:
		return NULL;

	case LOBJECT_METHOD_INSTANCE:
		return NULL;

	case LOBJECT_METHOD_STRING: {
		char buffer[32];
		snprintf(buffer, sizeof(buffer), "<object %p>", (void *)self);
		return lstring_create(lemon, buffer, strlen(buffer));
	}

	case LOBJECT_METHOD_HASH:
		return linteger_create_from_long(lemon, (long)self);

	case LOBJECT_METHOD_MARK:
		return NULL;

	case LOBJECT_METHOD_DESTROY:
		return NULL;

	default:
		return lobject_error_type(lemon,
		                          "unsupport method for %@",
		                          self);
	}
}

struct lobject *
lobject_get_default_subclassof(struct lemon *lemon,
                            struct lobject *self,
                            int argc, struct lobject *argv[])
{
	int i;
	struct lclass *clazz;
	struct lobject *item;

	if (lobject_is_class(lemon, self)) {
		clazz = (struct lclass *)self;
		for (i = 0; i < larray_length(lemon, clazz->bases); i++) {
			item = larray_get_item(lemon, clazz->bases, i);
			if (lobject_is_equal(lemon, item, argv[0])) {
				return lemon->l_true;
			}
		}
	}

	return lemon->l_false;
}

struct lobject *
lobject_get_default_instanceof(struct lemon *lemon,
                           struct lobject *self,
                           int argc, struct lobject *argv[])
{
	int i;

	if (lobject_is_instance(lemon, self)) {
		struct lobject *clazz;

		clazz = (struct lobject *)((struct linstance *)self)->clazz;
		if (!clazz) {
			return lemon->l_false;
		}
		for (i = 0; i < argc; i++) {
			if (lobject_is_equal(lemon, clazz, argv[i])) {
				return lemon->l_true;
			}
		}

		return lobject_get_default_subclassof(lemon, clazz, argc, argv);
	}

	if (lobject_is_pointer(lemon, self)) {
		for (i = 0; i < argc; i++) {
			struct ltype *type;

			if (lobject_is_type(lemon, argv[i])) {
				type = (struct ltype *)argv[i];
				if (self->l_method == type->method) {
					return lemon->l_true;
				}
			}
		}
		return lemon->l_false;
	}

	if (lobject_is_integer(lemon, self)) {
		for (i = 0; i < argc; i++) {
			if (lemon->l_integer_type == (struct ltype *)argv[i]) {
				return lemon->l_true;
			}
		}
		return lemon->l_false;
	}

	return lemon->l_false;
}

struct lobject *
lobject_get_default_callable(struct lemon *lemon,
                         struct lobject *self,
                         int argc, struct lobject *argv[])
{
	return self->l_method(lemon, self, LOBJECT_METHOD_CALLABLE, 0, NULL);
}

struct lobject *
lobject_get_default_string(struct lemon *lemon,
                       struct lobject *self,
                       int argc, struct lobject *argv[])
{
	return lobject_string(lemon, self);
}

struct lobject *
lobject_get_default_length(struct lemon *lemon,
                        struct lobject *self,
                        int argc, struct lobject *argv[])
{
	return lobject_length(lemon, self);
}

struct lobject *
lobject_get_default_get_attr(struct lemon *lemon,
                         struct lobject *self,
                         int argc, struct lobject *argv[])
{
	return lobject_get_attr(lemon, self, argv[0]);
}

struct lobject *
lobject_get_default_set_attr(struct lemon *lemon,
                         struct lobject *self,
                         int argc, struct lobject *argv[])
{
	return lobject_set_attr(lemon, self, argv[0], argv[1]);
}

struct lobject *
lobject_get_default_del_attr(struct lemon *lemon,
                             struct lobject *self,
                             int argc, struct lobject *argv[])
{
	return lobject_del_attr(lemon, self, argv[0]);
}

struct lobject *
lobject_get_default_has_attr(struct lemon *lemon,
                         struct lobject *self,
                         int argc, struct lobject *argv[])
{
	return lobject_has_attr(lemon, self, argv[0]);
}

struct lobject *
lobject_default_get_attr(struct lemon *lemon,
                         struct lobject *self,
                         struct lobject *name)
{
	const char *cstr;
	struct lobject *value;

	if (!lobject_is_string(lemon, name)) {
		return NULL;
	}

	value = lobject_get_attr(lemon, self, name);
	if (value) {
		return value;
	}

	cstr = lstring_to_cstr(lemon, name);
	if (strcmp(cstr, "__string__") == 0) {
		return lfunction_create(lemon,
		                        name,
		                        self,
		                        lobject_get_default_string);
	}

	if (strcmp(cstr, "__length__") == 0) {
		return lfunction_create(lemon,
		                        name,
		                        self,
		                        lobject_get_default_length);
	}

	if (strcmp(cstr, "__callable__") == 0) {
		return lfunction_create(lemon,
		                        name,
		                        self,
		                        lobject_get_default_callable);
	}

	if (strcmp(cstr, "__instanceof__") == 0) {
		return lfunction_create(lemon,
		                        name,
		                        self,
		                        lobject_get_default_instanceof);
	}

	if (strcmp(cstr, "__subclassof__") == 0) {
		return lfunction_create(lemon,
		                        name,
		                        self,
		                        lobject_get_default_subclassof);
	}

	if (strcmp(cstr, "__get_attr__") == 0) {
		return lfunction_create(lemon,
		                        name,
		                        self,
		                        lobject_get_default_get_attr);
	}

	if (strcmp(cstr, "__set_attr__") == 0) {
		return lfunction_create(lemon,
		                        name,
		                        self,
		                        lobject_get_default_set_attr);
	}

	if (strcmp(cstr, "__del_attr__") == 0) {
		return lfunction_create(lemon,
		                        name,
		                        self,
		                        lobject_get_default_del_attr);
	}

	if (strcmp(cstr, "__has_attr__") == 0) {
		return lfunction_create(lemon,
		                        name,
		                        self,
		                        lobject_get_default_has_attr);
	}

	return NULL;
}
