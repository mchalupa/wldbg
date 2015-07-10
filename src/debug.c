/*
 * Copyright (c) 2015 Marek Chalupa
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of the copyright holders not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no representations
 * about the suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
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

static int abort_on_check_failure;
static int dbg_on_check_failure;

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
		asm("int3");
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

void
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
