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
