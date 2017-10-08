#ifndef LEMON_BUILTIN_H
#define LEMON_BUILTIN_H

struct lemon;
struct lobject;

void
builtin_init(struct lemon *lemon);

struct lobject *
builtin_map(struct lemon *lemon,
            struct lobject *self,
            int argc, struct lobject *argv[]);

#endif /* LEMON_BUILTIN_H */
