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
#include <ctype.h>
#include <signal.h>
#include <assert.h>
#include <sys/signalfd.h>

#include "wldbg.h"
#include "wldbg-pass.h"
#include "wldbg-private.h"
#include "interactive.h"
#include "resolve.h"
#include "passes.h"
#include "print.h"

/* defined in interactive-commands.c */
int
cmd_quit(struct wldbg_interactive *wldbgi,
		struct message *message, char *buf);

void free_breakpoint(struct breakpoint *);

#define INPUT_BUFFER_SIZE 512
static void
query_user(struct wldbg_interactive *wldbgi, struct message *message)
{
	int ret;
	char buf[INPUT_BUFFER_SIZE];
	char *cmd;

	while (1) {
		if (wldbgi->wldbg->flags.exit
			|| wldbgi->wldbg->flags.error) {
			break;
		}

		/* print whatever should have been printed by now
		 * and then print the prompt */
		fflush(stdout);
		printf("(wldbg) ");

		if (fgets(buf, sizeof buf, stdin) == NULL) {
			if(cmd_quit(wldbgi, NULL, NULL) == CMD_END_QUERY)
				break;
			else
				continue;
		}

		cmd = skip_ws_to_newline(buf);

		if (*cmd == '\n' && wldbgi->last_command) {
			cmd = wldbgi->last_command;
		} else if (*cmd == '\n') {
			continue;
		} else {
			/* save last command */
			free(wldbgi->last_command);
			wldbgi->last_command = strdup(cmd);
		}

		ret = run_command(cmd, wldbgi, message);

		if (ret == CMD_END_QUERY)
			break;
		else if (ret == CMD_CONTINUE_QUERY)
			continue;
		else
			printf("Unknown command: %s", cmd);
	}
}

static int
process_message(struct wldbg_interactive *wldbgi, struct message *message)
{
	/* print message's description
	 * This is default behaviour. XXX add possibility to
	 * turn it off */
	wldbgi_print_message(wldbgi, message);

	if (wldbgi->stop) {
		dbg("Stopped at message no. %lu from %s\n",
			message->from == SERVER ?
				wldbgi->statistics.server_msg_no :
				wldbgi->statistics.client_msg_no,
			message->from == SERVER ?
				"server" : "client");
		/* reset flag */
		wldbgi->stop = 0;
		query_user(wldbgi, message);
	}

	return 0;
}

static int
process_interactive(void *user_data, struct message *message)
{
	struct wldbg_interactive *wldbgi = user_data;
	struct breakpoint *b;

	vdbg("Mesagge from %s\n",
		message->from == SERVER ? "SERVER" : "CLIENT");

	if (message->from == SERVER)
		++wldbgi->statistics.server_msg_no;
	else
		++wldbgi->statistics.client_msg_no;

	if (!wldbgi->skip_first_query
		&& (wldbgi->statistics.server_msg_no
		+ wldbgi->statistics.client_msg_no == 1)) {
		printf("Stopped on the first message\n");
		wldbgi->stop = 1;
	}

	wl_list_for_each(b, &wldbgi->breakpoints, link) {
		if (b->applies(message, b)) {
			wldbgi->stop = 1;
			break;
		}
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
	struct breakpoint *b, *btmp;
	struct print_filter *pf, *pftmp;

	dbg("Destroying wldbgi\n");

	wldbgi->wldbg->flags.exit = 1;

	if (wldbgi->client.path)
		free(wldbgi->client.path);

	if (wldbgi->last_command)
		free(wldbgi->last_command);

	wl_list_for_each_safe(b, btmp, &wldbgi->breakpoints, link)
		free(b);
	wl_list_for_each_safe(pf, pftmp, &wldbgi->print_filters, link) {
		free(pf->filter);
		free(pf);
	}

	free(wldbgi);
}

static int
handle_sigint(int fd, void *data)
{
	size_t len;
	struct signalfd_siginfo si;
	struct wldbg_interactive *wldbgi = data;

	len = read(fd, &si, sizeof si);
	if (len != sizeof si) {
		fprintf(stderr, "reading signal's fd failed\n");
		return -1;
	}

	vdbg("Wldbgi: Got interrupt (SIGINT)\n");

	putchar('\n');
	query_user(wldbgi, &wldbgi->wldbg->message);

	return 1;
}

int
interactive_init(struct wldbg *wldbg, struct cmd_options *opts,
		 int argc, const char *argv[])
{
	struct pass *pass;
	struct wldbg_interactive *wldbgi;
	sigset_t signals;

	dbg("Starting interactive mode.\n");

	wldbgi = malloc(sizeof *wldbgi);
	if (!wldbgi)
		return -1;

	memset(wldbgi, 0, sizeof *wldbgi);
	wl_list_init(&wldbgi->breakpoints);
	wl_list_init(&wldbgi->print_filters);
	wldbgi->wldbg = wldbg;

	pass = alloc_pass("interactive");
	if (!pass)
		goto err_wldbgi;

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
	pass->wldbg_pass.flags = WLDBG_PASS_LOAD_ONCE;

	wldbg->flags.one_by_one = 1;

	/* remove default SIGINT handler */
	sigdelset(&wldbg->handled_signals, SIGINT);
	wldbg->signals_fd = signalfd(wldbg->signals_fd, &wldbg->handled_signals,
					SFD_CLOEXEC);

	if (wldbg->signals_fd == -1)
		goto err_pass;

	sigemptyset(&signals);
	sigaddset(&signals, SIGINT);

	/* set our own signal handlers */
	wldbgi->sigint_fd = signalfd(-1, &signals, SFD_CLOEXEC);

	if (wldbgi->sigint_fd == -1)
		goto err_pass;

	vdbg("Adding interactive SIGINT handler (fd %d)\n", wldbgi->sigint_fd);
	if (wldbg_monitor_fd(wldbg, wldbgi->sigint_fd,
			     handle_sigint, wldbgi) < 0)
		goto err_pass;

	/* we can bail out here in server mode */
	if (wldbg->flags.server_mode)
		return 0;

	/* TODO use getopt */
	if (strcmp(argv[0], "--") == 0) {
		++argv;
		--argc;
	}

	opts->path = strdup(argv[0]);
	if (!opts->path)
		goto err_pass;

	opts->argc = copy_arguments(&opts->argv, argc, argv);
	if (opts->argc == -1)
		goto err_pass;

	assert(opts->argc == argc);

	return 0;

err_pass:
		dealloc_pass(pass);
err_wldbgi:
		free(wldbgi);

		return -1;
}
