#ifndef LEMON_EXTEND_H
#define LEMON_EXTEND_H

/*
 * ref: David R. Hanson C Interface and Implementation
 */

#define EXTEND_BITS 30
#define EXTEND_BASE (1UL << EXTEND_BITS)

typedef unsigned int extend_t;

unsigned long
extend_from_long(int n, extend_t *z, unsigned long u);

unsigned long
extend_to_long(int n, extend_t *x);

unsigned long
extend_from_str(int n, extend_t *z, const char *str, int base, char **end);

char *
extend_to_str(int n, extend_t *x, char *str, int size, int base);

int
extend_length(int n, extend_t *x);

unsigned long
extend_add(extend_t *z, int n,
           extend_t *x, int m,
           extend_t *y, unsigned long carry);

unsigned long
extend_sub(extend_t *z, int n,
           extend_t *x, int m,
           extend_t *y, unsigned long borrow);

unsigned long
extend_mul(extend_t *z, int n,
           extend_t *x, int m,
           extend_t *y);

int
extend_div(extend_t *q, int n,
           extend_t *x, int m,
           extend_t *y,
           extend_t *r,
           extend_t *tmp);

unsigned long
extend_sum(int n, extend_t *z, extend_t *x, unsigned long y);

unsigned long
extend_diff(int n, extend_t *z, extend_t *x, unsigned long y);

unsigned long
extend_product(int n, extend_t *z, extend_t *x, unsigned long y);

unsigned long
extend_quotient(int n, extend_t *z, extend_t *x, unsigned long y);

unsigned long
extend_bnot(extend_t *z, int n, extend_t *x, unsigned long carry);

unsigned long
extend_band(extend_t *z, int n,
            extend_t *x, int m,
            extend_t *y, unsigned long carry);

unsigned long
extend_bor(extend_t *z, int n,
           extend_t *x, int m,
           extend_t *y, unsigned long carry);

int
extend_cmp(int n, extend_t *x, extend_t *y);

void
extend_shl(int n, extend_t *z, int m, extend_t *x, int s, int fill);

void
extend_shr(int n, extend_t *z, int m, extend_t *x, int s, int fill);

#endif /* LEMON_EXTEND_H */
