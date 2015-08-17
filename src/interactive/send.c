/*
 * Copyright (c) 2015 Marek Chalupa
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
#include <ctype.h>
#include <errno.h>

#include "wayland/wayland-private.h"

#include "wldbg-private.h"
#include "interactive.h"
#include "print.h"
#include "util.h"

void
cmd_send_help(int oneline)
{
	printf("Send message to server/client");
	if (oneline)
		return;

	printf("\n\n");
	printf(" :: send server|s|client|c [message]\n");
	printf("\n"
	       "Send command will prompt you for object id (where to send the message)\n"
	       "and opcode (what is the message) and then for the data of the message.\n"
	       "Id and opcode are scanned as unsigned decimal numbers and the data\n"
	       "as a sequence of hexadecimal numbers. If the message is passed as an argument,\n"
	       "it must contain the header (object id + opcode + size) and whole message\n"
	       "is scanned as a sequence of hexadecimal nubers (i. e. this sequence of words\n"
	       "will be send as it is given)\n");
}

int
cmd_send(struct wldbg_interactive *wldbgi,
	 struct wldbg_message *message,
	 char *buf)
{
	struct wl_connection *conn;
	uint32_t buffer[1024]; /* size of wl_connection buffer */
	uint32_t size, opcode, i = 0;
	int where, interactive;
	struct wldbg_message send_message;
	char *endptr;
	long val;

	(void) message;
	(void) buf;
	(void) wldbgi;

	if (strncmp(buf, "server", 6) == 0) {
		where = SERVER;
		buf += 6;
	} else if (buf[0] == 's' && isspace(buf[1])) {
		where = SERVER;
		buf += 2;
	} else if (strncmp(buf, "client", 6) == 0) {
		where = CLIENT;
		buf += 6;
	} else if (buf[0] == 'c' && isspace(buf[1])) {
		where = CLIENT;
		buf += 2;
	} else {
		cmd_send_help(0);
		return CMD_CONTINUE_QUERY;
	}

	buf = skip_ws_to_newline(buf);
	interactive = (*buf == '\n' || !*buf);

	if (where == SERVER)
		conn = message->connection->server.connection;
	else
		conn = message->connection->client.connection;

	/* XXX later do translation from interface@obj.request etc.. */
	if (interactive) {
		printf("Id: ");
		scanf("%u", &buffer[0]);

		printf("Opcode: ");
		scanf("%u", &opcode);
		i = 2;

		printf("Data:\n");
		while(scanf("%x", &buffer[i]) != -1)
			++i;

		size = i * sizeof(uint32_t);
		buffer[1] = (size << 16) | (opcode & 0xffff);
	} else {
		while(*buf != '\n') {
			errno = 0;
			val = strtol(buf, &endptr, 16);
			if (errno != 0) {
				perror("Failed parsing data");
				goto out;
			}

			if (buf == endptr) {
				printf("No hexadecimal nubers on position %d\n", i);
				goto out;
			}

			if (!isspace(*endptr)) {
				printf("Invalid character in data\n");
				goto out;
			}


			buffer[i] = (uint32_t) val;
			++i;

			buf = skip_ws_to_newline(endptr);
		}

		opcode = buffer[1] & 0xffff;
		size = buffer[1] >> 16;
	}


	if (size != 4 * i)
		printf("Warning: size given in header (%uB) does not match size of given message (%uB)\n",
			size, i * 4);

	if (size % 4)
		printf("Warning: size is not a multiple of 4, this is buggy\n");

	if (size > 1024)
		printf("Warning: Message is too big...\n");

	send_message.connection = message->connection;
	send_message.data = buffer;
	send_message.size = size;
	send_message.from = where == CLIENT ? SERVER : CLIENT;

	printf("resolved as: ");
	wldbgi_print_message(wldbgi, &send_message, 1 /* force */);

	printf("Send this message? [y/n] ");
	if (getchar() != 'y')
		goto out;

	dbg("Sending id %u, opcode %u , size %u\n", buffer[0], opcode, size);
	if (wl_connection_write(conn, buffer, size) < 0)
		perror("Writing message to connection");

out:
	/* clean stdin buffer */
	while (getchar() != '\n')
		;

	return CMD_CONTINUE_QUERY;
}

