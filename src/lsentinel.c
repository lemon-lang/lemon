#include "lemon.h"
#include "lstring.h"
#include "lsentinel.h"

static struct lobject *
lsentinel_method(struct lemon *lemon,
                 struct lobject *self,
                 int method, int argc, struct lobject *argv[])
{
	switch (method) {
	case LOBJECT_METHOD_STRING:
		return lstring_create(lemon, "sentinel", 8);

	default:
		return lobject_default(lemon, self, method, argc, argv);
	}
}

void *
lsentinel_create(struct lemon *lemon)
{
	struct lsentinel *self;

	self = lobject_create(lemon, sizeof(*self), lsentinel_method);

	return self;
}
