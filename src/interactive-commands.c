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
#include <sys/wait.h>
#include <ctype.h>

#include "wayland/wayland-private.h"

#include "wldbg.h"
#include "wldbg-pass.h"
#include "interactive.h"
#include "passes.h"
#include "util.h"

int
cmd_quit(struct wldbg_interactive *wldbgi,
		WLDBG_UNUSED struct message *message,
		WLDBG_UNUSED char *buf)
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

static void
cmd_pass_help(int oneline)
{
	if (oneline) {
		printf("Add, remove, list passes\n");
		return;
	}

	printf("Possible arguments:\n");
	printf("\tlist\t\t- list available passes\n");
	printf("\tloaded\t\t- list loaded passes\n");
	printf("\tadd NAME\t- add pass NAME.so\n");
	printf("\tremove NAME\t- remove pass NAME\n");
}

static void
add_pass(struct wldbg *wldbg, const char *name)
{
	struct pass *pass;

	dbg("Adding pass '%s'\n", name);

	pass = create_pass(name);
	if (pass) {
		/* XXX add arguments */
		if (pass_init(wldbg, pass, 0, NULL) != 0) {
			fprintf(stderr, "Failed initializing pass '%s'\n",
				name);
			dealloc_pass(pass);
		} else {
			/* insert always at the head */
			wl_list_insert(wldbg->passes.next, &pass->link);

			dbg("Added pass '%s'\n", name);
		}
	} else {
		fprintf(stderr, "Failed adding pass '%s'\n", name);
	}
}

static void
loaded_passes(struct wldbg *wldbg)
{
	struct pass *pass;

	printf("Loaded passes:\n");
	wl_list_for_each(pass, &wldbg->passes, link) {
		printf("\t - %s\n", pass->name);
	}
}

static void
remove_pass(struct wldbg *wldbg, const char *name)
{
	struct pass *pass, *tmp;

	dbg("Removing pass '%s'\n", name);

	wl_list_for_each_safe(pass, tmp, &wldbg->passes, link) {
		if (strcmp(pass->name, name) == 0) {
			wl_list_remove(&pass->link);

			free(pass->name);
			free(pass);

			dbg("Removed pass '%s'\n", name);
			return;
		}
	}

	fprintf(stderr, "Didn't find pass '%s'\n", name);
}



static int
cmd_pass(struct wldbg_interactive *wldbgi,
		WLDBG_UNUSED struct message *message,
		WLDBG_UNUSED char *buf)
{
	if (strncmp(buf, "list\n", 5) == 0) {
		list_passes(1);
	} else if (strncmp(buf, "loaded\n", 5) == 0) {
		loaded_passes(wldbgi->wldbg);
	} else if (strncmp(buf, "add ", 4) == 0) {
		/* remove \n from it */
		buf[strlen(buf) - 1] = '\0';
		add_pass(wldbgi->wldbg, buf + 4);
	} else if (strncmp(buf, "remove ", 7) == 0) {
		buf[strlen(buf) - 1] = '\0';
		remove_pass(wldbgi->wldbg, buf + 7);
	} else
		cmd_pass_help(0);

	return CMD_CONTINUE_QUERY;
}

void
print_objects(struct wldbg *wldbg)
{
	struct wldbg_ids_map *map = &wldbg->resolved_objects;
	const struct wl_interface *intf;
	uint32_t id;

	for (id = 0; id < map->count; ++id) {
		intf = wldbg_ids_map_get(map, id);
		printf("\t%u -> %s\n", id, intf ? intf->name : "NULL");
	}
}

static int
cmd_info(struct wldbg_interactive *wldbgi,
		struct message *message, char *buf)
{
	if (strncmp(buf, "message\n", 8) == 0) {
		printf("Sender: %s (no. %lu), size: %lu\n",
			message->from == SERVER ? "server" : "client",
			message->from == SERVER ? wldbgi->statistics.server_msg_no
						: wldbgi->statistics.client_msg_no,
			message->size);
	} else if (strncmp(buf, "objects\n", 8) == 0) {
		print_objects(wldbgi->wldbg);
	} else {
		printf("Unknown arguments\n");
	}

	return CMD_CONTINUE_QUERY;
}

static int
cmd_run(struct wldbg_interactive *wldbgi,
	WLDBG_UNUSED struct message *message, char *buf)
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
		WLDBG_UNUSED struct message *message,
		WLDBG_UNUSED char *buf)
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
		WLDBG_UNUSED struct message *message,
		WLDBG_UNUSED char *buf)
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

static int
cmd_send(struct wldbg_interactive *wldbgi,
		WLDBG_UNUSED struct message *message,
		WLDBG_UNUSED char *buf)
{
	struct wl_connection *conn;
	uint32_t buffer[1024]; /* size of wl_connection buffer */
	uint32_t size, opcode;
	int where, interactive, i = 0;

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
		conn = wldbgi->wldbg->server.connection;
	else
		conn = wldbgi->wldbg->client.connection;

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

	dbg("Sending id %u, opcode %u , size %u\n", buffer[0], opcode, size);

	wl_connection_write(conn, buffer, size);

	return CMD_CONTINUE_QUERY;
}

/* XXX keep sorted! (in the future I'd like to do
 * binary search in this array */
const struct command commands[] = {
	{"continue", "c", cmd_continue, NULL},
	//{"edit", "e", cmd_exit, NULL},
	{"help", "h",  cmd_help, cmd_help_help},
	{"info", "i", cmd_info, NULL},
	{"next", "n",  cmd_next, NULL},
	{"pass", NULL, cmd_pass, cmd_pass_help},
	{"run",  NULL, cmd_run, NULL},
	{"send", "s", cmd_send, NULL},
	{"quit", "q", cmd_quit, NULL},

};

static int
cmd_help(WLDBG_UNUSED struct wldbg_interactive *wldbgi,
		WLDBG_UNUSED struct message *message, char *buf)
{
	size_t i;
	int all = 0;

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
	size_t n;

	for (n = 0; n < (sizeof commands / sizeof *commands); ++n) {
		if (is_the_cmd(buf, &commands[n]))
			return commands[n].func(wldbgi, message, next_word(buf));
	}

	return CMD_DONT_MATCH;
}
