#ifndef LEMON_HASH_H
#define LEMON_HASH_H

struct lemon;

long
lemon_hash(struct lemon *lemon, const void *key, long len);

#endif /* LEMON_HASH_H */

