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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>

#include "wayland/wayland-private.h"

#include "wldbg-private.h"
#include "interactive.h"
#include "print.h"

static char *
store_message_to_tmpfile(struct message *message)
{
	int fd, ret;
	/* I don't suppose anybody would attack this program,
	 * so use dangerous function */
	char *file = tempnam(NULL, "wldbg-msg");
	if (!file) {
		perror("Creating tmp file for storing a message");
		return NULL;
	}

	vdbg("Created %s for storing message\n", file);

	fd = open(file, O_WRONLY | O_CREAT | O_EXCL, 0700);
	if (!fd) {
		perror("Opening tmp file for writing");
		free(file);
		return NULL;
	}

	ret = write(fd, message->data, message->size);
	if ((size_t) ret != message->size) {
		if (ret < 0)
			perror("writing data to tmp file");
		else
			fprintf(stderr, "Wrote less bytes than expected, taking"
					"it as an error (%d vs %lu)\n", ret,
					message->size);
		free(file);
		close(fd);
		return NULL;
	}

	close(fd);
	return file;
}

static int
read_message_from_tmpfile(char *file, struct message *message)
{
	int fd, ret;
	assert(file);

	fd = open(file, O_RDONLY);
	if (!fd) {
		perror("Opening tmp file for reading");
		return -1;
	}

	/* XXX do not use hard-coded numbers... */
	/* size of the buffer is 4096 atm */
	ret = read(fd, message->data, 4096);
	if (ret < 0) {
		perror("Reading tmp file\n");
		close(fd);
		return -1;
	}

	assert(ret <= 4096);
	message->size = ret;

	close(fd);
	return 0;
}

static void
destroy_message_tmpfile(char *file)
{
	if (unlink(file) < 0)
		perror("Deleting tmp file");
	free(file);
}

void
cmd_edit_help(int oneline)
{
	printf("Edit message");
	if (oneline)
		return;

	printf(" :: edit [editor]\n\n"
	       "Edit message wldbg stopped on. If no editor is given,\n"
	       "wldbg looks for one in EDITOR env. variable.\n");
}

int
cmd_edit(struct wldbg_interactive *wldbgi,
	 struct message *message,
	 char *buf)
{
	const char *editor;
	char *cmd, *msg_file, edstr[128];
	size_t size = 1024;
	int ret;

	(void) wldbgi;

	if (*buf != '\0') {
		sscanf(buf, "%127s", edstr);
		editor = edstr;
	} else
		editor = getenv("$EDITOR");

	/* XXX add inline editing (just dump the message to command line
	 * and edit it in place */
	if (!editor) {
		fprintf(stderr, "No edtor to use. Use 'edit editor_name' or "
				"set $EDITOR environment variable\n");
		return CMD_CONTINUE_QUERY;
	}

	msg_file = store_message_to_tmpfile(message);
	if (!msg_file)
		/* the error was written out in the function */
		return CMD_CONTINUE_QUERY;

	do {
		cmd = malloc(sizeof(char) * size);
		if (!cmd) {
			fprintf(stderr, "No memory");
			destroy_message_tmpfile(msg_file);
			return CMD_CONTINUE_QUERY;
		}

		ret = snprintf(cmd, size, "%s %s", editor, msg_file);
		if (ret < 0) {
			perror("Constituting a command");
			destroy_message_tmpfile(msg_file);
			free(cmd);
			return CMD_CONTINUE_QUERY;
		} else if ((size_t) ret >= size) { /* cmd string too small */
			size *= 2;
			free(cmd);
			cmd = NULL;
		}
	} while (!cmd);

	printf("executing: %s\n", cmd);
	/* XXX maybe use popen? */
	if (system(cmd) != 0) {
		fprintf(stderr, "Executing edit command has returned"
				" non-zero value");
	} else {
		read_message_from_tmpfile(msg_file, message);
	}

	destroy_message_tmpfile(msg_file);
	/* continue or end?? */
	return CMD_CONTINUE_QUERY;
}

