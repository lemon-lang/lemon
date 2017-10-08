#include "lemon.h"
#include "arena.h"
#include "input.h"
#include "lexer.h"
#include "scope.h"
#include "table.h"
#include "symbol.h"
#include "syntax.h"
#include "parser.h"
#include "machine.h"
#include "compiler.h"
#include "peephole.h"
#include "generator.h"
#include "allocator.h"
#include "collector.h"
#include "lnil.h"
#include "lkarg.h"
#include "lvarg.h"
#include "lvkarg.h"
#include "ltable.h"
#include "larray.h"
#include "lsuper.h"
#include "lclass.h"
#include "lnumber.h"
#include "lstring.h"
#include "linteger.h"
#include "lmodule.h"
#include "lboolean.h"
#include "linstance.h"
#include "literator.h"
#include "lsentinel.h"
#include "lcoroutine.h"
#include "lcontinuation.h"
#include "lexception.h"
#include "ldictionary.h"

#include <stdlib.h>
#include <string.h>

#define CHECK_NULL(p) do {   \
	if (!p) {            \
		return NULL; \
	}                    \
} while(0)                   \

struct lobject *
lemon_init_types(struct lemon *lemon)
{
	size_t size;

	size = sizeof(struct slot) * 64;
	lemon->l_types_count = 0;
	lemon->l_types_length = 64;
	lemon->l_types_slots = lemon_allocator_alloc(lemon, size);
	if (!lemon->l_types_slots) {
		return NULL;
	}
	memset(lemon->l_types_slots, 0, size);

	/* init type and boolean's type first */
	lemon->l_type_type = ltype_type_create(lemon);
	CHECK_NULL(lemon->l_type_type);
	lemon->l_boolean_type = lboolean_type_create(lemon);
	CHECK_NULL(lemon->l_boolean_type);

	/* init essential values */
	lemon->l_nil = lnil_create(lemon);
	CHECK_NULL(lemon->l_nil);
	lemon->l_true = lboolean_create(lemon, 1);
	CHECK_NULL(lemon->l_true);
	lemon->l_false = lboolean_create(lemon, 0);
	CHECK_NULL(lemon->l_false);
	lemon->l_sentinel = lsentinel_create(lemon);
	CHECK_NULL(lemon->l_sentinel);

	/* init rest types */
	lemon->l_karg_type = lkarg_type_create(lemon);
	CHECK_NULL(lemon->l_sentinel);
	lemon->l_varg_type = lvarg_type_create(lemon);
	CHECK_NULL(lemon->l_sentinel);
	lemon->l_vkarg_type = lvkarg_type_create(lemon);
	CHECK_NULL(lemon->l_sentinel);
	lemon->l_table_type = ltable_type_create(lemon);
	CHECK_NULL(lemon->l_sentinel);
	lemon->l_super_type = lsuper_type_create(lemon);
	CHECK_NULL(lemon->l_sentinel);
	lemon->l_class_type = lclass_type_create(lemon);
	CHECK_NULL(lemon->l_sentinel);
	lemon->l_frame_type = lframe_type_create(lemon);
	CHECK_NULL(lemon->l_sentinel);
	lemon->l_array_type = larray_type_create(lemon);
	CHECK_NULL(lemon->l_sentinel);
	lemon->l_number_type = lnumber_type_create(lemon);
	CHECK_NULL(lemon->l_sentinel);
	lemon->l_string_type = lstring_type_create(lemon);
	CHECK_NULL(lemon->l_sentinel);
	lemon->l_module_type = lmodule_type_create(lemon);
	CHECK_NULL(lemon->l_sentinel);
	lemon->l_integer_type = linteger_type_create(lemon);
	CHECK_NULL(lemon->l_sentinel);
	lemon->l_function_type = lfunction_type_create(lemon);
	CHECK_NULL(lemon->l_sentinel);
	lemon->l_instance_type = linstance_type_create(lemon);
	CHECK_NULL(lemon->l_sentinel);
	lemon->l_iterator_type = literator_type_create(lemon);
	CHECK_NULL(lemon->l_sentinel);
	lemon->l_coroutine_type = lcoroutine_type_create(lemon);
	CHECK_NULL(lemon->l_sentinel);
	lemon->l_exception_type = lexception_type_create(lemon);
	CHECK_NULL(lemon->l_sentinel);
	lemon->l_dictionary_type = ldictionary_type_create(lemon);
	CHECK_NULL(lemon->l_sentinel);
	lemon->l_continuation_type = lcontinuation_type_create(lemon);
	CHECK_NULL(lemon->l_sentinel);

	return lemon->l_nil;
}

struct lobject *
lemon_init_errors(struct lemon *lemon)
{
	char *cstr;
	struct lobject *base;
	struct lobject *name;
	struct lobject *error;

	base = (struct lobject *)lemon->l_exception_type;
	lemon->l_base_error = base;

	cstr = "TypeError";
	name = lstring_create(lemon, cstr, strlen(cstr));
	CHECK_NULL(name);
	error = lclass_create(lemon, name, 1, &base, 0, NULL);
	CHECK_NULL(error);
	lemon_add_global(lemon, cstr, error);
	lemon->l_type_error = error;

	cstr = "ItemError";
	name = lstring_create(lemon, cstr, strlen(cstr));
	CHECK_NULL(name);
	error = lclass_create(lemon, name, 1, &base, 0, NULL);
	CHECK_NULL(error);
	lemon_add_global(lemon, cstr, error);
	lemon->l_item_error = error;

	cstr = "MemoryError";
	name = lstring_create(lemon, cstr, strlen(cstr));
	CHECK_NULL(name);
	error = lclass_create(lemon, name, 1, &base, 0, NULL);
	CHECK_NULL(error);
	lemon_add_global(lemon, cstr, error);
	lemon->l_memory_error = error;

	cstr = "RuntimeError";
	name = lstring_create(lemon, cstr, strlen(cstr));
	CHECK_NULL(name);
	error = lclass_create(lemon, name, 1, &base, 0, NULL);
	CHECK_NULL(error);
	lemon_add_global(lemon, cstr, error);
	lemon->l_runtime_error = error;

	cstr = "ArgumentError";
	name = lstring_create(lemon, cstr, strlen(cstr));
	CHECK_NULL(name);
	error = lclass_create(lemon, name, 1, &base, 0, NULL);
	CHECK_NULL(error);
	lemon_add_global(lemon, cstr, error);
	lemon->l_argument_error = error;

	cstr = "AttributeError";
	name = lstring_create(lemon, cstr, strlen(cstr));
	CHECK_NULL(name);
	error = lclass_create(lemon, name, 1, &base, 0, NULL);
	CHECK_NULL(error);
	lemon_add_global(lemon, cstr, error);
	lemon->l_attribute_error = error;

	cstr = "ArithmeticError";
	name = lstring_create(lemon, cstr, strlen(cstr));
	CHECK_NULL(name);
	error = lclass_create(lemon, name, 1, &base, 0, NULL);
	CHECK_NULL(error);
	lemon_add_global(lemon, cstr, error);
	lemon->l_arithmetic_error = error;

	cstr = "NotCallableError";
	name = lstring_create(lemon, cstr, strlen(cstr));
	CHECK_NULL(name);
	error = lclass_create(lemon, name, 1, &base, 0, NULL);
	CHECK_NULL(error);
	lemon_add_global(lemon, cstr, error);
	lemon->l_not_callable_error = error;

	cstr = "NotIterableError";
	name = lstring_create(lemon, cstr, strlen(cstr));
	CHECK_NULL(name);
	error = lclass_create(lemon, name, 1, &base, 0, NULL);
	CHECK_NULL(error);
	lemon_add_global(lemon, cstr, error);
	lemon->l_not_iterable_error = error;

	cstr = "NotImplementedError";
	name = lstring_create(lemon, cstr, strlen(cstr));
	CHECK_NULL(name);
	error = lclass_create(lemon, name, 1, &base, 0, NULL);
	CHECK_NULL(error);
	lemon_add_global(lemon, cstr, error);
	lemon->l_not_implemented_error = error;

	return lemon->l_nil;
}

struct lobject *
lemon_init_strings(struct lemon *lemon)
{
	lemon->l_empty_string = lstring_create(lemon, NULL, 0);
	CHECK_NULL(lemon->l_empty_string);
	lemon->l_space_string = lstring_create(lemon, " ", 1);
	CHECK_NULL(lemon->l_space_string);
	lemon->l_add_string = lstring_create(lemon, "__add__", 7);
	CHECK_NULL(lemon->l_add_string);
	lemon->l_sub_string = lstring_create(lemon, "__sub__", 7);
	CHECK_NULL(lemon->l_sub_string);
	lemon->l_mul_string = lstring_create(lemon, "__mul__", 7);
	CHECK_NULL(lemon->l_mul_string);
	lemon->l_div_string = lstring_create(lemon, "__div__", 7);
	CHECK_NULL(lemon->l_div_string);
	lemon->l_mod_string = lstring_create(lemon, "__mod__", 7);
	CHECK_NULL(lemon->l_mod_string);
	lemon->l_call_string = lstring_create(lemon, "__call__", 8);
	CHECK_NULL(lemon->l_call_string);
	lemon->l_get_item_string = lstring_create(lemon, "__get_item__", 12);
	CHECK_NULL(lemon->l_get_item_string);
	lemon->l_set_item_string = lstring_create(lemon, "__set_item__", 12);
	CHECK_NULL(lemon->l_set_item_string);
	lemon->l_get_attr_string = lstring_create(lemon, "__get_attr__", 12);
	CHECK_NULL(lemon->l_get_attr_string);
	lemon->l_set_attr_string = lstring_create(lemon, "__set_attr__", 12);
	CHECK_NULL(lemon->l_set_attr_string);
	lemon->l_del_attr_string = lstring_create(lemon, "__del_attr__", 12);
	CHECK_NULL(lemon->l_del_attr_string);
	lemon->l_init_string = lstring_create(lemon, "__init__", 8);
	CHECK_NULL(lemon->l_init_string);
	lemon->l_next_string = lstring_create(lemon, "__next__", 8);
	CHECK_NULL(lemon->l_next_string);
	lemon->l_array_string = lstring_create(lemon, "__array__", 9);
	CHECK_NULL(lemon->l_array_string);
	lemon->l_string_string = lstring_create(lemon, "__string__", 10);
	CHECK_NULL(lemon->l_string_string);
	lemon->l_iterator_string = lstring_create(lemon, "__iterator__", 12);
	CHECK_NULL(lemon->l_iterator_string);

	return lemon->l_nil;
}

#undef CHECK_NULL
#define CHECK_NULL(p) do { \
	if (!p) {          \
		goto err;  \
	}                  \
} while(0)                 \

struct lemon *
lemon_create()
{
	struct lemon *lemon;

	lemon = malloc(sizeof(*lemon));
	if (!lemon) {
		return NULL;
	}
	memset(lemon, 0, sizeof(*lemon));

	/*
	 * random number for seeding other module
	 * 0x4c454d9d == ('L'<<24) + ('E'<<16) + ('M'<<8) + 'O' + 'N')
	 */
	srandom(0x4c454d9d);
	lemon->l_random = random();
	lemon->l_allocator = allocator_create(lemon);
	CHECK_NULL(lemon->l_allocator);

	lemon->l_arena = arena_create(lemon);
	CHECK_NULL(lemon->l_arena);
	lemon->l_input = input_create(lemon);
	CHECK_NULL(lemon->l_input);
	lemon->l_lexer = lexer_create(lemon);
	CHECK_NULL(lemon->l_lexer);

	lemon->l_global = lemon_allocator_alloc(lemon, sizeof(struct scope));
	CHECK_NULL(lemon->l_global);
	memset(lemon->l_global, 0, sizeof(struct scope));

	lemon->l_generator = generator_create(lemon);
	CHECK_NULL(lemon->l_generator);

	lemon->l_collector = collector_create(lemon);
	CHECK_NULL(lemon->l_collector);
	lemon->l_machine = machine_create(lemon);
	CHECK_NULL(lemon->l_machine);

	CHECK_NULL(lemon_init_types(lemon));
	CHECK_NULL(lemon_init_errors(lemon));
	CHECK_NULL(lemon_init_strings(lemon));

	lemon->l_out_of_memory = lobject_error_memory(lemon, "Out of Memory");
	CHECK_NULL(lemon->l_out_of_memory);
	lemon->l_modules = ltable_create(lemon);
	CHECK_NULL(lemon->l_modules);

	return lemon;
err:
	lemon_destroy(lemon);

	return NULL;
}

void
lemon_destroy(struct lemon *lemon)
{
	input_destroy(lemon, lemon->l_input);
	lemon->l_input = NULL;

	lexer_destroy(lemon, lemon->l_lexer);
	lemon->l_lexer = NULL;

	machine_destroy(lemon, lemon->l_machine);
	lemon->l_machine = NULL;

	arena_destroy(lemon, lemon->l_arena);
	lemon->l_arena = NULL;

	collector_destroy(lemon, lemon->l_collector);
	lemon->l_collector = NULL;

	lemon_allocator_free(lemon, lemon->l_types_slots);
	lemon->l_types_slots = NULL;

	allocator_destroy(lemon, lemon->l_allocator);
	lemon->l_allocator = NULL;

	free(lemon);
}

#undef CHECK_NULL

int
lemon_compile(struct lemon *lemon)
{
	struct syntax *node;

	lexer_next_token(lemon);

	node = parser_parse(lemon);
	if (!node) {
		fprintf(stderr, "lemon: syntax error\n");

		return 0;
	}

	if (!compiler_compile(lemon, node)) {
		fprintf(stderr, "lemon: syntax error\n");

		return 0;
	}
	peephole_optimize(lemon);

	machine_reset(lemon);
	generator_emit(lemon);
	collector_full(lemon);

	return 1;
}

void
lemon_mark_types(struct lemon *lemon)
{
	unsigned long i;
	struct slot *slots;

	lobject_mark(lemon, lemon->l_nil);
	lobject_mark(lemon, lemon->l_true);
	lobject_mark(lemon, lemon->l_false);
	lobject_mark(lemon, lemon->l_sentinel);

	slots = lemon->l_types_slots;
	for (i = 0; i < lemon->l_types_length; i++) {
		if (slots[i].key && slots[i].key != lemon->l_sentinel) {
			lobject_mark(lemon, slots[i].value);
		}
	}
}

void
lemon_mark_errors(struct lemon *lemon)
{
	lobject_mark(lemon, lemon->l_base_error);
	lobject_mark(lemon, lemon->l_type_error);
	lobject_mark(lemon, lemon->l_item_error);
	lobject_mark(lemon, lemon->l_memory_error);
	lobject_mark(lemon, lemon->l_runtime_error);
	lobject_mark(lemon, lemon->l_argument_error);
	lobject_mark(lemon, lemon->l_attribute_error);
	lobject_mark(lemon, lemon->l_arithmetic_error);
	lobject_mark(lemon, lemon->l_not_callable_error);
	lobject_mark(lemon, lemon->l_not_iterable_error);
	lobject_mark(lemon, lemon->l_not_implemented_error);
}

void
lemon_mark_strings(struct lemon *lemon)
{
	lobject_mark(lemon, lemon->l_empty_string);
	lobject_mark(lemon, lemon->l_space_string);
	lobject_mark(lemon, lemon->l_add_string);
	lobject_mark(lemon, lemon->l_sub_string);
	lobject_mark(lemon, lemon->l_mul_string);
	lobject_mark(lemon, lemon->l_div_string);
	lobject_mark(lemon, lemon->l_mod_string);
	lobject_mark(lemon, lemon->l_call_string);
	lobject_mark(lemon, lemon->l_get_item_string);
	lobject_mark(lemon, lemon->l_set_item_string);
	lobject_mark(lemon, lemon->l_get_attr_string);
	lobject_mark(lemon, lemon->l_set_attr_string);
	lobject_mark(lemon, lemon->l_del_attr_string);
	lobject_mark(lemon, lemon->l_init_string);
	lobject_mark(lemon, lemon->l_next_string);
	lobject_mark(lemon, lemon->l_array_string);
	lobject_mark(lemon, lemon->l_string_string);
	lobject_mark(lemon, lemon->l_iterator_string);
}

static int
lemon_type_cmp(struct lemon *lemon, void *a, void *b)
{
	return a == b;
}

static unsigned long
lemon_type_hash(struct lemon *lemon, void *key)
{
	return (unsigned long)key;
}

struct lobject *
lemon_get_type(struct lemon *lemon, lobject_method_t method)
{
	struct lobject *type;

	type = table_search(lemon,
	                    (void *)(uintptr_t)method,
	                    lemon->l_types_slots,
	                    lemon->l_types_length,
	                    lemon_type_cmp,
	                    lemon_type_hash);
	if (!type && method) {
		return ltype_create(lemon, NULL, method, NULL);
	}

	return type;
}

int
lemon_add_type(struct lemon *lemon, struct ltype *type)
{
	void *slots;
	void *method;
	size_t size;
	unsigned long count;
	unsigned long length;

	slots = lemon->l_types_slots;
	count  = lemon->l_types_count;
	length = lemon->l_types_length;
	method = (void *)(uintptr_t)type->method;
	lemon->l_types_count += table_insert(lemon,
	                                     method,
	                                     type,
	                                     slots,
	                                     length,
	                                     lemon_type_cmp,
	                                     lemon_type_hash);

	if (TABLE_LOAD_FACTOR(count + 1) >= length) {
		length = TABLE_GROW_FACTOR(length);
		size = sizeof(struct slot) * length;
		slots = lemon_allocator_alloc(lemon, size);
		if (!slots) {
			return 0;
		}
		memset(slots, 0, size);

		table_rehash(lemon,
		             lemon->l_types_slots,
		             lemon->l_types_length,
		             slots,
		             length,
		             lemon_type_cmp,
		             lemon_type_hash);
		lemon_allocator_free(lemon, lemon->l_types_slots);
		lemon->l_types_slots = slots;
		lemon->l_types_length = length;
	}

	return 1;
}

void
lemon_del_type(struct lemon *lemon, struct ltype *type)
{
	if (lemon->l_types_slots) {
		table_delete(lemon,
		             (void *)(uintptr_t)type->method,
		             lemon->l_types_slots,
		             lemon->l_types_length,
		             lemon_type_cmp,
		             lemon_type_hash);
	}
}

struct lobject *
lemon_add_global(struct lemon *lemon, const char *name, void *object)
{
	struct symbol *symbol;

	symbol = scope_add_symbol(lemon, lemon->l_global, name, SYMBOL_GLOBAL);
	symbol->cpool = machine_add_const(lemon, object);

	return object;
}
