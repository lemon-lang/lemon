#include "lemon.h"
#include "shell.h"
#include "larray.h"
#include "lstring.h"
#include "lib/builtin.h"

#ifdef MODULE_OS
#include "lib/os.h"
#endif

#ifdef MODULE_SOCKET
#include "lib/socket.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int
main(int argc, char *argv[])
{
	int i;
	struct lemon *lemon;
	struct lobject *objects;

	lemon = lemon_create();
	if (!lemon) {
		printf("create lemon fail\n");
		return 0;
	}
	builtin_init(lemon);

	/* internal module */
#ifdef MODULE_OS
	lobject_set_item(lemon,
	                 lemon->l_modules,
	                 lstring_create(lemon, "os", 2),
	                 os_module(lemon));
#endif

#ifdef MODULE_SOCKET
	lobject_set_item(lemon,
	                 lemon->l_modules,
	                 lstring_create(lemon, "socket", 6),
	                 socket_module(lemon));
#endif

	if (argc < 2) {
		shell(lemon);
	} else {
		if (!lemon_input_set_file(lemon, argv[1])) {
			fprintf(stderr, "open '%s' file fail\n", argv[1]);
			lemon_destroy(lemon);
			exit(1);
		}

		objects = larray_create(lemon, 0, NULL);
		for (i = 1; i < argc; i++) {
			struct lobject *value;

			value = lstring_create(lemon, argv[i], strlen(argv[i]));
			larray_append(lemon, objects, 1, &value);
		}
		lemon_add_global(lemon, "argv", objects);

		if (!lemon_compile(lemon)) {
			fprintf(stderr, "lemon: syntax error\n");
			lemon_destroy(lemon);
			exit(1);
		}

		lemon_machine_reset(lemon);
		lemon_machine_execute(lemon);
	}

	lemon_destroy(lemon);

	return 0;
}
