/*
 * Copyright (c) 2014 - 2015 Marek Chalupa
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
#include <string.h>
#include <signal.h>
#include <assert.h>
#include <sys/signalfd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>

#include "wayland/wayland-private.h"

#include "wldbg.h"
#include "wldbg-pass.h"
#include "interactive.h"
#include "passes.h"
#include "util.h"
#include "print.h"
#include "wldbg-private.h"
#include "resolve.h"
#include "interactive-commands.h"

void
terminate_client(struct wldbg_connection *conn)
{
	pid_t pid = conn->client.pid;

	dbg("Terminating client %d\n", pid);

	kill(pid, SIGTERM);
	waitpid(pid, NULL, 0);
}

int
cmd_quit(struct wldbg_interactive *wldbgi,
		struct message *message,
		char *buf)
{
	int chr;

	(void) message;
	(void) buf;

	if (wldbgi->wldbg->flags.running
		&& !wldbgi->wldbg->flags.error
		&& !wl_list_empty(&wldbgi->wldbg->connections)) {

		printf("Program seems running. "
			"Do you really want to quit? (y)\n");

			chr = getchar();
			if (chr == 'y') {
				wldbg_foreach_connection(wldbgi->wldbg,
							 terminate_client);
			} else {
				/* clear buffer */
				while (getchar() != '\n')
					;

				return CMD_CONTINUE_QUERY;
			}
	}

	dbg("Exiting...\n");

	wldbgi->wldbg->flags.exit = 1;

	return CMD_END_QUERY;
}

static int
cmd_next(struct wldbg_interactive *wldbgi,
		struct message *message,
		char *buf)
{
	(void) message;
	(void) buf;

	if (!wldbgi->wldbg->flags.running) {
		printf("Client is not running\n");
		return CMD_CONTINUE_QUERY;
	}

	wldbgi->stop = 1;
	return CMD_END_QUERY;
}

static int
cmd_continue(struct wldbg_interactive *wldbgi,
		struct message *message,
		char *buf)
{
	(void) message;
	(void) buf;

	if (!wldbgi->wldbg->flags.running) {
		printf("Client is not running\n");
		return CMD_CONTINUE_QUERY;
	}

	return CMD_END_QUERY;
}

static int
cmd_help(struct wldbg_interactive *wldbgi,
	 struct message *message, char *buf);

static void
cmd_help_help(int oneline)
{
	if (oneline)
		printf("Show this help message");
	else
		printf("Print help message. Given argument 'all', print "
			"comprehensive help about all commands.");
}

static int
cmd_send(struct wldbg_interactive *wldbgi,
		struct message *message,
		char *buf)
{
	struct wl_connection *conn;
	uint32_t buffer[1024]; /* size of wl_connection buffer */
	uint32_t size, opcode;
	int where, interactive, i = 0;
	struct message send_message;

	(void) message;
	(void) buf;
	(void) wldbgi;

	if (strncmp(buf, "server", 6) == 0
		|| (buf[0] == 's' && isspace(buf[1]))) {
		where = SERVER;
	} else if (strncmp(buf, "client", 6) == 0
		|| (buf[0] == 'c' && isspace(buf[1]))) {
		where = CLIENT;
	} else {
		printf(" :: send [server|s|client|c]"
			"[message - NOT IMPLEMENTED YET]\n");
		return CMD_CONTINUE_QUERY;
	}

	interactive = 1;

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
	}

	while(scanf("%x", &buffer[i]) != -1)
		++i;

	if (interactive) {
		size = i * sizeof(uint32_t);
		buffer[1] = (size << 16) | (opcode & 0xffff);
	} else {
		/* for debug */
		opcode = buffer[1] & 0xffff;
		size = buffer[1] >> 16;
	}

	send_message.connection = message->connection;
	send_message.data = buffer;
	send_message.size = size;
	send_message.from = where == CLIENT ? SERVER : CLIENT;

	printf("send - ");
	wldbgi_print_message(wldbgi, &send_message, 1 /* force */);

	dbg("Sending id %u, opcode %u , size %u\n", buffer[0], opcode, size);
	wl_connection_write(conn, buffer, size);

	return CMD_CONTINUE_QUERY;
}

/* XXX keep sorted! (in the future I'd like to do
 * binary search in this array */
const struct command commands[] = {
	{"break", "b", cmd_break, NULL},
	{"continue", "c", cmd_continue, NULL},
	{"edit", "e", cmd_edit, NULL},
	{"help", NULL,  cmd_help, cmd_help_help},
	{"hide", "h",  cmd_hide, cmd_hide_help},
	{"info", "i", cmd_info, cmd_info_help},
	{"next", "n",  cmd_next, NULL},
	{"pass", NULL, cmd_pass, cmd_pass_help},
	{"send", "s", cmd_send, NULL},
	{"showonly", "so", cmd_showonly, cmd_showonly_help},
	{"quit", "q", cmd_quit, NULL},

};

static int
is_the_cmd(char *buf, const struct command *cmd)
{
	int len = 0;

	/* try short version first */
	if (cmd->shortcut)
		len = strlen(cmd->shortcut);

	if (len && strncmp(buf, cmd->shortcut, len) == 0
		&& (isspace(buf[len]) || buf[len] == '\0')) {
		vdbg("identifying command: short '%s' match\n", cmd->shortcut);
		return 1;
	}

	/* this must be set */
	assert(cmd->name && "Each command must have long form");

	len = strlen(cmd->name);
	assert(len);

	if (strncmp(buf, cmd->name, len) == 0
		&& (isspace(buf[len]) || buf[len] == '\0')) {
		vdbg("identifying command: long '%s' match\n", cmd->name);
		return 1;
	}

	vdbg("identifying command: no match\n");
	return 0;
}

static int
cmd_help(struct wldbg_interactive *wldbgi,
	 struct message *message, char *buf)
{
	size_t i;
	int all = 0, found = 0;

	(void) wldbgi;
	(void) message;

	if (strcmp(buf, "all\n") == 0)
		all = 1;

	buf = skip_ws_to_newline(buf);
	if (!all && *buf) {
		for (i = 0; i < sizeof commands / sizeof *commands; ++i) {
			if (is_the_cmd(buf, &commands[i])) {
				found = 1;
				if (commands[i].help)
					commands[i].help(0);
				else
					printf("No help for this command\n");
			}
		}

		if (!found)
			printf("No such command\n");

		return CMD_CONTINUE_QUERY;
	}

	putchar ('\n');

	for (i = 0; i < sizeof commands / sizeof *commands; ++i) {
		if (all)
			printf(" == ");
		else
			printf("\t");
		
		printf("%-12s (%s)",
		       commands[i].name,
		       commands[i].shortcut ? commands[i].shortcut : "-");

		if (all)
			printf(" ==\n\n");

		if (commands[i].help) {
			if (all) {
				commands[i].help(0);
			} else {
				printf("\t -- ");
				commands[i].help(1);
			}	
		}

		if (all)
			putchar('\n');
		putchar('\n');
	}

	fflush(stdout);
	return CMD_CONTINUE_QUERY;
}

static char *
next_word(char *str)
{
	int i = 0;

	/* skip the first word if anything left */
	while(isalpha(*(str + i)) && *(str + i) != 0)
		++i;
	/* skip whitespaces before the other word */
	while (isspace(*(str + i)) && *(str + i) != 0)
		++i;

	return str + i;
}

int
run_command(char *buf,
		struct wldbg_interactive *wldbgi, struct message *message)
{
	size_t n;

	for (n = 0; n < (sizeof commands / sizeof *commands); ++n) {
		if (is_the_cmd(buf, &commands[n]))
			return commands[n].func(wldbgi, message, next_word(buf));
	}

	return CMD_DONT_MATCH;
}
