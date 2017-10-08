#include "lemon.h"
#include "arena.h"
#include "input.h"
#include "syntax.h"

#include <string.h>

struct syntax *
syntax_make_kind_node(struct lemon *lemon,
                      int kind)
{
	struct syntax *node;

	node = arena_alloc(lemon, lemon->l_arena, sizeof(*node));
	memset(node, 0, sizeof(*node));

	node->kind = kind;
	node->line = input_line(lemon) + 1;
	node->column = input_column(lemon) + 1;
	node->filename = input_filename(lemon);

	return node;
}

struct syntax *
syntax_make_nil_node(struct lemon *lemon)
{
	struct syntax *node;

	node = syntax_make_kind_node(lemon, SYNTAX_KIND_NIL);

	return node;
}

struct syntax *
syntax_make_true_node(struct lemon *lemon)
{
	struct syntax *node;

	node = syntax_make_kind_node(lemon, SYNTAX_KIND_TRUE);

	return node;
}

struct syntax *
syntax_make_false_node(struct lemon *lemon)
{
	struct syntax *node;

	node = syntax_make_kind_node(lemon, SYNTAX_KIND_FALSE);

	return node;
}

struct syntax *
syntax_make_sentinel_node(struct lemon *lemon)
{
	struct syntax *node;

	node = syntax_make_kind_node(lemon, SYNTAX_KIND_SENTINEL);

	return node;
}

struct syntax *
syntax_make_self_node(struct lemon *lemon)
{
	struct syntax *node;

	node = syntax_make_kind_node(lemon, SYNTAX_KIND_SELF);

	return node;
}

struct syntax *
syntax_make_super_node(struct lemon *lemon)
{
	struct syntax *node;

	node = syntax_make_kind_node(lemon, SYNTAX_KIND_SUPER);

	return node;
}

struct syntax *
syntax_make_name_node(struct lemon *lemon,
                      long length,
                      char *buffer)
{
	struct syntax *node;

	node = syntax_make_kind_node(lemon, SYNTAX_KIND_NAME);
	node->buffer = arena_alloc(lemon, lemon->l_arena, length + 1);
	node->length = length;
	node->buffer[length] = '\0';
	memcpy(node->buffer, buffer, length);

	return node;
}

struct syntax *
syntax_make_number_node(struct lemon *lemon,
                        long length,
                        char *buffer)
{
	struct syntax *node;

	node = syntax_make_kind_node(lemon, SYNTAX_KIND_NUMBER_LITERAL);
	node->buffer = arena_alloc(lemon, lemon->l_arena, length + 1);
	node->length = length;
	node->buffer[node->length] = '\0';
	memcpy(node->buffer, buffer, length);

	return node;
}

struct syntax *
syntax_make_string_node(struct lemon *lemon,
                        long length,
                        char *buffer)
{
	struct syntax *node;

	node = syntax_make_kind_node(lemon, SYNTAX_KIND_STRING_LITERAL);
	node->buffer = arena_alloc(lemon, lemon->l_arena, length + 1);
	node->length = length;
	node->buffer[node->length] = '\0';
	memcpy(node->buffer, buffer, length);

	return node;
}

struct syntax *
syntax_make_array_node(struct lemon *lemon,
                       struct syntax *expr_list)
{
	struct syntax *node;

	node = syntax_make_kind_node(lemon, SYNTAX_KIND_ARRAY);
	node->u.array.expr_list = expr_list;

	return node;
}

struct syntax *
syntax_make_element_node(struct lemon *lemon,
                         struct syntax *name,
                         struct syntax *value)
{
	struct syntax *node;

	node = syntax_make_kind_node(lemon, SYNTAX_KIND_ELEMENT);
	node->u.element.name = name;
	node->u.element.value = value;

	return node;
}

struct syntax *
syntax_make_dictionary_node(struct lemon *lemon,
                            struct syntax *element_list)
{
	struct syntax *node;

	node = syntax_make_kind_node(lemon, SYNTAX_KIND_DICTIONARY);
	node->u.dictionary.element_list = element_list;

	return node;
}

struct syntax *
syntax_make_unop_node(struct lemon *lemon,
                      int opkind,
                      struct syntax *left)
{
	struct syntax *node;

	node = syntax_make_kind_node(lemon, SYNTAX_KIND_UNOP);
	node->opkind = opkind;
	node->u.unop.left = left;

	return node;
}

struct syntax *
syntax_make_binop_node(struct lemon *lemon,
                       int opkind,
                       struct syntax *left,
                       struct syntax *right)
{
	struct syntax *node;

	node = syntax_make_kind_node(lemon, SYNTAX_KIND_BINOP);
	node->opkind = opkind;
	node->u.binop.left = left;
	node->u.binop.right = right;

	return node;
}

struct syntax *
syntax_make_get_item_node(struct lemon *lemon,
                          struct syntax *left,
                          struct syntax *right)
{
	struct syntax *node;

	node = syntax_make_kind_node(lemon, SYNTAX_KIND_GET_ITEM);
	node->u.get_item.left = left;
	node->u.get_item.right = right;

	return node;
}

struct syntax *
syntax_make_get_attr_node(struct lemon *lemon,
                          struct syntax *left,
                          struct syntax *right)
{
	struct syntax *node;

	node = syntax_make_kind_node(lemon, SYNTAX_KIND_GET_ATTR);
	node->u.get_attr.left = left;
	node->u.get_attr.right = right;

	return node;
}

struct syntax *
syntax_make_get_slice_node(struct lemon *lemon,
                           struct syntax *left,
                           struct syntax *start,
                           struct syntax *stop,
                           struct syntax *step)
{
	struct syntax *node;

	node = syntax_make_kind_node(lemon, SYNTAX_KIND_GET_SLICE);
	node->u.get_slice.left = left;
	node->u.get_slice.start = start;
	node->u.get_slice.stop = stop;
	node->u.get_slice.step = step;

	return node;
}

struct syntax *
syntax_make_argument_node(struct lemon *lemon,
                          struct syntax *name,
                          struct syntax *expr,
                          int argument_type)
{
	struct syntax *node;

	node = syntax_make_kind_node(lemon, SYNTAX_KIND_ARGUMENT);
	node->u.argument.name = name;
	node->u.argument.expr = expr;
	node->argument_type = argument_type;

	return node;
}

struct syntax *
syntax_make_call_node(struct lemon *lemon,
                      struct syntax *callable,
                      struct syntax *argument_list)
{
	struct syntax *node;

	node = syntax_make_kind_node(lemon, SYNTAX_KIND_CALL);
	node->u.call.callable = callable;
	node->u.call.argument_list = argument_list;

	return node;
}

struct syntax *
syntax_make_conditional_node(struct lemon *lemon,
                             struct syntax *expr,
                             struct syntax *true_expr,
                             struct syntax *false_expr)
{
	struct syntax *node;

	node = syntax_make_kind_node(lemon, SYNTAX_KIND_CONDITIONAL);
	node->u.conditional.expr = expr;
	node->u.conditional.true_expr = true_expr;
	node->u.conditional.false_expr = false_expr;

	return node;
}

struct syntax *
syntax_make_unpack_node(struct lemon *lemon,
                        struct syntax *expr_list)
{
	struct syntax *node;

	node = syntax_make_kind_node(lemon, SYNTAX_KIND_UNPACK);
	node->u.unpack.expr_list = expr_list;

	return node;
}

struct syntax *
syntax_make_assign_node(struct lemon *lemon,
                        int opkind,
                        struct syntax *left,
                        struct syntax *right)
{
	struct syntax *node;

	node = syntax_make_kind_node(lemon, SYNTAX_KIND_ASSIGN_STMT);
	node->opkind = opkind;
	node->u.assign_stmt.left = left;
	node->u.assign_stmt.right = right;

	return node;
}

struct syntax *
syntax_make_if_node(struct lemon *lemon,
                    struct syntax *condition,
                    struct syntax *thenblock,
                    struct syntax *elseblock)
{
	struct syntax *node;

	node = syntax_make_kind_node(lemon, SYNTAX_KIND_IF_STMT);
	node->u.if_stmt.expr = condition;
	node->u.if_stmt.then_block_stmt = thenblock;
	node->u.if_stmt.else_block_stmt = elseblock;

	return node;
}

struct syntax *
syntax_make_for_node(struct lemon *lemon,
                     struct syntax *init_expr,
                     struct syntax *cond_expr,
                     struct syntax *step_expr,
                     struct syntax *block_stmt)
{
	struct syntax *node;

	node = syntax_make_kind_node(lemon, SYNTAX_KIND_FOR_STMT);
	node->u.for_stmt.init_expr = init_expr;
	node->u.for_stmt.cond_expr = cond_expr;
	node->u.for_stmt.step_expr = step_expr;
	node->u.for_stmt.block_stmt = block_stmt;

	return node;
}

struct syntax *
syntax_make_forin_node(struct lemon *lemon,
                       struct syntax *name,
                       struct syntax *iter,
                       struct syntax *block_stmt)
{
	struct syntax *node;

	node = syntax_make_kind_node(lemon, SYNTAX_KIND_FORIN_STMT);
	node->u.forin_stmt.name = name;
	node->u.forin_stmt.iter = iter;
	node->u.forin_stmt.block_stmt = block_stmt;

	return node;
}

struct syntax *
syntax_make_while_node(struct lemon *lemon,
                       struct syntax *condition,
                       struct syntax *block)
{
	struct syntax *node;

	node = syntax_make_kind_node(lemon, SYNTAX_KIND_WHILE_STMT);
	node->u.while_stmt.expr = condition;
	node->u.while_stmt.block_stmt= block;

	return node;
}

struct syntax *
syntax_make_delete_node(struct lemon *lemon,
                        struct syntax *expr)
{
	struct syntax *node;

	node = syntax_make_kind_node(lemon, SYNTAX_KIND_DELETE_STMT);
	node->u.delete_stmt.expr = expr;

	return node;
}

struct syntax *
syntax_make_return_node(struct lemon *lemon,
                        struct syntax *expr)
{
	struct syntax *node;

	node = syntax_make_kind_node(lemon, SYNTAX_KIND_RETURN_STMT);
	node->u.return_stmt.expr = expr;

	return node;
}

struct syntax *
syntax_make_import_node(struct lemon *lemon,
                        struct syntax *name,
                        struct syntax *path_string)
{
	struct syntax *node;

	node = syntax_make_kind_node(lemon, SYNTAX_KIND_IMPORT_STMT);
	node->u.import_stmt.name = name;
	node->u.import_stmt.path_string = path_string;

	return node;
}

struct syntax *
syntax_make_var_node(struct lemon *lemon,
                     struct syntax *name,
                     struct syntax *expr)
{
	struct syntax *node;

	node = syntax_make_kind_node(lemon, SYNTAX_KIND_VAR_STMT);
	node->u.var_stmt.name = name;
	node->u.var_stmt.expr = expr;

	return node;
}

struct syntax *
syntax_make_accessor_node(struct lemon *lemon,
                          struct syntax *name,
                          struct syntax *expr)
{
	struct syntax *node;

	node = syntax_make_kind_node(lemon, SYNTAX_KIND_ACCESSOR);
	node->u.accessor.name = name;
	node->u.accessor.expr = expr;

	return node;
}

struct syntax *
syntax_make_parameter_node(struct lemon *lemon,
                           struct syntax *name,
                           struct syntax *expr,
                           struct syntax *accessor_list,
                           int parameter_type)
{
	struct syntax *node;

	node = syntax_make_kind_node(lemon, SYNTAX_KIND_PARAMETER);
	node->u.parameter.name = name;
	node->u.parameter.expr = expr;
	node->u.parameter.accessor_list = accessor_list;
	node->parameter_type = parameter_type;

	return node;
}

struct syntax *
syntax_make_define_node(struct lemon *lemon,
                        struct syntax *name,
                        struct syntax *parameter_list,
                        struct syntax *block_stmt)
{
	struct syntax *node;

	node = syntax_make_kind_node(lemon, SYNTAX_KIND_DEFINE_STMT);
	node->u.define_stmt.name = name;
	node->u.define_stmt.parameter_list = parameter_list;
	node->u.define_stmt.block_stmt = block_stmt;

	return node;
}

struct syntax *
syntax_make_class_node(struct lemon *lemon,
                       struct syntax *name,
                       struct syntax *super_list,
                       struct syntax *block_stmt)
{
	struct syntax *node;

	node = syntax_make_kind_node(lemon, SYNTAX_KIND_CLASS_STMT);
	node->u.class_stmt.name = name;
	node->u.class_stmt.super_list = super_list;
	node->u.class_stmt.block_stmt = block_stmt;

	return node;
}

struct syntax *
syntax_make_try_node(struct lemon *lemon,
                     struct syntax *try_block_stmt,
                     struct syntax *catch_stmt_list,
                     struct syntax *finally_block_stmt)
{
	struct syntax *node;

	node = syntax_make_kind_node(lemon, SYNTAX_KIND_TRY_STMT);
	node->u.try_stmt.try_block_stmt = try_block_stmt;
	node->u.try_stmt.catch_stmt_list = catch_stmt_list;
	node->u.try_stmt.finally_block_stmt = finally_block_stmt;

	return node;
}

struct syntax *
syntax_make_catch_node(struct lemon *lemon,
                       struct syntax *catch_type,
                       struct syntax *catch_name,
                       struct syntax *block_stmt)
{
	struct syntax *node;

	node = syntax_make_kind_node(lemon, SYNTAX_KIND_CATCH_STMT);
	node->u.catch_stmt.catch_type = catch_type;
	node->u.catch_stmt.catch_name = catch_name;
	node->u.catch_stmt.block_stmt = block_stmt;

	return node;
}

struct syntax *
syntax_make_throw_node(struct lemon *lemon,
                       struct syntax *expr)
{
	struct syntax *node;

	node = syntax_make_kind_node(lemon, SYNTAX_KIND_THROW_STMT);
	node->u.throw_stmt.expr = expr;

	return node;
}

struct syntax *
syntax_make_block_node(struct lemon *lemon,
                       struct syntax *stmt_list)
{
	struct syntax *node;

	node = syntax_make_kind_node(lemon, SYNTAX_KIND_BLOCK_STMT);
	node->u.block_stmt.stmt_list = stmt_list;

	return node;
}

struct syntax *
syntax_make_break_node(struct lemon *lemon)
{
	struct syntax *node;

	node = syntax_make_kind_node(lemon, SYNTAX_KIND_BREAK_STMT);

	return node;
}

struct syntax *
syntax_make_continue_node(struct lemon *lemon)
{
	struct syntax *node;

	node = syntax_make_kind_node(lemon, SYNTAX_KIND_CONTINUE_STMT);

	return node;
}

struct syntax *
syntax_make_module_node(struct lemon *lemon,
                        struct syntax *stmt_list)
{
	struct syntax *node;

	node = syntax_make_kind_node(lemon, SYNTAX_KIND_MODULE_STMT);
	node->u.module_stmt.stmt_list = stmt_list;

	return node;
}
