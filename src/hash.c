#include "lemon.h"
#include "hash.h"

#include <stdint.h>

uint32_t
djb_hash(const void *key, long len, long seed)
{
	int i;
	unsigned int h;
	const unsigned char *p;

	p = key;
	h = ((unsigned int)seed & 0xffffffff);
	for (i = 0; i < len; i++) {
		h = (unsigned int)(33 * (long)h ^ p[i]);
	}

	return h;
}

long
lemon_hash(struct lemon *lemon, const void *key, long len)
{
	return djb_hash(key, len, lemon->l_random);
}
