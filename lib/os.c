#include "lemon.h"
#include "larray.h"
#include "lmodule.h"
#include "lstring.h"
#include "linteger.h"

#include <time.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>

#ifdef WINDOWS
#include <windows.h>
#include <winsock2.h>
#else
#include <sys/select.h>
#endif

static struct lobject *
os_open(struct lemon *lemon, struct lobject *self, int argc, struct lobject *argv[])
{
	int fd;
	int oflag;
	int mode;

#if WINDOWS
	mode = S_IREAD | S_IWRITE;
#else
	mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
#endif
	if (argc == 3) {
		mode = linteger_to_long(lemon, argv[2]);
	}

	if (argc >= 2) {
		oflag = linteger_to_long(lemon, argv[1]);
	} else {
		oflag = O_RDONLY;
	}

	if (oflag & O_CREAT) {
		fd = open(lstring_to_cstr(lemon, argv[0]), oflag, mode);
	} else {
		fd = open(lstring_to_cstr(lemon, argv[0]), oflag);
	}
	if (fd == -1) {
		return lobject_error_type(lemon, "open '%@' fail: %s", argv[0], strerror(errno));
	}

	return linteger_create_from_long(lemon, fd);
}

static struct lobject *
os_read(struct lemon *lemon, struct lobject *self, int argc, struct lobject *argv[])
{
	int fd;
	long pos;
	long end;
	size_t size;
	struct stat stat;
	struct lobject *string;

	fd = linteger_to_long(lemon, argv[0]);
	if (fd < 0) {
		return lobject_error_type(lemon, "can't read '%@'", argv[0]);
	}

	pos = lseek(fd, 0, SEEK_CUR);
	if (pos < 0) {
		return lobject_error_type(lemon, "can't read '%@'", argv[0]);
	}

	if (fstat(fd, &stat) < 0) {
		return lobject_error_type(lemon, "can't read '%@'", argv[0]);
	}

	end = stat.st_size;
	string = lstring_create(lemon, NULL, end - pos);
	size = read(fd, lstring_buffer(lemon, string), end - pos);
	if (size < (size_t)(end - pos)) {
		return lstring_create(lemon,
		                      lstring_buffer(lemon, string),
		                      size);
	}

	return string;
}

static struct lobject *
os_write(struct lemon *lemon, struct lobject *self, int argc, struct lobject *argv[])
{
	int fd;
	size_t size;

	fd = linteger_to_long(lemon, argv[0]);
	size = write(fd,
	             lstring_to_cstr(lemon, argv[1]),
	             lstring_length(lemon, argv[1]));

	if (size) {
		return lemon->l_true;
	}
	return lemon->l_false;
}

static struct lobject *
os_close(struct lemon *lemon, struct lobject *self, int argc, struct lobject *argv[])
{
	int fd;

	fd = linteger_to_long(lemon, argv[0]);
	close(fd);

	return lemon->l_nil;
}

#ifndef WINDOWS
static struct lobject *
os_fcntl(struct lemon *lemon, struct lobject *self, int argc, struct lobject *argv[])
{
	int fd;
	int cmd;
	int flags;
	int value;

	if (argc < 2 || argc > 3) {
		return lobject_error_argument(lemon,
		                              "fcntl() require fd, cmd and optional value");
	}

	value = 0;
	if (argc == 3 && lobject_is_integer(lemon, argv[2])) {
		value = linteger_to_long(lemon, argv[2]);
	}

	fd = linteger_to_long(lemon, argv[0]);
	cmd = linteger_to_long(lemon, argv[1]);
	flags = fcntl(fd, cmd, value);
	if (flags == -1) {
		perror("fcntl");
		return linteger_create_from_long(lemon, -1);
	}

	return linteger_create_from_long(lemon, flags);
}
#endif

static struct lobject *
os_select(struct lemon *lemon, struct lobject *self, int argc, struct lobject *argv[])
{
	int i;
	int n;
	int fd;
	int count;
	int maxfd;

	fd_set rfds;
	fd_set wfds;
	fd_set xfds;
	struct timeval tv;
	struct timeval *timeout;

	int rlen;
	int wlen;
	int xlen;
	struct lobject *rlist[256];
	struct lobject *wlist[256];
	struct lobject *xlist[256];

	struct lobject *item;
	struct lobject *items[3];

	if (argc != 3 && argc != 4) {
		return lobject_error_argument(lemon, "select() required 3 arrays");
	}

	if (!lobject_is_array(lemon, argv[0]) ||
	    !lobject_is_array(lemon, argv[1]) ||
	    !lobject_is_array(lemon, argv[2]))
	{
		return lobject_error_argument(lemon, "select() required 3 arrays");
	}

	if (argc == 4 && !lobject_is_integer(lemon, argv[3])) {
		return lobject_error_argument(lemon, "select() 4th not integer");
	}

	FD_ZERO(&rfds);
	FD_ZERO(&wfds);
	FD_ZERO(&xfds);

	maxfd = 0;
	count = larray_length(lemon, argv[0]);
	for (i = 0; i < count; i++) {
		item = larray_get_item(lemon, argv[0], i);
		fd = linteger_to_long(lemon, item);
		if (maxfd < fd) {
			maxfd = fd;
		}
		FD_SET(fd, &rfds);
	}

	count = larray_length(lemon, argv[1]);
	for (i = 0; i < count; i++) {
		item = larray_get_item(lemon, argv[1], i);
		fd = linteger_to_long(lemon, item);
		if (maxfd < fd) {
			maxfd = fd;
		}
		FD_SET(fd, &wfds);
	}

	count = larray_length(lemon, argv[2]);
	for (i = 0; i < count; i++) {
		item = larray_get_item(lemon, argv[2], i);
		fd = linteger_to_long(lemon, item);
		if (maxfd < fd) {
			maxfd = fd;
		}
		FD_SET(fd, &xfds);
	}

	timeout = NULL;
	if (argc == 4) {
		long us = linteger_to_long(lemon, argv[3]);

		tv.tv_sec = us / 1000000;
		tv.tv_usec = us % 1000000;
		timeout = &tv;
	}

	n = select(maxfd + 1, &rfds, &wfds, &xfds, timeout);
	if (n == 0) {
		/* timeout */
		items[0] = larray_create(lemon, 0, NULL);
		items[1] = larray_create(lemon, 0, NULL);
		items[2] = larray_create(lemon, 0, NULL);

		return larray_create(lemon, 3, items);
	}

	if (n < 0 && errno != EINTR) {
		/* error */
		return lobject_error_type(lemon, "select() %s", strerror(errno));
	}

	rlen = 0;
	wlen = 0;
	xlen = 0;
	items[0] = larray_create(lemon, rlen, rlist);
	items[1] = larray_create(lemon, wlen, wlist);
	items[2] = larray_create(lemon, xlen, xlist);
	if (n > 0) {
		for (i = 0; i < maxfd + 1; i++) {
			if (FD_ISSET(i, &rfds)) {
				item = linteger_create_from_long(lemon, i);
				rlist[rlen++] = item;
				if (rlen == 256) {
					larray_append(lemon, items[0], rlen, rlist);
					rlen = 0;
				}
			}

			if (FD_ISSET(i, &wfds)) {
				item = linteger_create_from_long(lemon, i);
				wlist[wlen++] = item;
				if (wlen == 256) {
					larray_append(lemon, items[1], wlen, wlist);
					wlen = 0;
				}
			}

			if (FD_ISSET(i, &xfds)) {
				item = linteger_create_from_long(lemon, i);
				xlist[xlen++] = item;
				if (xlen == 256) {
					larray_append(lemon, items[2], xlen, xlist);
					xlen = 0;
				}
			}
		}
	}
	larray_append(lemon, items[0], rlen, rlist);
	larray_append(lemon, items[1], wlen, wlist);
	larray_append(lemon, items[2], xlen, xlist);

	return larray_create(lemon, 3, items);
}

static struct lobject *
os_realpath(struct lemon *lemon, struct lobject *self, int argc, struct lobject *argv[])
{
	const char *path;
	char buffer[PATH_MAX];

#ifdef WINDOWS
	path = lstring_to_cstr(lemon, argv[0]);
	if (GetFullPathName(path, PATH_MAX, buffer, NULL)) {
		return lstring_create(lemon, buffer, strlen(buffer));
	}
#else
	path = realpath(lstring_to_cstr(lemon, argv[0]), buffer);
	if (path) {
		return lstring_create(lemon, path, strlen(path));
	}
#endif

	return lemon->l_nil;
}

static struct lobject *
os_time(struct lemon *lemon, struct lobject *self, int argc, struct lobject *argv[])
{
	time_t t;

	time(&t);

	return linteger_create_from_long(lemon, t);
}

static struct lobject *
os_ctime(struct lemon *lemon, struct lobject *self, int argc, struct lobject *argv[])
{
	time_t t;
	char *ptr;
	char buf[26];

	if (argc != 1 || lobject_is_integer(lemon, argv[0])) {
		return lobject_error_argument(lemon, "required 1 integer argument");
	}
	t = linteger_to_long(lemon, argv[0]);

#ifdef WINDOWS
	(void)buf;
	ptr = ctime(&t);
#else
	ptr = ctime_r(&t, buf);
#endif

	return lstring_create(lemon, ptr, strlen(ptr));
}

struct ltmobject {
	struct lobject object;

	struct tm tm;
};

static struct lobject *
ltmobject_method(struct lemon *lemon,
                 struct lobject *self,
                 int method, int argc, struct lobject *argv[])
{
	return lobject_default(lemon, self, method, argc, argv);
}

static struct lobject *
os_gmtime(struct lemon *lemon, struct lobject *self, int argc, struct lobject *argv[])
{
	time_t t;
	struct tm *tm;
	struct ltmobject *tmobject;

	tmobject = lobject_create(lemon, sizeof(struct ltmobject), ltmobject_method);

	if (argc != 1 || !lobject_is_integer(lemon, argv[0])) {
		return lobject_error_argument(lemon, "required 1 integer argument");
	}

	t = linteger_to_long(lemon, argv[0]);

#ifdef WINDOWS
	tm = gmtime(&t);
	if (!tm) {
		return NULL;
	}
	memcpy(&tmobject->tm, tm, sizeof(*tm));
#else
	tm = gmtime_r(&t, &tmobject->tm);
	if (!tm) {
		return NULL;
	}
#endif

	return (struct lobject *)tmobject;
}

static struct lobject *
os_strftime(struct lemon *lemon, struct lobject *self, int argc, struct lobject *argv[])
{
	char buf[256];
	const char *fmt;
	struct ltmobject *tm;

	if (argc != 2 || !lobject_is_string(lemon, argv[0]) || argv[1]->l_method != ltmobject_method) {
		return lobject_error_argument(lemon, "required 2 integer arguments");
	}

	tm = (struct ltmobject *)argv[1];
	fmt = lstring_to_cstr(lemon, argv[0]);

	strftime(buf, sizeof(buf), fmt, &tm->tm);

	return lstring_create(lemon, buf, strlen(buf));
}

static struct lobject *
os_exit(struct lemon *lemon, struct lobject *self, int argc, struct lobject *argv[])
{
	int status;

	status = 0;
	if (argc == 1) {
		if (!lobject_is_integer(lemon, argv[0])) {
			return lobject_error_argument(lemon, "required 1 integer argument");
		}

		status = linteger_to_long(lemon, argv[0]);
	}

	exit(status);

	return lemon->l_nil;
}

struct lobject *
os_module(struct lemon *lemon)
{
	char *cstr;
	struct lobject *name;
	struct lobject *module;

#define SET_FUNCTION(value) do {                                             \
	cstr = #value ;                                                      \
	name = lstring_create(lemon, cstr, strlen(cstr));                    \
	lobject_set_attr(lemon,                                              \
	                 module,                                             \
	                 name,                                               \
	                 lfunction_create(lemon, name, NULL, os_ ## value)); \
} while(0)

#define SET_INTEGER(value) do {                                    \
	cstr = #value ;                                            \
	name = lstring_create(lemon, cstr, strlen(cstr));          \
	lobject_set_attr(lemon,                                    \
	                 module,                                   \
	                 name,                                     \
	                 linteger_create_from_long(lemon, value)); \
} while(0)

	module = lmodule_create(lemon, lstring_create(lemon, "os", 2));

	SET_FUNCTION(open);
	SET_FUNCTION(read);
	SET_FUNCTION(write);
	SET_FUNCTION(close);

	SET_FUNCTION(time);
	SET_FUNCTION(ctime);
	SET_FUNCTION(gmtime);
	SET_FUNCTION(strftime);
	SET_FUNCTION(select);
	SET_FUNCTION(realpath);
	SET_FUNCTION(exit);

#ifndef WINDOWS
	SET_FUNCTION(fcntl);
#endif

	/* common */
	SET_INTEGER(EXIT_SUCCESS);
	SET_INTEGER(EXIT_FAILURE);

	SET_INTEGER(O_RDONLY);
	SET_INTEGER(O_WRONLY);
	SET_INTEGER(O_RDWR);
	SET_INTEGER(O_APPEND);
	SET_INTEGER(O_CREAT);
	SET_INTEGER(O_TRUNC);
	SET_INTEGER(O_EXCL);
#ifdef WINDOWS
	SET_INTEGER(O_TEXT);
	SET_INTEGER(O_BINARY);

	SET_INTEGER(S_IREAD);
	SET_INTEGER(S_IWRITE);
#else
	SET_INTEGER(O_NONBLOCK);
	SET_INTEGER(O_NOFOLLOW);
	SET_INTEGER(O_CLOEXEC);

	SET_INTEGER(S_IRWXU);
	SET_INTEGER(S_IRUSR);
	SET_INTEGER(S_IWUSR);
	SET_INTEGER(S_IXUSR);
	SET_INTEGER(S_IRWXG);
	SET_INTEGER(S_IRGRP);
	SET_INTEGER(S_IWGRP);
	SET_INTEGER(S_IXGRP);
	SET_INTEGER(S_IRWXO);
	SET_INTEGER(S_IROTH);
	SET_INTEGER(S_IWOTH);
	SET_INTEGER(S_IXOTH);

	SET_INTEGER(F_GETFL);
	SET_INTEGER(O_NONBLOCK);
#endif

	return module;
}
