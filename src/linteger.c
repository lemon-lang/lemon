#include "lemon.h"
#include "lnumber.h"
#include "lstring.h"
#include "linteger.h"

#include <stdio.h>
#include <limits.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define SMALLINT_MAX (long)(ULONG_MAX >> 2)
#define SMALLINT_MIN (long)(-SMALLINT_MAX)

void *
linteger_create(struct lemon *lemon, int digits);

void *
linteger_create_object_from_long(struct lemon *lemon, long value);

void *
linteger_create_object_from_integer(struct lemon *lemon,
                                    struct lobject *integer);

static void
normalize(struct lemon *lemon, struct linteger *a)
{
	a->ndigits = extend_length(a->length, a->digits);;

	if (a->ndigits == 0) {
		a->sign = 1;
	}
}

int
linteger_cmp(struct lemon *lemon, struct lobject *a, struct lobject *b)
{
	struct linteger *ia;
	struct linteger *ib;

	if (!lobject_is_pointer(lemon, a) && !lobject_is_pointer(lemon, b)) {
		long la;
		long lb;

		la = linteger_to_long(lemon, a);
		lb = linteger_to_long(lemon, b);

		if (la > lb) {
			return 1;
		}

		if (la < lb) {
			return -1;
		}

		return 0;
	}

	assert(lobject_is_pointer(lemon, a) && lobject_is_pointer(lemon, b));

	ia = (struct linteger *)a;
	ib = (struct linteger *)b;

	if (ia->sign == 1 && ib->sign == 0) {
		return 1;
	}

	if (ia->sign == 0 && ib->sign == 1) {
		return -1;
	}

	if (ia->ndigits > ib->ndigits) {
		return 1;
	}

	if (ia->ndigits < ib->ndigits) {
		return -1;
	}

	if (ia->sign) {
		return extend_cmp(ia->ndigits, ia->digits, ib->digits);
	} else {
		return extend_cmp(ia->ndigits, ib->digits, ia->digits);
	}

	return 0;
}

/*
 * CERT Coding Standard INT32-C Integer Overflow Check Algorithms
 */
static struct lobject *
linteger_add(struct lemon *lemon, struct lobject *a, struct lobject *b)
{
	int ndigits;
	struct linteger *ia;
	struct linteger *ib;
	struct linteger *ic;
	unsigned long carry;

	if (!lobject_is_pointer(lemon, a) && !lobject_is_pointer(lemon, b)) {
		long la;
		long lb;

		la = linteger_to_long(lemon, a);
		lb = linteger_to_long(lemon, b);

		if ((lb > 0 && la > (LONG_MAX - lb)) ||
		    (lb < 0 && la < (LONG_MIN - lb)))
		{
			goto promot;
		}

		return linteger_create_from_long(lemon, la + lb);
	}
promot:
	ia = linteger_create_object_from_integer(lemon, a);
	if (!ia) {
		return NULL;
	}

	ib = linteger_create_object_from_integer(lemon, b);
	if (!ib) {
		return NULL;
	}

	if (ia->sign == ib->sign) {
		if (ia->ndigits > ib->ndigits) {
			ndigits = ia->ndigits + 1;
		} else {
			ndigits = ib->ndigits + 1;
		}
		ic = linteger_create(lemon, ndigits);

		carry = extend_add(ic->digits,
		                   ia->ndigits,
		                   ia->digits,
		                   ib->ndigits,
		                   ib->digits,
		                   0);
		ic->digits[ic->length - 1] = (extend_t)(carry % EXTEND_BASE);
		ic->sign = ia->sign;
	} else if (linteger_cmp(lemon,
	                        (struct lobject *)ia,
	                        (struct lobject *)ib) > 0)
	{
		ic = linteger_create(lemon, ia->ndigits);
		extend_sub(ic->digits,
		           ia->ndigits,
		           ia->digits,
		           ib->ndigits,
		           ib->digits,
		           0);
		ic->sign = ia->sign;
	} else {
		ic = linteger_create(lemon, ib->ndigits);
		extend_sub(ic->digits,
		           ib->ndigits,
		           ib->digits,
		           ia->ndigits,
		           ia->digits,
		           0);
		ic->sign = ib->sign;
	}

	normalize(lemon, ic);
	return (struct lobject *)ic;
}

static struct lobject *
linteger_sub(struct lemon *lemon, struct lobject *a, struct lobject *b)
{
	int ndigits;
	struct linteger *ia;
	struct linteger *ib;
	struct linteger *ic;
	unsigned long carry;

	if (!lobject_is_pointer(lemon, a) && !lobject_is_pointer(lemon, b)) {
		long la;
		long lb;

		la = linteger_to_long(lemon, a);
		lb = linteger_to_long(lemon, b);

		if ((lb > 0 && la < LONG_MIN + lb) ||
		    (lb < 0 && la > LONG_MAX + lb))
		{
			goto promot;
		}

		return linteger_create_from_long(lemon, la - lb);
	}

promot:
	ia = linteger_create_object_from_integer(lemon, a);
	if (!ia) {
		return NULL;
	}

	ib = linteger_create_object_from_integer(lemon, b);
	if (!ib) {
		return NULL;
	}

	if (ia->sign != ib->sign) {
		if (ia->ndigits > ib->ndigits) {
			ndigits = ia->ndigits + 1;
		} else {
			ndigits = ib->ndigits + 1;
		}
		ic = linteger_create(lemon, ndigits);

		carry = extend_add(ic->digits,
		                   ia->ndigits,
		                   ia->digits,
		                   ib->ndigits,
		                   ib->digits,
		                   0);
		ic->digits[ic->length - 1] = (extend_t)(carry & EXTEND_BASE);
		ic->sign = ia->sign;
	} else if (linteger_cmp(lemon,
	                        (struct lobject *)ia,
	                        (struct lobject *)ib) > 0)
	{
		ic = linteger_create(lemon, ia->ndigits);
		extend_sub(ic->digits,
		           ia->ndigits,
		           ia->digits,
		           ib->ndigits,
		           ib->digits,
		           0);
		ic->sign = ia->sign;
	} else {
		ic = linteger_create(lemon, ib->ndigits);
		extend_sub(ic->digits,
		           ib->ndigits,
		           ib->digits,
		           ia->ndigits,
		           ia->digits,
		           0);
		ic->sign = -ib->sign;
	}

	normalize(lemon, ic);
	return (struct lobject *)ic;
}

static struct lobject *
linteger_mul(struct lemon *lemon, struct lobject *a, struct lobject *b)
{
	struct linteger *ia;
	struct linteger *ib;
	struct linteger *ic;

	if (!lobject_is_pointer(lemon, a) && !lobject_is_pointer(lemon, b)) {
		long la;
		long lb;

		la = linteger_to_long(lemon, a);
		lb = linteger_to_long(lemon, b);

		if (la > 0) {  /* la is positive */
			if (lb > 0) {  /* la and lb are positive */
				if (la > (LONG_MAX / lb)) {
					goto promot;
				}
			} else { /* la positive, lb nonpositive */
				if (lb < (LONG_MIN / la)) {
					goto promot;
				}
			} /* la positive, lb nonpositive */
		} else { /* la is nonpositive */
			if (lb > 0) { /* la is nonpositive, lb is positive */
				if (la < (LONG_MIN / lb)) {
					goto promot;
				}
			} else { /* la and lb are nonpositive */
				if ( (la != 0) && (lb < (LONG_MAX / la))) {
					goto promot;
				}
			} /* End if la and lb are nonpositive */
		} /* End if la is nonpositive */

		return linteger_create_from_long(lemon, la * lb);
	}

promot:
	ia = linteger_create_object_from_integer(lemon, a);
	if (!ia) {
		return NULL;
	}

	ib = linteger_create_object_from_integer(lemon, b);
	if (!ib) {
		return NULL;
	}

	ic = linteger_create(lemon, ia->ndigits + ib->ndigits);
	extend_mul(ic->digits,
	           ia->ndigits,
	           ia->digits,
	           ib->ndigits,
	           ib->digits);
	ic->sign = ia->sign == ib->sign ? 1 : -1;

	normalize(lemon, ic);
	return (struct lobject *)ic;
}

static struct lobject *
linteger_div(struct lemon *lemon, struct lobject *a, struct lobject *b)
{
	struct linteger *ia;
	struct linteger *ib;
	struct linteger *ic;
	struct linteger *id;
	struct linteger *ie;

	if (!lobject_is_pointer(lemon, a) && !lobject_is_pointer(lemon, b)) {
		long la;
		long lb;

		la = linteger_to_long(lemon, a);
		lb = linteger_to_long(lemon, b);

		if (lb == 0) {
			const char *fmt;

			fmt = "divide by zero '%@/0'";
			return lobject_error_arithmetic(lemon, fmt, a);
		}

		if ((la == LONG_MIN) && (lb == -1)) {
			goto promot;
		}

		return linteger_create_from_long(lemon, la / lb);
	}

promot:
	ia = linteger_create_object_from_integer(lemon, a);
	if (!ia) {
		return NULL;
	}

	ib = linteger_create_object_from_integer(lemon, b);
	if (!ib) {
		return NULL;
	}

	ic = linteger_create(lemon, ia->ndigits);
	if (!ic) {
		return NULL;
	}
	id = linteger_create(lemon, ib->ndigits);
	if (!id) {
		return NULL;
	}
	ie = linteger_create(lemon, ia->ndigits + ib->ndigits + 2);
	if (!ie) {
		return NULL;
	}
	extend_div(ic->digits,
	           ia->ndigits,
	           ia->digits,
	           ib->ndigits,
	           ib->digits,
	           id->digits,
	           ie->digits);
	ic->sign = ia->sign == ib->sign ? 1 : -1;

	normalize(lemon, ic);
	return (struct lobject *)ic;
}

static struct lobject *
linteger_mod(struct lemon *lemon, struct lobject *a, struct lobject *b)
{
	struct linteger *ia;
	struct linteger *ib;
	struct linteger *ic;
	struct linteger *id;
	struct linteger *ie;

	if (!lobject_is_pointer(lemon, a) && !lobject_is_pointer(lemon, b)) {
		long la;
		long lb;

		la = linteger_to_long(lemon, a);
		lb = linteger_to_long(lemon, b);

		if (lb == 0) {
			const char *fmt;

			fmt = "divide by zero '%@/0'";
			return lobject_error_arithmetic(lemon, fmt, a);
		}

		if ((la == LONG_MIN) && (lb == -1)) {
			goto promot;
		}

		return linteger_create_from_long(lemon, la % lb);
	}
promot:
	ia = linteger_create_object_from_integer(lemon, a);
	if (!ia) {
		return NULL;
	}

	ib = linteger_create_object_from_integer(lemon, b);
	if (!ib) {
		return NULL;
	}

	ic = linteger_create(lemon, ia->ndigits);
	if (!ic) {
		return NULL;
	}
	id = linteger_create(lemon, ib->ndigits);
	if (!id) {
		return NULL;
	}
	ie = linteger_create(lemon, ia->ndigits + ib->ndigits + 2);
	if (!ie) {
		return NULL;
	}
	extend_div(ic->digits,
	           ia->ndigits,
	           ia->digits,
	           ib->ndigits,
	           ib->digits,
	           id->digits,
	           ie->digits);
	ic->sign = ia->sign == ib->sign ? 1 : -1;

	normalize(lemon, ic);
	return (struct lobject *)id;
}

static struct lobject *
linteger_neg(struct lemon *lemon, struct lobject *a)
{
	struct linteger *ia;
	struct linteger *ic;

	if (!lobject_is_pointer(lemon, a)) {
		long la;

		la = linteger_to_long(lemon, a);

		return linteger_create_from_long(lemon, -la);
	}

	ia = (struct linteger *)a;
	ic = linteger_create(lemon, ia->ndigits);
	if (!ic) {
		return NULL;
	}
	memcpy(ic->digits, ia->digits, ia->ndigits * sizeof(extend_t));
	ic->sign = ia->sign ? 0 : 1;
	ic->ndigits = ia->ndigits;

	return (struct lobject *)ic;
}

static struct lobject *
linteger_shl(struct lemon *lemon, struct lobject *a, struct lobject *b)
{
	long s;
	int ndigits;
	struct linteger *ia;
	struct linteger *ic;

	if (!lobject_is_pointer(lemon, a) && !lobject_is_pointer(lemon, b)) {
		long la;
		long lb;

		la = linteger_to_long(lemon, a);
		lb = linteger_to_long(lemon, b);

		if (lb < 0) {
			const char *fmt;

			fmt = "negative shift '%@ << %@'";
			return lobject_error_arithmetic(lemon, fmt, a, b);
		}

		if ((lb >= LONG_BIT) || (la > (LONG_MAX >> lb))) {
			goto promot;
		}

		return linteger_create_from_long(lemon, la << lb);
	}
promot:
	ia = linteger_create_object_from_integer(lemon, a);
	if (!ia) {
		return NULL;
	}

	s = (linteger_to_long(lemon, b) + EXTEND_BITS-1) & ~(EXTEND_BITS-1);
	ndigits = ia->ndigits + (int)s / EXTEND_BITS;
	ic = linteger_create(lemon, ndigits);
	if (!ic) {
		return NULL;
	}
	extend_shl(ic->length, ic->digits, ia->ndigits, ia->digits, (int)s, 0);
	ic->sign = ia->sign;

	normalize(lemon, ic);
	return (struct lobject *)ic;
}

static struct lobject *
linteger_shr(struct lemon *lemon, struct lobject *a, struct lobject *b)
{
	int s;
	struct linteger *ia;
	struct linteger *ic;

	if (!lobject_is_pointer(lemon, a) && !lobject_is_pointer(lemon, b)) {
		long la;
		long lb;

		la = linteger_to_long(lemon, a);
		lb = linteger_to_long(lemon, b);

		if (lb < 0) {
			const char *fmt;

			fmt = "negative shift '%@ >> %@'";
			return lobject_error_arithmetic(lemon, fmt, a, b);
		}

		return linteger_create_from_long(lemon, la >> lb);
	}

	ia = linteger_create_object_from_integer(lemon, a);
	if (!ia) {
		return NULL;
	}

	s = (int)linteger_to_long(lemon, b);
	if (s >= 8 * ia->ndigits) {
		return linteger_create(lemon, 0);
	}

	ic = linteger_create(lemon, ia->ndigits - s/EXTEND_BITS);
	if (!ic) {
		return NULL;
	}
	extend_shr(ic->length, ic->digits, ia->ndigits, ia->digits, s, 0);
	ic->sign = ia->sign;

	normalize(lemon, ic);
	return (struct lobject *)ic;
}

static struct lobject *
linteger_bitwise_not(struct lemon *lemon, struct lobject *a)
{
	extend_t one[1];
	struct linteger *ia;
	struct linteger *ic;
	unsigned long carry;

	if (!lobject_is_pointer(lemon, a)) {
		long la;

		la = linteger_to_long(lemon, a);
		return linteger_create_from_long(lemon, ~la);
	}

	ia = (struct linteger *)a;
	ic = linteger_create(lemon, ia->ndigits + 1);
	if (!ic) {
		return NULL;
	}

	one[0] = 1;
	carry = extend_add(ic->digits, ia->ndigits, ia->digits, 1, one, 0);
	ic->digits[ic->length - 1] = (extend_t)(carry % EXTEND_BASE);
	ic->sign = ia->sign ? 0 : 1;

	normalize(lemon, ic);
	return (struct lobject *)ic;
}

static struct lobject *
linteger_bitwise_and(struct lemon *lemon, struct lobject *a, struct lobject *b)
{
	int i;
	int ndigits;
	struct linteger *ia;
	struct linteger *ib;
	struct linteger *ic;

	if (!lobject_is_pointer(lemon, a) && !lobject_is_pointer(lemon, b)) {
		long la;
		long lb;

		la = linteger_to_long(lemon, a);
		lb = linteger_to_long(lemon, b);

		return linteger_create_from_long(lemon, la & lb);
	}

	ia = linteger_create_object_from_integer(lemon, a);
	if (!ia) {
		return NULL;
	}

	ib = linteger_create_object_from_integer(lemon, b);
	if (!ib) {
		return NULL;
	}

	if (ia->ndigits < ib->ndigits) {
		ndigits = ia->ndigits;
	} else {
		ndigits = ib->ndigits;
	}
	ic = linteger_create(lemon, ndigits);
	if (!ic) {
		return NULL;
	}
	ic->sign = ia->sign;

	for (i = 0; i < ic->ndigits; i++) {
		ic->digits[i] = ia->digits[i] & ib->digits[i];
	}

	normalize(lemon, ic);

	return (struct lobject *)ic;
}

static struct lobject *
linteger_bitwise_xor(struct lemon *lemon, struct lobject *a, struct lobject *b)
{
	int i;
	int min;
	int max;
	struct linteger *ia;
	struct linteger *ib;
	struct linteger *ic;
	struct linteger *x;

	if (!lobject_is_pointer(lemon, a) && !lobject_is_pointer(lemon, b)) {
		long la;
		long lb;

		la = linteger_to_long(lemon, a);
		lb = linteger_to_long(lemon, b);

		return linteger_create_from_long(lemon, la ^ lb);
	}

	ia = linteger_create_object_from_integer(lemon, a);
	if (!ia) {
		return NULL;
	}

	ib = linteger_create_object_from_integer(lemon, b);
	if (!ib) {
		return NULL;
	}

	if (ia->ndigits > ib->ndigits) {
		max = ia->ndigits;
		min = ib->ndigits;
		x = ia;
	} else {
		min = ia->ndigits;
		max = ib->ndigits;
		x = ib;
	}
	ic = linteger_create(lemon, max);
	if (!ic) {
		return NULL;
	}
	ic->sign = ia->sign;

	for (i = 0; i < min; i++) {
		ic->digits[i] = ia->digits[i] ^ ib->digits[i];
	}

	for (; i < max; i++) {
		ic->digits[i] = x->digits[i] ^ 0;
	}

	normalize(lemon, ic);
	return (struct lobject *)ic;
}

static struct lobject *
linteger_bitwise_or(struct lemon *lemon, struct lobject *a, struct lobject *b)
{
	int i;
	int min;
	int max;
	struct linteger *ia;
	struct linteger *ib;
	struct linteger *ic;
	struct linteger *x;

	if (!lobject_is_pointer(lemon, a) && !lobject_is_pointer(lemon, b)) {
		long la;
		long lb;

		la = linteger_to_long(lemon, a);
		lb = linteger_to_long(lemon, b);

		return linteger_create_from_long(lemon, la | lb);
	}

	ia = linteger_create_object_from_integer(lemon, a);
	if (!ia) {
		return NULL;
	}

	ib = linteger_create_object_from_integer(lemon, b);
	if (!ib) {
		return NULL;
	}

	if (ia->ndigits > ib->ndigits) {
		x = ia;
		min = ib->ndigits;
		max = ia->ndigits;
	} else {
		x = ib;
		max = ib->ndigits;
		min = ia->ndigits;
	}
	ic = linteger_create(lemon, max);
	if (!ic) {
		return NULL;
	}
	ic->sign = ia->sign;

	for (i = 0; i < min; i++) {
		ic->digits[i] = ia->digits[i] | ib->digits[i];
	}

	for (; i < max; i++) {
		ic->digits[i] = x->digits[i];
	}

	normalize(lemon, ic);
	return (struct lobject *)ic;
}

static struct lobject *
linteger_string(struct lemon *lemon, struct lobject *self)
{
	char buffer[40960];
	char *p;
	int size;
	struct linteger *a;
	struct linteger *integer;

	if (!lobject_is_pointer(lemon, self)) {
		snprintf(buffer,
			 sizeof(buffer),
			 "%ld",
			 linteger_to_long(lemon, self));
	} else {
		integer = (struct linteger *)self;
		if (integer->ndigits == 0) {
			return lstring_create(lemon, "0", 1);
		}

		if (integer->sign == 0) {
			buffer[0] = '-';
			p = buffer + 1;
			size = sizeof(buffer) - 1;
		} else {
			p = buffer;
			size = sizeof(buffer);
		}
		a = linteger_create(lemon, integer->ndigits);
		memcpy(a->digits,
		       integer->digits,
		       integer->ndigits * sizeof(extend_t));
		a->ndigits = integer->ndigits;
		extend_to_str(a->ndigits, a->digits, p, size, 10);
	}
	buffer[sizeof(buffer) - 1] = '\0';

	return lstring_create(lemon, buffer, strlen(buffer));
}

struct lobject *
linteger_method(struct lemon *lemon,
                struct lobject *self,
                int method, int argc, struct lobject *argv[])
{

#define binop(op) do {                                                         \
	if (lobject_is_integer(lemon, argv[0])) {                              \
		return linteger_ ## op (lemon, self, argv[0]);                 \
	}                                                                      \
	if (lobject_is_number(lemon, argv[0])) {                               \
		struct lobject *number;                                        \
		if (lobject_is_pointer(lemon, self)) {                         \
			const char *cstr;                                      \
			struct lobject *string;                                \
			string = linteger_string(lemon, self);                 \
			cstr = lstring_to_cstr(lemon, string);                 \
			number = lnumber_create_from_cstr(lemon, cstr);        \
		} else {                                                       \
			long value;                                            \
			value = linteger_to_long(lemon, self);                 \
			number = lnumber_create_from_long(lemon, value);       \
		}                                                              \
		if (!number) {                                                 \
			return NULL;                                           \
		}                                                              \
		return lobject_method_call(lemon, number, method, argc, argv); \
	}                                                                      \
	return lobject_default(lemon, self, method, argc, argv);               \
} while (0)

#define cmpop(op) do {                                                         \
	if (lobject_is_integer(lemon, argv[0])) {                              \
		struct lobject *a = self;                                      \
		struct lobject *b = argv[0];                                   \
		if (lobject_is_pointer(lemon, a) ||                            \
		    lobject_is_pointer(lemon, b))                              \
		{                                                              \
			a = linteger_create_object_from_integer(lemon, a);     \
			if (!a) {                                              \
				return NULL;                                   \
			}                                                      \
			b = linteger_create_object_from_integer(lemon, b);     \
			if (!b) {                                              \
				return NULL;                                   \
			}                                                      \
		}                                                              \
		if (linteger_cmp(lemon, a, b) op 0) {                          \
			return lemon->l_true;                                  \
		}                                                              \
		return lemon->l_false;                                         \
	}                                                                      \
	if (lobject_is_number(lemon, argv[0])) {                               \
		struct lobject *number;                                        \
		if (lobject_is_pointer(lemon, self)) {                         \
			const char *cstr;                                      \
			struct lobject *string;                                \
			string = linteger_string(lemon, self);                 \
			cstr = lstring_to_cstr(lemon, string);                 \
			number = lnumber_create_from_cstr(lemon, cstr);        \
		} else {                                                       \
			long value;                                            \
			value = linteger_to_long(lemon, self);                 \
			number = lnumber_create_from_long(lemon, value);       \
		}                                                              \
		if (!number) {                                                 \
			return NULL;                                           \
		}                                                              \
		return lobject_method_call(lemon, number, method, argc, argv); \
	}                                                                      \
	return lobject_default(lemon, self, method, argc, argv);               \
} while (0)

	switch (method) {
	case LOBJECT_METHOD_ADD:
		binop(add);

	case LOBJECT_METHOD_SUB:
		binop(sub);

	case LOBJECT_METHOD_MUL:
		binop(mul);

	case LOBJECT_METHOD_DIV:
		binop(div);

	case LOBJECT_METHOD_MOD:
		binop(mod);

	case LOBJECT_METHOD_POS:
		return self;

	case LOBJECT_METHOD_NEG:
		return linteger_neg(lemon, self);

	case LOBJECT_METHOD_SHL:
		binop(shl);

	case LOBJECT_METHOD_SHR:
		binop(shr);

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

	case LOBJECT_METHOD_BITWISE_NOT:
		return linteger_bitwise_not(lemon, self);

	case LOBJECT_METHOD_BITWISE_AND:
		binop(bitwise_and);

	case LOBJECT_METHOD_BITWISE_XOR:
		binop(bitwise_xor);

	case LOBJECT_METHOD_BITWISE_OR:
		binop(bitwise_or);

	case LOBJECT_METHOD_HASH:
		return self;

	case LOBJECT_METHOD_NUMBER:
		return self;

	case LOBJECT_METHOD_INTEGER:
		return self;

	case LOBJECT_METHOD_BOOLEAN:
		if (linteger_to_long(lemon, self)) {
			return lemon->l_true;
		}
		return lemon->l_false;

	case LOBJECT_METHOD_STRING:
		return linteger_string(lemon, self);

	default:
		return lobject_default(lemon, self, method, argc, argv);
	}
}

long
linteger_to_long(struct lemon *lemon, struct lobject *object)
{
	uintptr_t sign;
	long value;

	if (lobject_is_pointer(lemon, object)) {
		unsigned long uvalue;
		struct linteger *integer;

		integer = (struct linteger *)object;
		uvalue = extend_to_long(integer->ndigits, integer->digits);
		if (uvalue > LONG_MAX) {
			return LONG_MAX;
		}

		value = uvalue;
		if (integer->sign == -1) {
			value = -value;
		}
	} else {
		sign = ((uintptr_t)object) & 0x2;
		value = ((uintptr_t)object) >> 2;
		if (sign) {
			value = -value;
		}
	}

	return value;
}

void *
linteger_create(struct lemon *lemon, int digits)
{
	size_t size;
	struct linteger *self;

	size = digits * sizeof(extend_t);

	self = lobject_create(lemon, sizeof(*self) + size, linteger_method);
	if (self) {
		self->sign = 1;
		self->length = digits;
		self->ndigits = 1;
	}

	return self;
}

void *
linteger_create_object_from_long(struct lemon *lemon, long value)
{
	struct linteger *self;

	self = linteger_create(lemon, sizeof(long));
	if (self) {
		if (value < 0) {
			self->sign = 0;
			value = -value;
		}
		extend_from_long(self->length, self->digits, value);
		normalize(lemon, self);
	}

	return self;
}

void *
linteger_create_object_from_integer(struct lemon *lemon,
                                    struct lobject *integer)
{
	long value;

	if (lobject_is_pointer(lemon, integer)) {
		return integer;
	}
	value = linteger_to_long(lemon, integer);

	return linteger_create_object_from_long(lemon, value);
}

void *
linteger_create_from_long(struct lemon *lemon, long value)
{
	struct linteger *integer;

	/* create smallint if possible */
	if (value > SMALLINT_MIN && value < SMALLINT_MAX) {
		/*
		 * signed value shifting is undefined behavior in C
		 * so change negative value to postivie and pack 1 sign bit
		 * never use value directly, two's complement is not necessary
		 */
		int sign; /* 0, positive, 1 negative */

		sign = 0;
		if (value < 0) {
			sign = 0x2;
			value = -value;
		}

		integer = (void *)(((uintptr_t)value << 2) | sign | 0x1);

		return integer;
	}

	return linteger_create_object_from_long(lemon, value);
}

void *
linteger_create_from_cstr(struct lemon *lemon, const char *cstr)
{
	long value;
	struct linteger *self;

	value = strtol(cstr, NULL, 0);
	if (value != LONG_MAX && value != LONG_MIN) {
		return linteger_create_from_long(lemon, value);
	}

	self = linteger_create(lemon, (int)strlen(cstr));
	if (self) {
		if (cstr[0] == '0') {
			if (cstr[1] == 'x' || cstr[1] == 'X') {
				extend_from_str(self->length,
				                self->digits,
				                cstr + 2,
				                16,
				                NULL);
			} else {
				extend_from_str(self->length,
				                self->digits,
				                cstr + 1,
				                8,
				                NULL);
			}
		} else {
			extend_from_str(self->length,
			                self->digits,
			                cstr,
			                10,
			                NULL);
		}
		normalize(lemon, self);
	}

	return self;
}

static struct lobject *
linteger_type_method(struct lemon *lemon,
                     struct lobject *self,
                     int method, int argc, struct lobject *argv[])
{
	switch (method) {
	case LOBJECT_METHOD_CALL: {
		long value;

		if (argc < 1) {
			return linteger_create_from_long(lemon, 0);
		}

		value = 0;
		if (lobject_is_number(lemon, argv[0])) {
			value = (long)lnumber_to_double(lemon, argv[0]);
		}

		if (lobject_is_string(lemon, argv[0])) {
			const char *cstr;

			cstr = lstring_to_cstr(lemon, argv[0]);
			value = strtol(cstr, NULL, 10);
		}

		return linteger_create_from_long(lemon, value);
	}

	case LOBJECT_METHOD_CALLABLE:
		return lemon->l_true;

	default:
		return lobject_default(lemon, self, method, argc, argv);
	}
}

struct ltype *
linteger_type_create(struct lemon *lemon)
{
	struct ltype *type;

	type = ltype_create(lemon,
	                    "integer",
	                    linteger_method,
	                    linteger_type_method);
	if (type) {
		lemon_add_global(lemon, "integer", type);
	}

	return type;
}

