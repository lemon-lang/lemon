#ifndef LEMON_SCOPE_H
#define LEMON_SCOPE_H

struct lemon;
struct symbol;

enum {
	SCOPE_CLASS,
	SCOPE_BLOCK,
	SCOPE_DEFINE,
	SCOPE_MODULE
};

struct scope {
	int type;
	struct scope *parent;

	struct symbol *symbol;
};

int
scope_enter(struct lemon *lemon, int type);

int
scope_leave(struct lemon *lemon);

struct symbol *
scope_add_symbol(struct lemon *lemon,
                 struct scope *scope,
                 const char *name,
                 int type);

struct symbol *
scope_get_symbol(struct lemon *lemon, struct scope *scope, char *name);

#endif /* LEMON_SCOPE_H */
