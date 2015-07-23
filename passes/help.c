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
#include <assert.h>
#include <string.h>

#include "../src/wldbg.h"
#include "../src/wldbg-pass.h"
#include "../wayland/wayland-util.h"

#include "wldbg-private.h"

/* XXX rename this pass to debug (and maybe move to src/ ? */

static int
help_in(void *user_data, struct message *message)
{
	(void) user_data;
	(void) message;

	return 0;
}

static int
help_out(void *user_data, struct message *message)
{
	struct wldbg *wldbg = user_data;
	int i;

	(void) message;
	assert(wldbg);

#if 0
	printf("Wldbg:\n-------\n\n");
	printf( "Server {\n"
		"\tfd = %d\n"
		"\tconnection = %p\n"
		"}\n\n"
		"Client {\n"
		"\tfd = %d\n"
		"\tconnection = %p\n\n"
		"\tpath = %s\n"
		"\targc = %d\n",
		wldbg->connection->server.fd,
		wldbg->connection->server.connection,
		wldbg->connection->client.fd,
		wldbg->connection->client.connection,
		wldbg->connection->client.path, wldbg->connection->client.argc);
	for (i = 0; i < wldbg->connection->client.argc; ++i)
		printf("\t    argv[%d] = %s\n",
			i, wldbg->connection->client.argv[i]);
	printf(
		"\n\tpid = %d\n"
		"}\n\n"
		"epoll fd = %d\n\n"
		"passes no = %d\n",
		wldbg->connection->client.pid, wldbg->epoll_fd,
		wl_list_length(&wldbg->passes));

	/* leaky leaky.. */
	exit(0);
#endif
}

static int
help_init(struct wldbg *wldbg, struct wldbg_pass *pass,
	  int argc, const char *argv[])
{
	int i;

	assert(argc > 0);

	printf("Help and debugging module\n\n");
	printf("Possible arguments:\n\tdebug\t\t"
		"-- print debugging information\n");

	if (argc == 1) {
		printf("Print usage etc...\n");
	}

	if (argc == 2 && strcmp(argv[1], "debug") == 0) {
		printf("Got %d arguments:\n", argc);
		for (i = 0; i < argc; ++i)
			printf("[%d]:\t%s\n", i, argv[i]);

	pass->user_data = wldbg;

	} else {
		printf("\nWhat to do? What to do?\n");
		exit(1);
	}

	return 0;
}

static void
help_destroy(void *data)
{
	(void) data;
}

struct wldbg_pass wldbg_pass = {
	.init = help_init,
	.destroy = help_destroy,
	.server_pass = help_in,
	.client_pass = help_out,
	.help = NULL
};
