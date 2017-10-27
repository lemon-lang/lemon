#include "lemon.h"
#include "hash.h"
#include "table.h"
#include "ltable.h"
#include "larray.h"
#include "lstring.h"
#include "linteger.h"

#include <stdio.h>
#include <string.h>

static int
table_cmp(struct lemon *lemon, void *a, void *b)
{
	return lobject_is_equal(lemon, a, b);
}

static unsigned long
table_hash(struct lemon *lemon, void *key)
{
	struct lobject *hash;

	hash = lobject_method_call(lemon, key, LOBJECT_METHOD_HASH, 0, NULL);
	if (!hash) {
		return 0;
	}

	return (unsigned long)linteger_to_long(lemon, hash);
}

static struct lobject *
ltable_eq(struct lemon *lemon, struct ltable *a, struct ltable *b)
{
	int i;
	struct slot *items;
	struct lobject *value;

	if (a->object.l_method != b->object.l_method) {
		return lemon->l_false;
	}

	if (a->count != b->count) {
		return lemon->l_false;
	}

	items = a->items;
	for (i = 0; i < a->length; i++) {
		if (items[i].key == NULL ||
		    items[i].key == lemon->l_sentinel ||
		    items[i].value == lemon->l_sentinel)
		{
			continue;
		}

		value = table_search(lemon,
		                     items[i].key,
		                     b->items,
		                     b->length,
		                     table_cmp,
		                     table_hash);
		if (!value) {
			return lemon->l_false;
		}

		if (!lobject_is_equal(lemon, items[i].value, value)) {
			return lemon->l_false;
		}
	}

	return lemon->l_true;
}

static struct lobject *
ltable_map_item(struct lemon *lemon, struct ltable *self)
{
	int i;
	int j;

	int count;
	struct lobject *array;
	struct lobject **map;
	struct lobject *item;
	struct lobject *kv[2];
	struct slot *items;

	j = 0;
	count = self->count;
	items = self->items;
	map = lemon_allocator_alloc(lemon, sizeof(struct lobject *) * count);
	if (!map) {
		return lemon->l_out_of_memory;
	}
	for (i = 0; i < self->length; i++) {
		if (items[i].key == NULL ||
		    items[i].key == lemon->l_sentinel ||
		    items[i].value == lemon->l_sentinel)
		{
			continue;
		}

		kv[0] = items[i].key;
		kv[1] = items[i].value;

		item = larray_create(lemon, 2, kv);
		if (!item) {
			return NULL;
		}
		map[j++] = item;
	}

	array = larray_create(lemon, count, map);
	lemon_allocator_free(lemon, map);

	return array;
}

static struct lobject *
ltable_get_item(struct lemon *lemon,
                struct ltable *self,
                struct lobject *name)
{
	return table_search(lemon,
	                    name,
	                    self->items,
	                    self->length,
	                    table_cmp,
	                    table_hash);
}

static struct lobject *
ltable_has_item(struct lemon *lemon,
                struct ltable *self,
                struct lobject *name)
{
	struct lobject *value;

	value =  table_search(lemon,
	                      name,
	                      self->items,
	                      self->length,
	                      table_cmp,
	                      table_hash);
	if (value) {
		return lemon->l_true;
	}
	return lemon->l_false;
}

static struct lobject *
ltable_set_item(struct lemon *lemon,
                struct ltable *self,
                struct lobject *name,
                struct lobject *value)
{
	int count;
	int length;
	size_t size;
	struct slot *items;

	count = table_insert(lemon,
	                     name,
	                     value,
	                     self->items,
	                     self->length,
	                     table_cmp,
	                     table_hash);

	if (count) {
		self->count += 1;
	}

	if (TABLE_LOAD_FACTOR(self->count) > self->length) {
		count = self->count;
		length = table_size(lemon, TABLE_GROW_FACTOR(self->length));

		size = sizeof(struct slot) * length;
		items = lemon_allocator_alloc(lemon, size);
		if (!items) {
			return NULL;
		}
		memset(items, 0, size);

		table_rehash(lemon,
		             self->items,
		             self->length,
		             items,
		             length,
		             table_cmp,
		             table_hash);
		lemon_allocator_free(lemon, self->items);
		self->items = items;
		self->count = count;
		self->length = length;
	}

	return lemon->l_nil;
}

static struct lobject *
ltable_del_item(struct lemon *lemon,
                struct ltable *self,
                struct lobject *name)
{
	struct lobject *value;

	value = table_delete(lemon,
	                     name,
	                     self->items,
	                     self->length,
	                     table_cmp,
	                     table_hash);
	if (value) {
		self->count -= 1;
	}

	return value;
}

static struct lobject *
ltable_keys(struct lemon *lemon, struct ltable *self)
{
	int i;
	struct slot *items;
	struct lobject *array;
	struct lobject *value;

	items = self->items;
	array = larray_create(lemon, 0, NULL);
	if (!array) {
		return NULL;
	}
	for (i = 0; i < self->length; i++) {
		if (items[i].key == NULL ||
		    items[i].key == lemon->l_sentinel ||
		    items[i].value == lemon->l_sentinel)
		{
			continue;
		}
		value = items[i].key;
		if (!larray_append(lemon, array, 1, &value)) {
			return NULL;
		}
	}

	return array;
}

static struct lobject *
ltable_get_keys_attr(struct lemon *lemon,
                     struct lobject *self,
                     int argc, struct lobject *argv[])
{
	struct lobject *array;

	array = ltable_keys(lemon, (struct ltable *)self);
	if (!array) {
		return lemon->l_out_of_memory;
	}

	return array;
}

static struct lobject *
ltable_get_attr(struct lemon *lemon,
                struct ltable *self, struct lobject *name)
{
	const char *cstr;

	cstr = lstring_to_cstr(lemon, name);
	if (strcmp(cstr, "__iterator__") == 0) {
		struct lobject *keys;
		keys = ltable_keys(lemon, self);

		return lobject_get_attr(lemon, keys, name);
	}
	if (strcmp(cstr, "keys") == 0) {
		return lfunction_create(lemon,
		                        name,
		                        (struct lobject *)self,
		                        ltable_get_keys_attr);
	}

	return NULL;
}

static struct lobject *
ltable_string(struct lemon *lemon, struct ltable *self)
{
	int i;
	int count;
	char *buffer;
	const char *fmt;
	unsigned long offset;
	unsigned long length;
	unsigned long maxlen;

	struct slot *items;
	struct lobject *string;

	struct lobject *key;
	struct lobject *value;

	count = 0;
	items = self->items;
	maxlen = 256;
	buffer = lemon_allocator_alloc(lemon, maxlen);
	if (!buffer) {
		return NULL;
	}
	offset = snprintf(buffer, sizeof(buffer), "{");
	for (i = 0; i < self->length; i++) {
		if (items[i].key == NULL ||
		    items[i].key == lemon->l_sentinel ||
		    items[i].value == lemon->l_sentinel)
		{
			continue;
		}

		count += 1;
		if (lobject_is_string(lemon, items[i].key) &&
		    lobject_is_string(lemon, items[i].value))
		{
			key = items[i].key;
			value = items[i].value;
			if (count < self->count) {
				fmt = "'%s': '%s', ";
			} else {
				fmt = "'%s': '%s'";
			}
		} else if (lobject_is_string(lemon, items[i].key)) {
			key = items[i].key;
			value = lobject_string(lemon, items[i].value);
			if (count < self->count) {
				fmt = "'%s': %s, ";
			} else {
				fmt = "'%s': %s";
			}
		} else if (lobject_is_string(lemon, items[i].value)) {
			key = lobject_string(lemon, items[i].key);
			value = items[i].value;
			if (count < self->count) {
				fmt = "%s: '%s', ";
			} else {
				fmt = "%s: '%s'";
			}
		} else {
			key = lobject_string(lemon, items[i].key);
			value = lobject_string(lemon, items[i].value);
			if (count < self->count) {
				fmt = "%s: %s, ";
			} else {
				fmt = "%s: %s";
			}
		}

again:
		length = snprintf(buffer + offset,
		                  maxlen - offset,
		                  fmt,
		                  lstring_to_cstr(lemon, key),
		                  lstring_to_cstr(lemon, value));
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
	buffer[offset++] = '}';

	string = lstring_create(lemon, buffer, offset);
	lemon_allocator_free(lemon, buffer);

	return string;
}

static struct lobject *
ltable_mark(struct lemon *lemon, struct ltable *self)
{
	int i;
	struct slot *items;

	items = self->items;
	for (i = 0; i < self->length; i++) {
		if (items[i].key == NULL ||
		    items[i].key == lemon->l_sentinel ||
		    items[i].value == lemon->l_sentinel)
		{
			continue;
		}
		lobject_mark(lemon, items[i].key);
		lobject_mark(lemon, items[i].value);
	}

	return NULL;
}

static struct lobject *
ltable_destroy(struct lemon *lemon, struct ltable *self)
{
	lemon_allocator_free(lemon, self->items);

	return NULL;
}

static struct lobject *
ltable_method(struct lemon *lemon,
              struct lobject *self,
              int method, int argc, struct lobject *argv[])
{
#define cast(a) ((struct ltable *)(a))

	switch (method) {
	case LOBJECT_METHOD_EQ:
		return ltable_eq(lemon, cast(self), cast(argv[0]));

	case LOBJECT_METHOD_GET_ITEM:
		return ltable_get_item(lemon, cast(self), argv[0]);

	case LOBJECT_METHOD_HAS_ITEM:
		return ltable_has_item(lemon, cast(self), argv[0]);

	case LOBJECT_METHOD_SET_ITEM:
		return ltable_set_item(lemon, cast(self), argv[0], argv[1]);

	case LOBJECT_METHOD_DEL_ITEM:
		return ltable_del_item(lemon, cast(self), argv[0]);

	case LOBJECT_METHOD_MAP_ITEM:
		return ltable_map_item(lemon, cast(self));

	case LOBJECT_METHOD_GET_ATTR:
		return ltable_get_attr(lemon, cast(self), argv[0]);

	case LOBJECT_METHOD_STRING:
		return ltable_string(lemon, cast(self));

	case LOBJECT_METHOD_LENGTH:
		return linteger_create_from_long(lemon, cast(self)->count);

	case LOBJECT_METHOD_MARK:
		return ltable_mark(lemon, cast(self));

	case LOBJECT_METHOD_DESTROY:
		return ltable_destroy(lemon, cast(self));

	default:
		return lobject_default(lemon, self, method, argc, argv);
	}
}

void *
ltable_create(struct lemon *lemon)
{
	size_t size;
	struct ltable *self;

	self = lobject_create(lemon, sizeof(*self), ltable_method);
	if (self) {
		size = sizeof(struct slot) * 3;
		self->items = lemon_allocator_alloc(lemon, size);
		if (!self->items) {
			return NULL;
		}
		memset(self->items, 0, size);
		self->length = 3;
	}

	return self;
}

struct ltype *
ltable_type_create(struct lemon *lemon)
{
	return ltype_create(lemon, "table", ltable_method, NULL);
}
