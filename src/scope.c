#include "lemon.h"
#include "arena.h"
#include "scope.h"
#include "symbol.h"

#include <stdio.h>
#include <string.h>

int
scope_enter(struct lemon *lemon, int type)
{
	struct scope *scope;

	scope = arena_alloc(lemon, lemon->l_arena, sizeof(*scope));
	memset(scope, 0, sizeof(*scope));

	scope->type = type;
	scope->parent = lemon->l_scope;
	lemon->l_scope = scope;

	return 1;
}

int
scope_leave(struct lemon *lemon)
{
	struct scope *scope;

	scope = lemon->l_scope;
	if (scope->parent) {
		lemon->l_scope = scope->parent;

		return 1;
	}

	return 0;
}

struct symbol *
scope_add_symbol(struct lemon *lemon,
                 struct scope *scope,
                 const char *name,
                 int type)
{
	struct symbol *symbol;

	if (symbol_get_symbol(scope->symbol, name)) {
		return NULL;
	}
	symbol = symbol_make_symbol(lemon, name, type);
	symbol->next = scope->symbol;
	scope->symbol = symbol;

	return symbol;
}

struct symbol *
scope_get_symbol(struct lemon *lemon, struct scope *scope, char *name)
{
	int level;
	struct scope *curr;
	struct scope *global;
	struct symbol *s;
	struct symbol *symbol;

	level = 0;
	for (curr = scope; curr; curr = curr->parent) {
		if (curr->type == SCOPE_CLASS) {
			continue;
		}
		s = symbol_get_symbol(curr->symbol, name);
		if (s) {
			if (level && s->type == SYMBOL_LOCAL) {
				symbol = scope_add_symbol(lemon,
				                          scope,
				                          name,
				                          SYMBOL_LOCAL);
				symbol->cpool = s->cpool;
				symbol->level = s->level + level;
				symbol->local = s->local;
				symbol->accessor_list = s->accessor_list;

				return symbol;
			}

			return s;
		}

		if (curr->type == SCOPE_DEFINE) {
			level++;
		}
	}

	global = lemon->l_global;
	return symbol_get_symbol(global->symbol, name);
}
