#include "lemon.h"
#include "opcode.h"
#include "machine.h"
#include "generator.h"

#define IS_JZ(a) ((a) && IS_OPCODE(a, OPCODE_JZ))
#define IS_JNZ(a) ((a) && IS_OPCODE(a, OPCODE_JNZ))
#define IS_JMP(a) ((a) && IS_OPCODE(a, OPCODE_JMP))
#define IS_DUP(a) ((a) && IS_OPCODE(a, OPCODE_DUP))
#define IS_POP(a) ((a) && IS_OPCODE(a, OPCODE_POP))
#define IS_NOP(a) ((a) && IS_OPCODE(a, OPCODE_NOP))
#define IS_LOAD(a) ((a) && IS_OPCODE(a, OPCODE_LOAD))
#define IS_STORE(a) ((a) && IS_OPCODE(a, OPCODE_STORE))
#define IS_CONST(a) ((a) && IS_OPCODE(a, OPCODE_CONST))
#define IS_LABEL(a) ((a) && IS_OPCODE(a, -1))
#define IS_OPCODE(a,b) ((a) && (a)->opcode == (b))

static struct generator_code *
peephole_rewrite_unop(struct lemon *lemon,
                      struct generator_code *a)
{
	/*
	 *    CONST a
	 *    OP
	 * ->
	 *    CONST `OP a'
	 */

	int pool;
	struct generator_code *b;
	struct lobject *result;

#define UNOP(m) do {                                                       \
	result = lobject_unop(lemon,                                       \
	                      (m),                                         \
	                      machine_get_const(lemon, b->arg[0]->value)); \
	pool = machine_add_const(lemon, result);                           \
	if (lobject_is_error(lemon, result)) {                             \
		return NULL;                                               \
	}                                                                  \
	a->opcode = OPCODE_CONST;                                          \
	a->arg[0] = generator_make_arg(lemon, 0, 4, pool);                 \
	generator_delete_code(lemon, b);                                   \
	return a;                                                          \
} while(0)

	if (!IS_CONST(a->prev)) {
		return NULL;
	}
	b = a->prev;

	switch (a->opcode) {
	case OPCODE_POS:
		UNOP(LOBJECT_METHOD_POS);
		break;

	case OPCODE_NEG:
		UNOP(LOBJECT_METHOD_NEG);
		break;

	case OPCODE_BNOT:
		UNOP(LOBJECT_METHOD_BITWISE_NOT);
		break;

	case OPCODE_LNOT:
		result = machine_get_const(lemon, b->arg[0]->value);
		if (lobject_boolean(lemon, result) == lemon->l_true) {
			pool = machine_add_const(lemon, lemon->l_false);
		} else {
			pool = machine_add_const(lemon, lemon->l_true);
		}
		a->opcode = OPCODE_CONST;
		a->arg[0] = generator_make_arg(lemon, 0, 4, pool);
		generator_delete_code(lemon, b);

		return a;

	default:
		return NULL;
	}
}

static struct generator_code *
peephole_rewrite_binop(struct lemon *lemon,
                       struct generator_code *c)
{
	/*
	 *    CONST a
	 *    CONST b
	 *    OP
	 * ->
	 *    CONST `a OP b'
	 */

	int pool;
	struct lobject *result;
	struct generator_code *a;
	struct generator_code *b;

#define BINOP(m) do {                                                       \
	result = lobject_binop(lemon,                                       \
	                       (m),                                         \
	                       machine_get_const(lemon, b->arg[0]->value),  \
	                       machine_get_const(lemon, c->arg[0]->value)); \
	pool = machine_add_const(lemon, result);                            \
	if (lobject_is_error(lemon, result)) {                              \
		return NULL;                                                \
	}                                                                   \
	a->opcode = OPCODE_CONST;                                           \
	a->arg[0] = generator_make_arg(lemon, 0, 4, pool);                  \
	generator_delete_code(lemon, b);                                    \
	generator_delete_code(lemon, c);                                    \
	return a;                                                           \
} while(0)

	if (!IS_CONST(c->prev)) {
		return NULL;
	}
	b = c->prev;

	if (!IS_CONST(b->prev)) {
		return NULL;
	}
	a = b->prev;

	switch (a->opcode) {
	case OPCODE_ADD:
		BINOP(LOBJECT_METHOD_ADD);
		break;

	case OPCODE_SUB:
		BINOP(LOBJECT_METHOD_SUB);
		break;

	case OPCODE_MUL:
		BINOP(LOBJECT_METHOD_MUL);
		break;

	case OPCODE_DIV:
		BINOP(LOBJECT_METHOD_DIV);
		break;

	case OPCODE_MOD:
		BINOP(LOBJECT_METHOD_MOD);
		break;

	case OPCODE_SHL:
		BINOP(LOBJECT_METHOD_SHL);
		break;

	case OPCODE_SHR:
		BINOP(LOBJECT_METHOD_SHR);
		break;

	case OPCODE_EQ:
		BINOP(LOBJECT_METHOD_EQ);
		break;

	case OPCODE_NE:
		BINOP(LOBJECT_METHOD_NE);
		break;

	case OPCODE_LT:
		BINOP(LOBJECT_METHOD_LT);
		break;

	case OPCODE_LE:
		BINOP(LOBJECT_METHOD_LE);
		break;

	case OPCODE_GT:
		BINOP(LOBJECT_METHOD_GT);
		break;

	case OPCODE_GE:
		BINOP(LOBJECT_METHOD_GE);
		break;

	case OPCODE_BAND:
		BINOP(LOBJECT_METHOD_BITWISE_AND);
		break;

	case OPCODE_BOR:
		BINOP(LOBJECT_METHOD_BITWISE_OR);
		break;

	default:
		return NULL;
	}
}

static struct generator_code *
peephole_rewrite_jmp_to_next(struct lemon *lemon,
                             struct generator_code *a)
{
	/*
	 *    JMP L1
	 * L1:
	 *    ...
	 * ->
	 * L1:
	 *    ...
	 */

	if (IS_JMP(a)) {
		if (a->next == a->arg[0]->label->code) {
			a = a->next;
			generator_delete_code(lemon, a->prev);

			return a;
		}
	}

	return NULL;
}

static struct generator_code *
peephole_rewrite_jz_or_jnz_to_next(struct lemon *lemon,
                                   struct generator_code *a)
{
	/*
	 *    JZ/JNZ L1
	 * L1:
	 *    ...
	 * ->
	 *    POP
	 * L1:
	 *    g...
	 */

	if (IS_JZ(a) || IS_JNZ(a)) {
		if (a->next == a->arg[0]->label->code) {
			a->arg[0]->label->count -= 1;
			a->opcode = OPCODE_POP;
			a->arg[0] = NULL;

			return a;
		}
	}

	return NULL;
}

static struct generator_code *
peephole_rewrite_dup_jz_to_dup_jz(struct lemon *lemon,
                                  struct generator_code *a)
{
	/*
	 *    DUP
	 *    JZ L1
	 *    ...
	 * L1:
	 *    DUP
	 *    JZ L2
	 *    ...
	 * ->
	 *    DUP
	 *    JZ L2
	 *    ...
	 * L1:
	 *    DUP
	 *    JZ L2
	 *    ...
	 */

	struct generator_code *b;

	if (IS_JZ(a) && IS_DUP(a->prev)) {
		b = a->arg[0]->label->code->next;
		if (IS_DUP(b) && IS_JZ(b->next)) {
			if (a->arg[0]->label == b->next->arg[0]->label) {
				return NULL;
			}

			a->arg[0]->label->count -= 1;
			b->next->arg[0]->label->count += 1;
			a->arg[0]->label = b->next->arg[0]->label;

			return a;
		}
	}

	return NULL;
}

static struct generator_code *
peephole_rewrite_const_jz(struct lemon *lemon,
                          struct generator_code *a)
{
	/*
	 *    CONST `true object'
	 *    JZ L1
	 *    ...
	 * L1:
	 *    ...
	 * ->
	 *    ...
	 * L1:
	 *    ...
	 */
	/*
	 *    CONST `false object'
	 *    JZ L1
	 *    ...
	 * L1:
	 *    ...
	 * ->
	 *    JMP L1
	 *    ...
	 * L1:
	 *    ...
	 */

	struct lobject *object;

	if (IS_JZ(a) && IS_CONST(a->prev)) {
		object = machine_get_const(lemon, a->prev->arg[0]->value);
		if (lobject_boolean(lemon, object) == lemon->l_true) {
			generator_delete_code(lemon, a->prev);
			a = a->next;
			generator_delete_code(lemon, a->prev);
		} else {
			generator_delete_code(lemon, a->prev);
			a->opcode = OPCODE_JMP;
		}
		return a;
	}

	return NULL;
}

static struct generator_code *
peephole_rewrite_const_dup_jz(struct lemon *lemon,
                              struct generator_code *a)
{
	/*
	 *    CONST `true object'
	 *    DUP
	 *    JZ L1
	 *    ...
	 * L1:
	 *    ...
	 * ->
	 *    CONST `true object'
	 *    ...
	 * L1:
	 *    ...
	 */
	/*
	 *    CONST `false object'
	 *    DUP
	 *    JZ L1
	 *    ...
	 * L1:
	 *    ...
	 * ->
	 *    CONST `false object'
	 *    JMP L1
	 *    ...
	 * L1:
	 *    ...
	 */

	struct lobject *object;

	if (IS_JZ(a) && IS_DUP(a->prev) && IS_CONST(a->prev->prev)) {
		a = a->prev->prev;
		object = machine_get_const(lemon, a->arg[0]->value);
		if (lobject_boolean(lemon, object) == lemon->l_true) {
			generator_delete_code(lemon, a->next->next);
			generator_delete_code(lemon, a->next);
		} else {
			generator_delete_code(lemon, a->next);
			a->next->opcode = OPCODE_JMP;
		}
		return a;
	}

	return NULL;
}

static struct generator_code *
peephole_rewrite_const_dup_jnz(struct lemon *lemon,
                               struct generator_code *a)
{
	/*
	 *    CONST `true object'
	 *    DUP
	 *    JNZ L1
	 *    ...
	 * L1:
	 *    ...
	 * ->
	 *    CONST `true object'
	 *    ...
	 * L1:
	 *    ...
	 */
	/*
	 *    CONST `false object'
	 *    DUP
	 *    JNZ L1
	 *    ...
	 * L1:
	 *    ...
	 * ->
	 *    CONST `false object'
	 *    JMP L1
	 *    ...
	 * L1:
	 *    ...
	 */

	struct lobject *object;

	if (IS_JNZ(a) && IS_DUP(a->prev) && IS_CONST(a->prev->prev)) {
		a = a->prev->prev;
		object = machine_get_const(lemon, a->arg[0]->value);
		if (lobject_boolean(lemon, object) == lemon->l_true) {
			generator_delete_code(lemon, a->next);
			a->next->opcode = OPCODE_JMP;
		} else {
			generator_delete_code(lemon, a->next->next);
			generator_delete_code(lemon, a->next);
		}
		return a;
	}

	return NULL;
}

static struct generator_code *
peephole_rewrite_pop(struct lemon *lemon,
                     struct generator_code *a)
{
	/*
	 *    CONST/DUP
	 *    POP
	 *    ...
	 * ->
	 *    ...
	 */

	if (IS_POP(a) && (IS_CONST(a->prev) || IS_DUP(a->prev))) {
		generator_delete_code(lemon, a->prev);
		a = a->next;
		generator_delete_code(lemon, a->prev);

		return a;
	}

	return NULL;
}

static struct generator_code *
peephole_rewrite_load_load(struct lemon *lemon,
                           struct generator_code *a)
{
	/*
	 *    LOAD i
	 *    LOAD i
	 *    ...
	 * ->
	 *    LOAD i
	 *    DUP
	 *    ...
	 */

	if (IS_LOAD(a) && IS_LOAD(a->prev)) {
		if (a->arg[0]->value == a->prev->arg[0]->value &&
		    a->arg[1]->value == a->prev->arg[1]->value)
		{
			a->opcode = OPCODE_DUP;
			a->arg[0] = NULL;
			a->arg[1] = NULL;

			return a->prev;
		}
	}

	return NULL;
}

static struct generator_code *
peephole_rewrite_store_load(struct lemon *lemon,
                            struct generator_code *a)
{
	/*
	 *    STORE i
	 *    LOAD i
	 *    ...
	 * ->
	 *    DUP
	 *    STORE ig
	 *    ...
	 */

	if (IS_LOAD(a) && IS_STORE(a->prev)) {
		if (a->arg[0]->value == a->prev->arg[0]->value &&
		    a->arg[1]->value == a->prev->arg[1]->value)
		{
			a->prev->opcode = OPCODE_DUP;
			a->prev->arg[0] = NULL;
			a->opcode = OPCODE_STORE;

			return a->prev;
		}
	}

	return NULL;
}

void
peephole_optimize(struct lemon *lemon)
{
	struct generator *gen;
	struct generator_code *a;
	struct generator_code *t;

	gen = lemon->l_generator;
	a = gen->head;
	while (a) {
		if (IS_LABEL(a)) {
			/* remove no reference label,
			 * peephole should use backward order match pattern
			 */
			if (a->label && a->label->count == 0) {
				a = a->next;
				generator_delete_code(lemon, a->prev);
			} else {
				a = a->next;
			}
			continue;
		}
		if (IS_NOP(a)) {
			a = a->next;
			generator_delete_code(lemon, a->prev);
			continue;
		}

		if ((t = peephole_rewrite_unop(lemon, a))) {
			a = t;
			continue;
		}

		if ((t = peephole_rewrite_binop(lemon, a))) {
			a = t;
			continue;
		}

		if ((t = peephole_rewrite_load_load(lemon, a))) {
			a = t;
			continue;
		}

		if ((t = peephole_rewrite_store_load(lemon, a))) {
			a = t;
			continue;
		}

		if ((t = peephole_rewrite_jmp_to_next(lemon, a))) {
			a = t;
			continue;
		}

		if ((t = peephole_rewrite_jz_or_jnz_to_next(lemon, a))) {
			a = t;
			continue;
		}

		if ((t = peephole_rewrite_dup_jz_to_dup_jz(lemon, a))) {
			a = t;
			continue;
		}

		if ((t = peephole_rewrite_const_jz(lemon, a))) {
			a = t;
			continue;
		}

		if ((t = peephole_rewrite_const_dup_jz(lemon, a))) {
			a = t;
			continue;
		}

		if ((t = peephole_rewrite_const_dup_jnz(lemon, a))) {
			a = t;
			continue;
		}

		if ((t = peephole_rewrite_pop(lemon, a))) {
			a = t;
			continue;
		}

		a = a->next;
	}
}
