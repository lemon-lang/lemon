#ifndef LEMON_LTYPE_H
#define LEMON_LTYPE_H

#include "lobject.h"

/*
 * three methods:
 * 1, `ltype->object->method' used for identity type's type
 * 2, `ltype->method' used for identity type's object's type
 * 3, `ltype->type_method' actual type object's method
 */
struct ltype {
	struct lobject object;

	const char *name;
	lobject_method_t method;      /* method of object */
	lobject_method_t type_method; /* method of type   */
};

void *
ltype_create(struct lemon *lemon,
             const char *name,
             lobject_method_t method,
             lobject_method_t type_method);

struct ltype *
ltype_type_create(struct lemon *lemon);

#endif /* LEMON_LTYPE_H */
