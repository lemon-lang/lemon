#include "lemon.h"
#include "mpool.h"
#include "allocator.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ALIGN 8
#define ROUNDUP(s) (((s) + ALIGN - 1) / ALIGN * ALIGN)

#ifdef USE_MALLOC
#define MALLOC(size) malloc(size)
#define REALLOC(ptr,size) realloc(size)
#define FREE(ptr) free(ptr)
#else
#define MALLOC(size) memalign_malloc(size)
#define REALLOC(ptr,size) memalign_realloc(ptr,size)
#define FREE(ptr) memalign_free(ptr)
#endif

#ifndef SIZE_SHIFT
#define SIZE_SHIFT 3
#endif

#define BLOCKS_PER_POOL 256

void *
memalign_malloc(size_t nbytes)
{
	void *ptr;
	void *alignptr;

	nbytes = ROUNDUP(nbytes + ROUNDUP(sizeof(void *)));
	ptr = malloc(nbytes);
	alignptr = (void *)ROUNDUP((uintptr_t)ptr);

	memcpy(alignptr, &ptr, sizeof(void *));

	return (void *)((char *)alignptr + ROUNDUP(sizeof(void *)));
}

void *
memalign_realloc(void *ptr, size_t nbytes)
{
	void *alignptr;
	void *unalignptr;

	if (!ptr) {
		return memalign_malloc(nbytes);
	}

	ptr = (char *)ptr - ROUNDUP(sizeof(void *));
	memcpy(&unalignptr, ptr, sizeof(void *));

	nbytes = ROUNDUP(nbytes + ROUNDUP(sizeof(void *)));
	ptr = realloc(unalignptr, nbytes);
	alignptr = (void *)ROUNDUP((uintptr_t)ptr);

	memcpy(alignptr, &ptr, sizeof(void *));

	return (void *)((char *)alignptr + ROUNDUP(sizeof(void *)));
}

void
memalign_free(void *ptr)
{
	void *unalignptr;

	if (!ptr) {
		return;
	}
	ptr = (char *)ptr - ROUNDUP(sizeof(void *));
	memcpy(&unalignptr, ptr, sizeof(void *));

	free(unalignptr);
}

void *
allocator_create(struct lemon *lemon)
{
	struct allocator *allocator;

	allocator = malloc(sizeof(*allocator));
	if (allocator) {
		memset(allocator, 0, sizeof(*allocator));
	}

	return allocator;
}

void
allocator_destroy(struct lemon *lemon, struct allocator *allocator)
{
	int i;
	struct mpool *p;
	struct mpool *next;

	for (i = 0; i < ALLOCATOR_POOL_SIZE; i++) {
		for (p = allocator->pool[i]; p; p = next) {
			next = p->next;
			mpool_destroy(p);
		}
	}
	free(allocator);
}

void *
allocator_alloc(struct lemon *lemon, long size)
{
	void *ptr;
	struct allocator *allocator;
	struct mpool **pp;

	allocator = lemon->l_allocator;
	size = ROUNDUP(size + ROUNDUP(sizeof(void *)));
	if ((size >> SIZE_SHIFT) >= ALLOCATOR_POOL_SIZE) {
		ptr = MALLOC((size_t)size);
		memset(ptr, 0, ROUNDUP(sizeof(void *)));
		ptr = (void *)((char *)ptr + ROUNDUP(sizeof(void *)));
	} else {
		pp = &allocator->pool[size >> SIZE_SHIFT];

		/*
		 * find a pool have a free block
		 */
		while (*pp && (*pp)->freeblocks == 0) {
			struct mpool *p;

			/* remove full pool from linked list */
			p = (*pp)->next;
			(*pp)->prev = NULL;
			(*pp)->next = NULL;
			*pp = p;
		}

		/* all pool run's out, create a new one */
		if (*pp == NULL) {
			*pp = mpool_create(BLOCKS_PER_POOL * size, size);
		}
		ptr = mpool_alloc(*pp);
		if (ptr) {
			/* pack pool to ptr */
			memcpy(ptr, pp, sizeof(void *));
			ptr = (void *)((char *)ptr + ROUNDUP(sizeof(void *)));
		}
	}

	return ptr;
}

void *
allocator_realloc(struct lemon *lemon, void *ptr, long size)
{
	void *newptr;
	struct mpool *p;

	if (!ptr) {
		return lemon_allocator_alloc(lemon, size);
	}
	memcpy(&p, (char *)ptr - ROUNDUP(sizeof(void *)), sizeof(void *));
	if (p) {
		newptr = lemon_allocator_alloc(lemon, size);
		if (size > (long)(p->blocksize - ROUNDUP(sizeof(void *)))) {
			size = p->blocksize - ROUNDUP(sizeof(void *));
		}
		memcpy(newptr, ptr, size);
		lemon_allocator_free(lemon, ptr);
	} else {
		size = ROUNDUP(size + ROUNDUP(sizeof(void *)));
		newptr = REALLOC((char *)ptr - ROUNDUP(sizeof(void *)), size);
		memset(newptr, 0, ROUNDUP(sizeof(void *)));
		newptr = (char *)newptr + ROUNDUP(sizeof(void *));
	}

	return newptr;
}

void
allocator_free(struct lemon *lemon, void *ptr)
{
	struct mpool *p;
	struct mpool **pp;
	struct allocator *allocator;

	allocator = lemon->l_allocator;
	if (ptr) {
		ptr = (char *)ptr - ROUNDUP(sizeof(void *));
		memcpy(&p, ptr, sizeof(void *));
		if (p) {
			pp = &allocator->pool[p->blocksize >> SIZE_SHIFT];
			mpool_free(p, ptr);

			/*
			 * all mpool's blocks are freed destroy this pool
			 */
			if (p->freeblocks == p->nblocks) {
				if (p->prev) {
					p->prev->next = p->next;
				}
				if (p->next) {
					p->next->prev = p->prev;
				}
				if (p == *pp) {
					*pp = p->next;
				}
				mpool_destroy(p);
			} else {
				/* add pool to linked list */
				if (p->prev == NULL &&
				    p->next == NULL &&
				    p != *pp)
				{
					p->next = *pp;
					if (*pp) {
						(*pp)->prev = p;
					}
					*pp = p;
				}
			}
		} else {
			FREE(ptr);
		}
	}
}

void *
lemon_allocator_alloc(struct lemon *lemon, long size)
{
	return allocator_alloc(lemon, size);
}

void *
lemon_allocator_realloc(struct lemon *lemon, void *ptr, long size)
{
	return allocator_realloc(lemon, ptr, size);
}

void
lemon_allocator_free(struct lemon *lemon, void *ptr)
{
	allocator_free(lemon, ptr);
}
