#ifndef LEMON_ARENA_H
#define LEMON_ARENA_H

struct arena {
	char *limit;
	char *avail;

	char **blocks;
	int iblocks;
	int nblocks;
};

struct arena *
arena_create(struct lemon *lemon);

void
arena_destroy(struct lemon *lemon, struct arena *arena);

void *
arena_alloc(struct lemon *lemon, struct arena *arena, long bytes);

#endif /* LEMON_ARENA_H */
