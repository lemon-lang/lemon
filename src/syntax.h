#ifndef LEMON_SYNTAX_H
#define LEMON_SYNTAX_H

#include <stdio.h>

struct lemon;
struct symbol;

/* opkind */
enum {
	SYNTAX_OPKIND_ADD,
	SYNTAX_OPKIND_SUB,
	SYNTAX_OPKIND_MUL,
	SYNTAX_OPKIND_DIV,
	SYNTAX_OPKIND_MOD,
	SYNTAX_OPKIND_POS,
	SYNTAX_OPKIND_NEG,
	SYNTAX_OPKIND_SHL,
	SYNTAX_OPKIND_SHR,

	SYNTAX_OPKIND_LT,
	SYNTAX_OPKIND_LE,

	SYNTAX_OPKIND_GT,
	SYNTAX_OPKIND_GE,

	SYNTAX_OPKIND_EQ,
	SYNTAX_OPKIND_NE,

	SYNTAX_OPKIND_IN,

	SYNTAX_OPKIND_BITWISE_NOT,
	SYNTAX_OPKIND_BITWISE_AND,
	SYNTAX_OPKIND_BITWISE_XOR,
	SYNTAX_OPKIND_BITWISE_OR,

	SYNTAX_OPKIND_LOGICAL_NOT,
	SYNTAX_OPKIND_LOGICAL_AND,
	SYNTAX_OPKIND_LOGICAL_OR,

	SYNTAX_OPKIND_ASSIGN,
	SYNTAX_OPKIND_ADD_ASSIGN,
	SYNTAX_OPKIND_SUB_ASSIGN,
	SYNTAX_OPKIND_MUL_ASSIGN,
	SYNTAX_OPKIND_DIV_ASSIGN,
	SYNTAX_OPKIND_MOD_ASSIGN,
	SYNTAX_OPKIND_SHL_ASSIGN,
	SYNTAX_OPKIND_SHR_ASSIGN,
	SYNTAX_OPKIND_BITWISE_AND_ASSIGN,
	SYNTAX_OPKIND_BITWISE_XOR_ASSIGN,
	SYNTAX_OPKIND_BITWISE_OR_ASSIGN
};

/* kind */
enum {
	SYNTAX_KIND_NIL,
	SYNTAX_KIND_TRUE,
	SYNTAX_KIND_FALSE,
	SYNTAX_KIND_SENTINEL,

	SYNTAX_KIND_SELF,
	SYNTAX_KIND_SUPER,

	SYNTAX_KIND_NAME,
	SYNTAX_KIND_NUMBER_LITERAL,
	SYNTAX_KIND_STRING_LITERAL,

	SYNTAX_KIND_ARRAY,
	SYNTAX_KIND_ELEMENT,
	SYNTAX_KIND_DICTIONARY,

	SYNTAX_KIND_CALL,
	SYNTAX_KIND_ARGUMENT,
	SYNTAX_KIND_PARAMETER,

	SYNTAX_KIND_UNOP,
	SYNTAX_KIND_BINOP,

	SYNTAX_KIND_CONDITIONAL,
	SYNTAX_KIND_UNPACK,

	SYNTAX_KIND_GET_ITEM,
	SYNTAX_KIND_GET_ATTR,
	SYNTAX_KIND_GET_SLICE,

	SYNTAX_KIND_ACCESSOR,

	SYNTAX_KIND_ASSIGN_STMT,
	SYNTAX_KIND_VAR_STMT,
	SYNTAX_KIND_DEFINE_STMT,
	SYNTAX_KIND_IF_STMT,
	SYNTAX_KIND_FOR_STMT,
	SYNTAX_KIND_FORIN_STMT,
	SYNTAX_KIND_WHILE_STMT,
	SYNTAX_KIND_BREAK_STMT,
	SYNTAX_KIND_CONTINUE_STMT,
	SYNTAX_KIND_DELETE_STMT,
	SYNTAX_KIND_RETURN_STMT,
	SYNTAX_KIND_IMPORT_STMT,
	SYNTAX_KIND_CLASS_STMT,
	SYNTAX_KIND_BLOCK_STMT,
	SYNTAX_KIND_MODULE_STMT,

	SYNTAX_KIND_TRY_STMT,
	SYNTAX_KIND_CATCH_STMT,
	SYNTAX_KIND_THROW_STMT
};

struct syntax {
	int kind;
	int opkind;

	/* function node */
	int nvalues;
	int nparams;
	int nlocals;

	long length;
	char *buffer;

	/*
	 * parameter type:
	 * 0, normal, 'define func(var a, var b, var c)'
	 *    a, b and c take exactly argument
	 *
	 * 1, to array, 'define func(var a, var b, var *c)'
	 *    turn all argument after b into array c
	 *
	 * 2, to dictionary, 'define func(var a, var b, var **c)'
	 *    turn all keyword argument after b into dictionary c
	 *
	 * see lfunction.h
	 *
	 */
	int parameter_type;

	/*
	 * argument type:
	 * same as paramter_type but reverse
	 * 0, normal, 'func(a, b, c)'
	 *    a, b and c is exactly argument
	 *
	 * 1, from array, 'func(a, b, *c)'
	 *    c will expend all element and become argument
	 *
	 * 2, from dictionary 'func(a, b, **c)'
	 *    c will expend all element and become keyword argument
	 *
	 */
	int argument_type;

	/* lemon->l_try_enclosing labels */
	void *l_catch;   /* label for catch */
	void *l_finally; /* label for finally */

	/* lemon->l_loop_enclosing labels */
	void *l_break;    /* label for break */
	void *l_continue; /* label for continue */

	/* mark block has certain return */
	int has_return;

	long line;
	long column;
	char *filename;

	struct syntax *parent;
	struct syntax *sibling;

	/*
	 * naming rule:
	 *
	 *     [name][_type][_list];
	 *
	 * [name] is semantic name can ignore if only one kind
	 * [_type] is node kind, `expr' is abstract name, see parser
	 * [_list] is the first element of linked list, linked by sibling
	 */
	union {
		struct syntax *child[5];

		struct {
			struct syntax *left;
		} unop; /* unary opkind */

		struct {
			struct syntax *left;
			struct syntax *right;
		} binop; /* binary opkind */

		struct {
			struct syntax *expr;
			struct syntax *true_expr;
			struct syntax *false_expr;
		} conditional; /* expr ? true_expr : false_expr */

		struct {
			struct syntax *name;
			struct syntax *expr;
		} argument;

		struct {
			struct syntax *expr_list;
		} array;

		struct {
			struct syntax *name;
			struct syntax *value;
		} element;

		struct {
			struct syntax *element_list;
		} dictionary;

		struct {
			struct syntax *callable;
			struct syntax *argument_list;
		} call;

		struct {
			struct syntax *left;
			struct syntax *right;
		} get_item;

		struct {
			struct syntax *left;
			struct syntax *right;
		} get_attr;

		struct {
			struct syntax *left;
			struct syntax *start;
			struct syntax *stop;
			struct syntax *step;
		} get_slice;

		struct {
			struct syntax *name; /* 'getter' or 'setter' */
			struct syntax *expr;
		} accessor;

		struct {
			struct syntax *expr_list;
		} unpack;

		struct {
			struct syntax *left;
			struct syntax *right;
		} assign_stmt;

		struct {
			struct syntax *expr;
			struct syntax *then_block_stmt;
			struct syntax *else_block_stmt;
		} if_stmt;

		struct {
			struct syntax *init_expr;
			struct syntax *cond_expr;
			struct syntax *step_expr;
			struct syntax *block_stmt;
		} for_stmt;

		struct {
			struct syntax *name;
			struct syntax *iter;
			struct syntax *block_stmt;
		} forin_stmt;

		struct {
			struct syntax *expr;
			struct syntax *block_stmt;
		} while_stmt;

		struct {
			struct syntax *expr;
		} delete_stmt;

		struct {
			struct syntax *expr;
		} return_stmt;

		struct {
			struct syntax *name;
			struct syntax *path_string;
			struct syntax *module_stmt;
		} import_stmt;

		struct {
			struct syntax *try_block_stmt;
			struct syntax *catch_stmt_list;
			struct syntax *finally_block_stmt;
		} try_stmt;

		struct {
			struct syntax *catch_type;
			struct syntax *catch_name;
			struct syntax *block_stmt;
		} catch_stmt;

		struct {
			struct syntax *expr;
		} throw_stmt;

		struct {
			struct syntax *name;
			struct syntax *expr;
			struct syntax *accessor_list;
		} var_stmt;

		struct {
			struct syntax *name;
			struct syntax *expr;
			struct syntax *accessor_list;
		} parameter;

		struct {
			struct syntax *name;
			struct syntax *parameter_list;
			struct syntax *block_stmt;
			struct syntax *accessor_list;
		} define_stmt;

		struct {
			struct syntax *name;
			struct syntax *super_list;
			struct syntax *block_stmt;
			struct syntax *accessor_list;
		} class_stmt;

		struct {
			struct syntax *stmt_list;
		} block_stmt;

		struct {
			struct syntax *stmt_list;
		} module_stmt;

		/* break and continue doesn't have child */
	} u;
};

struct syntax *
syntax_make_kind_node(struct lemon *lemon,
                      int kind);

struct syntax *
syntax_make_nil_node(struct lemon *lemon);

struct syntax *
syntax_make_true_node(struct lemon *lemon);

struct syntax *
syntax_make_false_node(struct lemon *lemon);

struct syntax *
syntax_make_sentinel_node(struct lemon *lemon);

struct syntax *
syntax_make_self_node(struct lemon *lemon);

struct syntax *
syntax_make_super_node(struct lemon *lemon);

struct syntax *
syntax_make_name_node(struct lemon *lemon,
                      long length,
                      char *buffer);

struct syntax *
syntax_make_number_node(struct lemon *lemon,
                        long length,
                        char *buffer);

struct syntax *
syntax_make_string_node(struct lemon *lemon,
                        long length,
                        char *buffer);

struct syntax *
syntax_make_array_node(struct lemon *lemon,
                       struct syntax *expr_list);

struct syntax *
syntax_make_element_node(struct lemon *lemon,
                         struct syntax *name,
                         struct syntax *value);

struct syntax *
syntax_make_dictionary_node(struct lemon *lemon,
                            struct syntax *element_list);

struct syntax *
syntax_make_unop_node(struct lemon *lemon,
                      int opkind,
                      struct syntax *left);

struct syntax *
syntax_make_binop_node(struct lemon *lemon,
                       int opkind,
                       struct syntax *left,
                       struct syntax *right);

struct syntax *
syntax_make_get_item_node(struct lemon *lemon,
                          struct syntax *left,
                          struct syntax *right);

struct syntax *
syntax_make_get_attr_node(struct lemon *lemon,
                          struct syntax *left,
                          struct syntax *right);

struct syntax *
syntax_make_get_slice_node(struct lemon *lemon,
                           struct syntax *left,
                           struct syntax *start,
                           struct syntax *stop,
                           struct syntax *step);

struct syntax *
syntax_make_argument_node(struct lemon *lemon,
                          struct syntax *name,
                          struct syntax *expr,
                          int argument_type);

struct syntax *
syntax_make_call_node(struct lemon *lemon,
                      struct syntax *function,
                      struct syntax *argument_list);

struct syntax *
syntax_make_conditional_node(struct lemon *lemon,
                             struct syntax *expr,
                             struct syntax *true_expr,
                             struct syntax *false_expr);

struct syntax *
syntax_make_unpack_node(struct lemon *lemon,
                        struct syntax *expr_list);

struct syntax *
syntax_make_assign_node(struct lemon *lemon,
                        int opkind,
                        struct syntax *left,
                        struct syntax *right);

struct syntax *
syntax_make_if_node(struct lemon *lemon,
                    struct syntax *condition,
                    struct syntax *thenblock,
                    struct syntax *elseblock);

struct syntax *
syntax_make_for_node(struct lemon *lemon,
                     struct syntax *init_expr,
                     struct syntax *cond_expr,
                     struct syntax *step_expr,
                     struct syntax *block_stmt);

struct syntax *
syntax_make_forin_node(struct lemon *lemon,
                       struct syntax *name,
                       struct syntax *iter,
                       struct syntax *block_stmt);

struct syntax *
syntax_make_while_node(struct lemon *lemon,
                       struct syntax *condition,
                       struct syntax *block);

struct syntax *
syntax_make_var_node(struct lemon *lemon,
                     struct syntax *left,
                     struct syntax *right);

struct syntax *
syntax_make_accessor_node(struct lemon *lemon,
                          struct syntax *name,
                          struct syntax *expr);

struct syntax *
syntax_make_parameter_node(struct lemon *lemon,
                           struct syntax *name,
                           struct syntax *expr,
                           struct syntax *accessor_list,
                           int parameter_type);

struct syntax *
syntax_make_define_node(struct lemon *lemon,
                        struct syntax *name,
                        struct syntax *parameter_list,
                        struct syntax *block_stmt);

struct syntax *
syntax_make_class_node(struct lemon *lemon,
                       struct syntax *name,
                       struct syntax *super_list,
                       struct syntax *block_stmt);

struct syntax *
syntax_make_try_node(struct lemon *lemon,
                     struct syntax *try_block_stmt,
                     struct syntax *catch_stmt_list,
                     struct syntax *finally_block_stmt);

struct syntax *
syntax_make_catch_node(struct lemon *lemon,
                       struct syntax *catch_type,
                       struct syntax *catch_name,
                       struct syntax *block_stmt);

struct syntax *
syntax_make_throw_node(struct lemon *lemon,
                       struct syntax *expr);

struct syntax *
syntax_make_delete_node(struct lemon *lemon,
                        struct syntax *left);

struct syntax *
syntax_make_return_node(struct lemon *lemon,
                        struct syntax *left);

struct syntax *
syntax_make_import_node(struct lemon *lemon,
                        struct syntax *name,
                        struct syntax *path_string);

struct syntax *
syntax_make_continue_node(struct lemon *lemon);

struct syntax *
syntax_make_break_node(struct lemon *lemon);

struct syntax *
syntax_make_block_node(struct lemon *lemon,
                       struct syntax *child);

struct syntax *
syntax_make_module_node(struct lemon *lemon,
                        struct syntax *child);

#endif /* LEMON_SYNTAX_H */
