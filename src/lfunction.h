#ifndef LEMON_LFUNCTION_H
#define LEMON_LFUNCTION_H

#include "lobject.h"

struct lframe;

typedef struct lobject *(*lfunction_call_t)(struct lemon *,
                                            struct lobject *,
                                            int,
                                            struct lobject *[]);

struct lfunction {
	struct lobject object;

	/*
	 * define values:
	 *
	 * 0, fixed argument function 'define func(var a, var b, var c)'
	 *    a, b and c take exactly argument
	 *
	 * 1, variable argument function 'define func(var a, var b, var *c)'
	 *    argument after b into array c
	 *
	 * 2, variable keyword argument function
	 *    'define func(var a, var b, var **c)'
	 *    keyword argument after b into dictionary c
	 *
	 * 3, variable argument and keyword argument function
	 *    'define func(var a, var *b, var **c)'
	 *    all non-keyword argument after a into array b
	 *    all keyword argument after non-keyword into dictionary c
	 */
	unsigned char define;
	unsigned char nlocals; /* all local variable (include parameters) */
	unsigned char nparams; /* number of parameters */
	unsigned char nvalues; /* number of parameters has default values */
	                       /* always nlocals >= nparams >= nvalues */

	int address; /* bytecode entry address */

	struct lframe *frame;
	struct lobject *name;
	struct lobject *self;
	lfunction_call_t callback; /* C function pointer */

	struct lobject *params[1]; /* parameters name reverse order */
};

void *
lfunction_bind(struct lemon *lemon,
               struct lobject *function,
               struct lobject *self);

void *
lfunction_create(struct lemon *lemon,
                 struct lobject *name,
                 struct lobject *self,
                 lfunction_call_t callback);

void *
lfunction_create_with_address(struct lemon *lemon,
                              struct lobject *name,
                              int define,
                              int nlocals,
                              int nparams,
                              int nvalues,
                              int address,
                              struct lobject *params[]);

struct ltype *
lfunction_type_create(struct lemon *lemon);

#endif /* LEMON_LFUNCTION_H */
