#ifndef LEMON_GENERATOR_H
#define LEMON_GENERATOR_H

struct generator_label;

#define MAX_ARG 5

struct generator_arg {
	int type; /* 0: value, 1: label */
	int size; /* size of this value */
	int value;

	int address;
	struct generator_label *label;
	struct generator_arg *prevlabel;
};

struct generator_code {
	int opcode;
	int address;
	int dead;
	struct generator_arg *arg[MAX_ARG];
	struct generator_label *label; /* if opcode == -1 */

	struct generator_code *prev;
	struct generator_code *next;
};

struct generator_label {
	int count; /* reference count */
	int address;
	struct generator_code *code;
	struct generator_arg *prevlabel;
};

struct generator {
	int address;

	struct generator_code *head;
	struct generator_code *tail;
};

struct generator *
generator_create(struct lemon *lemon);

struct generator_code *
generator_make_code(struct lemon *lemon,
                    int opcode,
                    struct generator_arg *arg1,
                    struct generator_arg *arg2,
                    struct generator_arg *arg3,
                    struct generator_arg *arg4,
                    struct generator_arg *arg5);

struct generator_arg *
generator_make_arg(struct lemon *lemon,
                   int type,
                   int size,
                   int value);

struct generator_arg *
generator_make_arg_label(struct lemon *lemon,
                         struct generator_label *label);

struct generator_label *
generator_make_label(struct lemon *lemon);

void
generator_emit_label(struct lemon *lemon,
                     struct generator_label *label);

struct generator_code *
generator_emit_code(struct lemon *lemon,
                    struct generator_code *code);

/* insert `newcode' before `code' */
void
generator_insert_code(struct lemon *lemon,
                      struct generator_code *code,
                      struct generator_code *newcode);

void
generator_delete_code(struct lemon *lemon,
                      struct generator_code *code);

struct generator_code *
generator_patch_code(struct lemon *lemon,
                     struct generator_code *oldcode,
                     struct generator_code *newcode);

void
generator_emit(struct lemon *lemon);

struct generator_code *
generator_emit_opcode(struct lemon *lemon,
                      int opcode);

struct generator_code *
generator_emit_const(struct lemon *lemon,
                     int pool);

struct generator_code *
generator_emit_unpack(struct lemon *lemon,
                      int count);

struct generator_code *
generator_emit_load(struct lemon *lemon,
                    int level,
                    int local);

struct generator_code *
generator_emit_store(struct lemon *lemon,
                     int level,
                     int local);

struct generator_code *
generator_emit_array(struct lemon *lemon,
                     int length);

struct generator_code *
generator_emit_dictionary(struct lemon *lemon,
                          int length);

struct generator_code *
generator_emit_jmp(struct lemon *lemon,
                   struct generator_label *label);

struct generator_code *
generator_emit_jz(struct lemon *lemon,
                  struct generator_label *label);

struct generator_code *
generator_emit_jnz(struct lemon *lemon,
                   struct generator_label *label);

struct generator_code *
generator_emit_call(struct lemon *lemon,
                    int argc);

struct generator_code *
generator_emit_tailcall(struct lemon *lemon,
                        int argc);

struct generator_code *
generator_emit_module(struct lemon *lemon,
                      int nlocals,
                      struct generator_label *label);

struct generator_code *
generator_patch_module(struct lemon *lemon,
                       struct generator_code *code,
                       int nlocals,
                       struct generator_label *label);

struct generator_code *
generator_emit_define(struct lemon *lemon,
                      int define,
                      int nvalues,
                      int nparams,
                      int nlocals,
                      struct generator_label *label);

struct generator_code *
generator_patch_define(struct lemon *lemon,
                       struct generator_code *code,
                       int define,
                       int nvalues,
                       int nparams,
                       int nlocals,
                       struct generator_label *label);

struct generator_code *
generator_emit_try(struct lemon *lemon,
                   struct generator_label *label);

struct generator_code *
generator_emit_class(struct lemon *lemon,
                     int nsupers,
                     int nattrs);

struct generator_code *
generator_emit_setgetter(struct lemon *lemon,
                         int ngetters);

struct generator_code *
generator_emit_setsetter(struct lemon *lemon,
                         int nsetters);

#endif /* LEMON_GENERATOR_H */
