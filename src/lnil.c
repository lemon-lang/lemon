#include "lemon.h"
#include "lnil.h"
#include "lstring.h"

static struct lobject *
lnil_method(struct lemon *lemon,
            struct lobject *self,
            int method, int argc, struct lobject *argv[])
{
	switch (method) {
	case LOBJECT_METHOD_STRING:
		return lstring_create(lemon, "nil", 3);

	case LOBJECT_METHOD_BOOLEAN:
		return lemon->l_false;

	default:
		return lobject_default(lemon, self, method, argc, argv);
	}
}

void *
lnil_create(struct lemon *lemon)
{
	return lobject_create(lemon, sizeof(struct lnil), lnil_method);
}
