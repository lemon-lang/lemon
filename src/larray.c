#include "lemon.h"
#include "larray.h"
#include "lstring.h"
#include "linteger.h"
#include "literator.h"

#include <stdio.h>
#include <string.h>

static struct lobject *
larray_eq(struct lemon *lemon, struct larray *a, struct larray *b)
{
	int i;

	if (!lobject_is_array(lemon, (struct lobject *)b)) {
		return lemon->l_false;
	}

	if (a->count != b->count) {
		return lemon->l_false;
	}

	for (i = 0; i < a->count; i++) {
		if (!lobject_is_equal(lemon, a->items[i], b->items[i])) {
			return lemon->l_false;
		}
	}

	return lemon->l_true;
}

static struct lobject *
larray_add(struct lemon *lemon, struct larray *a, struct larray *b)
{
	struct lobject *array;

	if (!lobject_is_array(lemon, (struct lobject *)b)) {
		return lemon->l_nil;
	}

	array = larray_create(lemon, a->count, a->items);
	if (array) {
		if (!larray_append(lemon, array, b->count, b->items)) {
			return NULL;
		}
	}

	return array;
}

static struct lobject *
larray_has_item(struct lemon *lemon, struct larray *a, struct lobject *b)
{
	int i;

	if (a == NULL || b == NULL) {
		return lemon->l_false;
	}

	for (i = 0; i < a->count; i++) {
		if (lobject_is_equal(lemon, a->items[i], b)) {
			return lemon->l_true;
		}
	}

	return lemon->l_false;
}

struct lobject *
larray_get_item(struct lemon *lemon,
                struct lobject *self,
                long i)
{
	struct larray *array;

	array = (struct larray *)self;
	if (i < 0) {
		i = array->count + i;
	}
	if (i < 0 || i >= array->count) {
		return lobject_error_item(lemon,
		                          "'%@' index out of range",
		                          self);
	}
	lemon_collector_barrierback(lemon, self, array->items[i]);

	return array->items[i];
}

struct lobject *
larray_set_item(struct lemon *lemon,
                struct lobject *self,
                long i,
                struct lobject *value)
{
	struct larray *array;

	array = (struct larray *)self;
	if (i < 0) {
		i = array->count + i;
	}
	if (i < 0 || i >= array->count) {
		return lobject_error_item(lemon,
		                          "'%@' index out of range",
		                          self);
	}
	array->items[i] = value;
	lemon_collector_barrierback(lemon, self, value);

	return value;
}

struct lobject *
larray_del_item(struct lemon *lemon,
                struct lobject *self,
                long i)
{
	struct larray *array;

	array = (struct larray *)self;
	if (i < 0) {
		i = array->count + i;
	}
	if (i < 0 || i >= array->count) {
		return lobject_error_item(lemon,
		                          "'%@' index out of range",
		                          self);
	}

	if (i != array->count - 1) {
		memmove(&array->items[i],
		        &array->items[i+1],
		        (array->count - i - 1) * sizeof(struct lobject *));
	}
	array->count -= 1;

	return lemon->l_nil;
}

struct lobject *
larray_append(struct lemon *lemon,
              struct lobject *self,
              int argc, struct lobject *argv[])
{
	int i;
	int count;
	size_t size;
	struct larray *array;

	array = (struct larray *)self;
	count = array->count + argc;
	if (count > array->alloc) {
		size = sizeof(struct lobject *) * count * 2;
		array->items = lemon_allocator_realloc(lemon,
		                                       array->items,
		                                       size);
		if (!array->items) {
			return NULL;
		}
		array->alloc = count * 2;
	}
	for (i = 0; i < argc; i++) {
		lemon_collector_barrierback(lemon, self, argv[i]);
		array->items[array->count + i] = argv[i];
	}
	array->count = count;

	return lemon->l_nil;
}

static struct lobject *
larray_pop(struct lemon *lemon,
           struct lobject *self,
           int argc, struct lobject *argv[])
{
	struct larray *array;

	array = (struct larray *)self;
	if (!array->count) {
		return lobject_error_item(lemon, "pop() from empty list");
	}

	return array->items[--array->count];
}

static struct lobject *
larray_iterator_next(struct lemon *lemon,
                     struct lobject *iterable,
                     struct lobject **context)
{
	long i;
	struct larray *array;

	array = (struct larray *)iterable;
	if (array == NULL) {
		return lemon->l_nil;
	}

	i = linteger_to_long(lemon, *context);
	if (i >= array->count) {
		return lemon->l_sentinel;
	}
	*context = linteger_create_from_long(lemon, i + 1);

	return array->items[i];
}

static struct lobject *
larray_iterator(struct lemon *lemon,
                struct lobject *self,
                int argc, struct lobject *argv[])
{
	struct lobject *context;

	context = linteger_create_from_long(lemon, 0);

	return literator_create(lemon, self, context, larray_iterator_next);
}

static struct lobject *
larray_get_attr(struct lemon *lemon,
                struct lobject *self,
                struct lobject *name)
{
	const char *cstr;

	cstr = lstring_to_cstr(lemon, name);
	if (strcmp(cstr, "append") == 0) {
		return lfunction_create(lemon, name, self, larray_append);
	}

	if (strcmp(cstr, "pop") == 0) {
		return lfunction_create(lemon, name, self, larray_pop);
	}

	if (strcmp(cstr, "__iterator__") == 0) {
		return lfunction_create(lemon, name, self, larray_iterator);
	}

	return NULL;
}

static struct lobject *
larray_get_slice(struct lemon *lemon,
                 struct lobject *self,
                 struct lobject *start,
                 struct lobject *stop,
                 struct lobject *step)
{
	long istart;
	long istop;
	long istep;

	struct lobject *slice;
	struct lobject *argv[1];

	slice = larray_create(lemon, 0, NULL);
	if (slice) {
		istart = linteger_to_long(lemon, start);
		if (stop == lemon->l_nil) {
			istop = ((struct larray *)self)->count;
		} else {
			istop = linteger_to_long(lemon, stop);
		}
		istep = linteger_to_long(lemon, step);
		for (; istart < istop; istart += istep) {
			argv[0] = larray_get_item(lemon, self, istart);
			if (!larray_append(lemon, slice, 1, argv)) {
				return NULL;
			}
		}
	}

	return slice;
}

static struct lobject *
larray_string(struct lemon *lemon, struct larray *self)
{
	int i;
	char *buffer;
	const char *fmt;
	unsigned long offset;
	unsigned long length;
	unsigned long maxlen;
	struct lobject *string;

	maxlen = 256;
	buffer = lemon_allocator_alloc(lemon, maxlen);
	if (!buffer) {
		return NULL;
	}
	offset = snprintf(buffer, sizeof(buffer), "[");
	for (i = 0; i < self->count; i++) {
		if (lobject_is_string(lemon, self->items[i])) {
			if (i <  self->count - 1) {
				fmt = "'%s', ";
			} else {
				fmt = "'%s'";
			}
			string = self->items[i];
		} else {
			if (i <  self->count - 1) {
				fmt = "%s, ";
			} else {
				fmt = "%s";
			}
			string = lobject_string(lemon, self->items[i]);
		}

again:
		length = snprintf(buffer + offset,
		                  maxlen - offset,
		                  fmt,
		                  lstring_to_cstr(lemon, string));
		if (offset + length >= maxlen - 1) {
			maxlen = (offset + length) * 2;
			buffer = lemon_allocator_realloc(lemon, buffer, maxlen);
			if (!buffer) {
				return NULL;
			}

			goto again;
		} else {
			offset += length;
		}
	}
	buffer[offset++] = ']';

	string = lstring_create(lemon, buffer, offset);
	lemon_allocator_free(lemon, buffer);

	return string;
}

static struct lobject *
larray_mark(struct lemon *lemon, struct larray *self)
{
	int i;

	for (i = 0; i < self->count; i++) {
		lobject_mark(lemon, self->items[i]);
	}

	return NULL;
}

static struct lobject *
larray_destroy(struct lemon *lemon, struct larray *self)
{
	lemon_allocator_free(lemon, self->items);

	return NULL;
}

static struct lobject *
larray_method(struct lemon *lemon,
              struct lobject *self,
              int method, int argc, struct lobject *argv[])
{
#define cast(a) ((struct larray *)(a))

	switch (method) {
	case LOBJECT_METHOD_EQ:
		return larray_eq(lemon, cast(self), cast(argv[0]));

	case LOBJECT_METHOD_ADD:
		return larray_add(lemon, cast(self), cast(argv[0]));

	case LOBJECT_METHOD_HAS_ITEM:
		return larray_has_item(lemon, cast(self), argv[0]);

	case LOBJECT_METHOD_GET_ITEM:
		if (lobject_is_integer(lemon, argv[0])) {
			long i = linteger_to_long(lemon, argv[0]);
			return larray_get_item(lemon, self, i);
		}
		return NULL;

	case LOBJECT_METHOD_SET_ITEM:
		if (lobject_is_integer(lemon, argv[0])) {
			long i = linteger_to_long(lemon, argv[0]);
			return larray_set_item(lemon, self, i, argv[1]);
		}
		return NULL;

	case LOBJECT_METHOD_DEL_ITEM:
		if (lobject_is_integer(lemon, argv[0])) {
			long i = linteger_to_long(lemon, argv[0]);
			return larray_del_item(lemon, self, i);
		}
		return NULL;

	case LOBJECT_METHOD_GET_ATTR:
		return larray_get_attr(lemon, self, argv[0]);

	case LOBJECT_METHOD_GET_SLICE:
		return larray_get_slice(lemon, self, argv[0], argv[1], argv[2]);

	case LOBJECT_METHOD_STRING:
		return larray_string(lemon, cast(self));

	case LOBJECT_METHOD_LENGTH:
		return linteger_create_from_long(lemon, cast(self)->count);

	case LOBJECT_METHOD_BOOLEAN:
		if (cast(self)->count) {
			return lemon->l_true;
		}
		return lemon->l_false;

	case LOBJECT_METHOD_MARK:
		return larray_mark(lemon, cast(self));

	case LOBJECT_METHOD_DESTROY:
		return larray_destroy(lemon, cast(self));

	default:
		return lobject_default(lemon, self, method, argc, argv);
	}
}

long
larray_length(struct lemon *lemon, struct lobject *self)
{
	return ((struct larray *)self)->count;
}

void *
larray_create(struct lemon *lemon, int count, struct lobject *items[])
{
	int i;
	size_t size;
	struct larray *self;

	self = lobject_create(lemon, sizeof(*self), larray_method);
	if (self) {
		if (count) {
			size = sizeof(struct lobject *) * (count);
			self->items = lemon_allocator_alloc(lemon, size);
			if (!self->items) {
				return NULL;
			}
			for (i = 0; i < count; i++) {
				self->items[i] = items[i];
			}
			self->count = count;
			self->alloc = count;
		}
	}

	return self;
}

static struct lobject *
larray_type_method(struct lemon *lemon,
                   struct lobject *self,
                   int method, int argc, struct lobject *argv[])
{
	switch (method) {
	case LOBJECT_METHOD_CALL:
		return larray_create(lemon, argc, argv);

	case LOBJECT_METHOD_CALLABLE:
		return lemon->l_true;

	default:
		return lobject_default(lemon, self, method, argc, argv);
	}
}

struct ltype *
larray_type_create(struct lemon *lemon)
{
	struct ltype *type;

	type = ltype_create(lemon, "array", larray_method, larray_type_method);
	if (type) {
		lemon_add_global(lemon, "array", type);
	}

	return type;
}
