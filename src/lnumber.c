#include "lemon.h"
#include "lnumber.h"
#include "lstring.h"
#include "linteger.h"
#include "lexception.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void *
lnumber_create_from_double(struct lemon *lemon, double value);

static struct lobject *
lnumber_div(struct lemon *lemon, struct lnumber *a, struct lobject *b)
{
	double value;
	if (lobject_is_number(lemon, b)) {
		value = ((struct lnumber *)b)->value;
		if (value == 0.0) {
			return lobject_error_arithmetic(lemon,
			                                "divide by zero '%@/0'",
			                                a);
		}

		return lnumber_create_from_double(lemon, a->value / value);
	}

	if (lobject_is_integer(lemon, b)) {
		if (linteger_to_long(lemon, b) == 0) {
			return lobject_error_arithmetic(lemon,
			                                "divide by zero '%@/0'",
			                                a);
		}
		value = linteger_to_long(lemon, b);

		return lnumber_create_from_double(lemon, a->value / value);
	}

	return lobject_default(lemon,
	                       (struct lobject *)a,
	                       LOBJECT_METHOD_DIV, 1, &b);
}

static struct lobject *
lnumber_destroy(struct lemon *lemon, struct lnumber *lnumber)
{
	return NULL;
}

static struct lobject *
lnumber_string(struct lemon *lemon, struct lnumber *lnumber)
{
	char buffer[256];

	snprintf(buffer, sizeof(buffer), "%f", lnumber->value);
	buffer[sizeof(buffer) - 1] = '\0';

	return lstring_create(lemon, buffer, strlen(buffer));
}

static struct lobject *
lnumber_method(struct lemon *lemon,
               struct lobject *self,
               int method, int argc, struct lobject *argv[])
{
#define cast(a) ((struct lnumber *)(a))

#define binop(op) do {                                             \
	double value;                                              \
	if (lobject_is_number(lemon, argv[0])) {                   \
		value = cast(argv[0])->value;                      \
		value = cast(self)->value op value;                \
		return lnumber_create_from_double(lemon, value);   \
	}                                                          \
	if (lobject_is_integer(lemon, argv[0])) {                  \
		value = (double)linteger_to_long(lemon, argv[0]);  \
		value = cast(self)->value op value;                \
		return lnumber_create_from_double(lemon, value);   \
	}                                                          \
	return lobject_default(lemon, self, method, argc, argv);   \
} while (0)

#define cmpop(op) do {                                             \
	double value;                                              \
	if (lobject_is_number(lemon, argv[0])) {                   \
		value = cast(argv[0])->value;                      \
		if (cast(self)->value op value) {                  \
			return lemon->l_true;                      \
		}                                                  \
		return lemon->l_false;                             \
	}                                                          \
	if (lobject_is_integer(lemon, argv[0])) {                  \
		value = (double)linteger_to_long(lemon, argv[0]);  \
		if (cast(self)->value op value) {                  \
			return lemon->l_true;                      \
		}                                                  \
		return lemon->l_false;                             \
	}                                                          \
	return lobject_default(lemon, self, method, argc, argv);   \
} while (0)

	switch (method) {
	case LOBJECT_METHOD_ADD:
		binop(+);

	case LOBJECT_METHOD_SUB:
		binop(-);

	case LOBJECT_METHOD_MUL:
		binop(*);

	case LOBJECT_METHOD_DIV:
		return lnumber_div(lemon, cast(self), argv[0]);

	case LOBJECT_METHOD_MOD: {
		double value;
		if (lobject_is_number(lemon, argv[0])) {
			value = cast(argv[0])->value;
			value = fmod(cast(self)->value, value);
			return lnumber_create_from_double(lemon, value);
		}
		if (lobject_is_integer(lemon, argv[0])) {
			value = (double)linteger_to_long(lemon, argv[0]);
			value = fmod(cast(self)->value, value);
			return lnumber_create_from_double(lemon, value);
		}
		return lobject_default(lemon, self, method, argc, argv);
	}

	case LOBJECT_METHOD_POS:
		return self;

	case LOBJECT_METHOD_NEG:
		return lnumber_create_from_double(lemon, -cast(self)->value);

	case LOBJECT_METHOD_LT:
		cmpop(<);

	case LOBJECT_METHOD_LE:
		cmpop(<=);

	case LOBJECT_METHOD_EQ:
		cmpop(==);

	case LOBJECT_METHOD_NE:
		cmpop(!=);

	case LOBJECT_METHOD_GE:
		cmpop(>=);

	case LOBJECT_METHOD_GT:
		cmpop(>);

	case LOBJECT_METHOD_HASH:
		return linteger_create_from_long(lemon,
		                                 (long)cast(self)->value);

	case LOBJECT_METHOD_NUMBER:
		return self;

	case LOBJECT_METHOD_BOOLEAN:
		if (cast(self)->value) {
			return lemon->l_true;
		}
		return lemon->l_false;

	case LOBJECT_METHOD_STRING:
		return lnumber_string(lemon, cast(self));

	case LOBJECT_METHOD_INTEGER:
		return linteger_create_from_long(lemon,
		                                 (long)cast(self)->value);

	case LOBJECT_METHOD_DESTROY:
		return lnumber_destroy(lemon, cast(self));

	default:
		return lobject_default(lemon, self, method, argc, argv);
	}
}

double
lnumber_to_double(struct lemon *lemon, struct lobject *self)
{
	return ((struct lnumber *)self)->value;
}

void *
lnumber_create(struct lemon *lemon)
{
	struct lnumber *self;

	self = lobject_create(lemon, sizeof(*self), lnumber_method);

	return self;
}

void *
lnumber_create_from_long(struct lemon *lemon, long value)
{
	struct lnumber *self;

	self = lnumber_create(lemon);
	if (self) {
		self->value = (double)value;
	}

	return self;
}

void *
lnumber_create_from_cstr(struct lemon *lemon, const char *value)
{
	struct lnumber *self;

	self = lnumber_create(lemon);
	if (self) {
		self->value = strtod(value, NULL);
	}

	return self;
}

void *
lnumber_create_from_double(struct lemon *lemon, double value)
{
	struct lnumber *self;

	self = lnumber_create(lemon);
	if (self) {
		self->value = value;
	}

	return self;
}

static struct lobject *
lnumber_type_method(struct lemon *lemon,
                    struct lobject *self,
                    int method, int argc, struct lobject *argv[])
{
	switch (method) {
	case LOBJECT_METHOD_CALL: {
		if (argc < 1) {
			return lnumber_create_from_double(lemon, 0.0);
		}

		if (lobject_is_integer(lemon, argv[0])) {
			double value;

			value = linteger_to_long(lemon, argv[0]);
			return lnumber_create_from_double(lemon, value);
		}

		if (lobject_is_string(lemon, argv[0])) {
			const char *cstr;

			cstr = lstring_to_cstr(lemon, argv[0]);
			return lnumber_create_from_cstr(lemon, cstr);
		}

		return lobject_error_type(lemon,
		                          "number() accept integer or string");
	}

	case LOBJECT_METHOD_CALLABLE:
		return lemon->l_true;

	default:
		return lobject_default(lemon, self, method, argc, argv);
	}
}

struct ltype *
lnumber_type_create(struct lemon *lemon)
{
	struct ltype *type;

	type = ltype_create(lemon,
	                    "number",
	                    lnumber_method,
	                    lnumber_type_method);
	if (type) {
		lemon_add_global(lemon, "number", type);
	}

	return type;
}
