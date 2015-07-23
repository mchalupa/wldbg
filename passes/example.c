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
