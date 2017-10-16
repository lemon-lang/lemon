#ifndef LEMON_LOBJECT_H
#define LEMON_LOBJECT_H

struct lemon;
struct ltype;

typedef struct lobject *(*lobject_method_t)(struct lemon *,
                                            struct lobject *,
                                            int,
                                            int,
                                            struct lobject *[]);

enum {
	LOBJECT_METHOD_ADD,
	LOBJECT_METHOD_SUB,
	LOBJECT_METHOD_MUL,
	LOBJECT_METHOD_DIV,
	LOBJECT_METHOD_MOD,
	LOBJECT_METHOD_POS,
	LOBJECT_METHOD_NEG,
	LOBJECT_METHOD_SHL,
	LOBJECT_METHOD_SHR,
	LOBJECT_METHOD_LT,
	LOBJECT_METHOD_LE,
	LOBJECT_METHOD_EQ,
	LOBJECT_METHOD_NE,
	LOBJECT_METHOD_GE,
	LOBJECT_METHOD_GT,
	LOBJECT_METHOD_BITWISE_NOT,
	LOBJECT_METHOD_BITWISE_AND,
	LOBJECT_METHOD_BITWISE_XOR,
	LOBJECT_METHOD_BITWISE_OR,

	LOBJECT_METHOD_ADD_ITEM, /* reserved */
	LOBJECT_METHOD_GET_ITEM,
	LOBJECT_METHOD_SET_ITEM,
	LOBJECT_METHOD_DEL_ITEM,
	LOBJECT_METHOD_HAS_ITEM,
	LOBJECT_METHOD_ALL_ITEM, /* [name, name, ...] or [value, value, ...] */
	LOBJECT_METHOD_MAP_ITEM, /* reutrn [[name, value], [name, value]] */

	LOBJECT_METHOD_ADD_ATTR, /* reserved */
	LOBJECT_METHOD_GET_ATTR,
	LOBJECT_METHOD_SET_ATTR,
	LOBJECT_METHOD_DEL_ATTR,
	LOBJECT_METHOD_HAS_ATTR,
	LOBJECT_METHOD_ALL_ATTR, /* return [name, name, name] */
	LOBJECT_METHOD_MAP_ATTR, /* reutrn {name: value, name: value} */

	LOBJECT_METHOD_ADD_SLICE, /* reserved */
	LOBJECT_METHOD_GET_SLICE,
	LOBJECT_METHOD_SET_SLICE,
	LOBJECT_METHOD_DEL_SLICE,

	LOBJECT_METHOD_ADD_GETTER, /* reserved */
	LOBJECT_METHOD_GET_GETTER,
	LOBJECT_METHOD_SET_GETTER,
	LOBJECT_METHOD_DEL_GETTER,

	LOBJECT_METHOD_ADD_SETTER, /* reserved */
	LOBJECT_METHOD_GET_SETTER,
	LOBJECT_METHOD_SET_SETTER,
	LOBJECT_METHOD_DEL_SETTER,

	/*
	 * different with '__call__' function attr:
	 * LOBJECT_METHOD_CALL don't push new frame
	 * when '__call__' function call will push new frame
	 */
	LOBJECT_METHOD_CALL,

	/*
	 * we don't know C function is implemented or not LOBJECT_METHOD_CALL,
	 * so need new method LOBJECT_METHOD_CALLABLE.
	 * '__callable__' attr is not required when use '__call__' attr
	 */
	LOBJECT_METHOD_CALLABLE,

	LOBJECT_METHOD_SUPER,
	LOBJECT_METHOD_SUBCLASS,
	LOBJECT_METHOD_INSTANCE,

	LOBJECT_METHOD_LENGTH,  /* return linteger */
	LOBJECT_METHOD_NUMBER,  /* return lnumber  */
	LOBJECT_METHOD_STRING,  /* return lstring  */
	LOBJECT_METHOD_FORMAT,  /* return lstring  */
	LOBJECT_METHOD_INTEGER, /* return linteger */
	LOBJECT_METHOD_BOOLEAN, /* return l_true or l_false */

	LOBJECT_METHOD_HASH, /* return linteger */
	LOBJECT_METHOD_MARK, /* gc scan mark */

	LOBJECT_METHOD_DESTROY
};

/*
 * `l_method' also use for identify object's type
 * `l_collector` also encoded current object gc mark(see collector.c)
 */
struct lobject {
	lobject_method_t l_method;
	struct lobject *l_next;
};

/*
 * lobject naming rule:
 *
 *     lobject_[name]
 *
 * [name] is lowercase method name
 * return value and parameter always `struct lobject *' (except `argc')
 *
 *     lobject_is_[name]
 *
 * [name] is type or method
 * return value 1 success or 0 failure
 *
 *     lobject_error_[name]
 *
 * [name] is kind of error
 * return value is throwed lexception instance
 *
 *     l[type]_[name]
 *
 * [type] is specific type of lobject
 * [name] is lowercase method name
 * return value always `struct lobject *' (except `argc' or size)
 * parameter use lobject and C type
 *
 */

/*
 * if `type' is not NULL, `method' will be discard,
 * object will use type->method instead (for dynamic library type stability).
 */
void *
lobject_create(struct lemon *lemon, size_t size, lobject_method_t method);

int
lobject_destroy(struct lemon *lemon, struct lobject *object);

/*
 * copy object if size > sizeof(struct lobject)
 * memcpy object may cause gc dead loop
 */
void
lobject_copy(struct lemon *lemon,
             struct lobject *newobject,
             struct lobject *oldobject,
             size_t size);

struct lobject *
lobject_eq(struct lemon *lemon, struct lobject *a, struct lobject *b);

struct lobject *
lobject_unop(struct lemon *lemon, int method, struct lobject *a);

struct lobject *
lobject_binop(struct lemon *lemon,
              int method, struct lobject *a, struct lobject *b);

struct lobject *
lobject_mark(struct lemon *lemon, struct lobject *self);

struct lobject *
lobject_string(struct lemon *lemon, struct lobject *self);

struct lobject *
lobject_length(struct lemon *lemon, struct lobject *self);

struct lobject *
lobject_integer(struct lemon *lemon, struct lobject *self);

struct lobject *
lobject_boolean(struct lemon *lemon, struct lobject *self);

struct lobject *
lobject_call(struct lemon *lemon,
             struct lobject *self,
             int argc, struct lobject *argv[]);

struct lobject *
lobject_all_item(struct lemon *lemon, struct lobject *self);

struct lobject *
lobject_map_item(struct lemon *lemon, struct lobject *self);

struct lobject *
lobject_get_item(struct lemon *lemon,
                 struct lobject *self,
                 struct lobject *name);

struct lobject *
lobject_has_item(struct lemon *lemon,
                 struct lobject *self,
                 struct lobject *name);

struct lobject *
lobject_set_item(struct lemon *lemon,
                 struct lobject *self,
                 struct lobject *name,
                 struct lobject *value);

struct lobject *
lobject_del_item(struct lemon *lemon,
                 struct lobject *self,
                 struct lobject *name);

struct lobject *
lobject_add_attr(struct lemon *lemon,
                 struct lobject *self,
                 struct lobject *name,
                 struct lobject *value);

struct lobject *
lobject_get_attr(struct lemon *lemon,
                 struct lobject *self,
                 struct lobject *name);

struct lobject *
lobject_set_attr(struct lemon *lemon,
                 struct lobject *self,
                 struct lobject *name,
                 struct lobject *value);

struct lobject *
lobject_del_attr(struct lemon *lemon,
                 struct lobject *self,
                 struct lobject *name);

struct lobject *
lobject_has_attr(struct lemon *lemon,
                 struct lobject *self,
                 struct lobject *name);

struct lobject *
lobject_get_slice(struct lemon *lemon,
                  struct lobject *self,
                  struct lobject *start,
                  struct lobject *stop,
                  struct lobject *step);

struct lobject *
lobject_set_slice(struct lemon *lemon,
                  struct lobject *self,
                  struct lobject *start,
                  struct lobject *stop,
                  struct lobject *step,
                  struct lobject *value);

struct lobject *
lobject_del_slice(struct lemon *lemon,
                  struct lobject *self,
                  struct lobject *start,
                  struct lobject *stop,
                  struct lobject *step);

struct lobject *
lobject_get_getter(struct lemon *lemon,
                   struct lobject *self,
                   struct lobject *name);

struct lobject *
lobject_get_setter(struct lemon *lemon,
                   struct lobject *self,
                   struct lobject *name);

struct lobject *
lobject_call_attr(struct lemon *lemon,
                  struct lobject *self,
                  struct lobject *name,
                  int argc, struct lobject *argv[]);

int
lobject_is_integer(struct lemon *lemon, struct lobject *object);

int
lobject_is_pointer(struct lemon *lemon, struct lobject *object);

int
lobject_is_type(struct lemon *lemon, struct lobject *object);

int
lobject_is_karg(struct lemon *lemon, struct lobject *object);

int
lobject_is_varg(struct lemon *lemon, struct lobject *object);

int
lobject_is_vkarg(struct lemon *lemon, struct lobject *object);

int
lobject_is_class(struct lemon *lemon, struct lobject *object);

int
lobject_is_array(struct lemon *lemon, struct lobject *object);

int
lobject_is_number(struct lemon *lemon, struct lobject *object);

int
lobject_is_double(struct lemon *lemon, struct lobject *object);

int
lobject_is_string(struct lemon *lemon, struct lobject *object);

int
lobject_is_iterator(struct lemon *lemon, struct lobject *object);

int
lobject_is_instance(struct lemon *lemon, struct lobject *object);

int
lobject_is_function(struct lemon *lemon, struct lobject *object);

int
lobject_is_coroutine(struct lemon *lemon, struct lobject *object);

int
lobject_is_exception(struct lemon *lemon, struct lobject *object);

int
lobject_is_dictionary(struct lemon *lemon, struct lobject *object);

int
lobject_is_error(struct lemon *lemon, struct lobject *object);

int
lobject_is_equal(struct lemon *lemon, struct lobject *a, struct lobject *b);

struct lobject *
lobject_error(struct lemon *lemon, struct lobject *self, const char *fmt, ...);

struct lobject *
lobject_error_type(struct lemon *lemon, const char *fmt, ...);

struct lobject *
lobject_error_item(struct lemon *lemon, const char *fmt, ...);

struct lobject *
lobject_error_memory(struct lemon *lemon, const char *fmt, ...);

struct lobject *
lobject_error_runtime(struct lemon *lemon, const char *fmt, ...);

struct lobject *
lobject_error_argument(struct lemon *lemon, const char *fmt, ...);

struct lobject *
lobject_error_attribute(struct lemon *lemon, const char *fmt, ...);

struct lobject *
lobject_error_arithmetic(struct lemon *lemon, const char *fmt, ...);

void *
lobject_error_not_callable(struct lemon *lemon, struct lobject *object);

void *
lobject_error_not_iterable(struct lemon *lemon, struct lobject *object);

void *
lobject_error_not_implemented(struct lemon *lemon);

struct lobject *
lobject_throw(struct lemon *lemon, struct lobject *error);

void
lobject_print(struct lemon *lemon, ...);

struct lobject *
lobject_method_call(struct lemon *lemon,
                    struct lobject *self,
                    int method, int argc, struct lobject *argv[]);

struct lobject *
lobject_default(struct lemon *lemon,
                struct lobject *self,
                int method, int argc, struct lobject *argv[]);

/*
 * get_attr with default value
 * `lobject_string' -> `__string__()'
 * `lobject_length' -> `__length__()'
 *  ...
 */
struct lobject *
lobject_default_get_attr(struct lemon *lemon,
                         struct lobject *self,
                         struct lobject *name);

#endif /* LEMON_LOBJECT_H */
