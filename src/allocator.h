#ifndef LEMON_ALLOCATOR_H
#define LEMON_ALLOCATOR_H

/*
 * there is three level memory management in Lemon
 *
 * 1. `lemon_allocator_alloc' and `lemon_allocator_free'
 *    basic alloc and free interface, ensured aligned pointer
 *
 * 2. `arena_alloc'
 *    internal allocator for lexer and parser no free action needed
 *
 * 3. `lemon_collector_trace' and `lemon_collector_mark'
 *    garbage collector for Lemon object system
 */

/*
 * allocator has two policy of alloc,
 * 1, (allocation size >> SIZE_SHIFT) <= ALLOCATOR_POOL_SIZE
 *    use fix size memory pool, pool[] element is double linked list
 * 2, (allocation size >> SIZE_SHIFT) > ALLOCATOR_POOL_SIZE
 *    use memalign_malloc dynamic size align allocation or system malloc,
 *    if malloc return align pointer, use `make USE_MALLOC=1', default is 0.
 */
struct lemon;
struct mpool;

#ifndef ALLOCATOR_POOL_SIZE
#define ALLOCATOR_POOL_SIZE 32
#endif

struct allocator {
	struct mpool *pool[ALLOCATOR_POOL_SIZE];
};

void *
allocator_create(struct lemon *lemon);

void
allocator_destroy(struct lemon *lemon, struct allocator *mem);

void *
allocator_alloc(struct lemon *lemon, long size);

void
allocator_free(struct lemon *lemon, void *ptr);

void *
allocator_realloc(struct lemon *lemon, void *ptr, long size);

#endif /* LEMON_ALLOCATOR_H */
