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
#include <fcntl.h>

#include "wldbg.h"
#include "wldbg-pass.h"

enum options {
	SEPARATE		= 1 ,
	DECIMAL			= 1 << 1,
	DECODE			= 1 << 2,
	TOFILE			= 1 << 3,
	RAW			= 1 << 4,
};

struct dump {
	uint64_t options;
	const char *file;
	int file_fd;
};

static void
dump_to_file(struct message *message, struct dump *dump)
{
	if (write(dump->file_fd, message->data, message->size)
		!= message->size) {
		perror("Dumping to file failed");
	}
}

static void
dump_message(struct message *message, struct dump *dump)
{
	int i;
	uint32_t *data = message->data;
	uint32_t options = dump->options;
	size_t size = 0;

	if (options & TOFILE) {
		dump_to_file(message, dump);

		/* if user want only dump to file, do not print anything */
		if (!(options & (~TOFILE)))
			return;
	}

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
	struct dump *dump = user_data;

	/* if we have are _only_ dumping to file, don't print msg */
	if (dump->options & (~TOFILE))
		printf("SERVER: ");

	dump_message(message, dump);

	return PASS_NEXT;
}

static int
dump_out(void *user_data, struct message *message)
{
	struct dump *dump = user_data;

	/* if we have are _only_ dumping to file, don't print msg */
	if (dump->options & (~TOFILE))
		printf("CLIENT: ");

	dump_message(message, dump);

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
	uint64_t flags = 0;
	struct dump *dump = malloc(sizeof *dump);
	if (!dump)
		return -1;

	(void) wldbg;

	for (i = 1; i < argc; ++i) {
		if (strcmp(argv[i], "raw") == 0)
			flags |= RAW;
		if (strcmp(argv[i], "decode") == 0)
			flags |= DECODE;
		else if (strcmp(argv[i], "separate") == 0)
			flags |= SEPARATE;
		else if (strcmp(argv[i], "decimal") == 0)
			flags |= DECIMAL;
		else if (strcmp(argv[i], "help") == 0)
			print_help(0);
		else if (strcmp(argv[i], "to-file") == 0) {
			flags |= TOFILE;
			dump->file = argv[i + 1];
		}
	}

	if (flags & TOFILE) {
		dump->file_fd = open(dump->file, O_WRONLY | O_CREAT | O_EXCL, 0755);
		if (dump->file_fd == -1) {
			perror("Opening file for dumping");
			free(dump);
			return -1;
		}
	}

	dump->options = flags;
	pass->user_data = dump;

	return 0;
}

static void
dump_destroy(void *data)
{
	struct dump *dump = data;

	if (dump->options & TOFILE) {
		close(dump->file_fd);
	}

	free(dump);
}

struct wldbg_pass wldbg_pass = {
	.init = dump_init,
	.destroy = dump_destroy,
	.server_pass = dump_in,
	.client_pass = dump_out,
};
