/*
 * Copyright (c) 2015 Marek Chalupa
 *
 * Example pass for wldbg
 *
 * compile with:
 *
 * cc -Wall -fPIC -shared -o example.so example.c
 * (maybe will need to add -I../src)
 *
 * run with (in directory with example.so):
 *
 * wldbg example -- wayland-client
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

#include <stdio.h>
#include <stdlib.h>

#include "wldbg.h"
#include "wldbg-pass.h"

struct example_data {
	unsigned int incoming_number;
	unsigned int outcoming_number;
	unsigned int in_trasfered;
	unsigned int out_trasfered;
};

static int
example_in(void *user_data, struct message *message)
{
	struct example_data *data = user_data;

	data->incoming_number++;
	data->in_trasfered += message->size;

	printf("Got incoming message: %u, size: %lu bytes\n",
	       data->incoming_number, message->size);

	/* run next pass, oposite is PASS_STOP */
	return PASS_NEXT;
}

static int
example_out(void *user_data, struct message *message)
{
	struct example_data *data = user_data;

	data->outcoming_number++;
	data->out_trasfered += message->size;

	printf("Got outcoming message: %u, size: %lu bytes\n",
	       data->outcoming_number, message->size);

	/* run next pass, oposite is PASS_STOP */
	return PASS_NEXT;
}

static void
example_help(void *user_data)
{
	(void) user_data;

	printf("This is an example pass!\n");
}

static int
example_init(struct wldbg *wldbg, struct wldbg_pass *pass,
	     int argc, const char *argv[])
{
	int i;
	struct example_data *data = calloc(1, sizeof *data);
	if (!data)
		return -1;

	printf("-- Initializing example pass --\n\n");
	for (i = 0; i < argc; ++i)
		printf("\targument[%d]: %s\n", i, argv[i]);

	printf("\n\n");

	pass->user_data = data;

	return 0;
}

static void
example_destroy(void *user_data)
{
	struct example_data *data = user_data;

	printf(" -- Destroying example pass --\n\n");
	printf("Totally trasfered %u bytes from client to server\n"
	       "and %u bytes from server to client\n",
	       data->out_trasfered, data->in_trasfered);

	free(data);
}

struct wldbg_pass wldbg_pass = {
	.init = example_init,
	.destroy = example_destroy,
	.server_pass = example_in,
	.client_pass = example_out,
	.help = example_help,
	.description = "Example wldbg pass"
};
