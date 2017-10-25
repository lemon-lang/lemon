#include "lemon.h"
#include "builtin.h"
#include "larray.h"
#include "lstring.h"
#include "linteger.h"
#include "literator.h"

#include <stdio.h>
#include <string.h>

static struct lobject *
builtin_print_string_callback(struct lemon *lemon,
                              struct lframe *frame,
                              struct lobject *retval)
{
	struct lobject *strings;

	if (!lobject_is_string(lemon, retval)) {
		const char *fmt;

		fmt = "'__string__()' return non-string";
		return lobject_error_type(lemon, fmt);
	}
	strings = lframe_get_item(lemon, frame, 0);

	return larray_append(lemon, strings, 1, &retval);
}

static struct lobject *
builtin_print_object(struct lemon *lemon,
                     long i,
                     struct lobject *objects,
                     struct lobject *strings);

static struct lobject *
builtin_print_callback(struct lemon *lemon,
                       struct lframe *frame,
                       struct lobject *retval)
{
	long i;
	struct lobject *offset;
	struct lobject *string;
	struct lobject *objects;
	struct lobject *strings;

	offset = lframe_get_item(lemon, frame, 0);
	objects = lframe_get_item(lemon, frame, 1);
	strings = lframe_get_item(lemon, frame, 2);

	i = linteger_to_long(lemon, offset);
	if (i < larray_length(lemon, objects)) {
		return builtin_print_object(lemon, i, objects, strings);
	}

	/* finish strings */
	for (i = 0; i < larray_length(lemon, strings) - 1; i++) {
		string = larray_get_item(lemon, strings, i);
		printf("%s ", lstring_to_cstr(lemon, string));
	}
	string = larray_get_item(lemon, strings, i);
	printf("%s\n", lstring_to_cstr(lemon, string));

	return lemon->l_nil;
}

static struct lobject *
builtin_print_instance(struct lemon *lemon,
                       struct lobject *object,
                       struct lobject *strings)
{
	struct lframe *frame;
	struct lobject *function;

	function = lobject_default_get_attr(lemon,
	                                    object,
	                                    lemon->l_string_string);
	if (!function) {
		const char *fmt;

		fmt = "'%@' has no attribute '__string__()'";
		return lobject_error_type(lemon, fmt, object);
	}

	if (lobject_is_error(lemon, function)) {
		return function;
	}

	frame = lemon_machine_push_new_frame(lemon,
	                                     object,
	                                     NULL,
	                                     builtin_print_string_callback,
	                                     1);
	if (!frame) {
		return NULL;
	}
	lframe_set_item(lemon, frame, 0, strings);

	return lobject_call(lemon, function, 0, NULL);
}

static struct lobject *
builtin_print_object(struct lemon *lemon,
                     long i,
                     struct lobject *objects,
                     struct lobject *strings)
{
	struct lframe *frame;
	struct lobject *offset;
	struct lobject *object;
	struct lobject *string;

	frame = lemon_machine_push_new_frame(lemon,
	                                     NULL,
	                                     NULL,
	                                     builtin_print_callback,
	                                     3);
	if (!frame) {
		return NULL;
	}

	offset = linteger_create_from_long(lemon, i + 1);
	lframe_set_item(lemon, frame, 0, offset);
	lframe_set_item(lemon, frame, 1, objects);
	lframe_set_item(lemon, frame, 2, strings);

	object = larray_get_item(lemon, objects, i);

	/*
	 * instance object use `__string__' attr
	 */
	if (lobject_is_instance(lemon, object)) {
		return builtin_print_instance(lemon, object, strings);
	}

	/*
	 * non-instance object use `lobject_string'
	 */
	string = lobject_string(lemon, object);

	return larray_append(lemon, strings, 1, &string);
}

static struct lobject *
builtin_print(struct lemon *lemon,
              struct lobject *self,
              int argc, struct lobject *argv[])
{
	struct lobject *objects;
	struct lobject *strings;

	objects = larray_create(lemon, argc, argv);
	if (!objects) {
		return NULL;
	}
	strings = larray_create(lemon, 0, NULL);
	if (!strings) {
		return NULL;
	}

	return builtin_print_object(lemon, 0, objects, strings);
}

static struct lobject *
builtin_input(struct lemon *lemon,
              struct lobject *self,
              int argc, struct lobject *argv[])
{
	char *p;
	const char *fmt;
	const char *cstr;
	char buffer[4096];

	if (argc > 1) {
		fmt = "input() takes 1 string argument";
		return lobject_error_argument(lemon, fmt);
	}

	if (argc && !lobject_is_string(lemon, argv[0])) {
		fmt = "input() takes 1 string argument";
		return lobject_error_argument(lemon, fmt);
	}

	if (argc) {
		cstr = lstring_to_cstr(lemon, argv[0]);
		printf("%s", cstr);
	}

	p = fgets(buffer, sizeof(buffer), stdin);
	if (p) {
		long length;

		length = strnlen(p, sizeof(buffer));
		/* remove trail '\n' */
		while (p[length--] == '\n') {
		}
		if (length > 0) {
			return lstring_create(lemon, p, length);
		}
	}

	return lemon->l_empty_string;
}

static struct lobject *
builtin_map_item_callback(struct lemon *lemon,
                          struct lframe *frame,
                          struct lobject *retval)
{
	long i;
	struct lobject *object;
	struct lobject *callable;
	struct lobject *iterable;

	if (!larray_append(lemon, frame->self, 1, &retval)) {
		return NULL;
	}

	callable = frame->callee;
	iterable = lframe_get_item(lemon, frame, 0);
	i = larray_length(lemon, frame->self);
	if (i < larray_length(lemon, iterable)) {
		object = larray_get_item(lemon, iterable, i);
		frame = lemon_machine_push_new_frame(lemon,
		                                     frame->self,
		                                     callable,
		                                     builtin_map_item_callback,
		                                     1);
		if (!frame) {
			return NULL;
		}
		lframe_set_item(lemon, frame, 0, iterable);

		return lobject_call(lemon, callable, 1, &object);
	}

	return larray_create(lemon, 0, NULL);
}

static struct lobject *
builtin_map_array_callback(struct lemon *lemon,
                           struct lframe *frame,
                           struct lobject *retval)
{
	return frame->self;
}

static struct lobject *
builtin_map_array(struct lemon *lemon,
                  struct lobject *callable,
                  struct lobject *iterable)
{
	struct lframe *frame;
	struct lobject *array;
	struct lobject *object;

	array = larray_create(lemon, 0, NULL);
	if (!array) {
		return NULL;
	}
	frame = lemon_machine_push_new_frame(lemon,
	                                     array,
	                                     NULL,
	                                     builtin_map_array_callback,
	                                     0);
	if (!frame) {
		return NULL;
	}

	frame = lemon_machine_push_new_frame(lemon,
	                                     array,
	                                     callable,
	                                     builtin_map_item_callback,
	                                     1);
	if (!frame) {
		return NULL;
	}
	lframe_set_item(lemon, frame, 0, iterable);

	object = larray_get_item(lemon, iterable, 0);
	if (!object) {
		return NULL;
	}

	return lobject_call(lemon, callable, 1, &object);
}

static struct lobject *
builtin_map_iterable_callback(struct lemon *lemon,
                              struct lframe *frame,
                              struct lobject *retval)
{
	if (frame->self != lemon->l_nil) {
		return builtin_map_array(lemon, frame->self, retval);
	}

	return retval;
}

static struct lobject *
builtin_map_iterable(struct lemon *lemon,
                     struct lobject *function,
                     struct lobject *iterable)
{
	struct lframe *frame;

	frame = lemon_machine_push_new_frame(lemon,
	                                     function,
	                                     NULL,
	                                     builtin_map_iterable_callback,
	                                     0);
	if (!frame) {
		return NULL;
	}

	return literator_to_array(lemon, iterable, 0);
}

struct lobject *
builtin_map(struct lemon *lemon,
            struct lobject *self,
            int argc, struct lobject *argv[])
{
	struct lobject *callable;
	struct lobject *iterable;

	if (argc != 2) {
		return lobject_error_argument(lemon,
		                              "'map()' requires 2 arguments");
	}

	callable = argv[0];
	iterable = argv[1];
	if (lobject_is_array(lemon, iterable)) {
		return builtin_map_array(lemon, callable, iterable);
	}

	return builtin_map_iterable(lemon, callable, iterable);
}

void
builtin_init(struct lemon *lemon)
{
	char *cstr;
	struct lobject *name;
	struct lobject *function;

	cstr = "map";
	name = lstring_create(lemon, cstr, strlen(cstr));
	function = lfunction_create(lemon, name, NULL, builtin_map);
	lemon_add_global(lemon, cstr, function);

	cstr = "print";
	name = lstring_create(lemon, cstr, strlen(cstr));
	function = lfunction_create(lemon, name, NULL, builtin_print);
	lemon_add_global(lemon, cstr, function);

	cstr = "input";
	name = lstring_create(lemon, cstr, strlen(cstr));
	function = lfunction_create(lemon, name, NULL, builtin_input);
	lemon_add_global(lemon, cstr, function);
}
