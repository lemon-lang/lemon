#include "lemon.h"
#include "arena.h"
#include "input.h"
#include "lexer.h"
#include "scope.h"
#include "symbol.h"
#include "syntax.h"
#include "parser.h"
#include "opcode.h"
#include "compiler.h"
#include "machine.h"
#include "generator.h"
#include "lmodule.h"
#include "lnumber.h"
#include "lstring.h"
#include "linteger.h"

#include <ctype.h>
#include <assert.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>

#ifndef STATICLINKED
#include <dlfcn.h>
#endif

#ifdef LINUX
#include <linux/limits.h> /* PATH_MAX */
#endif

static int
compiler_expr(struct lemon *lemon, struct syntax *node);

static int
compiler_stmt(struct lemon *lemon, struct syntax *node);

static int
compiler_block(struct lemon *lemon, struct syntax *node);

static int
node_is_getter(struct lemon *lemon, struct syntax *node)
{
	return strncmp(node->u.accessor.name->buffer,
	               "getter",
	               (size_t)node->u.accessor.name->length) == 0;
}

static int
node_is_setter(struct lemon *lemon, struct syntax *node)
{
	return strncmp(node->u.accessor.name->buffer,
	               "setter",
	               (size_t)node->u.accessor.name->length) == 0;
}

static int
node_is_integer(struct lemon *lemon, struct syntax *node)
{
	int i;

	for (i = 0; i < node->length; i++) {
		if (node->buffer[i] == '.') {
			return 0;
		}
	}

	return 1;
}

static char *
resolve_module_name(struct lemon *lemon, char *path)
{
	int i;
	int c;

	char *name;
	char *first;
	char *last;

	name = arena_alloc(lemon, lemon->l_arena, LEMON_NAME_MAX + 1);

	first = strrchr(path, '/');
	if (!first) {
		first = path;
	} else {
		first += 1;
	}

	last = strrchr(path, '.');
	if (!last) {
		last = path + strlen(path);
	}

	for (i = 0; i < LEMON_NAME_MAX && first != last; i++) {
		c = *first++;
		if (isalnum(c)) {
			name[i] = (char)c;
		} else {
			name[i] = '_';
		}
	}
	name[i] = 0;
	if (isdigit(name[0])) {
		name[0] = '_';
	}

	return name;
}

/*
 * make ./file/path to relative current open file
 */
static char *
resolve_module_path(struct lemon *lemon, struct syntax *node, char *path)
{
	char *first;
	char *resolved_name;
	char *resolved_path;

	if (path[0] != '.') {
		return path;
	}

	resolved_name = arena_alloc(lemon, lemon->l_arena, PATH_MAX);
	memset(resolved_name, 0, PATH_MAX);

	resolved_path = realpath(node->filename, resolved_name);
	if (!resolved_path) {
		return NULL;
	}

	first = strrchr(resolved_path, '/');
	assert(first);

	first[1] = '\0';
	strncat(resolved_path, path, PATH_MAX);

	return resolved_path;
}

#ifndef STATICLINKED
static int
module_path_is_native(struct lemon *lemon, char *path)
{
	char *first;

	first = strrchr(path, '.');
	if (!first) {
		return 0;
	}

	if (strcmp(first, ".lm") == 0 || strcmp(first, ".lemon") == 0) {
		return 0;
	}

	return 1;
}
#endif

static void
compiler_error(struct lemon *lemon, struct syntax *node, const char *message)
{
	fprintf(stderr,
	        "%s:%ld:%ld: error: %s",
	        node->filename,
	        node->line,
	        node->column,
	        message);
}

static void
compiler_error_undefined(struct lemon *lemon, struct syntax *node, char *name)
{
	char buffer[256];

	snprintf(buffer, sizeof(buffer), "undefined identifier '%s'\n", name);
	compiler_error(lemon, node, buffer);
}

static void
compiler_error_redefined(struct lemon *lemon, struct syntax *node, char *name)
{
	char buffer[256];

	snprintf(buffer, sizeof(buffer), "redefined identifier '%s'\n", name);
	compiler_error(lemon, node, buffer);
}

static int
compiler_const_object(struct lemon *lemon, struct lobject *object)
{
	int cpool;

	cpool = machine_add_const(lemon, object);
	generator_emit_const(lemon, cpool);

	return 1;
}

static int
compiler_const_string(struct lemon *lemon, char *buffer)
{
	struct lobject *object;

	object = lstring_create(lemon, buffer, strlen(buffer));

	return compiler_const_object(lemon, object);
}

static int
compiler_nil(struct lemon *lemon, struct syntax *node)
{
	if (!compiler_const_object(lemon, lemon->l_nil)) {
		return 0;
	}

	return 1;
}

static int
compiler_true(struct lemon *lemon, struct syntax *node)
{
	if (!compiler_const_object(lemon, lemon->l_true)) {
		return 0;
	}

	return 1;
}

static int
compiler_false(struct lemon *lemon, struct syntax *node)
{
	if (!compiler_const_object(lemon, lemon->l_false)) {
		return 0;
	}

	return 1;
}

static int
compiler_sentinel(struct lemon *lemon, struct syntax *node)
{
	if (!compiler_const_object(lemon, lemon->l_sentinel)) {
		return 0;
	}

	return 1;
}

static int
compiler_symbol(struct lemon *lemon, struct symbol *symbol)
{
	if (symbol->type == SYMBOL_LOCAL) {
		generator_emit_load(lemon, symbol->level, symbol->local);
	} else if (symbol->type == SYMBOL_EXCEPTION) {
		generator_emit_opcode(lemon, OPCODE_LOADEXC);
	} else {
		generator_emit_const(lemon, symbol->cpool);
	}

	return 1;
}

static int
compiler_array(struct lemon *lemon, struct syntax *node)
{
	int count;
	struct syntax *expr;

	count = 0;
	for (expr = node->u.array.expr_list; expr; expr = expr->sibling) {
		count += 1;
		if (!compiler_expr(lemon, expr)) {
			return 0;
		}
	}
	generator_emit_array(lemon, count);
	if (!lemon->l_stmt_enclosing) {
		generator_emit_opcode(lemon, OPCODE_POP);
	}

	return 1;
}

static int
compiler_dictionary(struct lemon *lemon, struct syntax *node)
{
	int count;
	struct syntax *element;

	count = 0;
	for (element = node->u.dictionary.element_list;
	     element;
	     element = element->sibling)
	{
		count += 2;
		if (!compiler_expr(lemon, element->u.element.value)) {
			return 0;
		}
		if (!compiler_expr(lemon, element->u.element.name)) {
			return 0;
		}
	}
	generator_emit_dictionary(lemon, count);
	if (!lemon->l_stmt_enclosing) {
		generator_emit_opcode(lemon, OPCODE_POP);
	}

	return 1;
}

static int
compiler_unop(struct lemon *lemon, struct syntax *node)
{
	struct syntax *stmt_enclosing;

	stmt_enclosing = lemon->l_stmt_enclosing;
	lemon->l_stmt_enclosing = node;
	if (!compiler_expr(lemon, node->u.unop.left)) {
		return 0;
	}
	lemon->l_stmt_enclosing = stmt_enclosing;

	switch (node->opkind) {
	case SYNTAX_OPKIND_BITWISE_NOT:
		generator_emit_opcode(lemon, OPCODE_BNOT);
		break;

	case SYNTAX_OPKIND_LOGICAL_NOT:
		generator_emit_opcode(lemon, OPCODE_LNOT);
		break;

	case SYNTAX_OPKIND_POS:
		generator_emit_opcode(lemon, OPCODE_POS);
		break;

	case SYNTAX_OPKIND_NEG:
		generator_emit_opcode(lemon, OPCODE_NEG);
		break;

	default:
		return 0;
	}

	return 1;
}

static int
compiler_binop(struct lemon *lemon, struct syntax *node)
{
	struct generator_label *l_exit;

	struct syntax *stmt_enclosing;

	stmt_enclosing = lemon->l_stmt_enclosing;
	lemon->l_stmt_enclosing = node;

	if (node->opkind == SYNTAX_OPKIND_LOGICAL_OR) {
		/*
		 *    left_expr
		 *    dup
		 *    jnz l_exit
		 *    pop
		 *    right_expr
		 * l_exit:
		 */

		/*
		 * if (left_expr || right_expr)
		 * if the left_expr is true, the right_expr won't execute
		 */
		l_exit = generator_make_label(lemon);

		if (!compiler_expr(lemon, node->u.binop.left)) {
			return 0;
		}

		generator_emit_opcode(lemon, OPCODE_DUP);
		generator_emit_jnz(lemon, l_exit);
		generator_emit_opcode(lemon, OPCODE_POP);

		if (!compiler_expr(lemon, node->u.binop.right)) {
			return 0;
		}

		generator_emit_label(lemon, l_exit);
	} else if (node->opkind == SYNTAX_OPKIND_LOGICAL_AND) {
		/*
		 *    left_expr
		 *    dup
		 *    jz l_exit
		 *    pop
		 *    right_expr
		 * l_exit:
		 */

		/*
		 * if (left_expr || right_expr)
		 * if the left_expr is false, the right_expr won't execute
		 */
		l_exit = generator_make_label(lemon);

		if (!compiler_expr(lemon, node->u.binop.left)) {
			return 0;
		}
		generator_emit_opcode(lemon, OPCODE_DUP);
		generator_emit_jz(lemon, l_exit);
		generator_emit_opcode(lemon, OPCODE_POP);

		if (!compiler_expr(lemon, node->u.binop.right)) {
			return 0;
		}

		generator_emit_label(lemon, l_exit);
	} else {
		if (!compiler_expr(lemon, node->u.binop.left)) {
			return 0;
		}

		if (!compiler_expr(lemon, node->u.binop.right)) {
			return 0;
		}
		lemon->l_stmt_enclosing = stmt_enclosing;

		switch (node->opkind) {
		case SYNTAX_OPKIND_ADD:
			generator_emit_opcode(lemon, OPCODE_ADD);
			break;

		case SYNTAX_OPKIND_SUB:
			generator_emit_opcode(lemon, OPCODE_SUB);
			break;

		case SYNTAX_OPKIND_MUL:
			generator_emit_opcode(lemon, OPCODE_MUL);
			break;

		case SYNTAX_OPKIND_DIV:
			generator_emit_opcode(lemon, OPCODE_DIV);
			break;

		case SYNTAX_OPKIND_MOD:
			generator_emit_opcode(lemon, OPCODE_MOD);
			break;

		case SYNTAX_OPKIND_SHL:
			generator_emit_opcode(lemon, OPCODE_SHL);
			break;

		case SYNTAX_OPKIND_SHR:
			generator_emit_opcode(lemon, OPCODE_SHR);
			break;

		case SYNTAX_OPKIND_GT:
			generator_emit_opcode(lemon, OPCODE_GT);
			break;

		case SYNTAX_OPKIND_GE:
			generator_emit_opcode(lemon, OPCODE_GE);
			break;

		case SYNTAX_OPKIND_LT:
			generator_emit_opcode(lemon, OPCODE_LT);
			break;

		case SYNTAX_OPKIND_LE:
			generator_emit_opcode(lemon, OPCODE_LE);
			break;

		case SYNTAX_OPKIND_EQ:
			generator_emit_opcode(lemon, OPCODE_EQ);
			break;

		case SYNTAX_OPKIND_NE:
			generator_emit_opcode(lemon, OPCODE_NE);
			break;

		case SYNTAX_OPKIND_IN:
			generator_emit_opcode(lemon, OPCODE_IN);
			break;

		case SYNTAX_OPKIND_BITWISE_AND:
			generator_emit_opcode(lemon, OPCODE_BAND);
			break;

		case SYNTAX_OPKIND_BITWISE_XOR:
			generator_emit_opcode(lemon, OPCODE_BXOR);
			break;

		case SYNTAX_OPKIND_BITWISE_OR:
			generator_emit_opcode(lemon, OPCODE_BOR);
			break;

		default:
			return 0;
		}
	}
	lemon->l_stmt_enclosing = stmt_enclosing;

	return 1;
}

static int
compiler_conditional(struct lemon *lemon, struct syntax *node)
{
	/*
	 *    expr
	 *    jz l_false
	 *    true_expr
	 *    jmp l_exit
	 * l_false:
	 *    false_expr
	 * l_exit:
	 */

	/*
	 *    expr
	 *    dup
	 *    jnz l_exit
	 *    pop
	 *    false_expr
	 * l_exit:
	 */

	struct generator_label *l_false;
	struct generator_label *l_exit;

	l_exit = generator_make_label(lemon);

	if (!compiler_expr(lemon, node->u.conditional.expr)) {
		return 0;
	}

	/* if not set true_expr then set condition as true_expr */
	if (node->u.conditional.true_expr) {
		l_false = generator_make_label(lemon);
		generator_emit_jz(lemon, l_false);
		if (!compiler_expr(lemon, node->u.conditional.true_expr)) {
			return 0;
		}
		generator_emit_jmp(lemon, l_exit);

		generator_emit_label(lemon, l_false);
		if (!compiler_expr(lemon, node->u.conditional.false_expr)) {
			return 0;
		}
	} else {
		generator_emit_opcode(lemon, OPCODE_DUP);
		generator_emit_jnz(lemon, l_exit);
		generator_emit_opcode(lemon, OPCODE_POP);
		if (!compiler_expr(lemon, node->u.conditional.false_expr)) {
			return 0;
		}
	}

	generator_emit_label(lemon, l_exit);

	return 1;
}

static int
compiler_argument(struct lemon *lemon, struct syntax *node)
{
	struct syntax *name;

	name = node->u.argument.name;
	if (node->argument_type == 0) {
		if (name) {
			if (!compiler_const_string(lemon, name->buffer)) {
				return 0;
			}
			if (!compiler_expr(lemon, node->u.argument.expr)) {
				return 0;
			}
			generator_emit_opcode(lemon, OPCODE_KARG);
		} else {
			if (!compiler_expr(lemon, node->u.argument.expr)) {
				return 0;
			}
		}
	} else if (node->argument_type == 1) {
		if (!compiler_expr(lemon, node->u.argument.expr)) {
			return 0;
		}
		generator_emit_opcode(lemon, OPCODE_VARG);
	} else if (node->argument_type == 2) {
		if (!compiler_expr(lemon, node->u.argument.expr)) {
			return 0;
		}
		generator_emit_opcode(lemon, OPCODE_VKARG);
	}

	return 1;
}

static int
compiler_call(struct lemon *lemon, struct syntax *node)
{
	/*
	 *    callable
	 *    argument n
	 *    ...
	 *    argument 0
	 *    call n
	 */

	int argc;
	struct syntax *argument;
	struct syntax *stmt_enclosing;

	stmt_enclosing = lemon->l_stmt_enclosing;
	lemon->l_stmt_enclosing = node;

	if (!compiler_expr(lemon, node->u.call.callable)) {
		return 0;
	}

	argc = 0;
	for (argument = node->u.call.argument_list;
	     argument;
	     argument = argument->sibling)
	{
		argc += 1;
		if (!compiler_argument(lemon, argument)) {
			return 0;
		}
	}
	generator_emit_call(lemon, argc);

	if (!stmt_enclosing) {
		/* dispose return value */
		generator_emit_opcode(lemon, OPCODE_POP);
	}
	lemon->l_stmt_enclosing = stmt_enclosing;

	return 1;
}

static int
compiler_tailcall(struct lemon *lemon, struct syntax *node)
{
	int argc;
	struct syntax *argument;
	struct syntax *stmt_enclosing;

	stmt_enclosing = lemon->l_stmt_enclosing;
	lemon->l_stmt_enclosing = node;

	if (!compiler_expr(lemon, node->u.call.callable)) {
		return 0;
	}

	argc = 0;
	for (argument = node->u.call.argument_list;
	     argument;
	     argument = argument->sibling)
	{
		argc += 1;
		if (!compiler_argument(lemon, argument)) {
			return 0;
		}
	}
	generator_emit_tailcall(lemon, argc);

	lemon->l_stmt_enclosing = stmt_enclosing;

	return 1;
}

static int
compiler_name(struct lemon *lemon, struct syntax *node)
{
	int i;
	int len;

	struct symbol *symbol;
	struct syntax *accessor;
	struct syntax *accessors[128];
	struct syntax *stmt_enclosing;

	symbol = scope_get_symbol(lemon, lemon->l_scope, node->buffer);
	if (!symbol) {
		compiler_error_undefined(lemon, node, node->buffer);

		return 0;
	}

	len = 0;
	for (accessor = symbol->accessor_list;
	     accessor;
	     accessor = accessor->sibling)
	{
		if (node_is_getter(lemon, accessor)) {
			accessors[len++] = accessor;

			if (len == 128) {
				compiler_error(lemon,
				               node,
				               "too many getters\n");

				return 0;
			}
		}
	}

	if (!len) {
		if (!compiler_symbol(lemon, symbol)) {
			return 0;
		}
	}

	stmt_enclosing = lemon->l_stmt_enclosing;
	lemon->l_stmt_enclosing = node;
	for (i = len - 1; i >= 0; i--) {
		accessor = accessors[i];

		if (!compiler_expr(lemon, accessor->u.accessor.expr)) {
			return 0;
		}

		if (i == 0) {
			if (!compiler_symbol(lemon, symbol)) {
				return 0;
			}
		}
	}
	lemon->l_stmt_enclosing = stmt_enclosing;

	for (i = 0; i < len; i++) {
		generator_emit_call(lemon, 1);
	}

	return 1;
}

static int
compiler_store(struct lemon *lemon, struct syntax *node)
{
	struct syntax *accessor;
	struct symbol *symbol;

	if (node->kind == SYNTAX_KIND_NAME) {
		symbol = scope_get_symbol(lemon,
		                          lemon->l_scope,
		                          node->buffer);
		if (!symbol) {
			compiler_error_undefined(lemon, node, node->buffer);

			return 0;
		}

		if (symbol->type != SYMBOL_LOCAL) {
			char buffer[256];

			snprintf(buffer,
			         sizeof(buffer),
			         "'%s' is not assignable\n",
			         symbol->name);
			compiler_error(lemon, node, buffer);

			return 0;
		}

		for (accessor = symbol->accessor_list;
		     accessor;
		     accessor = accessor->sibling)
		{
			if (node_is_setter(lemon, accessor)) {
				if (!compiler_expr(lemon, accessor)) {
					return 0;
				}
				generator_emit_opcode(lemon, OPCODE_SWAP);
				generator_emit_call(lemon, 1);
			}
		}

		generator_emit_store(lemon, symbol->level, symbol->local);
	} else if (node->kind == SYNTAX_KIND_GET_ATTR) {
		char *buffer;

		if (!compiler_expr(lemon, node->u.get_attr.left)) {
			return 0;
		}

		buffer = node->u.get_attr.right->buffer;
		if (!compiler_const_string(lemon, buffer)) {
			return 0;
		}
		generator_emit_opcode(lemon, OPCODE_SETATTR);
	} else if (node->kind == SYNTAX_KIND_GET_ITEM) {
		compiler_expr(lemon, node->u.get_item.left);
		compiler_expr(lemon, node->u.get_item.right);
		generator_emit_opcode(lemon, OPCODE_SETITEM);
	} else {
		compiler_error(lemon, node, "unsupport assignment\n");

		return 0;
	}

	return 1;
}

static int
compiler_assign_stmt(struct lemon *lemon, struct syntax *node)
{
	struct syntax *left;
	struct syntax *right;
	struct syntax *element;
	struct syntax *stmt_enclosing;

	stmt_enclosing = lemon->l_stmt_enclosing;
	lemon->l_stmt_enclosing = node;

	left = node->u.assign_stmt.left;
	right = node->u.assign_stmt.right;

	if (node->opkind != SYNTAX_OPKIND_ASSIGN) {
		if (left->kind == SYNTAX_KIND_NAME ||
		    left->kind == SYNTAX_KIND_GET_ATTR ||
		    left->kind == SYNTAX_KIND_GET_ITEM)
		{
			if (!compiler_expr(lemon, left)) {
				return 0;
			}
		} else {
			compiler_error(lemon, left, "unsupport operator\n");

			return 0;
		}
	}

	if (!compiler_expr(lemon, right)) {
		return 0;
	}

	switch (node->opkind) {
	case SYNTAX_OPKIND_ASSIGN:
		/* do nothing */
		break;

	case SYNTAX_OPKIND_ADD_ASSIGN:
		generator_emit_opcode(lemon, OPCODE_ADD);
		break;

	case SYNTAX_OPKIND_SUB_ASSIGN:
		generator_emit_opcode(lemon, OPCODE_SUB);
		break;

	case SYNTAX_OPKIND_MUL_ASSIGN:
		generator_emit_opcode(lemon, OPCODE_MUL);
		break;

	case SYNTAX_OPKIND_DIV_ASSIGN:
		generator_emit_opcode(lemon, OPCODE_DIV);
		break;

	case SYNTAX_OPKIND_MOD_ASSIGN:
		generator_emit_opcode(lemon, OPCODE_MOD);
		break;

	case SYNTAX_OPKIND_SHL_ASSIGN:
		generator_emit_opcode(lemon, OPCODE_SHL);
		break;

	case SYNTAX_OPKIND_SHR_ASSIGN:
		generator_emit_opcode(lemon, OPCODE_SHR);
		break;

	case SYNTAX_OPKIND_BITWISE_AND_ASSIGN:
		generator_emit_opcode(lemon, OPCODE_BAND);
		break;

	case SYNTAX_OPKIND_BITWISE_XOR_ASSIGN:
		generator_emit_opcode(lemon, OPCODE_BXOR);
		break;

	case SYNTAX_OPKIND_BITWISE_OR_ASSIGN:
		generator_emit_opcode(lemon, OPCODE_BOR);
		break;

	default:
		compiler_error(lemon, left, "unsupport operator\n");
		return 0;
	}

	if (left->kind == SYNTAX_KIND_UNPACK) {
		int unpack;

		unpack = 0;
		for (element = left->u.unpack.expr_list;
		     element;
		     element = element->sibling)
		{
			unpack += 1;
		}

		generator_emit_unpack(lemon, unpack);
		for (element = left->u.unpack.expr_list;
		     element;
		     element = element->sibling)
		{
			if (!compiler_store(lemon, element)) {
				return 0;
			}
		}
	} else {
		if (!compiler_store(lemon, left)) {
			return 0;
		}
	}
	lemon->l_stmt_enclosing = stmt_enclosing;

	return 1;
}

static int
compiler_var_stmt(struct lemon *lemon, struct syntax *node)
{
	struct symbol *symbol;
	struct syntax *stmt_enclosing;
	struct syntax *space_enclosing;

	stmt_enclosing = lemon->l_stmt_enclosing;
	lemon->l_stmt_enclosing = node;

	symbol = scope_add_symbol(lemon,
	                          lemon->l_scope,
	                          node->u.var_stmt.name->buffer,
	                          SYMBOL_LOCAL);
	if (!symbol) {
		compiler_error_redefined(lemon,
		                         node->u.var_stmt.name,
		                         node->u.var_stmt.name->buffer);

		return 0;
	}

	space_enclosing = lemon->l_space_enclosing;
	symbol->accessor_list = node->u.var_stmt.accessor_list;
	symbol->local = space_enclosing->nlocals++;

	if (node->u.var_stmt.expr) {
		if (!compiler_expr(lemon, node->u.var_stmt.expr)) {
			return 0;
		}

		generator_emit_store(lemon, 0, symbol->local);
	}

	lemon->l_stmt_enclosing = stmt_enclosing;

	return 1;
}

static int
compiler_if_stmt(struct lemon *lemon, struct syntax *node)
{
	/*
	 *    expr
	 *    jz l_else
	 *    then_block_stmt
	 *    jmp l_exit
	 * l_else:
	 *    else_block_stmt
	 * l_exit:
	 */

	struct generator_label *l_else;
	struct generator_label *l_exit;
	struct syntax *stmt_enclosing;

	stmt_enclosing = lemon->l_stmt_enclosing;
	lemon->l_stmt_enclosing = node;
	if (!compiler_expr(lemon, node->u.if_stmt.expr)) {
		return 0;
	}
	lemon->l_stmt_enclosing = stmt_enclosing;

	l_else = generator_make_label(lemon);
	l_exit = generator_make_label(lemon);

	generator_emit_jz(lemon, l_else);
	if (!compiler_block(lemon, node->u.if_stmt.then_block_stmt)) {
		return 0;
	}
	if (node->u.if_stmt.else_block_stmt) {
		generator_emit_jmp(lemon, l_exit);
		generator_emit_label(lemon, l_else);
		if (!compiler_stmt(lemon, node->u.if_stmt.else_block_stmt)) {
			return 0;
		}
	} else {
		generator_emit_label(lemon, l_else);
	}
	generator_emit_label(lemon, l_exit);

	return 1;
}

static int
compiler_for_stmt(struct lemon *lemon, struct syntax *node)
{
	/*
	 *    init_expr
	 * l_cond:
	 *    cond_expr
	 *    jz l_exit
	 *    block_stmt
	 * l_step:          ; l_continue
	 *    step_expr
	 *    jmp l_expr
	 * l_exit:
	 */

	struct generator_label *l_cond;
	struct generator_label *l_step;
	struct generator_label *l_exit;

	struct syntax *init_expr;
	struct syntax *step_expr;
	struct syntax *stmt_enclosing;
	struct syntax *loop_enclosing;

	loop_enclosing = lemon->l_loop_enclosing;
	lemon->l_loop_enclosing = node;

	l_cond = generator_make_label(lemon);
	l_step = generator_make_label(lemon);
	l_exit = generator_make_label(lemon);

	stmt_enclosing = lemon->l_stmt_enclosing;
	lemon->l_stmt_enclosing = node;

	init_expr = node->u.for_stmt.init_expr;
	if (init_expr->kind == SYNTAX_KIND_VAR_STMT) {
		if (!compiler_var_stmt(lemon, init_expr)) {
			return 0;
		}
	} else if (init_expr->kind == SYNTAX_KIND_ASSIGN_STMT) {
		if (!compiler_assign_stmt(lemon, init_expr)) {
			return 0;
		}
	} else {
		if (!compiler_expr(lemon, init_expr)) {
			return 0;
		}
	}

	generator_emit_label(lemon, l_cond);
	if (!compiler_expr(lemon, node->u.for_stmt.cond_expr)) {
		return 0;
	}
	lemon->l_stmt_enclosing = stmt_enclosing;
	generator_emit_jz(lemon, l_exit);

	node->l_break = l_exit;
	node->l_continue = l_step;
	if (!compiler_block(lemon, node->u.for_stmt.block_stmt)) {
		return 0;
	}

	stmt_enclosing = lemon->l_stmt_enclosing;
	lemon->l_stmt_enclosing = node;
	generator_emit_label(lemon, l_step);
	step_expr = node->u.for_stmt.step_expr;
	if (step_expr->kind == SYNTAX_KIND_ASSIGN_STMT) {
		if (!compiler_assign_stmt(lemon, step_expr)) {
			return 0;
		}
	} else {
		if (!compiler_expr(lemon, step_expr)) {
			return 0;
		}
	}
	lemon->l_stmt_enclosing = stmt_enclosing;
	generator_emit_jmp(lemon, l_cond);
	generator_emit_label(lemon, l_exit);
	lemon->l_loop_enclosing = loop_enclosing;

	return 1;
}

static int
compiler_forin_stmt(struct lemon *lemon, struct syntax *node)
{
	/*
	 *    iter
	 *    const "__iterator__"
	 *    getattr
	 *    call 0
	 *    store local
	 * l_next:
	 *    store local
	 *    const "__next__"
	 *    getattr
	 *    call 0
	 *    dup
	 *    store name
	 *    const sentinal
	 *    eq
	 *    jnz l_exit
	 *    step_expr
	 *    block_stmt
	 *    jmp l_next
	 * l_exit:
	 */

	int local;

	struct generator_label *l_next;
	struct generator_label *l_exit;

	struct syntax *name;
	struct syntax *stmt_enclosing;
	struct syntax *loop_enclosing;
	struct syntax *space_enclosing;
	struct symbol *symbol;

	space_enclosing = lemon->l_space_enclosing;
	local = space_enclosing->nlocals++;

	loop_enclosing = lemon->l_loop_enclosing;
	lemon->l_loop_enclosing = node;

	scope_enter(lemon, SCOPE_BLOCK);

	l_next = generator_make_label(lemon);
	l_exit = generator_make_label(lemon);

	stmt_enclosing = lemon->l_stmt_enclosing;
	lemon->l_stmt_enclosing = node;
	if (!compiler_expr(lemon, node->u.forin_stmt.iter)) {
		return 0;
	}

	if (!compiler_const_object(lemon, lemon->l_iterator_string)) {
		return 0;
	}
	lemon->l_stmt_enclosing = stmt_enclosing;
	generator_emit_opcode(lemon, OPCODE_GETATTR);
	generator_emit_call(lemon, 0);
	generator_emit_store(lemon, 0, local);

	generator_emit_label(lemon, l_next);

	generator_emit_load(lemon, 0, local);
	if (!compiler_const_object(lemon, lemon->l_next_string)) {
		return 0;
	}
	generator_emit_opcode(lemon, OPCODE_GETATTR);
	generator_emit_call(lemon, 0);
	generator_emit_opcode(lemon, OPCODE_DUP); /* store and cmp */

	name = node->u.forin_stmt.name;
	if (name->kind == SYNTAX_KIND_VAR_STMT) {
		name = name->u.var_stmt.name;
		symbol = scope_add_symbol(lemon,
		                          lemon->l_scope,
		                          name->buffer,
		                          SYMBOL_LOCAL);
		if (!symbol) {
			compiler_error_redefined(lemon, name, name->buffer);

			return 0;
		}
		symbol->local = space_enclosing->nlocals++;
	} else {
		symbol = scope_get_symbol(lemon, lemon->l_scope, name->buffer);
		if (!symbol) {
			compiler_error_undefined(lemon,
			                         name, name->buffer);

			return 0;
		}
	}
	generator_emit_store(lemon, symbol->level, symbol->local);

	if (!compiler_const_object(lemon, lemon->l_sentinel)) {
		return 0;
	}
	generator_emit_opcode(lemon, OPCODE_EQ);
	generator_emit_jnz(lemon, l_exit);

	node->l_break = l_exit;
	node->l_continue = l_next;

	if (!compiler_block(lemon, node->u.forin_stmt.block_stmt)) {
		return 0;
	}
	generator_emit_jmp(lemon, l_next);
	generator_emit_label(lemon, l_exit);

	lemon->l_loop_enclosing = loop_enclosing;
	scope_leave(lemon);

	return 1;
}

static int
compiler_while_stmt(struct lemon *lemon, struct syntax *node)
{
	/*
	 * l_cond:
	 *    expr
	 *    jz l_exit
	 *    block_stmt
	 *    jmp l_cond
	 * l_exit:
	 */

	struct generator_label *l_cond;
	struct generator_label *l_exit;

	struct syntax *stmt_enclosing;
	struct syntax *loop_enclosing;

	loop_enclosing = lemon->l_loop_enclosing;
	lemon->l_loop_enclosing = node;

	l_cond = generator_make_label(lemon);
	l_exit = generator_make_label(lemon);

	node->l_break = l_exit;
	node->l_continue = l_cond;

	stmt_enclosing = lemon->l_stmt_enclosing;
	lemon->l_stmt_enclosing = node;
	generator_emit_label(lemon, l_cond);
	if (!compiler_expr(lemon, node->u.while_stmt.expr)) {
		return 0;
	}
	lemon->l_stmt_enclosing = stmt_enclosing;
	generator_emit_jz(lemon, l_exit);

	if (!compiler_block(lemon, node->u.while_stmt.block_stmt)) {
		return 0;
	}
	generator_emit_jmp(lemon, l_cond);

	generator_emit_label(lemon, l_exit);
	lemon->l_loop_enclosing = loop_enclosing;

	return 1;
}

static int
compiler_break_stmt(struct lemon *lemon, struct syntax *node)
{
	struct syntax *loop_enclosing;

	loop_enclosing = lemon->l_loop_enclosing;
	if (!loop_enclosing) {
		compiler_error(lemon, node, "'break' statement not in loop\n");

		return 0;
	}
	generator_emit_jmp(lemon, loop_enclosing->l_break);

	return 1;
}

static int
compiler_continue_stmt(struct lemon *lemon, struct syntax *node)
{
	struct syntax *loop_enclosing;

	loop_enclosing = lemon->l_loop_enclosing;
	if (!loop_enclosing) {
		compiler_error(lemon,
		               node,
		               "'continue' statement not in loop\n");

		return 0;
	}
	generator_emit_jmp(lemon, loop_enclosing->l_continue);

	return 1;
}

static int
compiler_parameter(struct lemon *lemon, struct syntax *node)
{
	/*
	 *    const sentinel
	 *    load parameter
	 *    eq
	 *    jz l_exit
	 *    expr
	 *    store parameter
	 * l_exit:
	 */

	struct generator_label *l_exit;

	struct syntax *accessor;
	struct syntax *stmt_enclosing;
	struct syntax *space_enclosing;

	struct symbol *symbol;

	space_enclosing = lemon->l_space_enclosing;
	if (space_enclosing->kind != SYNTAX_KIND_DEFINE_STMT) {
		return 0;
	}
	space_enclosing->nparams++;

	symbol = scope_add_symbol(lemon,
	                          lemon->l_scope,
	                          node->u.parameter.name->buffer,
	                          SYMBOL_LOCAL);
	if (!symbol) {
		compiler_error_redefined(lemon,
		                         node->u.parameter.name,
		                         node->u.parameter.name->buffer);

		return 0;
	}
	symbol->local = space_enclosing->nlocals++;
	symbol->accessor_list = node->u.parameter.accessor_list;

	if (node->u.parameter.expr) {
		space_enclosing->nvalues++;

		l_exit = generator_make_label(lemon);

		/* make compare with sentinel(unset) */
		if (!compiler_const_object(lemon, lemon->l_sentinel)) {
			return 0;
		}
		generator_emit_load(lemon, 0, symbol->local);
		generator_emit_opcode(lemon, OPCODE_EQ);
		generator_emit_jz(lemon, l_exit);

		stmt_enclosing = lemon->l_stmt_enclosing;
		lemon->l_stmt_enclosing = node;
		if (!compiler_expr(lemon, node->u.parameter.expr)) {
			return 0;
		}
		lemon->l_stmt_enclosing = stmt_enclosing;

		generator_emit_store(lemon, 0, symbol->local);
		generator_emit_label(lemon, l_exit);

	}

	if (symbol->accessor_list) {
		generator_emit_load(lemon, symbol->level, symbol->local);
		for (accessor = symbol->accessor_list;
		     accessor;
		     accessor = accessor->sibling)
		{
			if (node_is_setter(lemon, accessor)) {
				if (!compiler_expr(lemon, accessor)) {
					return 0;
				}
				generator_emit_opcode(lemon, OPCODE_SWAP);
				generator_emit_call(lemon, 1);
			}
		}
	}

	return 1;
}

static int
compiler_define_stmt(struct lemon *lemon, struct syntax *node)
{
	/*
	 *    const parameter_name_0
	 *    ...
	 *    const parameter_name_n
	 *
	 *    const function_name
	 *    define define, nvalues, nparams, nlocals, l_exit
	 *    parameter_0
	 *    ...
	 *    parameter_n
	 *    block_stmt
	 *    const sentinel
	 *    return
	 * l_exit:
	 */

	int define;
	struct generator_label *l_exit;
	struct generator_code *c_define;

	struct scope *scope;
	struct symbol *symbol;

	struct syntax *name;
	struct syntax *parameter;

	struct syntax *try_enclosing;
	struct syntax *stmt_enclosing;
	struct syntax *space_enclosing;

	l_exit = generator_make_label(lemon);

	/* generate default parameters value */
	for (parameter = node->u.define_stmt.parameter_list;
	     parameter;
	     parameter = parameter->sibling)
	{
		name = parameter->u.parameter.name;
		if (!compiler_const_string(lemon, name->buffer)) {
			return 0;
		}
	}

	space_enclosing = lemon->l_space_enclosing;
	lemon->l_space_enclosing = node;

	scope = lemon->l_scope;
	scope_enter(lemon, SCOPE_DEFINE);

	name = node->u.define_stmt.name;
	if (name) {
		symbol = scope_add_symbol(lemon,
		                          scope,
		                          name->buffer,
		                          SYMBOL_LOCAL);
		if (!symbol) {
			compiler_error_redefined(lemon, name, name->buffer);

			return 0;
		}
		symbol->accessor_list = node->u.define_stmt.accessor_list;
		if (!compiler_const_string(lemon, name->buffer)) {
			return 0;
		}
		symbol->local = space_enclosing->nlocals++;;
	} else {
		if (!compiler_const_string(lemon, "")) {
			return 0;
		}
		symbol = NULL;
	}

	/* 'define' will create a function object and jmp function end */
	c_define = generator_emit_define(lemon, 0, 0, 0, 0, 0);

	try_enclosing = lemon->l_try_enclosing;
	lemon->l_try_enclosing = NULL;

	define = 0;
	/* generate function parameters name(for call keyword arguments) */
	for (parameter = node->u.define_stmt.parameter_list;
	     parameter;
	     parameter = parameter->sibling)
	{
		if (parameter->parameter_type == 0 && define == 0) {
			/* define func(var a, var b, var c) */
			define = 0;
		} else if (parameter->parameter_type == 1 && define == 0) {
			/* define func(var a, var b, var *c) */
			define = 1;
		} else if (parameter->parameter_type == 2 && define == 0) {
			/* define func(var a, var b, var **c) */
			define = 2;
		} else if (parameter->parameter_type == 2 && define == 1) {
			/* define func(var a, var *b, var **c) */
			define = 3;
		} else {
			compiler_error(lemon, node, "wrong parameter type\n");

			return 0;
		}

		if (!compiler_parameter(lemon, parameter)) {
			return 0;
		}
	}

	stmt_enclosing = lemon->l_stmt_enclosing;
	lemon->l_stmt_enclosing = NULL;

	if (!compiler_block(lemon, node->u.define_stmt.block_stmt)) {
		return 0;
	}

	if (!node->u.define_stmt.block_stmt->has_return) {
		if (!compiler_const_object(lemon, lemon->l_nil)) {
			return 0;
		}
		generator_emit_opcode(lemon, OPCODE_RETURN);
	}

	generator_emit_label(lemon, l_exit);

	/* patch define */
	generator_patch_define(lemon,
	                       c_define,
	                       define,
	                       node->nvalues,
	                       node->nparams,
	                       node->nlocals,
	                       l_exit);

	/* if define in class don't emit store */
	if (scope && scope->type != SCOPE_CLASS) {
		if (name) {
			/*
			 * 'store' will pop 'define' function object
			 * so if we're in assignment or call stmt,
			 * 'dup' function for assignment or call
			 * like 'var foo = define func(){};'
			 * or 'define func(){}();'
			 */
			if (stmt_enclosing) {
				generator_emit_opcode(lemon, OPCODE_DUP);
			}
			generator_emit_store(lemon, 0, symbol->local);
		}
	}

	scope_leave(lemon);
	lemon->l_try_enclosing = try_enclosing;
	lemon->l_stmt_enclosing = stmt_enclosing;
	lemon->l_space_enclosing = space_enclosing;

	return 1;
}

static int
compiler_delete_stmt(struct lemon *lemon, struct syntax *node)
{
	struct syntax *expr;
	struct syntax *stmt_enclosing;

	stmt_enclosing = lemon->l_stmt_enclosing;
	lemon->l_stmt_enclosing = node;
	if (!node->u.delete_stmt.expr) {
		return 0;
	}

	expr = node->u.return_stmt.expr;
	if (expr->kind == SYNTAX_KIND_GET_ITEM) {
		if (!compiler_expr(lemon, expr->u.get_item.left)) {
			return 0;
		}
		if (!compiler_expr(lemon, expr->u.get_item.right)) {
			return 0;
		}
		generator_emit_opcode(lemon, OPCODE_DELITEM);
	} else if (expr->kind == SYNTAX_KIND_GET_ATTR) {
		if (!compiler_expr(lemon, expr->u.get_attr.left)) {
			return 0;
		}
		if (!compiler_const_string(lemon,
		                           expr->u.get_attr.right->buffer))
		{
			return 0;
		}
		generator_emit_opcode(lemon, OPCODE_DELATTR);
	} else {
		compiler_error(lemon, node, "unsupport delete stmt\n");

		return 0;
	}
	lemon->l_stmt_enclosing = stmt_enclosing;

	return 1;
}

static int
compiler_return_stmt(struct lemon *lemon, struct syntax *node)
{
	struct syntax *try_enclosing;
	struct syntax *stmt_enclosing;
	struct syntax *space_enclosing;

	space_enclosing = lemon->l_space_enclosing;
	if (space_enclosing->kind != SYNTAX_KIND_DEFINE_STMT) {
		compiler_error(lemon,
		               node,
		               "'return' statement not in function\n");

		return 0;
	}

	try_enclosing = lemon->l_try_enclosing;
	if (try_enclosing && try_enclosing->l_finally) {
		/* compiling return's expr but not return */
		if (node->u.return_stmt.expr) {
			if (!compiler_expr(lemon, node->u.return_stmt.expr)) {
				return 0;
			}
		}
		generator_emit_jmp(lemon, try_enclosing->l_finally);
	} else {
		stmt_enclosing = lemon->l_stmt_enclosing;
		lemon->l_stmt_enclosing = node;
		if (node->u.return_stmt.expr) {
			struct syntax *expr;

			expr = node->u.return_stmt.expr;
			if (expr->kind == SYNTAX_KIND_CALL) {
				if (!compiler_tailcall(lemon, expr)) {
					return 0;
				}
			} else {
				if (!compiler_expr(lemon, expr)) {
					return 0;
				}
			}
		} else {
			if (!compiler_const_object(lemon, lemon->l_nil)) {
				return 0;
			}
		}
		node->has_return = 1;
		generator_emit_opcode(lemon, OPCODE_RETURN);
		lemon->l_stmt_enclosing = stmt_enclosing;
	}
	/* remove all code after return stmt */
	node->sibling = NULL;

	return 1;
}

static int
compiler_class_stmt(struct lemon *lemon, struct syntax *node)
{
	/*
	 *    attr_value
	 *    attr_name
	 *    attr_value
	 *    attr_name
	 *    super_n
	 *    ...
	 *    super_1
	 *    class nsupers, nattrs
	 */

	int local;
	int nattrs;
	int nsupers;
	char *class_name;

	struct syntax *name;
	struct syntax *stmt;
	struct syntax *block;
	struct syntax *super_expr;
	struct syntax *stmt_enclosing;
	struct syntax *space_enclosing;
	struct symbol *symbol;

	space_enclosing = lemon->l_space_enclosing;
	local = space_enclosing->nlocals++;

	name = node->u.class_stmt.name;
	if (name) {
		symbol = scope_add_symbol(lemon,
		                          lemon->l_scope,
		                          name->buffer,
		                          SYMBOL_LOCAL);
		if (!symbol) {
			compiler_error_redefined(lemon, name, name->buffer);

			return 0;
		}
		symbol->local = local;
		symbol->accessor_list = node->u.class_stmt.accessor_list;
		class_name = name->buffer;
	} else {
		class_name = "";
	}

	scope_enter(lemon, SCOPE_CLASS);
	/* generate name and value pair */
	nattrs = 0;
	block = node->u.class_stmt.block_stmt;
	for (stmt = block->u.block_stmt.stmt_list;
	     stmt;
	     stmt = stmt->sibling)
	{
		nattrs += 2;
		if (stmt->kind == SYNTAX_KIND_VAR_STMT) {
			if (stmt->u.var_stmt.expr) {
				stmt_enclosing = lemon->l_stmt_enclosing;
				lemon->l_stmt_enclosing = stmt;
				if (!compiler_expr(lemon,
				                   stmt->u.var_stmt.expr))
				{
					return 0;
				}
				lemon->l_stmt_enclosing = stmt_enclosing;
			} else {
				if (!compiler_const_object(lemon,
				                           lemon->l_nil))
				{
					return 0;
				}
			}
			name = stmt->u.var_stmt.name;
		} else if (stmt->kind == SYNTAX_KIND_DEFINE_STMT) {
			if (!compiler_define_stmt(lemon, stmt)) {
				return 0;
			}
			name = stmt->u.define_stmt.name;
		} else {
			compiler_error(lemon,
			               node,
			               "class define only support "
			               "`define' and `var'\n");

			return 0;
		}

		if (!compiler_const_string(lemon, name->buffer)) {
			return 0;
		}
	}
	scope_leave(lemon);

	/* generate super list */
	nsupers = 0;
	for (super_expr = node->u.class_stmt.super_list;
	     super_expr;
	     super_expr = super_expr->sibling)
	{
		nsupers += 1;
		if (!compiler_expr(lemon, super_expr)) {
			return 0;
		}
	}

	if (!compiler_const_string(lemon, class_name)) {
		return 0;
	}

	generator_emit_class(lemon, nsupers, nattrs);

	/*
	 * 'store' will pop 'class' object
	 * so if we're in assignment or call stmt,
	 * 'dup' object for assignment or call
	 * like 'var foo = class bar{};' or 'class foo{}();'
	 */
	stmt_enclosing = lemon->l_stmt_enclosing;
	if (stmt_enclosing) {
		generator_emit_opcode(lemon, OPCODE_DUP);
	}

	generator_emit_store(lemon, 0, local);

	/* generate class getter and setters */
	block = node->u.class_stmt.block_stmt;
	for (stmt = block->u.block_stmt.stmt_list;
	     stmt;
	     stmt = stmt->sibling)
	{
		int n;
		struct syntax *accessor;
		struct syntax *accessor_list;

		if (stmt->kind == SYNTAX_KIND_VAR_STMT) {
			name = stmt->u.var_stmt.name;
			accessor_list = stmt->u.var_stmt.accessor_list;
		} else if (stmt->kind == SYNTAX_KIND_CLASS_STMT) {
			name = stmt->u.class_stmt.name;
			accessor_list = stmt->u.class_stmt.accessor_list;
		} else if (stmt->kind == SYNTAX_KIND_DEFINE_STMT) {
			name = stmt->u.define_stmt.name;
			accessor_list = stmt->u.define_stmt.accessor_list;
		} else {
			return 0;
		}

		if (accessor_list) {
			lemon->l_stmt_enclosing = stmt;

			n = 0;
			for (accessor = accessor_list;
			     accessor;
			     accessor = accessor->sibling)
			{
				if (node_is_getter(lemon, accessor)) {
					if (!compiler_expr(lemon, accessor)) {
						return 0;
					}
					n += 1;
				}
			}
			if (n) {
				generator_emit_load(lemon, 0, local);
				if (!compiler_const_string(lemon,
				                           name->buffer))
				{
					return 0;
				}
				generator_emit_setgetter(lemon, n);
			}

			n = 0;
			for (accessor = accessor_list;
			     accessor;
			     accessor = accessor->sibling)
			{
				if (node_is_setter(lemon, accessor)) {
					if (!compiler_expr(lemon, accessor)) {
						return 0;
					}
					n += 1;
				}
			}

			if (n) {
				generator_emit_load(lemon, 0, local);
				if (!compiler_const_string(lemon,
				                           name->buffer))
				{
					return 0;
				}
				generator_emit_setsetter(lemon, n);
			}
			lemon->l_stmt_enclosing = stmt_enclosing;
		}
	}

	return 1;
}

static int
compiler_try_stmt(struct lemon *lemon, struct syntax *node)
{
	/* try */
	/*
	 *    try: l_catch
	 *    try_block_stmt
	 * l_catched_finally:
	 *    finally_block_stmt
	 *    jmp l_exit
	 * l_catch:
	 *    ...
	 *    catch_stmt
	 *    ...
	 *    finally_block_stmt
	 *    untry
	 *    loadexc
	 *    throw
	 */

	/* catch */
	/*
	 *    loadexc
	 *    const "__instanceof__"
	 *    getattr
	 *    catch_type
	 *    call 1           ; exception.__instanceof__(catch_type)
	 *    jz l_catch_exit
	 *    block_stmt
	 *    jmp l_catched_finally
	 * l_catch_exit:
	 */

	struct generator_label *l_exit;
	struct generator_label *l_catch;
	struct generator_label *l_catch_exit;

	/* catched finally or unthrow finally */
	struct generator_label *l_catched_finally;

	struct syntax *catch_stmt;
	struct syntax *try_enclosing;
	struct syntax *finally_block_stmt;

	try_enclosing = lemon->l_try_enclosing;
	lemon->l_try_enclosing = node;

	l_exit = generator_make_label(lemon);
	l_catch = generator_make_label(lemon);
	l_catched_finally = generator_make_label(lemon);

	generator_emit_try(lemon, l_catch);
	if (!compiler_block(lemon, node->u.try_stmt.try_block_stmt)) {
		return 0;
	}

	generator_emit_label(lemon, l_catched_finally);

	finally_block_stmt = node->u.try_stmt.finally_block_stmt;
	if (finally_block_stmt) {
		if (!compiler_block(lemon, finally_block_stmt)) {
			return 0;
		}
	}
	generator_emit_opcode(lemon, OPCODE_UNTRY);
	generator_emit_jmp(lemon, l_exit);

	generator_emit_label(lemon, l_catch);
	node->l_catch = l_catch;
	for (catch_stmt = node->u.try_stmt.catch_stmt_list;
	     catch_stmt;
	     catch_stmt = catch_stmt->sibling)
	{
		struct syntax *catch_name;
		struct symbol *symbol;

		l_catch_exit = generator_make_label(lemon);

		/* compare exception class */
		generator_emit_opcode(lemon, OPCODE_LOADEXC);
		if (!compiler_const_string(lemon, "__instanceof__")) {
			return 0;
		}
		generator_emit_opcode(lemon, OPCODE_GETATTR);
		if (!compiler_expr(lemon,
		                   catch_stmt->u.catch_stmt.catch_type))
		{
			return 0;
		}
		generator_emit_call(lemon, 1);
		generator_emit_jz(lemon, l_catch_exit);

		scope_enter(lemon, SCOPE_BLOCK);
		catch_name = catch_stmt->u.catch_stmt.catch_name;
		symbol = scope_add_symbol(lemon,
		                          lemon->l_scope,
		                          catch_name->buffer,
		                          SYMBOL_EXCEPTION);
		if (!symbol) {
			compiler_error_redefined(lemon,
			                         catch_name,
			                         catch_name->buffer);

			return 0;
		}
		if (!compiler_block(lemon,
		                    catch_stmt->u.catch_stmt.block_stmt))
		{
			return 0;
		}
		scope_leave(lemon);
		generator_emit_jmp(lemon, l_catched_finally);

		generator_emit_label(lemon, l_catch_exit);
	}

	/* rethrow finally */
	if (finally_block_stmt) {
		if (!compiler_block(lemon, finally_block_stmt)) {
			return 0;
		}
	}
	if (try_enclosing && try_enclosing->l_catch) {
		/* upper level catch block */
		generator_emit_jmp(lemon, try_enclosing->l_catch);
	} else {
		/* top level try catch block, rethrow */
		generator_emit_opcode(lemon, OPCODE_UNTRY);
		generator_emit_opcode(lemon, OPCODE_LOADEXC);
		generator_emit_opcode(lemon, OPCODE_THROW);
	}

	generator_emit_label(lemon, l_exit);
	lemon->l_try_enclosing = try_enclosing;

	return 1;
}

static int
compiler_throw_stmt(struct lemon *lemon, struct syntax *node)
{
	struct syntax *stmt_enclosing;

	stmt_enclosing = lemon->l_stmt_enclosing;
	lemon->l_stmt_enclosing = node;
	if (!compiler_expr(lemon, node->u.return_stmt.expr)) {
		return 0;
	}
	generator_emit_opcode(lemon, OPCODE_THROW);
	/* remove all code after throw stmt */
	node->sibling = NULL;
	node->has_return = 1;
	lemon->l_stmt_enclosing = stmt_enclosing;

	return 1;
}

static int
compiler_block(struct lemon *lemon, struct syntax *node)
{
	struct syntax *stmt;

	scope_enter(lemon, SCOPE_MODULE);
	for (stmt = node->u.block_stmt.stmt_list;
	     stmt;
	     stmt = stmt->sibling)
	{
		if (!compiler_stmt(lemon, stmt)) {
			return 0;
		}

		if (stmt->has_return) {
			node->has_return = 1;
		}
	}
	scope_leave(lemon);

	return 1;
}

static int
compiler_module(struct lemon *lemon, struct syntax *node)
{
	struct generator_label *l_exit;
	struct generator_code *c_module;

	char *module_name;
	char *module_path;
	struct scope *module_scope;
	struct lobject *module_key;
	struct lobject *module;

	struct syntax *stmt;
	struct syntax *stmt_enclosing;
	struct syntax *space_enclosing;
	struct symbol *symbol;

	scope_enter(lemon, SCOPE_MODULE);
	l_exit = generator_make_label(lemon);

	module_name = resolve_module_name(lemon, node->filename);
	module = lmodule_create(lemon,
	                        lstring_create(lemon,
	                                       module_name,
	                                       strlen(module_name)));
	if (!module) {
		return 0;
	}
	module_path = resolve_module_path(lemon, node, input_filename(lemon));
	module_key = lstring_create(lemon, module_path, strlen(module_path));
	lobject_set_item(lemon, lemon->l_modules, module_key, module);

	compiler_const_object(lemon, module);
	c_module = generator_emit_module(lemon, 0, l_exit);

	space_enclosing = lemon->l_space_enclosing;
	lemon->l_space_enclosing = node;

	for (stmt = node->u.module_stmt.stmt_list;
	     stmt;
	     stmt = stmt->sibling)
	{
		int n;
		struct syntax *name;
		struct syntax *accessor;
		struct syntax *accessor_list;

		if (!compiler_stmt(lemon, stmt)) {
			return 0;
		}

		name = NULL;
		accessor_list = NULL;
		if (stmt->kind == SYNTAX_KIND_VAR_STMT) {
			name = stmt->u.var_stmt.name;
			accessor_list = stmt->u.var_stmt.accessor_list;
		}

		if (stmt->kind == SYNTAX_KIND_CLASS_STMT) {
			name = stmt->u.class_stmt.name;
			accessor_list = stmt->u.class_stmt.accessor_list;
		}

		if (stmt->kind == SYNTAX_KIND_DEFINE_STMT) {
			name = stmt->u.define_stmt.name;
			accessor_list = stmt->u.define_stmt.accessor_list;
		}

		stmt_enclosing = lemon->l_stmt_enclosing;
		lemon->l_stmt_enclosing = stmt;
		if (accessor_list) {
			n = 0;
			for (accessor = accessor_list;
			     accessor;
			     accessor = accessor->sibling)
			{
				if (node_is_getter(lemon, accessor)) {
					if (!compiler_expr(lemon, accessor)) {
						return 0;
					}
					n += 1;
				}
			}
			if (n) {
				if (!compiler_const_object(lemon, module)) {
					return 0;
				}
				if (!compiler_const_string(lemon,
				                           name->buffer))
				{
					return 0;
				}
				generator_emit_setgetter(lemon, n);
			}

			n = 0;
			for (accessor = accessor_list;
			     accessor;
			     accessor = accessor->sibling)
			{
				if (node_is_setter(lemon, accessor)) {
					if (!compiler_expr(lemon, accessor)) {
						return 0;
					}
					n += 1;
				}
			}

			if (n) {
				if (!compiler_const_object(lemon, module)) {
					return 0;
				}
				if (!compiler_const_string(lemon,
				                           name->buffer))
				{
					return 0;
				}
				generator_emit_setsetter(lemon, n);
			}
		}
		lemon->l_stmt_enclosing = stmt_enclosing;
	}

	module_scope = lemon->l_scope;
	for (symbol = module_scope->symbol; symbol; symbol = symbol->next) {
		lobject_set_item(lemon,
		                 ((struct lmodule *)module)->attr,
		                 lstring_create(lemon,
		                                symbol->name,
		                                strlen(symbol->name)),
		                 linteger_create_from_long(lemon,
		                                           symbol->local));
	}

	generator_emit_label(lemon, l_exit);
	generator_patch_module(lemon, c_module, node->nlocals, l_exit);
	lemon->l_space_enclosing = space_enclosing;

	return 1;
}

static int
compiler_import_stmt(struct lemon *lemon, struct syntax *node)
{
	struct generator_label *l_exit;
	struct generator_code *c_module;

	struct syntax *stmt;
	struct scope *scope;
	struct symbol *symbol;

	char *module_name;
	char *module_path;
	struct syntax *module_stmt;
	struct scope *module_scope;
	struct lobject *module;

	struct syntax *stmt_enclosing;
	struct syntax *space_enclosing;

	space_enclosing = lemon->l_space_enclosing;
	module_path = node->u.import_stmt.path_string->buffer;
	module_path = resolve_module_path(lemon, node, module_path);
	if (node->u.import_stmt.name) {
		module_name = node->u.import_stmt.name->buffer;
	} else {
		module_name = resolve_module_name(lemon, module_path);
	}

	symbol = scope_add_symbol(lemon,
	                          lemon->l_scope,
	                          module_name,
	                          SYMBOL_LOCAL);
	if (!symbol) {
		compiler_error_redefined(lemon, node, module_name);

		return 0;
	}
	symbol->local = space_enclosing->nlocals++;

	module = lobject_get_item(lemon,
	                          lemon->l_modules,
	                          lstring_create(lemon,
	                                         module_path,
	                                         strlen(module_path)));
	if (module) {
		if (!compiler_const_object(lemon, module)) {
			return 0;
		}
		generator_emit_store(lemon, 0, symbol->local);

		return 1;
	}

#ifndef STATICLINKED
	if (module_path_is_native(lemon, module_path)) {
		typedef struct lobject *(*module_init_t)(struct lemon *);
		void *handle;
		char *init_name;
		module_init_t module_init;

		handle = dlopen(module_path, RTLD_NOW);
		if (handle == NULL) {
			fprintf(stderr, "dlopen: %s\n", dlerror());

			return 0;
		}
		module_name = resolve_module_name(lemon, module_path);

		init_name = arena_alloc(lemon, lemon->l_arena, LEMON_NAME_MAX);
		strcpy(init_name, module_name);
		strcat(init_name, "_module");
		/*
		 * cast 'void *' -> 'uintptr_t' -> function pointer
		 * can make compiler happy
		 */
		module_init = (module_init_t)(uintptr_t)dlsym(handle,
		                                              init_name);
		if (!module_init) {
			return 0;
		}
		module = module_init(lemon);
		if (!module) {
			return 0;
		}
		if (!compiler_const_object(lemon, module)) {
			return 0;
		}
		generator_emit_store(lemon, 0, symbol->local);

		lobject_set_item(lemon,
	                         lemon->l_modules,
		                 lstring_create(lemon,
		                                module_path,
		                                strlen(module_path)),
		                 module);

		return 1;
	}
#endif

	if (!input_set_file(lemon, module_path)) {
		char buffer[PATH_MAX];
		snprintf(buffer,
		         sizeof(buffer),
		         "open '%s' fail\n",
		         module_path);
		compiler_error(lemon, node, buffer);

		return 0;
	}

	lexer_next_token(lemon);
	module_stmt = parser_parse(lemon);
	if (!module_stmt) {
		compiler_error(lemon, node, "syntax error\n");

		return 0;
	}
	node->u.import_stmt.module_stmt = module_stmt;

	/* make module's scope start from NULL */
	scope = lemon->l_scope;
	lemon->l_scope = NULL;
	scope_enter(lemon, SCOPE_MODULE);
	lemon->l_space_enclosing = module_stmt;

	module = lmodule_create(lemon,
	                        lstring_create(lemon,
	                                       module_name,
	                                       strlen(module_name)));

	if (!compiler_const_object(lemon, module)) {
		return 0;
	}
	generator_emit_store(lemon, 0, symbol->local);

	lobject_set_item(lemon,
	                 lemon->l_modules,
	                 lstring_create(lemon,
	                                module_path,
	                                strlen(module_path)),
	                 module);
	if (!module) {
		return 0;
	}
	compiler_const_object(lemon, module);

	l_exit = generator_make_label(lemon);
	c_module = generator_emit_module(lemon, 0, l_exit);
	for (stmt = module_stmt->u.module_stmt.stmt_list;
	     stmt;
	     stmt = stmt->sibling)
	{
		int n;
		struct syntax *name;
		struct syntax *accessor;
		struct syntax *accessor_list;

		if (!compiler_stmt(lemon, stmt)) {
			return 0;
		}

		name = NULL;
		accessor_list = NULL;
		if (stmt->kind == SYNTAX_KIND_VAR_STMT) {
			name = stmt->u.var_stmt.name;
			accessor_list = stmt->u.var_stmt.accessor_list;
		}

		if (stmt->kind == SYNTAX_KIND_CLASS_STMT) {
			name = stmt->u.class_stmt.name;
			accessor_list = stmt->u.class_stmt.accessor_list;
		}

		if (stmt->kind == SYNTAX_KIND_DEFINE_STMT) {
			name = stmt->u.define_stmt.name;
			accessor_list = stmt->u.define_stmt.accessor_list;
		}

		stmt_enclosing = lemon->l_stmt_enclosing;
		lemon->l_stmt_enclosing = stmt;
		if (accessor_list) {
			n = 0;
			for (accessor = accessor_list;
			     accessor;
			     accessor = accessor->sibling)
			{
				if (node_is_getter(lemon, accessor)) {
					if (!compiler_expr(lemon, accessor)) {
						return 0;
					}
					n += 1;
				}
			}
			if (n) {
				if (!compiler_const_object(lemon, module)) {
					return 0;
				}
				if (!compiler_const_string(lemon,
				                           name->buffer))
				{
					return 0;
				}
				generator_emit_setgetter(lemon, n);
			}

			n = 0;
			for (accessor = accessor_list;
			     accessor;
			     accessor = accessor->sibling)
			{
				if (node_is_setter(lemon, accessor)) {
					if (!compiler_expr(lemon, accessor)) {
						return 0;
					}
					n += 1;
				}
			}

			if (n) {
				if (!compiler_const_object(lemon, module)) {
					return 0;
				}
				if (!compiler_const_string(lemon,
				                           name->buffer))
				{
					return 0;
				}
				generator_emit_setsetter(lemon, n);
			}
		}
		lemon->l_stmt_enclosing = stmt_enclosing;
	}

	module_scope = lemon->l_scope;
	for (symbol = module_scope->symbol; symbol; symbol = symbol->next) {
		lobject_set_item(lemon,
		                 ((struct lmodule *)module)->attr,
		                 lstring_create(lemon,
		                                symbol->name,
		                                strlen(symbol->name)),
		                 linteger_create_from_long(lemon,
		                                           symbol->local));
	}

	if (!compiler_const_object(lemon, lemon->l_nil)) {
		return 0;
	}
	generator_emit_opcode(lemon, OPCODE_RETURN);

	generator_emit_label(lemon, l_exit);
	generator_patch_module(lemon, c_module, module_stmt->nlocals, l_exit);

	lemon->l_space_enclosing = space_enclosing;
	scope_leave(lemon);
	lemon->l_scope = scope;

	return 1;
}

static int
compiler_expr(struct lemon *lemon, struct syntax *node)
{
	struct lobject *object;

	if (!node) {
		return 0;
	}

	switch (node->kind) {
	case SYNTAX_KIND_NIL:
		if (!compiler_nil(lemon, node)) {
			return 0;
		}
		break;

	case SYNTAX_KIND_TRUE:
		if (!compiler_true(lemon, node)) {
			return 0;
		}
		break;

	case SYNTAX_KIND_FALSE:
		if (!compiler_false(lemon, node)) {
			return 0;
		}
		break;

	case SYNTAX_KIND_SENTINEL:
		if (!compiler_sentinel(lemon, node)) {
			return 0;
		}
		break;

	case SYNTAX_KIND_SELF:
		generator_emit_opcode(lemon, OPCODE_SELF);
		break;

	case SYNTAX_KIND_SUPER:
		generator_emit_opcode(lemon, OPCODE_SUPER);
		break;

	case SYNTAX_KIND_NAME:
		if (!compiler_name(lemon, node)) {
			return 0;
		}
		break;

	case SYNTAX_KIND_NUMBER_LITERAL:
		if (node_is_integer(lemon, node)) {
			object = linteger_create_from_cstr(lemon, node->buffer);
		} else {
			object = lnumber_create_from_cstr(lemon, node->buffer);
		}
		if (!compiler_const_object(lemon, object)) {
			return 0;
		}
		break;

	case SYNTAX_KIND_STRING_LITERAL:
		object = lstring_create(lemon, node->buffer, node->length);
		if (!compiler_const_object(lemon, object)) {
			return 0;
		}
		break;

	case SYNTAX_KIND_ARRAY:
		if (!compiler_array(lemon, node)) {
			return 0;
		}
		break;

	case SYNTAX_KIND_DICTIONARY:
		if (!compiler_dictionary(lemon, node)) {
			return 0;
		}
		break;

	case SYNTAX_KIND_UNOP:
		if (!compiler_unop(lemon, node)) {
			return 0;
		}
		break;

	case SYNTAX_KIND_BINOP:
		compiler_binop(lemon, node);
		break;

	case SYNTAX_KIND_CONDITIONAL:
		if (!compiler_conditional(lemon, node)) {
			return 0;
		}
		break;

	case SYNTAX_KIND_CALL:
		if (!compiler_call(lemon, node)) {
			return 0;
		}
		break;

	case SYNTAX_KIND_GET_ITEM:
		if (!compiler_expr(lemon, node->u.get_item.left)) {
			return 0;
		}
		if (!compiler_expr(lemon, node->u.get_item.right)) {
			return 0;
		}
		generator_emit_opcode(lemon, OPCODE_GETITEM);
		break;

	case SYNTAX_KIND_GET_ATTR:
		if (!compiler_expr(lemon, node->u.get_attr.left)) {
			return 0;
		}
		if (!compiler_const_string(lemon,
		                           node->u.get_attr.right->buffer))
		{
			return 0;
		}
		generator_emit_opcode(lemon, OPCODE_GETATTR);
		break;

	case SYNTAX_KIND_GET_SLICE:
		if (!compiler_expr(lemon, node->u.get_slice.left)) {
			return 0;
		}

		if (node->u.get_slice.start) {
			if (!compiler_expr(lemon, node->u.get_slice.start)) {
				return 0;
			}
		} else {
			object = linteger_create_from_long(lemon, 0);
			if (!compiler_const_object(lemon, object)) {
				return 0;
			}
		}

		if (node->u.get_slice.stop) {
			if (!compiler_expr(lemon, node->u.get_slice.stop)) {
				return 0;
			}
		} else {
			if (!compiler_const_object(lemon, lemon->l_nil)) {
				return 0;
			}
		}

		if (node->u.get_slice.step) {
			if (!compiler_expr(lemon, node->u.get_slice.step)) {
				return 0;
			}
		} else {
			object = linteger_create_from_long(lemon, 1);
			if (!compiler_const_object(lemon, object)) {
				return 0;
			}
		}
		generator_emit_opcode(lemon, OPCODE_GETSLICE);
		break;

	/* define is also a expr value */
	case SYNTAX_KIND_CLASS_STMT:
		if (!compiler_class_stmt(lemon, node)) {
			return 0;
		}
			break;

	/* define is also a expr value */
	case SYNTAX_KIND_DEFINE_STMT:
		if (!compiler_define_stmt(lemon, node)) {
			return 0;
		}
			break;

	case SYNTAX_KIND_ACCESSOR:
		if (!compiler_expr(lemon, node->u.accessor.expr)) {
			return 0;
		}
		break;

	default:
		compiler_error(lemon, node, "unknown syntax node\n");
		return 0;
	}

	return 1;
}

static int
compiler_stmt(struct lemon *lemon, struct syntax *node)
{
	if (!node) {
		return 0;
	}

	switch (node->kind) {
	case SYNTAX_KIND_ASSIGN_STMT:
		if (!compiler_assign_stmt(lemon, node)) {
			return 0;
		}
	break;

	case SYNTAX_KIND_IF_STMT:
		if (!compiler_if_stmt(lemon, node)) {
			return 0;
		}
		break;

	case SYNTAX_KIND_FOR_STMT:
		if (!compiler_for_stmt(lemon, node)) {
			return 0;
		}
		break;

	case SYNTAX_KIND_FORIN_STMT:
		if (!compiler_forin_stmt(lemon, node)) {
			return 0;
		}
		break;

	case SYNTAX_KIND_WHILE_STMT:
		if (!compiler_while_stmt(lemon, node)) {
			return 0;
		}
		break;

	case SYNTAX_KIND_MODULE_STMT:
		if (!compiler_module(lemon, node)) {
			return 0;
		}
		break;

	case SYNTAX_KIND_BLOCK_STMT:
		scope_enter(lemon, SCOPE_BLOCK);
		if (!compiler_block(lemon, node)) {
			return 0;
		}
		scope_leave(lemon);
		break;

	case SYNTAX_KIND_VAR_STMT:
		if (!compiler_var_stmt(lemon, node)) {
			return 0;
		}
		break;

	case SYNTAX_KIND_DEFINE_STMT:
		if (!compiler_define_stmt(lemon, node)) {
			return 0;
		}
		break;

	case SYNTAX_KIND_TRY_STMT:
		if (!compiler_try_stmt(lemon, node)) {
			return 0;
		}
		break;

	case SYNTAX_KIND_IMPORT_STMT:
		if (!compiler_import_stmt(lemon, node)) {
			return 0;
		}
		break;

	case SYNTAX_KIND_BREAK_STMT:
		if (!compiler_break_stmt(lemon, node)) {
			return 0;
		}
		break;

	case SYNTAX_KIND_CONTINUE_STMT:
		if (!compiler_continue_stmt(lemon, node)) {
			return 0;
		}
		break;

	case SYNTAX_KIND_DELETE_STMT:
		if (!compiler_delete_stmt(lemon, node)) {
			return 0;
		}
		break;

	case SYNTAX_KIND_RETURN_STMT:
		if (!compiler_return_stmt(lemon, node)) {
			return 0;
		}
		break;

	case SYNTAX_KIND_THROW_STMT:
		if (!compiler_throw_stmt(lemon, node)) {
			return 0;
		}
		break;

	case SYNTAX_KIND_CLASS_STMT:
		if (!compiler_class_stmt(lemon, node)) {
			return 0;
		}
		break;

	default:
		if (!compiler_expr(lemon, node)) {
			return 0;
		}
	}
	return 1;
}

int
compiler_compile(struct lemon *lemon, struct syntax *node)
{
	return compiler_module(lemon, node);
}
