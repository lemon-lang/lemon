#include "lemon.h"
#include "collector.h"
#include "machine.h"

#include <limits.h>
#include <assert.h>
#include <string.h>

#define GC_STEP_RATIO 200
#define GC_STEP_THRESHOLD 2048

#define GC_FULL_RATIO 200
#define GC_FULL_THRESHOLD 65535

#define GC_MARK_MASK (uintptr_t)0x1UL
#define GC_SET_GRAY_MASK (uintptr_t)0x2UL
#define GC_ALL_MASK (GC_MARK_MASK | GC_SET_GRAY_MASK)

#define GC_SET_MASK(p,m) ((p) = (void *)((uintptr_t)(p) | (m)))
#define GC_CLR_MASK(p,m) ((p) = (void *)((uintptr_t)(p) & ~(m)))

#define GC_SET_MARK(a) (GC_SET_MASK((a)->l_next, GC_MARK_MASK))
#define GC_CLR_MARK(a) (GC_CLR_MASK((a)->l_next, GC_MARK_MASK))
#define GC_HAS_MARK(a) ((uintptr_t)(a)->l_next & GC_MARK_MASK)

#define GC_SET_GRAY(a) (GC_SET_MASK((a)->l_next, GC_SET_GRAY_MASK))
#define GC_CLR_GRAY(a) (GC_CLR_MASK((a)->l_next, GC_SET_GRAY_MASK))
#define GC_HAS_GRAY(a) ((uintptr_t)(a)->l_next & GC_SET_GRAY_MASK)

#define GC_GET_NEXT(a) ((void *)((uintptr_t)(a)->l_next & ~GC_ALL_MASK))

/*
 * Classic Tricolor Mark & Sweep GC Algorithm
 *
 * an object can have three state in GC_SWEEP_PHASE
 *
 * 	UNMARKED and UNGRAYED destroyable
 * 	UNMARKED and GRAYED   undestroyable
 * 	MARKED   and UNGRAYED undestroyable
 *
 * GRAY mask design to avoid destroy object in sweep,
 * because we can't mark and unmark an object while collector is sweeping.
 * so GRAYED object is still alive but don't need barrier any more.
 */

enum {
	GC_SCAN_PHASE,
	GC_MARK_PHASE,
	GC_SWEEP_PHASE
};

void *
collector_create(struct lemon *lemon)
{
	struct collector *collector;

	collector = lemon_allocator_alloc(lemon, sizeof(*collector));
	if (collector) {
		memset(collector, 0, sizeof(*collector));
		collector->phase = GC_SCAN_PHASE;
		collector->enabled = 1;

		collector->stacklen = 0;
		collector->stacktop = -1;

		collector->step_ratio = GC_STEP_RATIO;
		collector->step_threshold = GC_STEP_THRESHOLD;

		collector->full_ratio = GC_FULL_RATIO;
		collector->full_threshold = GC_FULL_THRESHOLD;
	}

	return collector;
}

void
collector_destroy(struct lemon *lemon, struct collector *collector)
{
	struct lobject *curr;
	struct lobject *next;

	curr = collector->object_list;
	while (curr) {
		next = GC_GET_NEXT(curr);
		lobject_destroy(lemon, curr);
		curr = next;
	}

	lemon_allocator_free(lemon, collector->stack);
	lemon_allocator_free(lemon, collector);
}

int
collector_stack_is_empty(struct lemon *lemon)
{
	struct collector *collector;

	collector = lemon->l_collector;

	return collector->stacktop < 0;
}

int
collector_stack_is_full(struct lemon *lemon)
{
	struct collector *collector;

	collector = lemon->l_collector;

	return (collector->stacktop + 1) >= collector->stacklen;
}

void
collector_stack_push(struct lemon *lemon, struct lobject *object)
{
	struct collector *collector;

	collector = lemon->l_collector;
	if (collector_stack_is_full(lemon)) {
		int len;

		len = collector->stacklen ? collector->stacklen * 2 : 2;
		collector->stack = lemon_allocator_realloc(lemon,
		                              collector->stack,
		                              sizeof(struct lobject *) * len);
		if (!collector->stack) {
			return;
		}
		collector->stacklen = len;
	}

	collector->stack[++collector->stacktop] = object;
}

struct lobject *
collector_stack_pop(struct lemon *lemon)
{
	struct collector *collector;

	collector = lemon->l_collector;
	if (!collector_stack_is_empty(lemon)) {
		return collector->stack[collector->stacktop--];
	}

	return NULL;
}

void
lemon_collector_enable(struct lemon *lemon)
{
	struct collector *collector;

	collector = lemon->l_collector;
	collector->enabled = 1;
}

void
lemon_collector_disable(struct lemon *lemon)
{
	struct collector *collector;

	collector = lemon->l_collector;
	collector->enabled = 0;
}

int
lemon_collector_enabled(struct lemon *lemon)
{
	struct collector *collector;

	collector = lemon->l_collector;
	return collector->enabled;
}

void
lemon_collector_mark(struct lemon *lemon, struct lobject *object)
{
	if (lobject_is_pointer(lemon, object)) {
		if (!GC_HAS_MARK(object)) {
			GC_SET_MARK(object);
			GC_CLR_GRAY(object);
			collector_stack_push(lemon, object);
		}
	}
}

void
collector_mark_children(struct lemon *lemon, struct lobject *object)
{
	GC_SET_MARK(object);
	GC_CLR_GRAY(object);
	lobject_method_call(lemon, object, LOBJECT_METHOD_MARK, 0, NULL);
}

void
lemon_collector_trace(struct lemon *lemon, struct lobject *object)
{
	struct collector *collector;

	collector = lemon->l_collector;
	object->l_next = collector->object_list;
	collector->object_list = object;
	collector->live++;
	if (collector->sweeping && object->l_next == collector->sweeping) {
		collector->sweeping_prev = object;
	}
}

void
lemon_collector_untrace(struct lemon *lemon, struct lobject *object)
{
	uintptr_t marked;
	struct collector *collector;
	struct lobject *curr;

	collector = lemon->l_collector;
	curr = collector->object_list;

	if (curr == object) {
		collector->object_list = GC_GET_NEXT(curr);
	}

	while (curr && GC_GET_NEXT(curr) != object) {
		curr = GC_GET_NEXT(curr);
	}

	if (curr) {
		marked = GC_HAS_MARK(curr);
		curr->l_next = GC_GET_NEXT(object);
		if (marked) {
			GC_SET_MARK(curr);
		}
	}
}

/*
 * forward barrier
 */
void
lemon_collector_barrier(struct lemon *lemon,
                        struct lobject *a,
                        struct lobject *b)
{
	struct collector *collector;

	collector = lemon->l_collector;
	if (lobject_is_pointer(lemon, b) && GC_HAS_MARK(a) && !GC_HAS_MARK(b)) {
		/*
		 * unmark object avoid repeat barrier
		 * gray object avoid sweep
		 */
		if (collector->phase == GC_SWEEP_PHASE) {
			GC_SET_GRAY(a);
			GC_CLR_MARK(a);
		}
		collector_stack_push(lemon, b);
	}
}

/*
 * back barrier
 */
void
lemon_collector_barrierback(struct lemon *lemon,
                            struct lobject *a,
                            struct lobject *b)
{
	struct collector *collector;

	collector = lemon->l_collector;
	if (lobject_is_pointer(lemon, b) && GC_HAS_MARK(a) && !GC_HAS_MARK(b)) {
		/*
		 * gray object avoid sweep
		 */
		if (collector->phase == GC_SWEEP_PHASE) {
			GC_SET_GRAY(a);
		}
		GC_CLR_MARK(a);
		collector_stack_push(lemon, a);
	}
}

void
collector_scan_phase(struct lemon *lemon)
{
	int i;
	struct collector *collector;
	struct machine *machine;

	lemon_mark_types(lemon);
	lemon_mark_errors(lemon);
	lemon_mark_strings(lemon);

	machine = lemon->l_machine;
	for (i = 0; i < machine->cpoollen; i++) {
		if (!machine->cpool[i]) {
			break;
		}
		lemon_collector_mark(lemon, machine->cpool[i]);
	}
	for (i = 0; i <= machine->sp; i++) {
		lemon_collector_mark(lemon, machine->stack[i]);
	}

	for (i = 0; i <= machine->fp; i++) {
		lemon_collector_mark(lemon,
		                     (struct lobject *)machine->frame[i]);
	}

	collector = lemon->l_collector;
	collector->phase = GC_MARK_PHASE;
}

void
collector_mark_phase(struct lemon *lemon, long mark_max)
{
	long i;

	struct collector *collector;
	struct lobject *object;

	collector = lemon->l_collector;
	for (i = 0; i < mark_max && !collector_stack_is_empty(lemon); i++) {
		object = collector_stack_pop(lemon);
		collector_mark_children(lemon, object);
	}

	if (collector_stack_is_empty(lemon)) {
		collector_scan_phase(lemon);

		while (!collector_stack_is_empty(lemon)) {
			object = collector_stack_pop(lemon);
			collector_mark_children(lemon, object);
		}

		collector->phase = GC_SWEEP_PHASE;
		collector->sweeping = collector->object_list;
	}
}

void
collector_sweep_phase(struct lemon *lemon, long swept_max)
{
	long swept_count;
	struct collector *collector;
	struct lobject *curr;
	struct lobject *prev;
	struct lobject *next;

	collector = lemon->l_collector;
	prev = collector->sweeping_prev;
	curr = collector->sweeping;
	swept_count = 0;
	while (swept_count < swept_max && curr) {
		if (GC_HAS_MARK(curr) || GC_HAS_GRAY(curr)) {
			GC_CLR_MARK(curr);
			GC_CLR_GRAY(curr);
			prev = curr;
			curr = GC_GET_NEXT(curr);
		} else {
			next = NULL;
			if (prev) {
				prev->l_next = GC_GET_NEXT(curr);
				next = GC_GET_NEXT(curr);
			} else if (curr == collector->sweeping) {
				next = GC_GET_NEXT(curr);
				if (curr == collector->object_list) {
					collector->object_list = next;
				}
				collector->sweeping = next;
			}
			assert(next);

			lobject_destroy(lemon, curr);
			curr = next;
			collector->live -= 1;
			swept_count += 1;
		}
		collector->sweeping_prev = prev;
		collector->sweeping = curr;
	}

	if (!curr) {
		collector->sweeping_prev = NULL;
		collector->sweeping = NULL;
		collector->phase = GC_SCAN_PHASE;
	}
}

void
collector_step(struct lemon *lemon, long step_max)
{
	struct collector *collector;

	collector = lemon->l_collector;
	if (collector->phase == GC_SCAN_PHASE) {
		collector_scan_phase(lemon);
	} else if (collector->phase == GC_MARK_PHASE) {
		collector_mark_phase(lemon, step_max);
	} else {
		collector_sweep_phase(lemon, step_max);
	}
	collector->step_threshold = collector->live + GC_STEP_THRESHOLD;
}

void
collector_full(struct lemon *lemon)
{
	long max;
	struct collector *collector;

	collector = lemon->l_collector;
	if (collector->phase != GC_SCAN_PHASE) {
		do {
			collector_step(lemon, LONG_MAX);
		} while (collector->phase != GC_SCAN_PHASE);
	}

	do {
		collector_step(lemon, LONG_MAX);
	} while (collector->phase != GC_SCAN_PHASE);

	max = collector->live/100 * collector->full_ratio;
	if (max < GC_FULL_THRESHOLD) {
		max = GC_FULL_THRESHOLD;
	}

	collector->full_threshold = max;
}

void
collector_collect(struct lemon *lemon)
{
	long max;
	struct collector *collector;

	collector = lemon->l_collector;
	if (collector->enabled) {
		if (collector->live >= collector->step_threshold) {
			max = GC_STEP_THRESHOLD/100 * collector->step_ratio;
			collector_step(lemon, max);
		}

		if (collector->live >= collector->full_threshold) {
			collector_full(lemon);
		}
	}
}
