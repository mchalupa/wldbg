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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wldbg.h"
#include "wldbg-pass.h"

enum options {
	SEPARATE		= 1 ,
	DECIMAL			= 1 << 1,
	DECODE			= 1 << 2
};

static void
dump_message(struct message *message, uint64_t options)
{
	int i;
	uint32_t *data = message->data;
	size_t size = 0;

	if (options & DECODE)
		options |= SEPARATE;

	for (i = 0; i < message->size / sizeof(uint32_t) ; ++i) {
		if (options & SEPARATE) {
			if (size == 0 && message->size > i + 1) {
				size = data[i + 1] >> 16;

				if (options & DECODE) {
					printf("\n id: %u opcode: %u size: %u:\n\t",
						data[i], data[i + 1] & 0xffff,
						size);
				} else {
					printf("\n | %2u | ", size);
				}
			}

			size -= sizeof(uint32_t);
		}

		if (options & DECIMAL)
			printf("%d ", data[i]);
		else
			printf("%08x ", data[i]);
	}

	putchar('\n');
}

static int
dump_in(void *user_data, struct message *message)
{
	uint64_t *flags = user_data;

	printf("SERVER: ");
	dump_message(message, *flags);

	return PASS_NEXT;
}

static int
dump_out(void *user_data, struct message *message)
{
	uint64_t *flags = user_data;

	printf("CLIENT: ");
	dump_message(message, *flags);

	return PASS_NEXT;
}


static void
print_help(int exit_st)
{
	printf("Help!\n");

	exit(exit_st);
}

static int
dump_init(struct wldbg *wldbg, struct wldbg_pass *pass, int argc, const char *argv[])
{
	int i;
	static uint64_t flags = 0;

	for (i = 1; i < argc; ++i) {
		if (strcmp(argv[i], "decode") == 0)
			flags |= DECODE;
		else if (strcmp(argv[i], "separate") == 0)
			flags |= SEPARATE;
		else if (strcmp(argv[i], "decimal") == 0)
			flags |= DECIMAL;
		else if (strcmp(argv[i], "help") == 0)
			print_help(0);
	}

	pass->user_data = &flags;

	return 0;
}

static void
dump_destroy(void *data)
{
}

struct wldbg_pass wldbg_pass = {
	.init = dump_init,
	.destroy = dump_destroy,
	.server_pass = dump_in,
	.client_pass = dump_out,
};
