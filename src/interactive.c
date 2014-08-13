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

#include "wldbg.h"
#include "wldbg-pass.h"

struct wldbg_interactive {
	struct wldbg *wldbg;

	struct {
		uint64_t client_msg_no;
		uint64_t server_msg_no;
	} statistics;

	int skip_first_query;

	/* when we run client from interactive mode,
	 * we need to store it's credential here, so that
	 * we can free the allocated memory */
	struct {
		char *path;
		/* XXX
		 * add arguments */
	} client;
};

static int
cmd_help(struct wldbg_interactive *wldbgi,
		struct message *message, char *buf)
{
	printf("-----\n");
	printf("wldbg interactive:\n\n");

	/* XXX automatizite it */
	printf("    help\n");
	printf("    quit (Ctrl+^D)\n");
	printf("    pass\n");

	printf("-----\n");
}

static int
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

				return 0;
			}
	}

	dbg("Exiting...\n");

	wldbgi->wldbg->flags.running = 0;
	wldbgi->wldbg->flags.exit = 1;

	return 1;
}

static int
cmd_pass(struct wldbg_interactive *wldbgi,
		struct message *message, char *buf)
{
	vdbg("cmd: pass\n");
	return 0;
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

	return 0;
}

static int
cmd_run(struct wldbg_interactive *wldbgi, char *buf)
{
	char *nl;

	vdbg("cmd: run\n");

	nl = strrchr(buf, '\n');
	assert(nl);

	*nl = '\0';
	wldbgi->wldbg->client.path
		= wldbgi->client.path = strdup(buf);

	wldbgi->skip_first_query = 1;

	return 0;
}

static char *
skip_ws(char *str)
{
	int i = 0;
	while (isspace(*(str + i)) && *(str + i) != 0)
		++i;

	return str + i;
}

static int
run_cmd(char *buf, const char *opt,
	struct wldbg_interactive *wldbgi, struct message *message,
	int (*func)(struct wldbg_interactive *wldbgi,
			struct message *message,
			char *buf))
{
	int len = strlen(opt);
	assert(len);

	if (strncmp(buf, opt, len) == 0
		&& (isspace(buf[len]) || buf[len] == '\0')) {
		func(wldbgi, message, skip_ws(buf + len));
		return 1;
	} else {
		return 0;
	}
}

static void
query_user(struct wldbg_interactive *wldbgi, struct message *message)
{
	char buf[1024];
	int chr;

#define CMD(opt, func)						\
	if (run_cmd(buf, (opt), wldbgi, message, (func)))	\
		continue

	while (1) {
		if (wldbgi->wldbg->flags.exit
			|| wldbgi->wldbg->flags.error)
			break;

		printf("wldbg: ");

		if (fgets(buf, sizeof buf, stdin) == NULL) {
			if(cmd_quit(wldbgi, NULL, NULL))
				break;
			else
				continue;
		}

		CMD("quit", cmd_quit);
		CMD("help", cmd_help);
		CMD("pass", cmd_pass);
		CMD("info", cmd_info);

		/* we need the extra break, so we cannot use CMD */
		if (strncmp(buf, "run", 3) == 0
			&& (isspace(buf[3]) || buf[3] == '\0')) {
			cmd_run(wldbgi, skip_ws(buf + 3));
			break;
		}

		if (strncmp(buf, "continue\n", 10) == 0
			|| strncmp(buf, "c\n", 3) == 0) {
			if (!wldbgi->wldbg->flags.running) {
				printf("No client is running\n");
				continue;
			} else {
				printf("Continuing...\n");
				break;
			}
		}

		if (buf[0] != '\n')
			printf("Unknown command: %s", buf);
	}
}

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

	if (message->from == SERVER)
		++wldbgi->statistics.server_msg_no;
	else
		++wldbgi->statistics.client_msg_no;

	if (!wldbgi->skip_first_query
		&& (wldbgi->statistics.server_msg_no
		+ wldbgi->statistics.client_msg_no == 1)) {
		printf("Stopped on the first message\n");
		query_user(wldbgi, message);
	}

	process_message(wldbgi, message);

	/* This is always the last pass. Even when user will add
	 * some passes interactively, they will be added before
	 * this one */
	return PASS_STOP;
}

static void
wldbgi_destory(void *data)
{
	struct wldbg_interactive *wldbgi = data;

	dbg("Destroying wldbgi\n");

	wldbgi->wldbg->flags.exit = 1;

	if (wldbgi->client.path)
		free(wldbgi->client.path);

	free(wldbgi);
}

int
run_interactive(struct wldbg *wldbg, int argc, const char *argv[])
{
	struct pass *pass;
	struct wldbg_interactive *wldbgi;

	dbg("Starting interactive mode.\n");

	/* Interactive mode must be isolated - at least on startup */
	assert(wl_list_empty(&wldbg->passes)
		&& "Interactive mode must not be used with other passes");

	wldbgi = malloc(sizeof *wldbgi);
	if (!wldbgi)
		return -1;

	wldbgi->wldbg = wldbg;

	pass = malloc(sizeof *pass);
	if (!pass) {
		free(wldbgi);
		return -1;
	}

	wl_list_insert(wldbg->passes.next, &pass->link);

	pass->wldbg_pass.init = NULL;
	/* XXX ! */
	pass->wldbg_pass.help = NULL;
	pass->wldbg_pass.destroy = wldbgi_destory;
	pass->wldbg_pass.server_pass = process_interactive;
	pass->wldbg_pass.client_pass = process_interactive;
	pass->wldbg_pass.user_data = wldbgi;
	pass->wldbg_pass.description
		= "Interactive pass for wldbg (hardcoded)";

	wldbg->flags.one_by_one = 1;

	if (argc == 0) {
		query_user(wldbgi, NULL);
		return 0;
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

	return 0;
}
