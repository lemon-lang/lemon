#include "lemon.h"
#include "opcode.h"
#include "machine.h"
#include "allocator.h"
#include "collector.h"
#include "lkarg.h"
#include "lvarg.h"
#include "lvkarg.h"
#include "larray.h"
#include "lclass.h"
#include "lsuper.h"
#include "lmodule.h"
#include "lstring.h"
#include "linteger.h"
#include "literator.h"
#include "ldictionary.h"

#include <stdio.h>
#include <assert.h>
#include <string.h>

struct machine *
machine_create(struct lemon *lemon)
{
	size_t size;
	struct machine *machine;

	machine = allocator_alloc(lemon, sizeof(*machine));
	if (!machine) {
		return NULL;
	}
	memset(machine, 0, sizeof(*machine));
	machine->fp = -1;
	machine->sp = -1;

	machine->codelen = 4096;
	machine->cpoollen = 256;
	machine->framelen = 256;
	machine->stacklen = 256;

	size = sizeof(unsigned char) * machine->codelen;
	machine->code = allocator_alloc(lemon, size);
	if (!machine->code) {
		return NULL;
	}
	memset(machine->code, 0, size);

	size = sizeof(struct lobject *) * machine->cpoollen;
	machine->cpool = allocator_alloc(lemon, size);
	if (!machine->cpool) {
		return NULL;
	}
	memset(machine->cpool, 0, size);

	size = sizeof(struct lframe *) * machine->framelen;
	machine->frame = allocator_alloc(lemon, size);
	if (!machine->frame) {
		return NULL;
	}
	memset(machine->frame, 0, size);

	size = sizeof(struct lobject *) * machine->stacklen;
	machine->stack = allocator_alloc(lemon, size);
	if (!machine->stack) {
		return NULL;
	}
	memset(machine->stack, 0, size);

	return machine;
}

void
machine_destroy(struct lemon *lemon, struct machine *machine)
{
	allocator_free(lemon, machine->code);
	allocator_free(lemon, machine->cpool);
	allocator_free(lemon, machine->frame);
	allocator_free(lemon, machine->stack);
	allocator_free(lemon, machine);
}

void
machine_reset(struct lemon *lemon)
{
	lemon_machine_set_pc(lemon, 0);
}

void
lemon_machine_reset(struct lemon *lemon)
{
	machine_reset(lemon);
}

int
machine_add_code1(struct lemon *lemon, int value)
{
	int location;
	struct machine *machine;

	machine = lemon->l_machine;
	if (machine->pc + 1 >= machine->codelen) {
		size_t size;

		size = sizeof(unsigned char) * machine->codelen * 2;
		machine->code = allocator_realloc(lemon, machine->code, size);
		if (!machine->code) {
			return 0;
		}

		machine->codelen *= 2;
	}

	location = machine->pc;
	machine_set_code1(lemon, location, value);
	machine->pc += 1;

	return location;
}

int
machine_set_code1(struct lemon *lemon, int location, int opcode)
{
	struct machine *machine;

	machine = lemon->l_machine;
	machine->code[location] = (unsigned char)opcode;

	return location;
}

int
machine_add_code4(struct lemon *lemon, int value)
{
	int location;
	struct machine *machine;

	machine = lemon->l_machine;
	if (machine->pc + 4 >= machine->codelen) {
		size_t size;

		size = sizeof(unsigned char) * machine->codelen * 2;
		machine->code = allocator_realloc(lemon, machine->code, size);
		if (!machine->code) {
			return 0;
		}

		machine->codelen *= 2;
	}
	location = machine->pc;
	machine_set_code4(lemon, location, value);
	machine->pc += 4;

	return location;
}

int
machine_set_code4(struct lemon *lemon, int location, int value)
{
	struct machine *machine;

	machine = lemon->l_machine;
	machine->code[location + 0] = (unsigned char)((value >> 24) & 0xFF);
	machine->code[location + 1] = (unsigned char)((value >> 16) & 0xFF);
	machine->code[location + 2] = (unsigned char)((value >> 8)  & 0xFF);
	machine->code[location + 3] = (unsigned char)(value         & 0xFF);

	return location;
}

int
machine_fetch_code1(struct lemon *lemon)
{
	struct machine *machine;

	machine = lemon->l_machine;
	if (machine->pc < machine->maxpc) {
		return machine->code[machine->pc++];
	}

	printf("fetch overflow\n");
	machine->halt = 1;

	return 0;
}

int
machine_fetch_code4(struct lemon *lemon)
{
	int pc;
	int value;
	struct machine *machine;

	machine = lemon->l_machine;
	pc = machine->pc;
	if (pc < machine->maxpc - 3) {
		value = ((machine->code[pc] << 24)   & 0xFF000000) |
		        ((machine->code[pc+1] << 16) & 0xFF0000) |
		        ((machine->code[pc+2] << 8)  & 0xFF00) |
		         (machine->code[pc+3]        & 0xFF);
		machine->pc += 4;

		return value;
	}

	printf("fetch over size\n");
	machine->halt = 1;

	return 0;
}

int
machine_add_const(struct lemon *lemon, struct lobject *object)
{
	int i;
	struct machine *machine;

	machine = lemon->l_machine;
	for (i = 0; i < machine->cpoollen; i++) {
		if (machine->cpool[i] == NULL) {
			machine->cpool[i] = object;

			return i;
		}

		if (object == machine->cpool[i]) {
			return i;
		}

		if (lobject_is_pointer(lemon, object) &&
		    lobject_is_pointer(lemon, machine->cpool[i]) &&
		    object->l_method == machine->cpool[i]->l_method &&
		    lobject_is_equal(lemon, object, machine->cpool[i]))
		{
			return i;
		}
	}

	return 0;
}

struct lobject *
machine_get_const(struct lemon *lemon, int pool)
{
	struct machine *machine;

	machine = lemon->l_machine;
	return machine->cpool[pool];
}

struct lobject *
machine_stack_underflow(struct lemon *lemon)
{
	struct lobject *error;

	error = lobject_error_runtime(lemon, "stack underflow");
	machine_push_object(lemon, error);
	machine_throw(lemon);

	return error;
}

struct lobject *
machine_stack_overflow(struct lemon *lemon)
{
	struct lobject *error;

	error = lobject_error_runtime(lemon, "stack overflow");
	machine_push_object(lemon, error);
	machine_throw(lemon);

	return error;
}

void
machine_frame_underflow(struct lemon *lemon)
{
	struct machine *machine;

	machine = lemon->l_machine;
	machine->halt = 1;
	printf("machine frame underflow");
}

void
machine_frame_overflow(struct lemon *lemon)
{
	struct machine *machine;

	machine = lemon->l_machine;
	machine->halt = 1;
	printf("machine frame overflow");
}

struct lobject *
machine_out_of_memory(struct lemon *lemon)
{
	struct lobject *error;

	error = lemon->l_out_of_memory;
	machine_push_object(lemon, error);
	machine_throw(lemon);

	return error;
}

int
lemon_machine_get_ra(struct lemon *lemon)
{
	struct machine *machine;

	machine = lemon->l_machine;
	if (machine->fp >= 1) {
		return machine->frame[1]->ra;
	}

	return machine->pc;
}

void
lemon_machine_set_ra(struct lemon *lemon, int ra)
{
	struct machine *machine;

	machine = lemon->l_machine;
	if (machine->fp >= 1) {
		machine->frame[1]->ra = ra;
	}
}

int
lemon_machine_get_pc(struct lemon *lemon)
{
	struct machine *machine;

	machine = lemon->l_machine;
	return machine->pc;
}

void
lemon_machine_set_pc(struct lemon *lemon, int pc)
{
	struct machine *machine;

	machine = lemon->l_machine;
	machine->pc = pc;
}

int
lemon_machine_get_fp(struct lemon *lemon)
{
	struct machine *machine;

	machine = lemon->l_machine;
	return machine->fp;
}

void
lemon_machine_set_fp(struct lemon *lemon, int fp)
{
	struct machine *machine;

	machine = lemon->l_machine;
	machine->fp = fp;
}

int
lemon_machine_get_sp(struct lemon *lemon)
{
	struct machine *machine;

	machine = lemon->l_machine;
	return machine->sp;
}

void
lemon_machine_set_sp(struct lemon *lemon, int sp)
{
	struct machine *machine;

	machine = lemon->l_machine;
	machine->sp = sp;
}

struct lframe *
lemon_machine_get_frame(struct lemon *lemon, int fp)
{
	struct machine *machine;

	machine = lemon->l_machine;
	if (fp <= machine->fp) {
		return machine->frame[fp];
	}

	return NULL;
}

void
lemon_machine_set_frame(struct lemon *lemon, int fp, struct lframe *frame)
{
	struct machine *machine;

	machine = lemon->l_machine;
	if (fp <= machine->fp) {
		machine->frame[fp] = frame;
	}
}

struct lobject *
lemon_machine_get_stack(struct lemon *lemon, int sp)
{
	struct machine *machine;

	machine = lemon->l_machine;
	if (sp <= machine->sp) {
		return machine->stack[sp];
	}

	return NULL;
}

void
machine_push_object(struct lemon *lemon, struct lobject *object)
{
	struct machine *machine;
	struct lobject **stack;

	machine = lemon->l_machine;
	if (machine->sp < machine->stacklen - 1) {
		machine->stack[++machine->sp] = object;
	} else {
		size_t size;

		size = sizeof(struct lobject *) * machine->stacklen * 2;
		stack = allocator_realloc(lemon, machine->stack, size);
		if (stack) {
			machine->stack = stack;
			machine->stacklen *= 2;
			machine->stack[++machine->sp] = object;
		} else {
			machine_stack_overflow(lemon);
		}
	}
}

void
lemon_machine_push_object(struct lemon *lemon, struct lobject *object)
{
	machine_push_object(lemon, object);
}

struct lobject *
machine_pop_object(struct lemon *lemon)
{
	struct machine *machine;

	machine = lemon->l_machine;
	if (machine->sp >= 0) {
		return machine->stack[machine->sp--];
	}

	machine_stack_underflow(lemon);

	return NULL;
}

struct lobject *
lemon_machine_pop_object(struct lemon *lemon)
{
	return machine_pop_object(lemon);
}

struct lframe *
machine_push_new_frame(struct lemon *lemon,
                       struct lobject *self,
                       struct lobject *callee,
                       lframe_call_t callback,
                       int nlocals)
{
	struct lframe *frame;
	struct machine *machine;

	machine = lemon->l_machine;
	if (machine->fp < machine->framelen) {
		frame = lframe_create(lemon, self, callee, callback, nlocals);
		if (frame) {
			machine_store_frame(lemon, frame);
			machine_push_frame(lemon, frame);
		}

		return frame;
	}

	machine_frame_overflow(lemon);

	return NULL;
}

struct lframe *
lemon_machine_push_new_frame(struct lemon *lemon,
                             struct lobject *self,
                             struct lobject *callee,
                             lframe_call_t callback,
                             int nlocals)
{
	return machine_push_new_frame(lemon, self, callee, callback, nlocals);
}

struct lobject *
machine_return_frame(struct lemon *lemon, struct lobject *retval)
{
	struct machine *machine;
	struct lframe *frame;

	if (!retval) {
		return lemon->l_out_of_memory;
	}
	machine = lemon->l_machine;
	while (machine->fp >= 0 && machine->frame[machine->fp]->callback) {
		frame = lemon_machine_pop_frame(lemon);
		lemon_machine_restore_frame(lemon, frame);
		retval = frame->callback(lemon, frame, retval);

		/* make sure function has return value */
		if (!retval) {
			return lemon->l_out_of_memory;
		}

		if (lobject_is_error(lemon, retval)) {
			return retval;
		}
		machine_push_object(lemon, retval);
	}

	return retval;
}

struct lobject *
lemon_machine_return_frame(struct lemon *lemon, struct lobject *retval)
{
	return machine_return_frame(lemon, retval);
}

void
machine_push_frame(struct lemon *lemon, struct lframe *frame)
{
	struct machine *machine;

	machine = lemon->l_machine;
	if (machine->fp < machine->framelen - 1) {
		machine->frame[++machine->fp] = frame;
	} else {
		printf("frame overflow\n");
		machine->halt = 1;
	}
}

void
lemon_machine_push_frame(struct lemon *lemon, struct lframe *frame)
{
	machine_push_frame(lemon, frame);
}

struct lframe *
machine_peek_frame(struct lemon *lemon)
{
	struct machine *machine;

	machine = lemon->l_machine;
	if (machine->fp >= 0) {
		return machine->frame[machine->fp];
	}

	machine_frame_underflow(lemon);

	return NULL;
}

struct lframe *
lemon_machine_peek_frame(struct lemon *lemon)
{
	return machine_peek_frame(lemon);
}

struct lframe *
machine_pop_frame(struct lemon *lemon)
{
	struct machine *machine;

	machine = lemon->l_machine;
	if (machine->fp >= 0) {
		return machine->frame[machine->fp--];
	}

	machine_frame_underflow(lemon);

	return NULL;
}

struct lframe *
lemon_machine_pop_frame(struct lemon *lemon)
{
	return machine_pop_frame(lemon);
}

void
machine_store_frame(struct lemon *lemon, struct lframe *frame)
{
	struct machine *machine;

	machine = lemon->l_machine;
	frame->sp = machine->sp;
	frame->ra = machine->pc;
}

void
lemon_machine_store_frame(struct lemon *lemon, struct lframe *frame)
{
	machine_store_frame(lemon, frame);
}

void
machine_restore_frame(struct lemon *lemon, struct lframe *frame)
{
	struct machine *machine;

	machine = lemon->l_machine;
	machine->sp = frame->sp;
	machine->pc = frame->ra;
}

void
lemon_machine_restore_frame(struct lemon *lemon, struct lframe *frame)
{
	machine_restore_frame(lemon, frame);
}

struct lobject *
lemon_machine_parse_args(struct lemon *lemon,
                    struct lobject *callee,
                    struct lframe *frame,
                    int define,
                    int nvalues,
                    int nparams,
                    struct lobject *params[],
                    int argc,
                    struct lobject *argv[])
{
	int i;
	int a; /* index of argv */
	int local; /* index of frame->locals */
	const char *fmt; /* error format string */

	struct lobject *key;
	struct lobject *arg;

	long v_argc;
	struct lobject *v_arg;
	struct lobject *v_args;

	int x_argc;
	struct lobject *x_argv[256];
	struct lobject *x_args;

	int x_kargc;
	struct lobject *x_kargv[512];
	struct lobject *x_kargs;

	/* too many arguments */
	if (define == 0 && argc > nparams) {
		fmt = "%@() takes %d arguments (%d given)";
		return lobject_error_argument(lemon,
		                              fmt,
		                              callee,
		                              nparams,
		                              argc);
	}

	/* remove not assignable params */
	if (define == 1) {
		nparams -= 1;
	} else if (define == 2) {
		nparams -= 1;
	} else if (define == 3) {
		nparams -= 2;
	}

	if (nparams < 0) {
		return NULL;
	}

#define machine_parse_args_set_arg(arg) do {                \
	if (local < nparams) {                              \
		lframe_set_item(lemon,                      \
		                frame,                      \
		                local++,                    \
		                (arg));                     \
	} else if (define == 1 || define == 3) {            \
		if (x_argc == 256) {                        \
			return NULL;                        \
		}                                           \
		x_argv[x_argc++] = (arg);                   \
	} else {                                            \
		fmt = "%@() takes %d arguments (%d given)"; \
		return lobject_error_argument(lemon,        \
		                              fmt,          \
		                              callee,       \
		                              nparams,      \
		                              argc);        \
	}                                                   \
} while (0)

#define machine_parse_args_set_karg(key, arg) do {                      \
	int j;                                                          \
	int k = -1;                                                     \
	for (j = 0; j < nparams; j++) {                                 \
		if (lobject_is_equal(lemon, params[j], key)) {          \
			k = j;                                          \
			break;                                          \
		}                                                       \
	}                                                               \
	if (k < 0) {                                                    \
		if (define == 2 || define == 3) {                       \
			if (x_kargc == 512) {                           \
				return NULL;                            \
			}                                               \
			x_kargv[x_kargc++] = (key);                     \
			x_kargv[x_kargc++] = (arg);                     \
		} else {                                                \
			fmt = "'%@'() don't have parameter '%@'";       \
			return lobject_error_argument(lemon,            \
			                              fmt,              \
			                              callee,           \
			                              (key));           \
		}                                                       \
	} else {                                                        \
		if (lframe_get_item(lemon, frame, k)) {                 \
			fmt = "'%@' set multiple value parameter '%@'"; \
			return lobject_error_argument(lemon,            \
			                              fmt,              \
			                              callee,           \
			                              (key));           \
		}                                                       \
		lframe_set_item(lemon, frame, k, (arg));                \
	}                                                               \
} while (0)

	local = 0;
	x_argc = 0;
	for (a = 0;
	     a < argc &&
	     !lobject_is_karg(lemon, argv[a]) &&
	     !lobject_is_varg(lemon, argv[a]) &&
	     !lobject_is_vkarg(lemon, argv[a]);
	     a++)
	{
		machine_parse_args_set_arg(argv[a]);
	}

	if (a < argc && lobject_is_varg(lemon, argv[a])) {
		v_args = ((struct lvarg *)argv[a++])->arguments;

		if (!lobject_is_array(lemon, v_args)) {
			return NULL;
		}

		v_argc = larray_length(lemon, v_args);
		for (i = 0; i < v_argc; i++) {
			arg = larray_get_item(lemon, v_args, i);

			machine_parse_args_set_arg(arg);
		}
	}

	x_kargc = 0;
	for (; a < argc && lobject_is_karg(lemon, argv[a]); a++) {
		key = ((struct lkarg *)argv[a])->keyword;
		arg = ((struct lkarg *)argv[a])->argument;

		machine_parse_args_set_karg(key, arg);
	}

	if (a < argc && lobject_is_vkarg(lemon, argv[a])) {
		v_args = lobject_map_item(lemon, argv[a]);

		assert(lobject_is_array(lemon, v_args));

		v_argc = larray_length(lemon, v_args);
		for (i = 0; i < v_argc; i++) {
			v_arg = larray_get_item(lemon, v_args, i);

			assert(lobject_is_array(lemon, v_arg));
			assert(larray_length(lemon, v_arg) == 2);

			key = larray_get_item(lemon, v_arg, 0);
			arg = larray_get_item(lemon, v_arg, 1);

			machine_parse_args_set_karg(key, arg);
		}
	}

	if (define == 1) {
		x_args = larray_create(lemon, x_argc, x_argv);
		lframe_set_item(lemon, frame, nparams, x_args);
	} else if (define == 2) {
		x_kargs = ldictionary_create(lemon, x_kargc, x_kargv);
		lframe_set_item(lemon, frame, nparams, x_kargs);
	} else if (define == 3) {
		x_args = larray_create(lemon, x_argc, x_argv);
		lframe_set_item(lemon, frame, nparams++, x_args);

		x_kargs = ldictionary_create(lemon, x_kargc, x_kargv);
		lframe_set_item(lemon, frame, nparams, x_kargs);
	}

	/* check required args */
	for (i = 0; i < nparams - nvalues; i++) {
		if (lframe_get_item(lemon, frame, i) == NULL) {
			fmt = "%@() takes %d arguments (%d given)";
			return lobject_error_argument(lemon,
			                              fmt,
			                              callee,
			                              nparams,
			                              i);
		}
	}

	/* set sentinel for optional args */
	for (; i < nparams; i++) {
		if (lframe_get_item(lemon, frame, i) == NULL) {
			lframe_set_item(lemon, frame, i, lemon->l_sentinel);
		}
	}

	/* set nil for non args */
	for (; i < frame->nlocals; i++) {
		if (lframe_get_item(lemon, frame, i) == NULL) {
			lframe_set_item(lemon, frame, i, lemon->l_nil);
		}
	}

	return NULL;
}

int
machine_throw(struct lemon *lemon)
{
	int argc;
	struct machine *machine;

	struct lframe *frame;
	struct lobject *exception;
	struct lobject *argv[128];

	exception = machine_pop_object(lemon);
	exception = lobject_throw(lemon, exception);

	argc = 0;
	machine = lemon->l_machine;
	while (machine->fp >= 0 && !machine->halt) {
		int l_try;

		frame = machine_peek_frame(lemon);
		l_try = frame->ea;
		if (l_try) {
			machine->pc = l_try;
			machine->sp = frame->sp;
			machine->exception = exception;
			lobject_call_attr(lemon,
			                  exception,
			                  lstring_create(lemon, "addtrace", 8),
			                  argc,
			                  argv);

			machine_return_frame(lemon, lemon->l_nil);
			return 1;
		}
		machine_pop_frame(lemon);
		machine_restore_frame(lemon, frame);
		argv[argc++] = (struct lobject *)frame;
	}

	printf("Uncaught Exception: ");
	lobject_print(lemon, exception, NULL);
	lobject_call_attr(lemon,
	                  exception,
	                  lstring_create(lemon, "addtrace", 8),
	                  argc,
	                  argv);

	lobject_call_attr(lemon,
	                  exception,
	                  lstring_create(lemon, "traceback", 9),
	                  0,
	                  NULL);
	machine->halt = 1;
	machine_return_frame(lemon, lemon->l_nil);

	return 0;
}

void
machine_halt(struct lemon *lemon)
{
	struct machine *machine;

	machine = lemon->l_machine;
	machine->halt = 1;
}

int
machine_extend_stack(struct lemon *lemon)
{
	size_t size;
	struct machine *machine;
	struct lobject **stack;

	machine = lemon->l_machine;
	size = sizeof(struct lobject *) * machine->stacklen * 2;
	stack = allocator_realloc(lemon, machine->stack, size);
	if (stack) {
		machine->stack = stack;
		machine->stacklen *= 2;
	} else {
		machine->stacklen = 0;
		machine_halt(lemon);

		return 0;
	}

	return 1;
}

struct lobject *
machine_unpack_callback(struct lemon *lemon,
                   struct lframe *frame,
                   struct lobject *retval)
{
	long i;
	long n;
	long unpack;
	struct lobject *last;

	n = larray_length(lemon, retval);
	unpack = linteger_to_long(lemon, lframe_get_item(lemon, frame, 0));
	if (n < unpack) {
		last = lemon->l_nil;
	} else {
		n -= 1;
		last = larray_get_item(lemon, retval, n);
	}

	for (i = 0; i < n; i++) {
		machine_push_object(lemon, larray_get_item(lemon, retval, i));
	}
	for (; i < unpack - 1; i++) {
		machine_push_object(lemon, lemon->l_nil);
	}

	/* push last */
	return last;
}

struct lobject *
machine_unpack_iterable(struct lemon *lemon, struct lobject *iterable, int n)
{
	struct lframe *frame;

	frame = machine_push_new_frame(lemon,
	                               NULL,
	                               NULL,
	                               machine_unpack_callback,
	                               1);
	if (!frame) {
		return NULL;
	}
	lframe_set_item(lemon, frame, 0, linteger_create_from_long(lemon, n));

	return literator_to_array(lemon, iterable, n);
}

struct lobject *
machine_varg_callback(struct lemon *lemon,
                 struct lframe *frame,
                 struct lobject *retval)
{
	return lvarg_create(lemon, retval);
}

struct lobject *
machine_varg_iterable(struct lemon *lemon, struct lobject *iterable)
{
	struct lframe *frame;

	frame = machine_push_new_frame(lemon,
	                               NULL,
	                               NULL,
	                               machine_varg_callback,
	                               0);
	if (!frame) {
		return NULL;
	}

	return literator_to_array(lemon, iterable, 256);
}

struct lobject *
machine_call_getter(struct lemon *lemon,
               struct lobject *getter,
               struct lobject *self,
               struct lobject *name,
               struct lobject *value)
{
	return lobject_call(lemon, getter, 1, &value);
}

struct lobject *
machine_setattr_callback(struct lemon *lemon,
                    struct lframe *frame,
                    struct lobject *retval)
{
	struct lobject *name;

	name = lframe_get_item(lemon, frame, 0);
	lobject_set_attr(lemon, frame->self, name, retval);

	return retval;
}

struct lobject *
machine_call_setter(struct lemon *lemon,
               struct lobject *setter,
               struct lobject *self,
               struct lobject *name,
               struct lobject *value)
{
	struct lframe *frame;

	frame = machine_push_new_frame(lemon,
	                               self,
	                               0,
	                               machine_setattr_callback,
	                               1);
	if (!frame) {
		return NULL;
	}
	lframe_set_item(lemon, frame, 0, name);

	return lobject_call(lemon, setter, 1, &value);
}

int
lemon_machine_execute(struct lemon *lemon)
{
	struct machine *machine;
	struct lobject *object;

	machine = lemon->l_machine;
	machine->sp = -1;
	machine->fp = -1;
	machine->halt = 0;

	lemon_collector_enable(lemon);
	object = lemon_machine_execute_loop(lemon);
	if (lobject_is_error(lemon, object)) {
		printf("Uncaught Exception: ");
		lobject_print(lemon, object, NULL);
		lobject_call_attr(lemon,
		                  object,
		                  lstring_create(lemon, "traceback", 9),
		                  0,
		                  NULL);
	}
	machine->sp = -1;
	machine->fp = -1;
	collector_full(lemon);

	return 1;
}

struct lobject *
lemon_machine_execute_loop(struct lemon *lemon)
{
	int i;
	int argc;
	int opcode;

	struct machine *machine;
	struct lframe *frame;

	struct lobject *a; /* src value 1 or base value */
	struct lobject *b; /* src value 2               */
	struct lobject *c; /* src value 3 or dest value */
	struct lobject *d; /* src value 4               */
	struct lobject *e; /* src value 5 or exception value */

	struct lobject *argv[256]; /* base value call arguments */

#define CHECK_NULL(p) do {                      \
	if (!(p)) {                             \
		machine_out_of_memory(lemon);   \
		break;                          \
	}                                       \
} while (0)                                     \

#define CHECK_ERROR(object) do {                 \
	if (lobject_is_error(lemon, (object))) { \
		PUSH_OBJECT((object));           \
		machine_throw(lemon);            \
		break;                           \
	}                                        \
} while (0)                                      \

#define UNOP(m) do {                             \
	if (machine->sp >= 0) {                  \
		a = POP_OBJECT();                \
		c = lobject_unop(lemon, (m), a); \
		CHECK_NULL(c);                   \
		CHECK_ERROR(c);                  \
		PUSH_OBJECT(c);                  \
	} else {                                 \
		machine_stack_underflow(lemon);  \
	}                                        \
} while(0)

#define BINOP(m) do {                                \
	if (machine->sp >= 1) {                      \
		b = POP_OBJECT();                    \
		a = POP_OBJECT();                    \
		c = lobject_binop(lemon, (m), a, b); \
		CHECK_NULL(c);                       \
		CHECK_ERROR(c);                      \
		PUSH_OBJECT(c);                      \
	} else {                                     \
		machine_stack_underflow(lemon);      \
	}                                            \
} while(0)

#define POP_OBJECT() machine->stack[machine->sp--]

#define PUSH_OBJECT(object) do {                                  \
	if (machine->sp < machine->stacklen - 1) {                \
		machine->stack[++machine->sp] = (object);         \
	} else {                                                  \
		if (machine_extend_stack(lemon)) {                \
			machine->stack[++machine->sp] = (object); \
		} else {                                          \
			machine_stack_overflow(lemon);            \
		}                                                 \
	}                                                         \
} while(0)

#define CHECK_STACK(size) do {                  \
	if (machine->sp < (size) - 1) {         \
		machine_stack_underflow(lemon); \
		break;                          \
	}                                       \
} while (0)                                     \

#define FETCH_CODE1() machine->code[machine->pc++]
#define FETCH_CODE4() machine_fetch_code4(lemon)

#define CHECK_FETCH(size) do {                       \
	if (machine->pc + (size) > machine->maxpc) { \
		printf("fetch underflow\n");         \
		break;                               \
	}                                            \
} while (0)                                          \

#define POP_CALLBACK_FRAME(retval) do {                                     \
	while (machine->fp >= 0 && machine->frame[machine->fp]->callback) { \
		frame = machine_pop_frame(lemon);                           \
		machine_restore_frame(lemon, frame);                        \
		(retval) = frame->callback(lemon, frame, (retval));         \
		CHECK_NULL((retval));                                       \
		CHECK_ERROR((retval));                                      \
		PUSH_OBJECT((retval));                                      \
	}                                                                   \
} while (0)

	machine = lemon->l_machine;
	while (!machine->halt && machine->pc < machine->maxpc) {
		opcode = machine->code[machine->pc++];

		switch (opcode) {
		case OPCODE_HALT:
			machine->fp = -1;
			machine->sp = -1;
			machine->halt = 1;
			collector_full(lemon);
			break;

		case OPCODE_NOP:
			break;

		case OPCODE_POS:
			UNOP(LOBJECT_METHOD_POS);
			break;

		case OPCODE_NEG:
			UNOP(LOBJECT_METHOD_NEG);
			break;

		case OPCODE_BNOT:
			UNOP(LOBJECT_METHOD_BITWISE_NOT);
			break;

		case OPCODE_ADD:
			BINOP(LOBJECT_METHOD_ADD);
			break;

		case OPCODE_SUB:
			BINOP(LOBJECT_METHOD_SUB);
			break;

		case OPCODE_MUL:
			BINOP(LOBJECT_METHOD_MUL);
			break;

		case OPCODE_DIV:
			BINOP(LOBJECT_METHOD_DIV);
			break;

		case OPCODE_MOD:
			BINOP(LOBJECT_METHOD_MOD);
			break;

		case OPCODE_SHL:
			BINOP(LOBJECT_METHOD_SHL);
			break;

		case OPCODE_SHR:
			BINOP(LOBJECT_METHOD_SHR);
			break;

		case OPCODE_EQ:
			BINOP(LOBJECT_METHOD_EQ);
			break;

		case OPCODE_NE:
			BINOP(LOBJECT_METHOD_NE);
			break;

		case OPCODE_IN: {
			CHECK_STACK(2);
			a = POP_OBJECT();
			b = POP_OBJECT();
			c = lobject_binop(lemon,
			                  LOBJECT_METHOD_HAS_ITEM,
			                  a,
			                  b);
			CHECK_NULL(c);
			CHECK_ERROR(c);
			PUSH_OBJECT(c);
			break;
		}

		case OPCODE_LT:
			BINOP(LOBJECT_METHOD_LT);
			break;

		case OPCODE_LE:
			BINOP(LOBJECT_METHOD_LE);
			break;

		case OPCODE_GT:
			BINOP(LOBJECT_METHOD_GT);
			break;

		case OPCODE_GE:
			BINOP(LOBJECT_METHOD_GE);
			break;

		case OPCODE_BAND:
			BINOP(LOBJECT_METHOD_BITWISE_AND);
			break;

		case OPCODE_BOR:
			BINOP(LOBJECT_METHOD_BITWISE_OR);
			break;

		case OPCODE_BXOR:
			BINOP(LOBJECT_METHOD_BITWISE_XOR);
			break;

		case OPCODE_LNOT:
			CHECK_STACK(1);
			a = POP_OBJECT();
			if (lobject_boolean(lemon, a) == lemon->l_true) {
				PUSH_OBJECT(lemon->l_false);
			} else {
				PUSH_OBJECT(lemon->l_true);
			}
			break;

		case OPCODE_POP:
			machine->sp -= 1;
			break;

		case OPCODE_DUP:
			CHECK_STACK(1);
			a = machine->stack[machine->sp];
			PUSH_OBJECT(a);
			break;

		case OPCODE_SWAP: {
			CHECK_STACK(2);
			a = POP_OBJECT();
			b = POP_OBJECT();
			PUSH_OBJECT(a);
			PUSH_OBJECT(b);
			break;
		}

		case OPCODE_LOAD: {
			int level;
			int local;

			CHECK_FETCH(2);
			level = FETCH_CODE1();
			local = FETCH_CODE1();
			frame = machine_peek_frame(lemon);
			while (level-- > 0) {
				frame = frame->upframe;
			}
			PUSH_OBJECT(lframe_get_item(lemon, frame, local));
			break;
		}

		case OPCODE_STORE: {
			int level;
			int local;

			CHECK_FETCH(2);
			level = FETCH_CODE1();
			local = FETCH_CODE1();
			frame = machine_peek_frame(lemon);
			while (level-- > 0) {
				frame = frame->upframe;
			}
			CHECK_STACK(1);
			a = POP_OBJECT();
			c = lframe_set_item(lemon, frame, local, a);
			CHECK_NULL(c);
			CHECK_ERROR(c);
			break;
		}

		case OPCODE_CONST: {
			CHECK_FETCH(4);
			i = FETCH_CODE4();
			PUSH_OBJECT(machine->cpool[i]);
			break;
		}

		case OPCODE_UNPACK: {
			int n;
			long length;

			CHECK_FETCH(1);
			n = FETCH_CODE1();
			CHECK_STACK(1);
			a = POP_OBJECT();
			if (lobject_is_array(lemon, a)) {
				length = larray_length(lemon, a);
				for (i = 0; i < n && i < length; i++) {
					c = larray_get_item(lemon, a, i);
					CHECK_NULL(c);
					CHECK_ERROR(c);
					PUSH_OBJECT(c);
				}

				for (; i < n; i++) {
					PUSH_OBJECT(lemon->l_nil);
				}
			} else {
				c = machine_unpack_iterable(lemon, a, n);
				CHECK_NULL(c);
				CHECK_ERROR(c);
			}
			break;
		}

		case OPCODE_GETITEM: {
			CHECK_STACK(2);
			b = POP_OBJECT();
			a = POP_OBJECT();
			c = lobject_get_item(lemon, a, b);
			if (!c) {
				c = lobject_error_item(lemon,
				                       "'%@' has no item '%@'",
				                       a,
				                       b);
			}
			CHECK_ERROR(c);
			PUSH_OBJECT(c);
			break;
		}

		case OPCODE_SETITEM: {
			CHECK_STACK(3);
			b = POP_OBJECT();
			a = POP_OBJECT();
			c = POP_OBJECT();
			e = lobject_set_item(lemon, a, b, c);
			if (!e) {
				e = lobject_error_item(lemon,
				                       "'%@' has no item '%@'",
				                       a,
				                       b);
			}
			CHECK_ERROR(e);
			break;
		}

		case OPCODE_DELITEM: {
			CHECK_STACK(2);
			b = POP_OBJECT();
			a = POP_OBJECT();
			e = lobject_del_item(lemon, a, b);
			if (!e) {
				e = lobject_error_item(lemon,
				                       "'%@' has no item '%@'",
				                       a,
				                       b);
			}
			CHECK_ERROR(e);
			break;
		}

		case OPCODE_GETATTR: {
			struct lobject *getter;

			CHECK_STACK(2);
			b = POP_OBJECT(); /* name */
			a = POP_OBJECT(); /* object */
			c = lobject_default_get_attr(lemon, a, b);
			if (!c) {
				const char *fmt;

				fmt = "'%@' has no attribute '%@'";
				c = lobject_error_attribute(lemon, fmt, a, b);
			}
			CHECK_ERROR(c);

			getter = lobject_get_getter(lemon, a, b);
			if (getter) {
				c = machine_call_getter(lemon, getter, a, b, c);
				CHECK_NULL(c);
				CHECK_ERROR(c);
				POP_CALLBACK_FRAME(c);
			}
			PUSH_OBJECT(c);
			break;
		}

		case OPCODE_SETATTR: {
			struct lobject *setter;

			CHECK_STACK(3);
			b = POP_OBJECT(); /* name */
			a = POP_OBJECT(); /* object */
			c = POP_OBJECT(); /* value */
			e = lobject_set_attr(lemon, a, b, c);
			if (!e) {
				const char *fmt;

				fmt = "'%@' has no attribute '%@'";
				e = lobject_error_attribute(lemon, fmt, a, b);
			}
			CHECK_ERROR(e);

			setter = lobject_get_setter(lemon, a, b);
			if (setter) {
				e = machine_call_setter(lemon, setter, a, b, c);
				CHECK_NULL(e);
				CHECK_ERROR(e);
				POP_CALLBACK_FRAME(e);
			}
			break;
		}

		case OPCODE_DELATTR: {
			CHECK_STACK(2);
			b = POP_OBJECT();
			a = POP_OBJECT();
			e = lobject_del_attr(lemon, a, b);
			if (!e) {
				const char *fmt;

				fmt = "'%@' has no attribute '%@'";
				e = lobject_error_attribute(lemon, fmt, a, b);
			}
			CHECK_ERROR(e);
			break;
		}

		case OPCODE_GETSLICE: {
			CHECK_STACK(4);
			d = POP_OBJECT();
			c = POP_OBJECT();
			b = POP_OBJECT();
			a = POP_OBJECT();
			c = lobject_get_slice(lemon, a, b, c, d);
			CHECK_NULL(c);
			CHECK_ERROR(c);
			PUSH_OBJECT(c);
			break;
		}

		case OPCODE_SETSLICE: {
			CHECK_STACK(4);
			e = POP_OBJECT();
			d = POP_OBJECT();
			c = POP_OBJECT();
			b = POP_OBJECT();
			a = POP_OBJECT();
			e = lobject_set_slice(lemon, a, b, c, d, e);
			CHECK_NULL(e);
			CHECK_ERROR(e);
			break;
		}

		case OPCODE_DELSLICE: {
			CHECK_STACK(4);
			d = POP_OBJECT();
			c = POP_OBJECT();
			b = POP_OBJECT();
			a = POP_OBJECT();
			e = lobject_del_slice(lemon, a, b, c, d);
			CHECK_NULL(e);
			CHECK_ERROR(e);
			break;
		}

		case OPCODE_SETGETTER: {
			CHECK_FETCH(1);
			argc = FETCH_CODE1();

			CHECK_STACK(2);
			b = POP_OBJECT(); /* name */
			a = POP_OBJECT();

			argv[0] = b;
			CHECK_STACK(argc);
			/* start from 1 */
			argc += 1;
			for (i = 1; i < argc; i++) {
				argv[i] = POP_OBJECT();
			}

			e = lobject_method_call(lemon,
			                        a,
			                        LOBJECT_METHOD_SET_GETTER,
			                        argc,
			                        argv);
			CHECK_NULL(e);
			CHECK_ERROR(e);
			break;
		}

		case OPCODE_SETSETTER: {
			CHECK_FETCH(1);
			argc = FETCH_CODE1();

			CHECK_STACK(2);
			b = POP_OBJECT(); /* name */
			a = POP_OBJECT();

			argv[0] = b;
			CHECK_STACK(argc);
			/* start from 1 */
			argc += 1;
			for (i = 1; i < argc; i++) {
				argv[i] = POP_OBJECT();
			}

			e = lobject_method_call(lemon,
			                        a,
			                        LOBJECT_METHOD_SET_SETTER,
			                        argc,
			                        argv);
			CHECK_NULL(e);
			CHECK_ERROR(e);
			break;
		}

		case OPCODE_JZ: {
			int address;
			CHECK_FETCH(4);
			address = FETCH_CODE4();
			CHECK_STACK(1);
			a = POP_OBJECT();
			if (lobject_boolean(lemon, a) == lemon->l_false) {
				machine->pc = address;
			}
			break;
		}

		case OPCODE_JNZ: {
			int address;
			CHECK_FETCH(4);
			address = FETCH_CODE4();
			CHECK_STACK(1);
			a = POP_OBJECT();
			if (lobject_boolean(lemon, a) == lemon->l_true) {
				machine->pc = address;
			}
			break;
		}

		case OPCODE_JMP: {
			CHECK_FETCH(4);
			machine->pc = FETCH_CODE4();
			break;
		}

		case OPCODE_ARRAY: {
			size_t size;
			struct lobject **items;

			CHECK_FETCH(4);
			argc = FETCH_CODE4();
			size = sizeof(struct lobject *) * argc;
			items = allocator_alloc(lemon, size);
			CHECK_NULL(items);

			CHECK_STACK(argc);
			for (i = 0; i < argc; i++) {
				items[i] = POP_OBJECT();
			}
			c = larray_create(lemon, argc, items);
			allocator_free(lemon, items);
			CHECK_NULL(c);
			CHECK_ERROR(c);
			PUSH_OBJECT(c);
			break;
		}

		case OPCODE_DICTIONARY: {
			size_t size;
			struct lobject **items;

			CHECK_FETCH(4);
			argc = FETCH_CODE4();
			size = sizeof(struct lobject *) * argc;
			items = allocator_alloc(lemon, size);
			CHECK_NULL(items);

			CHECK_STACK(argc);
			for (i = 0; i < argc; i++) {
				items[i] = POP_OBJECT();
			}
			c = ldictionary_create(lemon, argc, items);
			allocator_free(lemon, items);
			CHECK_NULL(c);
			CHECK_ERROR(c);
			PUSH_OBJECT(c);
			break;
		}

		case OPCODE_DEFINE: {
			int define;
			int address;
			int nvalues;
			int nparams;
			int nlocals;

			struct lobject *name;
			struct lobject **params;
			struct lfunction *function;

			CHECK_FETCH(8);
			define = FETCH_CODE1();
			nvalues = FETCH_CODE1();
			nparams = FETCH_CODE1();
			nlocals = FETCH_CODE1();
			address = FETCH_CODE4();

			CHECK_STACK(1);
			name = POP_OBJECT();

			/*
			 * parameters is 0...n order list,
			 * so pop order is n..0
			 */
			CHECK_STACK(nparams);
			params = argv;
			for (i = nparams; i > 0; i--) {
				params[i - 1] = POP_OBJECT();
			}

			function = lfunction_create_with_address(lemon,
			                                         name,
			                                         define,
			                                         nlocals,
			                                         nparams,
			                                         nvalues,
			                                         machine->pc,
			                                         params);
			CHECK_NULL(function);
			CHECK_ERROR((struct lobject *)function);

			function->frame = machine_peek_frame(lemon);
			PUSH_OBJECT((struct lobject *)function);

			machine = lemon->l_machine;
			machine->pc = address; /* jmp out of function's body */
			break;
		}

		case OPCODE_KARG: {
			CHECK_STACK(2);
			b = POP_OBJECT();
			a = POP_OBJECT();
			c = lkarg_create(lemon, a, b);
			CHECK_NULL(c);
			CHECK_ERROR(c);
			PUSH_OBJECT(c);
			break;
		}

		case OPCODE_VARG: {
			CHECK_STACK(1);
			a = POP_OBJECT();
			if (lobject_is_array(lemon, a)) {
				c = lvarg_create(lemon, a);
				CHECK_NULL(c);
				CHECK_ERROR(c);
				PUSH_OBJECT(c);
			} else {
				c = machine_varg_iterable(lemon, a);
				CHECK_NULL(c);
				CHECK_ERROR(c);
			}
			break;
		}

		case OPCODE_VKARG: {
			CHECK_STACK(1);
			a = POP_OBJECT();
			c = lvkarg_create(lemon, a);
			CHECK_NULL(c);
			CHECK_ERROR(c);
			PUSH_OBJECT(c);
			break;
		}

		case OPCODE_CALL: {
			CHECK_FETCH(1);
			argc = FETCH_CODE1();
			CHECK_STACK(argc);
			for (i = 0; i < argc; i++) {
				argv[i] = POP_OBJECT();
			}

			a = POP_OBJECT();
			c = lobject_call(lemon, a, argc, argv);
			CHECK_NULL(c);
			CHECK_ERROR(c);
			POP_CALLBACK_FRAME(c);
			break;
		}

		case OPCODE_TAILCALL: {
			struct lframe *newframe;
			struct lframe *oldframe;

			CHECK_FETCH(1);
			argc = FETCH_CODE1();
			CHECK_STACK(argc);
			for (i = 0; i < argc; i++) {
				argv[i] = POP_OBJECT();
			}

			/* pop the function */
			a = POP_OBJECT();
			c = lobject_call(lemon, a, argc, argv);
			/*
			 * make sure callee is a Lemon function
			 * otherwise tailcall is just call
			 */
			if (machine_peek_frame(lemon)->callee == a) {
				newframe = machine_pop_frame(lemon);
				oldframe = machine_pop_frame(lemon);
				newframe->ra = oldframe->ra;
				machine_push_frame(lemon, newframe);
			}

			CHECK_NULL(c);
			CHECK_ERROR(c);
			POP_CALLBACK_FRAME(c);
			break;
		}

		case OPCODE_RETURN: {
			CHECK_STACK(1);
			a = POP_OBJECT();
			frame = machine_pop_frame(lemon);
			machine_restore_frame(lemon, frame);
			PUSH_OBJECT(a);
			POP_CALLBACK_FRAME(a);
			break;
		}

		case OPCODE_THROW:
			machine_throw(lemon);
			break;

		case OPCODE_TRY: {
			int address;
			CHECK_FETCH(4);
			address = FETCH_CODE4();
			frame = machine_peek_frame(lemon);
			frame->ea = address;
			break;
		}

		case OPCODE_UNTRY: {
			frame = machine_peek_frame(lemon);
			frame->ea = 0;
			break;
		}

		case OPCODE_LOADEXC: {
			PUSH_OBJECT(machine->exception);
			break;
		}

		case OPCODE_SELF: {
			frame = machine_peek_frame(lemon);
			if (frame->self) {
				PUSH_OBJECT(frame->self);
			} else {
				const char *fmt;

				fmt = "'%@' not binding to instance";
				c = lobject_error_runtime(lemon,
				                          fmt,
				                          frame->callee);
				PUSH_OBJECT(c);
				machine_throw(lemon);
			}
			break;
		}

		case OPCODE_SUPER: {
			frame = machine_peek_frame(lemon);
			if (frame->self) {
				c = lsuper_create(lemon, frame->self);
				PUSH_OBJECT(c);
			} else {
				const char *fmt;

				fmt = "'%@' not binding to instance";
				c = lobject_error_runtime(lemon,
				                          fmt,
				                          frame->callee);
				PUSH_OBJECT(c);
				machine_throw(lemon);
			}
			break;
		}

		case OPCODE_CLASS: {
			int nattrs;
			int nsupers;
			struct lobject *name;
			struct lobject *clazz;
			struct lobject *attrs[128];
			struct lobject *supers[128];

			CHECK_FETCH(2);
			nsupers = FETCH_CODE1();
			nattrs = FETCH_CODE1();
			if (nsupers > 128) {
				printf("max super is 128\n");
			}

			CHECK_STACK(nsupers + nattrs + 1);
			name = POP_OBJECT();
			for (i = 0; i < nsupers; i++) {
				supers[i] = POP_OBJECT();
			}

			for (i = 0; i < nattrs; i += 2) {
				attrs[i] = POP_OBJECT(); /* name */
				attrs[i + 1] = POP_OBJECT(); /* value */
			}

			clazz = lclass_create(lemon,
			                      name,
			                      nsupers,
			                      supers,
			                      nattrs,
			                      attrs);
			CHECK_NULL(clazz);
			CHECK_ERROR(clazz);
			PUSH_OBJECT(clazz);
			break;
		}

		case OPCODE_MODULE: {
			int address;
			int nlocals;
			struct lobject *callee;
			struct lmodule *module;

			CHECK_FETCH(5);
			nlocals = FETCH_CODE1();
			address = FETCH_CODE4();
			module = (struct lmodule *)POP_OBJECT();
			callee = (struct lobject *)module;
			frame = machine_push_new_frame(lemon,
			                               NULL,
			                               callee,
			                               NULL,
			                               nlocals);
			CHECK_NULL(frame);
			frame->ra = address;
			for (i = 0; i < nlocals; i++) {
				lframe_set_item(lemon, frame, i, lemon->l_nil);
			}
			module->frame = frame;
			module->nlocals = nlocals;
			break;
		}

		default:
			return 0;
		}

		/*
		 * we're not run gc on allocate new object
		 * Pros:
		 *     every time run stack is stable
		 *     not require a lot of barrier thing.
		 * Cons:
		 *     if one opcode create too many objects
		 *     will use a lot of memory.
		 * keep opcode simple and tight
		 *
		 * NOTE: may change in future
		 */
		collector_collect(lemon);
	}

	return 0;
}

void
machine_disassemble(struct lemon *lemon)
{
	int a;
	int b;
	int c;
	int d;
	int e;
	int opcode;
	struct machine *machine;

	machine = lemon->l_machine;
	machine->pc = 0;
	while (machine->pc < machine->maxpc) {
		opcode = machine_fetch_code1(lemon);

		printf("%d: ", machine->pc-1);

		switch (opcode) {
		case OPCODE_HALT:
			printf("halt\n");
			return;

		case OPCODE_NOP:
			printf("nop\n");
			break;

		case OPCODE_ADD:
			printf("add\n");
			break;

		case OPCODE_SUB:
			printf("sub\n");
			break;

		case OPCODE_MUL:
			printf("mul\n");
			break;

		case OPCODE_DIV:
			printf("div\n");
			break;

		case OPCODE_MOD:
			printf("mod\n");
			break;

		case OPCODE_POS:
			printf("pos\n");
			break;

		case OPCODE_NEG:
			printf("neg\n");
			break;

		case OPCODE_SHL:
			printf("shl\n");
			break;

		case OPCODE_SHR:
			printf("shr\n");
			break;

		case OPCODE_GT:
			printf("gt\n");
			break;

		case OPCODE_GE:
			printf("ge\n");
			break;

		case OPCODE_LT:
			printf("lt\n");
			break;

		case OPCODE_LE:
			printf("le\n");
			break;

		case OPCODE_EQ:
			printf("eq\n");
			break;

		case OPCODE_NE:
			printf("ne\n");
			break;

		case OPCODE_IN:
			printf("in\n");
			break;

		case OPCODE_BNOT:
			printf("bnot\n");
			break;

		case OPCODE_BAND:
			printf("band\n");
			break;

		case OPCODE_BXOR:
			printf("bxor\n");
			break;

		case OPCODE_BOR:
			printf("bor\n");
			break;

		case OPCODE_LNOT:
			printf("lnot\n");
			break;

		case OPCODE_POP:
			printf("pop\n");
			break;

		case OPCODE_DUP:
			printf("dup\n");
			break;

		case OPCODE_SWAP:
			printf("swap\n");
			break;

		case OPCODE_LOAD:
			a = machine_fetch_code1(lemon);
			b = machine_fetch_code1(lemon);
			printf("load %d %d\n", a, b);
			break;

		case OPCODE_STORE:
			a = machine_fetch_code1(lemon);
			b = machine_fetch_code1(lemon);
			printf("store %d %d\n", a, b);
			break;

		case OPCODE_CONST:
			a = machine_fetch_code4(lemon);
			printf("const %d ; ", a);
			lobject_print(lemon, machine->cpool[a], NULL);
			break;

		case OPCODE_UNPACK:
			a = machine_fetch_code1(lemon);
			printf("unpack %d\n", a);
			break;

		case OPCODE_ADDITEM:
			printf("additem\n");
			break;

		case OPCODE_GETITEM:
			printf("getitem\n");
			break;

		case OPCODE_SETITEM:
			printf("setitem\n");
			break;

		case OPCODE_DELITEM:
			printf("delitem\n");
			break;

		case OPCODE_GETATTR:
			printf("getattr\n");
			break;

		case OPCODE_SETATTR:
			printf("setattr\n");
			break;

		case OPCODE_DELATTR:
			printf("delattr\n");
			break;

		case OPCODE_GETSLICE:
			printf("getslice\n");
			break;

		case OPCODE_SETSLICE:
			printf("setslice\n");
			break;

		case OPCODE_DELSLICE:
			printf("delslice\n");
			break;

		case OPCODE_JZ:
			a = machine_fetch_code4(lemon);
			printf("jz %d\n", a);
			break;

		case OPCODE_JNZ:
			a = machine_fetch_code4(lemon);
			printf("jnz %d\n", a);
			break;

		case OPCODE_JMP:
			a = machine_fetch_code4(lemon);
			printf("jmp %d\n", a);
			break;

		case OPCODE_ARRAY:
			printf("array %d\n", machine_fetch_code4(lemon));
			break;

		case OPCODE_DICTIONARY:
			printf("dictionary %d\n", machine_fetch_code4(lemon));
			break;

		case OPCODE_DEFINE:
			a = machine_fetch_code1(lemon);
			b = machine_fetch_code1(lemon);
			c = machine_fetch_code1(lemon);
			d = machine_fetch_code1(lemon);
			e = machine_fetch_code4(lemon);
			printf("define %d %d %d %d %d\n", a, b, c, d, e);
			break;

		case OPCODE_KARG:
			printf("karg\n");
			break;

		case OPCODE_VARG:
			printf("varg\n");
			break;

		case OPCODE_VKARG:
			printf("vkarg\n");
			break;

		case OPCODE_CALL:
			a = machine_fetch_code1(lemon);
			printf("call %d\n", a);
			break;

		case OPCODE_TAILCALL:
			a = machine_fetch_code1(lemon);
			printf("tailcall %d\n", a);
			break;

		case OPCODE_RETURN:
			printf("return\n");
			break;

		case OPCODE_SELF:
			printf("self\n");
			break;

		case OPCODE_SUPER:
			printf("super\n");
			break;

		case OPCODE_CLASS:
			printf("class %d %d\n",
			       machine_fetch_code1(lemon),
			       machine_fetch_code1(lemon));
			break;

		case OPCODE_MODULE:
			a = machine_fetch_code1(lemon);
			b = machine_fetch_code4(lemon);
			printf("module %d %d\n", a, b);
			break;

		case OPCODE_SETGETTER:
			printf("setgetter %d\n", machine_fetch_code1(lemon));
			break;

		case OPCODE_SETSETTER:
			printf("setsetter\n");
			break;

		case OPCODE_TRY:
			a = machine_fetch_code4(lemon);
			printf("try %d\n", a);
			break;

		case OPCODE_UNTRY:
			printf("untry\n");
			break;

		case OPCODE_THROW:
			printf("throw\n");
			break;

		case OPCODE_LOADEXC:
			printf("loadexc\n");
			break;

		default:
			printf("error\n");
			return;
		}
	}
}
