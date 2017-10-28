#ifndef LEMON_MPOOL_H
#define LEMON_MPOOL_H

struct mpool {
	long size;
	long nblocks;
	long blocksize;
	long freeblocks;

	void *firstptr;
	void *freeptr;
	void *blockptr;

	struct mpool *prev;
	struct mpool *next;
};

void *
mpool_create(long size, long blocksize);

void
mpool_destroy(struct mpool *pool);

void *
mpool_alloc(struct mpool *pool);

void
mpool_free(struct mpool *pool, void *ptr);

#endif /* LEMON_MPOOL_H */
