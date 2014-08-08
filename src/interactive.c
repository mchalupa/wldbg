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

#include "wldbg.h"
#include "wldbg-pass.h"

struct wldbg_interactive {
	struct wldbg *wldbg;

	struct {
		uint64_t client_msg_no;
		uint64_t server_msg_no;
	} statistics;
};

static int
process_message(struct wldbg_interactive *wldbgi, struct message *message)
{
}

static int
process_interactive(void *user_data, struct message *message)
{
	struct wldbg_interactive *wldbgi = user_data;
	struct message msg;
	size_t rest = message->size;

	dbg("Mesagge from %s\n", message->from == SERVER ?
		"SERVER" : "CLIENT");

	process_message(wldbgi, message);

	/* This is always the last pass. Even when user will add
	 * some passes interactively, they will be added before
	 * this one */
	return PASS_STOP;
}

int
run_interactive(struct wldbg *wldbg, int argc, const char *argv[])
{
	struct pass *pass;
	struct wldbg_interactive *wldbgi;

	dbg("Starting interactive mode.\n");

	if (argc == 0) {
		fprintf(stderr, "Need specify client\n");
		return -1;
	}

	/* TODO use getopt */
	if (strcmp(argv[0], "--") == 0) {
		wldbg->client.path = argv[1];
		wldbg->client.argc = argc - 1;
		wldbg->client.argv = (char * const *) argv + 2;
	} else {
		wldbg->client.path = argv[0];
		wldbg->client.argc = argc;
		wldbg->client.argv = (char * const *) argv + 1;
	}

	wldbgi = malloc(sizeof *wldbgi);
	if (!wldbgi)
		return -1;

	wldbgi->wldbg = wldbg;

	/* Interactive mode must be isolated - at least on startup */
	assert(wl_list_empty(&wldbg->passes));

	pass = malloc(sizeof *pass);
	if (!pass) {
		free(wldbgi);
		return -1;
	}

	wl_list_insert(wldbg->passes.next, &pass->link);

	pass->wldbg_pass.init = NULL;
	pass->wldbg_pass.destroy = free;
	pass->wldbg_pass.server_pass = process_interactive;
	pass->wldbg_pass.client_pass = process_interactive;
	pass->wldbg_pass.user_data = wldbgi;

	return 0;
}
