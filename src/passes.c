/*
 * Copyright (c) 2014 Marek Chalupa
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

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <assert.h>
#include <dlfcn.h>

#include "wldbg.h"
#include "wldbg-pass.h"

extern struct wldbg_pass wldbg_pass_dump;

static struct wldbg_pass *
load_pass(const char *path)
{
	void *handle;
	struct wldbg_pass *ret;

	handle = dlopen(path, RTLD_NOW | RTLD_NOLOAD);
	if (handle) {
		fprintf(stderr, "Pass already loaded\n");
		dlclose(handle);
		return NULL;
	}

	handle = dlopen(path, RTLD_NOW);
	if (!handle) {
		fprintf(stderr, "Loading pass: %s\n", dlerror());
		return NULL;
	}

	ret = dlsym(handle, "wldbg_pass");
	if (!ret) {
		fprintf(stderr, "Failed loading wldbg_pass struct: %s\n",
			dlerror());
		dlclose(handle);
		return NULL;
	}

	return ret;
}

static struct pass *
create_pass(const char *name)
{
	struct pass *pass;
	struct wldbg_pass *wldbg_pass;
	char path[256];
	size_t len;

	/* hardcoded passes */
	if (strcmp(name, "dump") == 0) {
		wldbg_pass = &wldbg_pass_dump;
	} else {
		/* XXX make it relative path */
		len = snprintf(path, sizeof path, "../passes/%s.so", name);
		if (len >= sizeof path) {
			fprintf(stderr, "Pass name too long\n");
			return NULL;
		}

		wldbg_pass = load_pass(path);
		if (!wldbg_pass)
			return NULL;
	}

	pass = malloc(sizeof *pass);
	if (!pass)
		return NULL;

	wl_list_init(&pass->link);
	pass->wldbg_pass = *wldbg_pass;

	return pass;
}

static int
pass_init(struct wldbg *wldbg, struct pass *pass,
		int argc, const char *argv[])
{
	struct wldbg_pass_data data;

	data.server_fd = wldbg->server.fd;
	data.client_fd = wldbg->client.fd;
	data.user_data = &pass->wldbg_pass.user_data;

	return pass->wldbg_pass.init(&data, argc, argv);
}

static int
count_args(int argc, const char *argv[])
{
	int count;

	for (count = 0; count < argc; ++count) {
		if ((strcmp(argv[count], "--") == 0)
			|| (strcmp(argv[count], ",") == 0))
			break;

		if (strrchr(argv[count], ','))
			return count + 1;
	}

	return count;
}

int
load_passes(struct wldbg *wldbg, int argc, const char *argv[])
{
	int i, rest, count, pass_num = 0, pass_created = 0;
	struct pass *pass;
	char *comma;

	dbg("Loading passes\n");

	rest = argc;
	while (rest) {
		count = count_args(rest, argv + argc - rest);

		if (count == 0) {
			/* -- or , or no arguments left */
			if (rest == 0)
				break;

			if (strcmp(argv[argc - rest], "--") == 0) {
				wldbg->client.path = argv[argc - rest + 1];
				wldbg->client.argv
					= (char * const *) argv + argc - rest + 1;
				wldbg->client.argc = rest - 1;
#ifdef DEBUG
				dbg("Program: %s, argc == %d\n",
					wldbg->client.path,
					wldbg->client.argc);
				for (i = 0; i < wldbg->client.argc; ++i)
					dbg("\targ[%d]: %s\n",
						i, wldbg->client.argv[i]);
#endif /* DEBUG */
				break;
			} else if (strcmp(argv[argc - rest], ",") == 0) {
				/* just skip the comma and process next pass */
				--rest;
				continue;
			}
		} else {
			++pass_num;

			dbg("Pass %d:\n", pass_num);
#ifdef DEBUG
			for (i = 0; i < count; ++i) {
				dbg("\targ[%d]: %s\n", i, argv[argc - rest + i]);
			}
#endif

			pass = create_pass(argv[argc - rest]);
			if (pass) {
				if (pass_init(wldbg, pass, count,
						argv + argc - rest) != 0) {
					free(pass);
					continue;
				}

				++pass_created;
				wl_list_insert(wldbg->passes.next, &pass->link);
				dbg("Pass '%s' loaded\n", argv[argc - rest]);
			} else {
				dbg("Loading pass '%s' failed\n", argv[argc - rest]);
			}

			rest -= count;
		}
	}

	dbg("Loaded %d passes\n", pass_created);
	return pass_created;
}
