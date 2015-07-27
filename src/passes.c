/*
 * Copyright (c) 2014 Marek Chalupa
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

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <assert.h>
#include <dlfcn.h>

#include "wldbg.h"
#include "wldbg-private.h"
#include "util.h"
#include "wldbg-pass.h"

/* hardcoded passes */
extern struct wldbg_pass wldbg_pass_list;

static struct wldbg_pass *
load_pass(const char *path)
{
	void *handle;
	struct wldbg_pass *ret;
	int loaded = 0;

	/* check if file exists */
	if (access(path, F_OK) == -1) {
		return NULL;
	}

	/* check if the pass is already loaded */
	handle = dlopen(path, RTLD_NOW | RTLD_NOLOAD);
	if (handle) {
		loaded = 1;
		dlclose(handle);
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

	if ((ret->flags & WLDBG_PASS_LOAD_ONCE) && loaded) {
		fprintf(stderr, "This pass can be loaded only once\n");
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

void
dealloc_pass(struct pass *pass)
{
	if (pass->name)
		free(pass->name);

	free(pass);
}

struct pass *
alloc_pass(const char *name)
{
	struct pass *pass = malloc(sizeof *pass);
	if (!pass)
		return NULL;

	pass->name = strdup(name);

	if (!pass->name) {
		dealloc_pass(pass);
		return NULL;
	}

	return pass;
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

	/* hardcoded passes */
	if (strcmp(name, "list") == 0) {
		wldbg_pass = &wldbg_pass_list;
	} else {
		/* try current directory */
		if (build_path(path, "./", NULL, name) < 0)
			return NULL;

		dbg("Trying '%s'\n", path);
		wldbg_pass = load_pass(path);

		/* try passes/ if we're in build directory */
		if (!wldbg_pass && errno != EEXIST) {
			if (build_path(path, "passes/",
				       LT_OBJDIR, name) < 0)
				return NULL;

			dbg("Trying '%s'\n", path);
			wldbg_pass = load_pass(path);
		}

		/* home dir */
		if (!wldbg_pass && errno != EEXIST) {
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

		/* default paths
		 * XXX use LD_LIBRARY_PATH */
		if (!wldbg_pass && errno != EEXIST) {
			if (build_path(path, "/usr/local/lib/wldbg/",
					NULL, name) < 0)
				return NULL;

			dbg("Trying '%s'\n", path);
			wldbg_pass = load_pass(path);
		}

		if (!wldbg_pass && errno != EEXIST) {
			if (build_path(path, "/usr/lib/wldbg/",
					NULL, name) < 0)
				return NULL;

			dbg("Trying '%s'\n", path);
			wldbg_pass = load_pass(path);
		}

		if (!wldbg_pass && errno != EEXIST) {
			if (build_path(path, "/lib/wldbg/",
					NULL, name) < 0)
				return NULL;

			dbg("Trying '%s'\n", path);
			wldbg_pass = load_pass(path);
		}

		if (!wldbg_pass) {
			fprintf(stderr, "Didn't find pass '%s'\n", name);
			return NULL;
		}
	}

	pass = alloc_pass(name);
	if (!pass)
		return NULL;

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
load_passes(struct wldbg *wldbg, struct wldbg_options *opts,
	    int argc, const char *argv[])
{
	int i, rest, count, pass_num = 0, pass_created = 0;
	struct pass *pass;
	char *comma;

	rest = argc;
	while (rest) {
		count = count_args(rest, argv + argc - rest);

		if (count == 0) {
			/* -- or , or no arguments left */
			if (rest == 0)
				break;

			if (strcmp(argv[argc - rest], "--") == 0) {
				opts->path = strdup(argv[argc - rest + 1]);
				opts->argc
					= copy_arguments(&opts->argv,
							 rest - 1,
							 argv + argc - rest + 1);
				assert(opts->argc == rest - 1);

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
					dealloc_pass(pass);
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
