#include "mpool.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <inttypes.h>

#define ALIGN 8
#define ROUNDUP(s) (((s) + ALIGN - 1) / ALIGN * ALIGN)

void *
mpool_create(long size, long blocksize)
{
	int i;
	struct mpool *mpool;

	if (size < blocksize) {
		printf("size must bigger then blocksize\n");

		return NULL;
	}

	if (size % blocksize != 0) {
		printf("size mod block must be 0\n");

		return NULL;
	}

	mpool = malloc(sizeof(*mpool));
	if (!mpool) {
		return NULL;
	}

	memset(mpool, 0, sizeof(*mpool));

	mpool->blockptr = malloc(ROUNDUP(size));
	if (!mpool->blockptr) {
		free(mpool);

		return NULL;
	}
	mpool->firstptr = (void *)ROUNDUP((uintptr_t)mpool->blockptr);
	memset(mpool->blockptr, 0, ROUNDUP(size));
	mpool->size = size;
	mpool->nblocks = size / blocksize;
	mpool->blocksize = blocksize;
	mpool->freeptr = NULL;
	mpool->freeblocks = 0;

	for (i = 0; i < mpool->nblocks; i++) {
		mpool_free(mpool, (char *)mpool->firstptr + blocksize * i);
	}

	return mpool;
}

void
mpool_destroy(struct mpool *mpool)
{
	free(mpool->blockptr);
	free(mpool);
}

void *
mpool_alloc(struct mpool *mpool)
{
	void *ptr;
	void *freeptr;

	if (mpool->freeblocks > 0) {
		ptr = mpool->freeptr;
		memcpy(&freeptr, ptr, sizeof(void *));
		mpool->freeptr = freeptr;
		mpool->freeblocks -= 1;

		return ptr;
	}
	printf("error\n");

	return NULL;
}

void
mpool_free(struct mpool *mpool, void *ptr)
{
	void *freeptr;
	if (ptr) {
		assert(ptr >= mpool->blockptr &&
		       (char *)ptr < (char *)mpool->blockptr + mpool->size);

		assert((uintptr_t)((char *)ptr -
		                   (char *)mpool->firstptr) %
		       mpool->blocksize == 0);

		freeptr = mpool->freeptr;
		memcpy(ptr, &freeptr, sizeof(void *));
		mpool->freeptr = ptr;
		mpool->freeblocks += 1;
	}
}
