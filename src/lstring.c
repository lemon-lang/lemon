#include "lemon.h"
#include "hash.h"
#include "larray.h"
#include "lstring.h"
#include "linteger.h"
#include "literator.h"
#include "lib/builtin.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

static struct lobject *
lstring_format_string_function(struct lemon *lemon,
                               struct lobject *self,
                               int argc, struct lobject *argv[])
{
	struct lobject *callable;

	callable = lobject_get_attr(lemon,
	                            argv[0],
	                            lemon->l_string_string);
	if (callable && !lobject_is_error(lemon, callable)) {
		return lobject_call(lemon, callable, 0, NULL);
	}

	return lobject_string(lemon, argv[0]);
}

static struct lobject *
lstring_format_callback(struct lemon *lemon,
                        struct lframe *frame,
                        struct lobject *retval)
{
	int i;
	int j;
	int k;
	int r;
	long n;
	long len;
	long count;
	struct lobject *item;
	struct lobject *array;
	struct lstring *string;
	struct lstring *newstring;

	array = retval;
	string = (struct lstring *)frame->self;

	count = 0;
	for (i = 0; i < string->length - 1; i++) {
		if (string->buffer[i] == '{' &&
		    string->buffer[i + 1] == '}')
		{
			count += 1;
		}
	}

	n = larray_length(lemon, array);
	if (count > n) {
		return lobject_error_item(lemon,
		                          "'%@' index out of range",
		                          string);
	}

	len = string->length;
	for (i = 0; i < n; i++) {
		item = larray_get_item(lemon, array, i);
		len = len - 2 + lstring_length(lemon, item);
	}

	j = 0;
	k = 0;
	newstring = lstring_create(lemon, NULL, len);
	if (!newstring) {
		return NULL;
	}
	for (i = 0; i < string->length - 1; i++) {
		if (string->buffer[i] == '{' &&
		    string->buffer[i + 1] == '}')
		{
			item = larray_get_item(lemon, array, k++);
			memcpy(newstring->buffer + j,
			       ((struct lstring *)item)->buffer,
			       ((struct lstring *)item)->length);
			j += ((struct lstring *)item)->length;
			i += 1;
		} else {
			r = string->buffer[i];
			newstring->buffer[j++] = (char)r;
		}
	}
	newstring->buffer[j++] = string->buffer[i];

	return (struct lobject *)newstring;
}

static struct lobject *
lstring_format(struct lemon *lemon,
               struct lobject *self,
               int argc, struct lobject *argv[])
{
	struct lframe *frame;
	struct lobject *name;
	struct lobject *items[2];

	frame = lemon_machine_push_new_frame(lemon,
	                                     self,
	                                     NULL,
	                                     lstring_format_callback,
	                                     0);
	if (!frame) {
		return NULL;
	}

	name = lstring_create(lemon, "lstring_format_string", 21);
	if (!name) {
		return NULL;
	}
	items[0] = lfunction_create(lemon,
	                            name,
	                            self,
	                            lstring_format_string_function);
	if (!items[0]) {
		return NULL;
	}
	items[1] = larray_create(lemon, argc, argv);
	if (!items[1]) {
		return NULL;
	}

	return builtin_map(lemon, NULL, 2, items);
}

static struct lobject *
lstring_join_array(struct lemon *lemon,
                   struct lobject *join,
                   struct lobject *array)
{
	long i;
	long n;
	long off;
	long len;
	struct lobject *item;
	struct lstring *string;

	n = larray_length(lemon, array);
	len = 0;
	for (i = 0; i < n; i++) {
		item = larray_get_item(lemon, array, i);
		if (!lobject_is_string(lemon, item)) {
			const char *fmt;

			fmt = "'%@' required string value";
			return lobject_error_argument(lemon, fmt, join);

		}

		len += ((struct lstring *)item)->length;
	}
	len += ((struct lstring *)join)->length * (n - 1);
	string = lstring_create(lemon, NULL, len);
	if (!string) {
		return NULL;
	}

	off = 0;
	for (i = 0; i < n; i++) {
		item = larray_get_item(lemon, array, i);

		if (off) {
			memcpy(string->buffer + off,
			       ((struct lstring *)join)->buffer,
			       ((struct lstring *)join)->length);
			off += ((struct lstring *)join)->length;

			memcpy(string->buffer + off,
			       ((struct lstring *)item)->buffer,
			       ((struct lstring *)item)->length);
			off += ((struct lstring *)item)->length;
		} else {
			memcpy(string->buffer + off,
			       ((struct lstring *)item)->buffer,
			       ((struct lstring *)item)->length);
			off += ((struct lstring *)item)->length;
		}
	}

	if (string) {
		return (struct lobject *)string;
	}

	return lemon->l_empty_string;
}

static struct lobject *
lstring_join_callback(struct lemon *lemon,
                      struct lframe *frame,
                      struct lobject *retval)
{
	return lstring_join_array(lemon, frame->self, retval);
}

static struct lobject *
lstring_join_iterable(struct lemon *lemon,
                      struct lobject *join,
                      struct lobject *iterable)
{
	struct lframe *frame;

	frame = lemon_machine_push_new_frame(lemon,
	                                     join,
	                                     NULL,
	                                     lstring_join_callback,
	                                     0);
	if (!frame) {
		return NULL;
	}

	return literator_to_array(lemon, iterable, 0);
}

static struct lobject *
lstring_join(struct lemon *lemon,
             struct lobject *self,
             int argc, struct lobject *argv[])
{
	if (lobject_is_array(lemon, argv[0])) {
		return lstring_join_array(lemon, self, argv[0]);
	}

	return lstring_join_iterable(lemon, self, argv[0]);
}

static struct lobject *
lstring_trim(struct lemon *lemon,
             struct lobject *self,
             int argc, struct lobject *argv[])
{
	int i;
	int j;
	int k;
	int c;
	long nchars;
	const char *chars;
	struct lstring *string;

	chars = " \t\r\n";
	nchars = 4;
	if (argc) {
		if (argc != 1 || !lobject_is_string(lemon, argv[0])) {
			const char *fmt;

			fmt = "'%@' accept 1 string argument";
			return lobject_error_argument(lemon, fmt, self);
		}

		chars = lstring_to_cstr(lemon, argv[0]);
		nchars = lstring_length(lemon, argv[0]);
	}

	string = (struct lstring *)self;

	for (i = 0; i < string->length; i++) {
		c = string->buffer[i];
		for (k = 0; k < nchars; k++) {
			if (c == chars[k]) {
				break;
			}
		}
		if (k == nchars) {
			break;
		}
	}

	for (j = (int)string->length; j > 0; j--) {
		c = string->buffer[j - 1];
		for (k = 0; k < nchars; k++) {
			if (c == chars[k]) {
				break;
			}
		}
		if (k == nchars) {
			break;
		}
	}

	if (j <= i) {
		return lemon->l_empty_string;
	}

	return lstring_create(lemon, &string->buffer[i], j - i);
}

static struct lobject *
lstring_ltrim(struct lemon *lemon,
              struct lobject *self,
              int argc, struct lobject *argv[])
{
	int i;
	int k;
	int c;
	long nchars;
	const char *chars;
	struct lstring *string;

	chars = " \t\r\n";
	nchars = 4;
	if (argc) {
		if (argc != 1 || !lobject_is_string(lemon, argv[0])) {
			const char *fmt;

			fmt = "'%@' accept 1 string argument";
			return lobject_error_argument(lemon, fmt, self);
		}

		chars = lstring_to_cstr(lemon, argv[0]);
		nchars = lstring_length(lemon, argv[0]);
	}

	string = (struct lstring *)self;

	for (i = 0; i < string->length; i++) {
		c = string->buffer[i];
		for (k = 0; k < nchars; k++) {
			if (c == chars[k]) {
				break;
			}
		}
		if (k == nchars) {
			break;
		}
	}

	if (i == string->length) {
		return lemon->l_empty_string;
	}

	return lstring_create(lemon, &string->buffer[i], string->length - i);
}

static struct lobject *
lstring_rtrim(struct lemon *lemon,
              struct lobject *self,
              int argc, struct lobject *argv[])
{
	int j;
	int k;
	int c;
	long nchars;
	const char *chars;
	struct lstring *string;

	chars = " \t\r\n";
	nchars = 4;
	if (argc) {
		if (argc != 1 || !lobject_is_string(lemon, argv[0])) {
			const char *fmt;

			fmt = "'%@' accept 1 string argument";
			return lobject_error_argument(lemon, fmt, self);
		}

		chars = lstring_to_cstr(lemon, argv[0]);
		nchars = lstring_length(lemon, argv[0]);
	}

	string = (struct lstring *)self;

	for (j = (int)string->length; j > 0; j--) {
		c = string->buffer[j - 1];
		for (k = 0; k < nchars; k++) {
			if (c == chars[k]) {
				break;
			}
		}
		if (k == nchars) {
			break;
		}
	}

	if (j == 0) {
		return lemon->l_empty_string;
	}

	return lstring_create(lemon, string->buffer, j);
}

static struct lobject *
lstring_lower(struct lemon *lemon,
              struct lobject *self,
              int argc, struct lobject *argv[])
{
	int i;
	struct lstring *string;
	struct lstring *newstring;

	string = (struct lstring *)self;
	newstring = lstring_create(lemon, NULL, string->length);
	if (!newstring) {
		return NULL;
	}

	for (i = 0; i < string->length; i++) {
		newstring->buffer[i] = (char)tolower(string->buffer[i]);
	}

	return (struct lobject *)newstring;
}

static struct lobject *
lstring_upper(struct lemon *lemon,
              struct lobject *self,
              int argc, struct lobject *argv[])
{
	int i;
	struct lstring *string;
	struct lstring *newstring;

	string = (struct lstring *)self;
	newstring = lstring_create(lemon, NULL, string->length);
	if (!newstring) {
		return NULL;
	}

	for (i = 0; i < string->length; i++) {
		newstring->buffer[i] = (char)toupper(string->buffer[i]);
	}

	return (struct lobject *)newstring;
}

static struct lobject *
lstring_find(struct lemon *lemon,
             struct lobject *self,
             int argc, struct lobject *argv[])
{
	int i;
	int c;
	long len;
	struct lstring *string;
	struct lstring *substring;

	if (argc && lobject_is_string(lemon, argv[0])) {
		string = (struct lstring *)self;
		substring = (struct lstring *)argv[0];

		if (substring->length == 0) {
			return linteger_create_from_long(lemon, 0);
		}

		if (substring->length == 1) {
			c = substring->buffer[0];
			for (i = 0; i < string->length; i++) {
				if (string->buffer[i] == c) {
					return linteger_create_from_long(lemon,
					                                 i);
				}
			}

			return linteger_create_from_long(lemon, -1);
		}

		len = string->length - substring->length + 1;
		for (i = 0; i < len; i++) {
			if (strncmp(string->buffer + i,
			            substring->buffer,
			            substring->length) == 0)
			{
				return linteger_create_from_long(lemon, i);
			}
		}
	}

	return linteger_create_from_long(lemon, -1);
}

static struct lobject *
lstring_rfind(struct lemon *lemon,
              struct lobject *self,
              int argc, struct lobject *argv[])
{
	int c;
	long i;
	long len;
	struct lstring *string;
	struct lstring *substring;

	if (argc && lobject_is_string(lemon, argv[0])) {
		string = (struct lstring *)self;
		substring = (struct lstring *)argv[0];

		if (substring->length == 0) {
			return linteger_create_from_long(lemon, 0);
		}

		if (substring->length == 1) {
			c = substring->buffer[0];
			for (i = string->length; i > 0; i--) {
				if (string->buffer[i - 1] == c) {
					return linteger_create_from_long(lemon,
					                                 i - 1);
				}
			}

			return linteger_create_from_long(lemon, -1);
		}

		len = string->length - substring->length + 1;
		for (i = len; i > 0; i--) {
			if (strncmp(string->buffer + i - 1,
			            substring->buffer,
			            substring->length) == 0)
			{
				return linteger_create_from_long(lemon, i - 1);
			}
		}
	}

	return linteger_create_from_long(lemon, -1);
}

static struct lobject *
lstring_replace(struct lemon *lemon,
                struct lobject *self,
                int argc, struct lobject *argv[])
{
	int i;
	int j;
	int c;
	int r;
	long len;
	long diff;
	int count;
	struct lstring *string;
	struct lstring *substring;
	struct lstring *repstring;
	struct lstring *newstring;

	if (argc == 2 &&
	    lobject_is_string(lemon, argv[0]) &&
	    lobject_is_string(lemon, argv[1]))
	{
		string = (struct lstring *)self;
		substring = (struct lstring *)argv[0];
		repstring = (struct lstring *)argv[1];

		if (substring->length == 0) {
			return self;
		}

		count = 0;
		if (substring->length == 1) {
			c = substring->buffer[0];
			for (i = 0; i < string->length; i++) {
				if (string->buffer[i] == c) {
					count += 1;
				}
			}
		} else {
			len = string->length - substring->length + 1;
			for (i = 0; i < len; i++) {
				if (strncmp(string->buffer + i,
				            substring->buffer,
				            substring->length) == 0)
				{
					count += 1;
				}
			}
		}
		if (!count) {
			return self;
		}

		diff = repstring->length - substring->length;
		len = string->length + diff * count;
		newstring = lstring_create(lemon, NULL, len);
		if (!newstring) {
			return NULL;
		}

		j = 0;
		/* replace('x', 'y') */
		if (substring->length == 1 && repstring->length == 1) {
			c = substring->buffer[0];
			r = repstring->buffer[0];
			memcpy(newstring->buffer,
			       string->buffer,
			       string->length);
			for (i = 0; i < newstring->length; i++) {
				if (newstring->buffer[i] == c) {
					newstring->buffer[j++] = (char)r;
				} else {
					r = string->buffer[i];
					newstring->buffer[j++] = (char)r;
				}
			}

		/* replace('x', 'yyy') */
		} else if (substring->length == 1) {
			c = substring->buffer[0];
			for (i = 0; i < string->length; i++) {
				if (string->buffer[i] == c) {
					if (repstring->length >= 1) {
						memcpy(newstring->buffer + j,
						       repstring->buffer,
						       repstring->length);
						j += repstring->length;
					}
				} else {
					r = string->buffer[i];
					newstring->buffer[j++] = (char)r;
				}
			}

		/* replace('xxx', 'y') */
		} else if (repstring->length == 1) {
			for (i = 0; i < string->length; i++) {
				r = repstring->buffer[0];
				if (string->length - i >= substring->length &&
				    strncmp(string->buffer + i,
				            substring->buffer,
				            substring->length) == 0)
				{
					newstring->buffer[j++] = (char)r;
					i += substring->length - 1;
				} else {
					r = string->buffer[i];
					newstring->buffer[j++] = (char)r;
				}
			}

		/* replace('xxx', 'yyy') */
		} else {
			for (i = 0; i < string->length; i++) {
				if (string->length - i >= substring->length &&
				    strncmp(string->buffer + i,
				            substring->buffer,
				            substring->length) == 0)
				{
					memcpy(newstring->buffer + j,
					       repstring->buffer,
					       repstring->length);
					j += repstring->length;
					i += substring->length - 1;
				} else {
					r = string->buffer[i];
					newstring->buffer[j++] = (char)r;
				}
			}
		}

		return (struct lobject *)newstring;
	}

	return lemon->l_nil;
}

static struct lobject *
lstring_split(struct lemon *lemon,
              struct lobject *self,
              int argc, struct lobject *argv[])
{
	int c;
	long i;
	long j;
	long max;
	long len;
	struct lstring *string;
	struct lstring *substring;
	struct lobject *item;
	struct lobject *array;

	if (argc && lobject_is_string(lemon, argv[0])) {
		string = (struct lstring *)self;
		substring = (struct lstring *)argv[0];

		if (substring->length == 0) {
			return larray_create(lemon, 1, &self);
		}

		if (argc == 2 && lobject_is_integer(lemon, argv[1])) {
			max = linteger_to_long(lemon, argv[1]);
		} else {
			max = 0;
			if (substring->length == 1) {
				c = substring->buffer[0];
				for (i = 0; i < string->length; i++) {
					if (string->buffer[i] == c) {
						max += 1;
					}
				}
			} else {
				len = string->length - substring->length + 1;
				for (i = 0; i < len; i++) {
					if (strncmp(string->buffer + i,
					            substring->buffer,
					            substring->length) == 0)
					{
						max += 1;
					}
				}
			}
		}

		if (max == 0) {
			return larray_create(lemon, 1, &self);
		}

		array = larray_create(lemon, 0, NULL);
		if (!array) {
			return NULL;
		}
		j = 0;
		if (substring->length == 1) {
			c = substring->buffer[0];
			for (i = 0; i < string->length && max > 0; i++) {
				if (string->buffer[i] != c) {
					continue;
				}
				item = lstring_create(lemon,
				                      string->buffer + j,
				                      i - j);
				if (!item) {
					return NULL;
				}
				if (!larray_append(lemon, array, 1, &item)) {
					return NULL;
				}

				j = i + 1;
				max -= 1;
			}

			if (j < string->length) {
				item = lstring_create(lemon,
				                      string->buffer + j,
				                      string->length - j);
				if (!item) {
					return NULL;
				}
				if (!larray_append(lemon, array, 1, &item)) {
					return NULL;
				}
			}

			return array;
		}

		len = string->length - substring->length + 1;
		for (i = 0; i < len && max > 0; i++) {
			if (strncmp(string->buffer + i,
			            substring->buffer,
			            substring->length) != 0)
			{
				continue;
			}
			item = lstring_create(lemon,
			                      string->buffer + j,
			                      i - j);
			if (!item) {
				return NULL;
			}
			if (!larray_append(lemon, array, 1, &item)) {
				return NULL;
			}
			j = i + substring->length;
			i = i + substring->length - 1;
			max -= 1;
		}
		if (j < string->length) {
			item = lstring_create(lemon,
			                      string->buffer + j,
			                      string->length - j);
			if (!item) {
				return NULL;
			}
			if (!larray_append(lemon, array, 1, &item)) {
				return NULL;
			}
		}

		return array;
	}

	return lemon->l_false;
}

static struct lobject *
lstring_startswith(struct lemon *lemon,
                   struct lobject *self,
                   int argc, struct lobject *argv[])
{
	struct lstring *string;
	struct lstring *substring;

	if (argc && lobject_is_string(lemon, argv[0])) {
		string = (struct lstring *)self;
		substring = (struct lstring *)argv[0];

		if (substring->length == 0) {
			return lemon->l_true;
		}
		if (substring->length > string->length) {
			return lemon->l_false;
		}

		if (memcmp(string->buffer,
		           substring->buffer,
		           substring->length) == 0)
		{
			return lemon->l_true;
		}
	}

	return lemon->l_false;
}

static struct lobject *
lstring_endswith(struct lemon *lemon,
                 struct lobject *self,
                 int argc, struct lobject *argv[])
{
	struct lstring *string;
	struct lstring *substring;

	if (argc && lobject_is_string(lemon, argv[0])) {
		string = (struct lstring *)self;
		substring = (struct lstring *)argv[0];

		if (substring->length == 0) {
			return lemon->l_true;
		}
		if (substring->length > string->length) {
			return lemon->l_false;
		}

		if (memcmp(string->buffer + string->length - substring->length,
		           substring->buffer,
		           substring->length) == 0)
		{
			return lemon->l_true;
		}
	}

	return lemon->l_false;
}

static struct lobject *
lstring_eq(struct lemon *lemon, struct lstring *a, struct lstring *b)
{
	if (!lobject_is_string(lemon, (struct lobject *)a)) {
		return lemon->l_false;
	}

	if (!lobject_is_string(lemon, (struct lobject *)b)) {
		return lemon->l_false;
	}

	if (a->length != b->length) {
		return lemon->l_false;
	}

	if (memcmp(a->buffer, b->buffer, a->length) != 0) {
		return lemon->l_false;
	}

	return lemon->l_true;
}

static struct lobject *
lstring_add(struct lemon *lemon, struct lstring *a, struct lstring *b)
{
	struct lstring *string;

	if (!lobject_is_string(lemon, (struct lobject *)b)) {
		return lobject_error_type(lemon,
		                          "'%@' unsupport operand '%@'",
		                          a,
		                          b);
	}

	string = lstring_create(lemon, NULL, a->length + b->length);
	if (!string) {
		return NULL;
	}
	memcpy(string->buffer, a->buffer, a->length);
	memcpy(string->buffer + a->length, b->buffer, b->length);
	string->buffer[string->length] = '\0';

	return (struct lobject *)string;
}

static struct lobject *
lstring_get_item(struct lemon *lemon,
                 struct lstring *self, struct lobject *name)
{
	long i;
	char buffer[1];

	if (lobject_is_integer(lemon, name)) {
		i = linteger_to_long(lemon, name);
		if (i < self->length) {
			if (i < 0) {
				i = self->length + i;
			}
			buffer[0] = self->buffer[i];

			return lstring_create(lemon, buffer, 1);
		}

		return NULL;
	}

	return NULL;
}

static struct lobject *
lstring_has_item(struct lemon *lemon,
                 struct lobject *self,
                 struct lobject *items)
{
	struct lobject *argv[1];

	argv[0] = items;
	return lstring_find(lemon, self, 1, argv);
}

static struct lobject *
lstring_get_attr(struct lemon *lemon,
                 struct lobject *self,
                 struct lobject *name)
{
	const char *cstr;

	cstr = lstring_to_cstr(lemon, name);
	if (strcmp(cstr, "upper") == 0) {
		return lfunction_create(lemon, name, self, lstring_upper);
	}

	if (strcmp(cstr, "lower") == 0) {
		return lfunction_create(lemon, name, self, lstring_lower);
	}

	if (strcmp(cstr, "trim") == 0) {
		return lfunction_create(lemon, name, self, lstring_trim);
	}

	if (strcmp(cstr, "ltrim") == 0) {
		return lfunction_create(lemon, name, self, lstring_ltrim);
	}

	if (strcmp(cstr, "rtrim") == 0) {
		return lfunction_create(lemon, name, self, lstring_rtrim);
	}

	if (strcmp(cstr, "find") == 0) {
		return lfunction_create(lemon, name, self, lstring_find);
	}

	if (strcmp(cstr, "rfind") == 0) {
		return lfunction_create(lemon, name, self, lstring_rfind);
	}

	if (strcmp(cstr, "replace") == 0) {
		return lfunction_create(lemon, name, self, lstring_replace);
	}

	if (strcmp(cstr, "split") == 0) {
		return lfunction_create(lemon, name, self, lstring_split);
	}

	if (strcmp(cstr, "join") == 0) {
		return lfunction_create(lemon, name, self, lstring_join);
	}

	if (strcmp(cstr, "format") == 0) {
		return lfunction_create(lemon, name, self, lstring_format);
	}

	if (strcmp(cstr, "startswith") == 0) {
		return lfunction_create(lemon, name, self, lstring_startswith);
	}

	if (strcmp(cstr, "endswith") == 0) {
		return lfunction_create(lemon, name, self, lstring_endswith);
	}

	return NULL;
}

static struct lobject *
lstring_get_slice(struct lemon *lemon,
                  struct lstring *self,
                  struct lobject *start,
                  struct lobject *stop,
                  struct lobject *step)
{
	long off;
	long istart;
	long istop;
	long istep;
	struct lstring *string;

	istart = linteger_to_long(lemon, start);
	if (stop == lemon->l_nil) {
		istop = self->length;
	} else {
		istop = linteger_to_long(lemon, stop);
	}
	istep = linteger_to_long(lemon, step);

	if (istart < 0) {
		istart = self->length + istart;
	}
	if (istop < 0) {
		istop = self->length + istop;
	}
	if (istart >= istop) {
		return lemon->l_empty_string;
	}

	string = lstring_create(lemon, NULL, (istop - istart) / istep);
	if (string) {
		off = 0;
		for (; istart < istop; istart += istep) {
			string->buffer[off++] = self->buffer[istart];
		}
	}

	return (struct lobject *)string;
}

static struct lobject *
lstring_method(struct lemon *lemon,
               struct lobject *self,
               int method, int argc, struct lobject *argv[])
{
#define cast(a) ((struct lstring *)(a))

	switch (method) {
	case LOBJECT_METHOD_EQ:
		return lstring_eq(lemon, cast(self), cast(argv[0]));

	case LOBJECT_METHOD_ADD:
		return lstring_add(lemon, cast(self), cast(argv[0]));

	case LOBJECT_METHOD_GET_ITEM:
		return lstring_get_item(lemon, cast(self), argv[0]);

	case LOBJECT_METHOD_HAS_ITEM:
		return lstring_has_item(lemon, self, argv[0]);

	case LOBJECT_METHOD_GET_ATTR:
		return lstring_get_attr(lemon, self, argv[0]);

	case LOBJECT_METHOD_GET_SLICE:
		return lstring_get_slice(lemon,
		                         cast(self),
		                         argv[0],
		                         argv[1],
		                         argv[2]);

	case LOBJECT_METHOD_HASH:
		return linteger_create_from_long(lemon,
		                       lemon_hash(lemon,
		                                  cast(self)->buffer,
		                                  cast(self)->length));

	case LOBJECT_METHOD_STRING:
		return self;

	case LOBJECT_METHOD_LENGTH:
		return linteger_create_from_long(lemon, cast(self)->length);

	case LOBJECT_METHOD_BOOLEAN:
		if (cast(self)->length) {
			return lemon->l_true;
		}
		return lemon->l_false;

	case LOBJECT_METHOD_DESTROY:
		return NULL;

	default:
		return lobject_default(lemon, self, method, argc, argv);
	}
}

const char *
lstring_to_cstr(struct lemon *lemon, struct lobject *object)
{
	return ((struct lstring *)object)->buffer;
}

char *
lstring_buffer(struct lemon *lemon, struct lobject *object)
{
	return ((struct lstring *)object)->buffer;
}

long
lstring_length(struct lemon *lemon, struct lobject *object)
{
	return ((struct lstring *)object)->length;
}

void *
lstring_create(struct lemon *lemon, const char *buffer, long length)
{
	struct lstring *self;

	/*
	 * lstring->buffer has one byte more then length for '\0'
	 */
	self = lobject_create(lemon, sizeof(*self) + length, lstring_method);
	if (self) {
		self->length = length;

		if (buffer) {
			memcpy(self->buffer, buffer, length);
			self->buffer[length] = '\0';
		}
	}

	return self;
}

static struct lobject *
lstring_type_method(struct lemon *lemon,
                    struct lobject *self,
                    int method, int argc, struct lobject *argv[])
{
	switch (method) {
	case LOBJECT_METHOD_CALL:
		if (argc) {
			return lobject_string(lemon, argv[0]);
		}
		return lemon->l_empty_string;

	case LOBJECT_METHOD_CALLABLE:
		return lemon->l_true;

	default:
		return lobject_default(lemon, self, method, argc, argv);
	}
}

struct ltype *
lstring_type_create(struct lemon *lemon)
{
	struct ltype *type;

	type = ltype_create(lemon,
	                    "string",
	                    lstring_method,
	                    lstring_type_method);
	if (type) {
		lemon_add_global(lemon, "string", type);
	}

	return type;
}
