CC=cc
CFLAGS = -std=c89 -pedantic -Wall -Wextra -Wno-unused-parameter -fPIC -I. -I./src
LDFLAGS = -lm -ldl -lreadline

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
	AR=gcc-ar
	CFLAGS += -DLINUX
	LDFLAGS += -Wl,-rpath=./
endif
ifeq ($(UNAME_S),Darwin)
	AR=ar
	CFLAGS += -DDARWIN
endif

ifeq ($(UNAME_S),Linux)
	CFLAGS += -D_XOPEN_SOURCE=700 -D_GNU_SOURCE
endif

DEBUG ?= 0
ifeq ($(DEBUG),0)
	CFLAGS += -O3 -flto -DNDEBUG
else ifeq ($(UNAME_S),Darwin)
	CFLAGS += -g -DDEBUG
	CFLAGS += -fsanitize=address
	CFLAGS += -fsanitize=integer
	CFLAGS += -fsanitize=undefined
	LDFLAGS += -fsanitize=address
else
	CFLAGS += -g -O1 -DDEBUG -fsanitize=address
endif

ifeq ($USE_MALLOC),1)
	CFLAGS += -DUSE_MALLOC
endif

SRCS  = src/lemon.c
SRCS += src/hash.c
SRCS += src/shell.c
SRCS += src/mpool.c
SRCS += src/arena.c
SRCS += src/table.c
SRCS += src/token.c
SRCS += src/input.c
SRCS += src/lexer.c
SRCS += src/scope.c
SRCS += src/syntax.c
SRCS += src/parser.c
SRCS += src/symbol.c
SRCS += src/extend.c
SRCS += src/compiler.c
SRCS += src/peephole.c
SRCS += src/generator.c
SRCS += src/allocator.c
SRCS += src/collector.c
SRCS += src/machine.c
SRCS += src/lnil.c
SRCS += src/ltype.c
SRCS += src/lkarg.c
SRCS += src/lvarg.c
SRCS += src/ltable.c
SRCS += src/lvkarg.c
SRCS += src/larray.c
SRCS += src/lframe.c
SRCS += src/lclass.c
SRCS += src/lsuper.c
SRCS += src/lobject.c
SRCS += src/lmodule.c
SRCS += src/lnumber.c
SRCS += src/lstring.c
SRCS += src/linteger.c
SRCS += src/lboolean.c
SRCS += src/linstance.c
SRCS += src/literator.c
SRCS += src/lfunction.c
SRCS += src/lsentinel.c
SRCS += src/laccessor.c
SRCS += src/lexception.c
SRCS += src/lcoroutine.c
SRCS += src/ldictionary.c
SRCS += src/lcontinuation.c
SRCS += lib/builtin.c

MODULE_OS ?= 1
ifeq ($(MODULE_OS), 1)
SRCS += lib/os.c
CFLAGS += -DMODULE_OS
endif

MODULE_SOCKET ?= 1
ifeq ($(MODULE_SOCKET), 1)
SRCS += lib/socket.c
CFLAGS += -DMODULE_SOCKET
endif

OBJS = $(addprefix obj/,$(notdir $(SRCS:.c=.o)))

STATIC ?= 0
ifeq ($(STATIC),0)
LIB = liblemon.so
LIBFLAGS = -L. -llemon
else
LIB = liblemon.a
CFLAGS += -DSTATICLINKED
endif

VPATH  = ./src:./lib

INCS := $(wildcard src/*.h)

TESTS = $(wildcard test/test_*.lm)

.PHONY: mkdir test

all: mkdir lemon

mkdir: obj

obj:
	@mkdir -p obj

lemon: obj/main.o $(LIB) Makefile
	$(CC) $(CFLAGS) obj/main.o $(LIB) $(LIBFLAGS) $(LDFLAGS) -o $@
	@echo CC lemon

liblemon.a: $(OBJS) $(INCS) Makefile
	@$(AR) rcu $@ $(OBJS)
	@echo CC liblemon.a

liblemon.so: $(OBJS) $(INCS) Makefile
	$(CC) $(CFLAGS) $(LDFLAGS) -shared $(OBJS) -o $@
	@echo CC liblemon.so

obj/%.o: %.c $(INCS) Makefile
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo CC $<

test: $(TESTS) lemon Makefile
	@for test in $(TESTS); do \
		./lemon $$test >> /dev/null && echo "$$test [ok]" || \
		{ echo "$$test [fail]" && exit 1; } \
	done

clean:
	@rm -f lemon $(OBJS) liblemon.a liblemon.so obj/main.o
	@rmdir obj
	@echo clean lemon $(OBJS)
