#ifndef LEMON_LEXCEPTION_H
#define LEMON_LEXCEPTION_H

#include "lobject.h"

struct lexception {
	struct lobject object;

	int throwed;
	struct lobject *clazz;
	struct lobject *message;

	int nframe;
	struct lobject *frame[128];
};

void *
lexception_create(struct lemon *lemon,
                  struct lobject *message);

struct ltype *
lexception_type_create(struct lemon *lemon);

#endif /* LEMON_LEXCEPTION_H */
