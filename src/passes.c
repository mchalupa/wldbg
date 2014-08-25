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

/* hardcoded passes */
extern struct wldbg_pass wldbg_pass_list;

static struct wldbg_pass *
load_pass(const char *path)
{
	void *handle;
	struct wldbg_pass *ret;

	/* check if file exists */
	if (access(path, F_OK) == -1)
		return NULL;

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

#define PATH_LENGTH 256

static int
build_path(char *path, const char *prefix,
		const char *middle, const char *name)
{
	size_t len;

	/* we must have name */
	assert(name);

	len = snprintf(path, PATH_LENGTH, "%s%s%s.so",
			prefix ? prefix : "",
			middle ? middle : "",
			name);
	if (len >= PATH_LENGTH) {
		fprintf(stderr, "Pass name too long\n");
		return -1;
	}

	return 0;
}

/* searching for .so file in folders (in this order)
 *   ./passes/.libs/file.so
 *   $HOME/.wldbg/file.so
 *   /usr/local/lib/wldbg/file.so
 *   /usr/lib/wldbg/file.so
 *   /lib/wldbg/file.so
 */
struct pass *
create_pass(const char *name)
{
	struct pass *pass;
	struct wldbg_pass *wldbg_pass;
	char path[PATH_LENGTH];
	const char *env;
	size_t len;

	/* hardcoded passes */
	if (strcmp(name, "list") == 0) {
		wldbg_pass = &wldbg_pass_list;
	} else {
		/* try root dir, its nice when developing */
		if (build_path(path, "passes/", LT_OBJDIR, name) < 0)
			return NULL;

		dbg("Trying '%s'\n", path);
		wldbg_pass = load_pass(path);

		/* home dir */
		if (!wldbg_pass) {
			env = getenv("HOME");
			if (!env) {
				fprintf(stderr, "$HOME is not set!\n");
			} else {
				if (build_path(path, env,
						"/.wldbg/", name) < 0)
					return NULL;

				dbg("Trying '%s'\n", path);
				wldbg_pass = load_pass(path);
			}
		}

		/* default paths */
		if (!wldbg_pass) {
			if (build_path(path, "/usr/local/lib/wldbg",
					NULL, name) < 0)
				return NULL;

			dbg("Trying '%s'\n", path);
			wldbg_pass = load_pass(path);
		}

		if (!wldbg_pass) {
			if (build_path(path, "/usr/lib/wldbg",
					NULL, name) < 0)
				return NULL;

			dbg("Trying '%s'\n", path);
			wldbg_pass = load_pass(path);
		}

		if (!wldbg_pass) {
			if (build_path(path, "/lib/wldbg",
					NULL, name) < 0)
				return NULL;

			dbg("Trying '%s'\n", path);
			wldbg_pass = load_pass(path);
		}

		if (!wldbg_pass)
			return NULL;
	}

	pass = malloc(sizeof *pass);
	if (!pass)
		return NULL;

	pass->name = strdup(name);

	if (!pass->name) {
		free(pass);
		return NULL;
	}

	pass->wldbg_pass = *wldbg_pass;

	return pass;
}

int
pass_init(struct wldbg *wldbg, struct pass *pass,
		int argc, const char *argv[])
{
	if (pass->wldbg_pass.init)
		return pass->wldbg_pass.init(wldbg, &pass->wldbg_pass,
						argc, argv);
	else
		return 0;
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

			/* Delete the comma from arguments for the pass
			 * if present */
			if ((comma = strrchr(argv[argc - rest + count - 1], ',')))
				*comma = '\0';

#ifdef DEBUG
			dbg("Pass %d:\n", pass_num);
			for (i = 0; i < count; ++i) {
				dbg("\targ[%d]: %s\n", i, argv[argc - rest + i]);
			}
#endif

			pass = create_pass(argv[argc - rest]);
			if (pass) {
				if (pass_init(wldbg, pass, count,
						argv + argc - rest) != 0) {
					free(pass->name);
					free(pass);
					continue;
				}

				++pass_created;
				wl_list_insert(wldbg->passes.next, &pass->link);
				dbg("Pass '%s' loaded\n", argv[argc - rest]);
			} else {
				dbg("Loading pass '%s' failed\n",
					argv[argc - rest]);
			}

			rest -= count;
		}
	}

	dbg("Loaded %d passes\n", pass_created);
	return pass_created;
}
