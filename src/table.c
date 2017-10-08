#include "lemon.h"
#include "table.h"

static unsigned long primes[] = {
	3,
	7,
	13,
	27,
	53,
	97,
	193,
	389,
	769,
	1543,
	3079,
	6151,
	12289,
	24593,
	49157,
	98317,
	196613,
	393241,
	786433,
	1572869,
	3145739,
	6291469,
	12582917,
	25165843,
	50331653,
	100663319,
	201326611,
	402653189,
	805306457,
	1610612741
};

unsigned long
table_size(struct lemon *lemon, unsigned long size)
{
	unsigned long i;
	for (i = 0; i < sizeof(primes)/sizeof(primes[0]); i++) {
		if (primes[i] > size) {
			return primes[i];
		}
	}

	return size;
}

int
table_insert(struct lemon *lemon,
             void *key,
             void *value,
             struct slot *slots,
             unsigned long nslots,
             table_cmp_t cmp,
             table_hash_t hash)
{
	unsigned long i;
	unsigned long h;

	h = hash(lemon, key);
	for (i = h % nslots; i != (h - 1) % nslots; i = (i+1) % nslots) {
		if (slots[i].key == lemon->l_sentinel) {
			break;
		}

		if (slots[i].key == NULL) {
			break;
		}

		if (cmp(lemon, key, slots[i].key)) {
			slots[i].value = value;

			return 0;
		}
	}
	slots[i].key = key;
	slots[i].value = value;

	return 1;
}

void *
table_search(struct lemon *lemon,
             void *key,
             struct slot *slots,
             unsigned long nslots,
             table_cmp_t cmp,
             table_hash_t hash)
{
	unsigned long i;
	unsigned long h;

	h = hash(lemon, key);
	for (i = h % nslots; i != (h - 1) % nslots; i = (i+1) % nslots) {
		if (slots[i].key == lemon->l_sentinel) {
			continue;
		}

		if (slots[i].key == NULL) {
			return NULL;
		}

		if (cmp(lemon, key, slots[i].key)) {
			return slots[i].value;
		}
	}

	return NULL;
}

void *
table_delete(struct lemon *lemon,
             void *key,
             struct slot *slots,
             unsigned long nslots,
             table_cmp_t cmp,
             table_hash_t hash)
{
	unsigned long i;
	unsigned long h;

	h = hash(lemon, key);
	for (i = h % nslots; i != (h - 1) % nslots; i = (i+1) % nslots) {
		if (slots[i].key == lemon->l_sentinel) {
			continue;
		}

		if (slots[i].key == NULL) {
			return NULL;
		}

		if (cmp(lemon, key, slots[i].key)) {
			slots[i].key = lemon->l_sentinel;
			return slots[i].value;
		}
	}

	return NULL;
}

void
table_rehash(struct lemon *lemon,
             struct slot *oldslots,
             unsigned long noldslots,
             struct slot *newslots,
             unsigned long nnewslots,
             table_cmp_t cmp,
             table_hash_t hash)
{
	unsigned long i;

	for (i = 0; i < noldslots; i++) {
		if (oldslots[i].key && oldslots[i].key != lemon->l_sentinel) {
			table_insert(lemon,
			             oldslots[i].key,
			             oldslots[i].value,
			             newslots,
			             nnewslots,
			             cmp,
			             hash);
		}
	}
}
