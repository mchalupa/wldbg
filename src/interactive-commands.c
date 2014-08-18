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
#include <string.h>
#include <signal.h>
#include <assert.h>
#include <sys/signalfd.h>

#include "wldbg.h"
#include "wldbg-pass.h"
#include "interactive.h"

int
cmd_quit(struct wldbg_interactive *wldbgi,
		struct message *message, char *buf)
{
	int chr;

	if (wldbgi->wldbg->flags.running
		&& !wldbgi->wldbg->flags.error
		&& wldbgi->wldbg->client.pid > 0) {

		printf("Program seems running. "
			"Do you really want to quit? (y)\n");

			chr = getchar();
			if (chr == 'y') {
				dbg("Killing the client\n");
				kill(wldbgi->wldbg->client.pid, SIGTERM);
				dbg("Waiting for the client to terminate\n");
				waitpid(wldbgi->wldbg->client.pid, NULL, 0);
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
cmd_pass(struct wldbg_interactive *wldbgi,
		struct message *message, char *buf)
{
	vdbg("cmd: pass\n");
	return CMD_CONTINUE_QUERY;
}

static int
cmd_info(struct wldbg_interactive *wldbgi,
		struct message *message, char *buf)
{
	if (strncmp(buf, "message", 7) == 0
		&& buf[7] == '\n') {
		printf("Sender: %s (no. %u), size: %u\n",
			message->from == SERVER ? "server" : "client",
			message->from == SERVER ? wldbgi->statistics.server_msg_no
						: wldbgi->statistics.client_msg_no,
			message->size);
	} else {
		printf("Unknown arguments\n");
	}

	return CMD_CONTINUE_QUERY;
}

static int
cmd_run(struct wldbg_interactive *wldbgi,
	struct message *message, char *buf)
{
	char *nl;

	vdbg("cmd: run\n");

	nl = strrchr(buf, '\n');
	assert(nl);

	*nl = '\0';
	wldbgi->wldbg->client.path
		= wldbgi->client.path = strdup(buf);

	wldbgi->skip_first_query = 1;

	return CMD_END_QUERY;
}

static int
cmd_next(struct wldbg_interactive *wldbgi,
		struct message *message, char *buf)
{
	if (!wldbgi->wldbg->flags.running) {
		printf("Client is not running\n");
		return CMD_CONTINUE_QUERY;
	}

	wldbgi->stop = 1;
	return CMD_END_QUERY;
}

static int
cmd_continue(struct wldbg_interactive *wldbgi,
		struct message *message, char *buf)
{
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
cmd_help_help(int ol)
{
	if (ol)
		printf("Show this help message");
	else
		printf("Print help message. Given argument 'all', print "
			"comprehensive help about all commands.");
}

/* XXX keep sorted! (in the future I'd like to do
 * binary search in this array */
const struct command commands[] = {
	{"continue", "c", cmd_continue, NULL},
	//{"edit", "e", cmd_exit, NULL},
	{"help", "h",  cmd_help, cmd_help_help},
	{"info", NULL, cmd_info, NULL},
	{"next", "n",  cmd_next, NULL},
	{"pass", NULL, cmd_pass, NULL},
	{"run",  NULL, cmd_run, NULL},
	{"quit", "q", cmd_quit, NULL},

};

static int
cmd_help(struct wldbg_interactive *wldbgi,
		struct message *message, char *buf)
{
	int i, all = 0;

	if (strcmp(buf, "all\n") == 0)
		all = 1;

	putchar ('\n');

	for (i = 0; i < sizeof commands / sizeof *commands; ++i) {
		if (all)
			printf(" == ");
		else
			putchar('\t');
		
		printf("%s ", commands[i].name);

		if (commands[i].shortcut)
			printf("(%s)", commands[i].shortcut);

		if (all)
			printf(" ==\n\n");

		if (commands[i].help)
			if (all) {
				commands[i].help(0);
			} else {
				printf("\t -- ");
				commands[i].help(1);
			}	

		if (all)
			putchar('\n');
		putchar('\n');
	}

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

int
run_command(char *buf,
		struct wldbg_interactive *wldbgi, struct message *message)
{
	int n;

	for (n = 0; n < (sizeof commands / sizeof *commands); ++n) {
		if (is_the_cmd(buf, &commands[n]))
			return commands[n].func(wldbgi, message, next_word(buf));
	}

	return CMD_DONT_MATCH;
}
