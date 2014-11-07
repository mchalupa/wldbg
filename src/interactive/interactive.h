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

#ifndef _WLDBG_INTERACTIVE_H_
#define _WLDBG_INTERACTIVE_H_

#include "wldbg.h"

struct wldbg_interactive {
	struct wldbg *wldbg;

	struct {
		uint64_t client_msg_no;
		uint64_t server_msg_no;
	} statistics;

	/* when we run client from interactive mode,
	 * we need to store it's credential here, so that
	 * we can free the allocated memory */
	struct {
		char *path;
		/* XXX
		 * add arguments */
	} client;

	int sigint_fd;

	/* query user on the next message */
	int stop;
	int skip_first_query;

	/* commands history */
	char *last_command;
};

struct command {
	/* long form of a command i. e. 'continue' */
	const char *name;
	/* short form of a command i. e. 'c' (continue) */
	const char *shortcut;
	/* function to be called for the command. The last argument
	 * is the rest of user's input */
	int (*func)(struct wldbg_interactive *, struct message *, char *);
	/* this function prints help for the command.
	 * If argument is not zero, then print short, oneline description */
	void (*help)(int oneline);
};

/* exit state of command */
enum {
	CMD_DONT_MATCH,
	CMD_END_QUERY,
	CMD_CONTINUE_QUERY,
};

/* array of all commands */
extern const struct command commands[];

int
run_command(char *buf,
		struct wldbg_interactive *wldbgi, struct message *message);

#endif /* _WLDBG_INTERACTIVE_H_ */
