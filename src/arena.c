#include "lemon.h"
#include "arena.h"
#include "allocator.h"

#include <stdio.h>
#include <string.h>

static const int block_size = 4096;

struct arena *
arena_create(struct lemon *lemon)
{
	struct arena *arena;

	arena = allocator_alloc(lemon, sizeof(*arena));
	if (!arena) {
		return NULL;
	}
	memset(arena, 0, sizeof(*arena));

	return arena;
}

void
arena_destroy(struct lemon *lemon, struct arena *arena)
{
	int i;

	for (i = 0; i < arena->iblocks; i++) {
		allocator_free(lemon, arena->blocks[i]);
	}

	allocator_free(lemon, arena->blocks);
	allocator_free(lemon, arena);
}

void *
arena_alloc_block(struct lemon *lemon, struct arena *arena, long bytes)
{
	char *block;

	block = allocator_alloc(lemon, bytes);
	if (!block) {
		return NULL;
	}

	if (arena->iblocks >= arena->nblocks) {
		int nblocks;
		size_t size;
		if (arena->blocks) {
			nblocks = arena->nblocks * 2;
		} else {
			nblocks = 2;
		}
		size = sizeof(*arena->blocks) * nblocks;
		arena->blocks = allocator_realloc(lemon, arena->blocks, size);
		if (!arena->blocks) {
			allocator_free(lemon, block);

			return NULL;
		}
		arena->nblocks = nblocks;
	}
	arena->blocks[arena->iblocks++] = block;

	return block;
}

void *
arena_alloc(struct lemon *lemon, struct arena *arena, long bytes)
{
	char *ptr;
	char *block;

	bytes = ((bytes + sizeof(void *) - 1)/sizeof(void *)) * sizeof(void *);
	if (bytes <= arena->limit - arena->avail) {
		ptr = arena->avail;
		arena->avail += bytes;
		return ptr;
	}

	if (bytes > (block_size / 4)) {
		ptr = arena_alloc_block(lemon, arena, bytes);

		return ptr;
	}
	block = arena_alloc_block(lemon, arena, block_size);
	arena->avail = block + bytes;
	arena->limit = block + block_size;
	ptr = block;

	return ptr;
}
