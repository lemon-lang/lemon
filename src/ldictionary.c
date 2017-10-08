#include "lemon.h"
#include "lkarg.h"
#include "ltable.h"
#include "lstring.h"
#include "ldictionary.h"

#include <string.h>

static struct lobject *
ldictionary_eq(struct lemon *lemon,
               struct ldictionary *a,
               struct ldictionary *b)
{
	if (lobject_is_dictionary(lemon, (struct lobject *)b)) {
		return lobject_eq(lemon, a->table, b->table);
	}

	return lemon->l_false;
}

static struct lobject *
ldictionary_get_item(struct lemon *lemon,
                     struct ldictionary *self,
                     struct lobject *key)
{
	return lobject_get_item(lemon, self->table, key);
}

static struct lobject *
ldictionary_has_item(struct lemon *lemon,
                     struct ldictionary *self,
                     struct lobject *key)
{
	return lobject_has_item(lemon, self->table, key);
}

static struct lobject *
ldictionary_set_item(struct lemon *lemon,
                     struct ldictionary *self,
                     struct lobject *key,
                     struct lobject *value)
{
	return lobject_set_item(lemon, self->table, key, value);
}

static struct lobject *
ldictionary_del_item(struct lemon *lemon,
                     struct ldictionary *self,
                     struct lobject *key)
{
	return lobject_del_item(lemon, self->table, key);
}

static struct lobject *
ldictionary_map_item(struct lemon *lemon, struct ldictionary *self)
{
	return lobject_map_item(lemon, self->table);
}

static struct lobject *
ldictionary_get_attr(struct lemon *lemon,
                     struct ldictionary *self,
                     struct lobject *name)
{
	const char *cstr;

	cstr = lstring_to_cstr(lemon, name);
	if (strcmp(cstr, "__iterator__") == 0) {
		return lobject_get_attr(lemon, self->table, name);
	}

	if (strcmp(cstr, "keys") == 0) {
		return lobject_get_attr(lemon, self->table, name);
	}

	return NULL;
}

static struct lobject *
ldictionary_string(struct lemon *lemon, struct ldictionary *self)
{
	return lobject_string(lemon, self->table);
}

static struct lobject *
ldictionary_mark(struct lemon *lemon, struct ldictionary *self)
{
	return lobject_mark(lemon, self->table);
}

static struct lobject *
ldictionary_method(struct lemon *lemon,
                   struct lobject *self,
                   int method, int argc, struct lobject *argv[])
{
#define cast(a) ((struct ldictionary *)(a))

	switch (method) {
	case LOBJECT_METHOD_EQ:
		return ldictionary_eq(lemon, cast(self), cast(argv[0]));

	case LOBJECT_METHOD_GET_ITEM:
		return ldictionary_get_item(lemon, cast(self), argv[0]);

	case LOBJECT_METHOD_HAS_ITEM:
		return ldictionary_has_item(lemon, cast(self), argv[0]);

	case LOBJECT_METHOD_SET_ITEM:
		return ldictionary_set_item(lemon,
		                            cast(self),
		                            argv[0],
		                            argv[1]);

	case LOBJECT_METHOD_DEL_ITEM:
		return ldictionary_del_item(lemon, cast(self), argv[0]);

	case LOBJECT_METHOD_MAP_ITEM:
		return ldictionary_map_item(lemon, cast(self));

	case LOBJECT_METHOD_GET_ATTR:
		return ldictionary_get_attr(lemon, cast(self), argv[0]);

	case LOBJECT_METHOD_STRING:
		return ldictionary_string(lemon, cast(self));

	case LOBJECT_METHOD_LENGTH:
		return lobject_length(lemon, cast(self)->table);

	case LOBJECT_METHOD_MARK:
		return ldictionary_mark(lemon, cast(self));

	default:
		return lobject_default(lemon, self, method, argc, argv);
	}
}

void *
ldictionary_create(struct lemon *lemon, int count, struct lobject *items[])
{
	int i;
	struct ldictionary *self;

	self = lobject_create(lemon, sizeof(*self), ldictionary_method);
	if (self) {
		self->table = ltable_create(lemon);
		if (!self->table) {
			return NULL;
		}
		for (i = 0; i < count; i += 2) {
			struct lobject *key;
			struct lobject *value;

			key = items[i];
			value = items[i+1];
			ldictionary_set_item(lemon, self, key, value);
		}
	}

	return self;
}

static struct lobject *
ldictionary_type_method(struct lemon *lemon,
                       struct lobject *self,
                       int method, int argc, struct lobject *argv[])
{
	switch (method) {
	case LOBJECT_METHOD_CALL: {
		int i;
		struct lkarg *karg;
		struct ldictionary *dictionary;

		dictionary = ldictionary_create(lemon, 0, NULL);
		for (i = 0; i < argc; i++) {
			if (!lobject_is_karg(lemon, argv[i])) {
				const char *fmt;

				fmt = "'%@' accept keyword arguments";
				return lobject_error_type(lemon, fmt, self);
			}
			karg = (struct lkarg *)argv[i];
			ldictionary_set_item(lemon,
					     dictionary,
					     karg->keyword,
					     karg->argument);
		}

		return (struct lobject *)dictionary;
	}

	case LOBJECT_METHOD_CALLABLE:
		return lemon->l_true;

	default:
		return lobject_default(lemon, self, method, argc, argv);
	}
}

struct ltype *
ldictionary_type_create(struct lemon *lemon)
{
	struct ltype *type;

	type = ltype_create(lemon,
	                    "dictionary",
	                    ldictionary_method,
	                    ldictionary_type_method);
	if (type) {
		lemon_add_global(lemon, "dictionary", type);
	}

	return type;
}
