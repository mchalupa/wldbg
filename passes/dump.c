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

#include <unistd.h>
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
	CLIENTONLY		= 1 << 5,
	SERVERONLY		= 1 << 6,
	STATS			= 1 << 7,
	NOOUT			= 1 << 8,
};

struct dump {
	uint64_t options;
	const char *file;
	int file_fd;

	struct {
		uint64_t in_msg;
		uint64_t out_msg;
		uint64_t in_bytes;
		uint64_t out_bytes;
	} stats;
};

static void
dump_to_file(struct message *message, struct dump *dump)
{
	if (write(dump->file_fd, message->data, message->size)
		!= (ssize_t) message->size) {
		perror("Dumping to file failed");
	}
}

static void
dump_message(struct message *message, struct dump *dump)
{
	uint32_t i;
	uint32_t *data = message->data;
	uint32_t options = dump->options;
	size_t size = 0;

	if (options & TOFILE) {
		dump_to_file(message, dump);

		/* if user want only dump to file, do not print anything */
		if (!(options & (~TOFILE)))
			return;
	}

	if (options & NOOUT)
		return;

	if (options & DECODE)
		options |= SEPARATE;

	for (i = 0; i < message->size / sizeof(uint32_t) ; ++i) {
		if (options & SEPARATE) {
			if (size == 0 && message->size > i + 1) {
				size = data[i + 1] >> 16;

				if (options & DECODE) {
					printf("\n id: %u opcode: %u size: %lu:\n\t",
						data[i], data[i + 1] & 0xffff,
						size);
				} else {
					printf("\n | %2lu | ", size);
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

	if (dump->options & STATS) {
		++dump->stats.in_msg;
		dump->stats.in_bytes += message->size;
	}

	if (dump->options & CLIENTONLY && !(dump->options & SERVERONLY))
		return PASS_NEXT;

	/* if we have are _only_ dumping to file, don't print msg */
	if (dump->options & (~TOFILE) && !(dump->options & NOOUT))
		printf("SERVER: ");

	dump_message(message, dump);

	return PASS_NEXT;
}

static int
dump_out(void *user_data, struct message *message)
{
	struct dump *dump = user_data;

	if (dump->options & STATS) {
		++dump->stats.out_msg;
		dump->stats.out_bytes += message->size;
	}

	if (dump->options & SERVERONLY && !(dump->options & CLIENTONLY))
		return PASS_NEXT;

	/* if we have are _only_ dumping to file, don't print msg */
	if (dump->options & (~TOFILE) && !(dump->options & NOOUT))
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
		else if (strcmp(argv[i], "client") == 0)
			flags |= CLIENTONLY;
		else if (strcmp(argv[i], "server") == 0)
			flags |= SERVERONLY;
		else if (strcmp(argv[i], "stats") == 0)
			flags |= STATS;
		else if (strcmp(argv[i], "statistics") == 0)
			flags |= STATS;
		else if (strcmp(argv[i], "no-output") == 0)
			flags |= NOOUT;
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

	fflush(stdout);
	if (dump->options & STATS) {
		printf("----------------------\n"
		       "Messages from server: %lu (%lu bytes)\n"
		       "Messages from client: %lu (%lu bytes)\n"
		       "----------------------\n",
		       dump->stats.in_msg, dump->stats.in_bytes,
		       dump->stats.out_msg, dump->stats.out_bytes);
	}

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
