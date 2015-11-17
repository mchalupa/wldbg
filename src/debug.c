/*
 * Copyright (c) 2015 Marek Chalupa
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "config.h"

#ifdef DEBUG

#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <stdarg.h>
#include <execinfo.h>
#include <signal.h>

int debug = 0;
int debug_verbose = 0;
const char *debug_domain = NULL;

static int abort_on_check_failure;
static int dbg_on_check_failure;

static void
init_syscalls_check(void);

void
debug_init(void)
{
	const char *dbg_env = getenv("WLDBG_DEBUG");
	if (dbg_env) {
		debug = 1;

		if (strcmp(dbg_env, "verbose") == 0
			|| strcmp(dbg_env, "v") == 0) {
			debug_verbose = 1;
		} else if (strncmp(dbg_env + strlen(dbg_env) - 2, ".c", 2) == 0) {
			debug_verbose = 1;
			debug_domain = dbg_env;
		}
	}

	init_syscalls_check();
}

/* copied from weston, modified */
static void
print_backtrace(void)
{
	void *buffer[32];
	int i, count;
	Dl_info info;

	count = backtrace(buffer, sizeof(buffer)/sizeof *buffer);
	for (i = 0; i < count; i++) {
		dladdr(buffer[i], &info);
		fprintf(stderr, "  [%016lx]  %s  (%s)\n",
			(long) buffer[i],
			info.dli_sname ? info.dli_sname : "--",
			info.dli_fname);
	}
}

static int (*sys_close)(int fd);

static void
check_failed(const char *msg, ...)
{
	va_list args;
	int err;

	if (abort_on_check_failure)
		abort();
	else if (dbg_on_check_failure)
		#if defined(__amd64__) || defined(__i386__)
			asm("int3");
		#else
			raise(SIGTRAP);
		#endif
	else {
		err = errno; /* save for sure */

		fprintf(stderr, "wldbg - ");

		va_start(args, msg);
		vfprintf(stderr, msg, args);
		va_end(args);

		fprintf(stderr, ": %s\n", strerror(err));

		print_backtrace();
	}
}

__attribute__ ((visibility("default"))) int
close(int fd)
{
	int ret = sys_close(fd);
	if (ret == -1)
		check_failed("close fd %d", fd);

	return ret;
}

static void
init_syscalls_check(void)
{
	const char *env = getenv("WLDBG_SYSCALLS_CHECK");
	if (env) {
		if (strcmp(env, "abort") == 0)
			abort_on_check_failure = 1;
		else if (strcmp(env, "trap") == 0)
			dbg_on_check_failure = 1;
	}

	sys_close = dlsym(RTLD_NEXT, "close");
}

#endif /* DEBUG */
