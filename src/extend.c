#include "extend.h"

#include <ctype.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

unsigned long
extend_from_long(int n, extend_t *z, unsigned long u)
{
	int i;

	i = 0;
	do {
		z[i++] = u % EXTEND_BASE;
	} while ((u /= EXTEND_BASE) > 0 && i < n);

	return u;
}

unsigned long
extend_to_long(int n, extend_t *x)
{
	int i;
	unsigned long u;

	u = 0;
	i = sizeof(u) * 8;

	if (i > n) {
		i = n;
	}

	while (--i >= 0) {
		u = u * EXTEND_BASE + x[i];
	}

	return u;
}

unsigned long
extend_from_str(int n, extend_t *z, const char *str, int base, char **end)
{
	const char *p;
	unsigned long carry;

	for (p = str; *p && isspace(*p); p++) {
		/* NULL */
	}

	carry = 0;
	if (*p && isalnum(*p)) {
		for (; *p && isalnum(*p); p++) {
			int digit;
			carry = extend_product(n, z, z, base);
			if (carry) {
				break;
			}
			digit = 0;
			switch (*p) {
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				digit = *p - '0';
				break;
			case 'a':
			case 'A':
				digit = 10;
				break;
			case 'b':
			case 'B':
				digit = 11;
				break;
			case 'c':
			case 'C':
				digit = 12;
				break;
			case 'd':
			case 'D':
				digit = 13;
				break;
			case 'e':
			case 'E':
				digit = 14;
				break;
			case 'f':
			case 'F':
				digit = 15;
				break;
			default:
				break;
			}
			assert(digit < base);
			extend_sum(n, z, z, digit);
		}
		if (end) {
			*end = (char *)p;
		}

		return carry;
	} else {
		if (end) {
			*end = (char *)str;
		}
	}

	return carry;
}

char *
extend_to_str(int n, extend_t *x, char *str, int size, int base)
{
	int i;
	int j;

	i = 0;
	do {
		int r = extend_quotient(n, x, x, base);
		str[i++] = "0123456789abcdef"[r];
		while (n > 1 && x[n - 1] == 0) {
			n--;
		}
	} while (n > 1 || x[0] != 0);
	str[i] = '\0';

	for (j = 0; j < --i; j++) {
		char c = str[j];
		str[j] = str[i];
		str[i] = c;
	}

	return str;
}

int
extend_length(int n, extend_t *x)
{
	while (n > 1 && x[n - 1] == 0) {
		n -= 1;
	}

	return n;
}

unsigned long
extend_add(extend_t *z, int n,
           extend_t *x, int m,
           extend_t *y, unsigned long carry)
{
	int i;
	int min;
	int max;
	extend_t *a;

	if (n > m) {
		a = x;
		max = n;
		min = m;
	} else {
		a = y;
		max = m;
		min = n;
	}

	for (i = 0; i < min; i++) {
		carry += x[i] + y[i];
		z[i] = carry % EXTEND_BASE;
		carry /= EXTEND_BASE;
	}

	for (; i < max; i++) {
		carry += a[i];
		z[i] = carry % EXTEND_BASE;
		carry /= EXTEND_BASE;
	}

	return carry;
}

unsigned long
extend_sub(extend_t *z, int n,
           extend_t *x, int m,
           extend_t *y, unsigned long borrow)
{
	int i;
	unsigned long d;

	for (i = 0; i < m; i++) {
		d = (x[i] + EXTEND_BASE) - y[i] - borrow;
		z[i] = d % EXTEND_BASE;
		borrow /= EXTEND_BASE;
	}

	for (; i < n; i++) {
		d = (x[i] + EXTEND_BASE) - borrow;
		z[i] = d % EXTEND_BASE;
		borrow /= EXTEND_BASE;
	}

	return borrow;
}

unsigned long
extend_mul(extend_t *z, int n, extend_t *x, int m, extend_t *y)
{
	int i;
	int j;
	unsigned long carry;
	unsigned long carryout;

	carryout = 0;
	for (i = 0; i < n; i++) {
		carry = 0;
		for (j = 0; j < m; j++) {
			carry += x[i] * y[j] + z[i+j];
			z[i+j] = carry % EXTEND_BASE;
			carry /= EXTEND_BASE;
		}
		for (; j < n + m - i; j++) {
			carry += z[i + j];
			z[i + j] = carry % EXTEND_BASE;
			carry /= EXTEND_BASE;
		}
		carryout |= carry;
	}

	return carryout;
}

int
extend_div(extend_t *q, int n,
           extend_t *x, int m,
           extend_t *y,
           extend_t *r,
           extend_t *tmp)
{
	int i;
	int k;

	int nx;
	int my;

	extend_t *rem;
	extend_t *dq;

	nx = n;
	my = m;

	n = extend_length(n, x);
	m = extend_length(m, y);

	if (m == 1) {
		if (y[0] == 0) {
			return 0;
		}
		r[0] = extend_quotient(nx, q, x, y[0]);
		memset(r + 1, 0, my - 1);
	} else if (m > n) {
		memset(q, 0, nx * sizeof(extend_t));
		memcpy(r, x, n * sizeof(extend_t));
		memset(r + n, 0, (my - n) * sizeof(extend_t));
	} else {
		rem = tmp;
		dq = tmp + n + 1;
		memcpy(rem, x, n * sizeof(extend_t));
		rem[n] = 0;
		for (k = n - m; k >= 0; k--) {
			int km;
			unsigned long qk;
			unsigned long y2;
			unsigned long r3;
			unsigned long borrow;

			km = k + m;
			y2 = y[m - 1] * EXTEND_BASE + y[m-2];
			r3 = rem[km] * (EXTEND_BASE * EXTEND_BASE) +
			     rem[km-1] * EXTEND_BASE + rem[km-2];

			qk = r3/y2;
			if (qk >= EXTEND_BASE) {
				qk = EXTEND_BASE - 1;
			}
			dq[m] = extend_product(m, dq, y, qk);
			for (i = m; i > 0; i--) {
				if (rem[i + k] != dq[i]) {
					break;
				}
			}
			if (rem[i+k] < dq[i]) {
				dq[m] = extend_product(m, dq, y, --qk);
			}
			q[k] = qk;
			borrow = extend_sub(&rem[k], m + 1,
			                    &rem[k], m + 1,
			                    dq, 0);
			(void)borrow;
			assert(borrow == 0);
		}
		memcpy(r, rem, m * sizeof(extend_t));
		for (i = n - m + 1; i < nx; i++) {
			q[i] = 0;
		}
		for (i = m; i < my; i++) {
			r[i] = 0;
		}
	}

	return 1;
}

unsigned long
extend_sum(int n, extend_t *z, extend_t *x, unsigned long y)
{
	int i;

	for (i = 0; i < n; i++) {
		y += x[i];
		z[i] = y % EXTEND_BASE;
		y /= EXTEND_BASE;
	}

	return y;
}

unsigned long
extend_diff(int n, extend_t *z, extend_t *x, unsigned long y)
{
	int i;
	unsigned long d;

	for (i = 0; i < n; i++) {
		d = (x[i] + EXTEND_BASE) - y;
		z[i] = d % EXTEND_BASE;
		y = 1 - d / EXTEND_BASE;
	}

	return y;
}

unsigned long
extend_product(int n, extend_t *z, extend_t *x, unsigned long y)
{
	int i;
	unsigned long carry;

	carry = 0;
	for (i = 0; i < n; i++) {
		carry += x[i] * y;
		z[i] = carry % EXTEND_BASE;
		carry /= EXTEND_BASE;
	}

	return carry;
}

unsigned long
extend_quotient(int n, extend_t *z, extend_t *x, unsigned long y)
{
	int i;
	unsigned long carry;

	carry = 0;
	for (i = n - 1; i >= 0; i--) {
		carry = carry * EXTEND_BASE + x[i];
		z[i] = carry / y;
		carry %= y;
	}

	return carry;
}

void
extend_shl(int n, extend_t *z, int m, extend_t *x, int s, int fill)
{
	int i;
	int j;

	fill = fill ? 0xffffffff : 0;
	if (n > m) {
		i = m - 1;
	} else {
		i = n - s/EXTEND_BITS - 1;
	}

	j = n - 1;
	for (; j >= m + s/EXTEND_BITS; j--) {
		z[j] = 0;
	}

	for (; i >= 0; i--, j--) {
		z[j] = x[i];
	}

	for (; j >= 0; j--) {
		z[j] = fill;
	}

	s %= EXTEND_BITS;
	if (s > 0) {
		extend_product(n, z, z, 1 << s);
		z[0] |= fill >> (EXTEND_BITS - s);
	}
}

void
extend_shr(int n, extend_t *z, int m, extend_t *x, int s, int fill)
{
	int i;
	int j;

	fill = fill ? 0xffffffff : 0;
	j = 0;
	for (i = s/EXTEND_BITS; i < m && j < n; i++, j++) {
		z[j] = x[i];
	}

	for (; j < n; j++) {
		z[j] = fill;
	}

	s %= EXTEND_BITS;
	if (s > 0) {
		extend_quotient(n, z, z, 1<<s);
		z[n-1] |= fill << (EXTEND_BITS - s);
	}
}

int
extend_cmp(int n, extend_t *x, extend_t *y)
{
	int i;

	for (i = n - 1; i > 0 && x[i] == y[i]; i--) {
		/* NULL */
	}

	if (x[i] > y[i]) {
		return 1;
	}

	if (x[i] < y[i]) {
		return -1;
	}

	return 0;
}
