#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "interactive.h"
#include "input.h"
#include "util.h"

#define WLDBG_PROMPT "(wldbg)"

#ifdef HAVE_LIBREADLINE

#include <readline/readline.h>
#include <readline/history.h>

char *
wldbgi_read_input(void)
{
	return readline(WLDBG_PROMPT " ");
}

#else /* HAVE_LIBREADLINE */

char *
wldbgi_read_input(void)
{
	#define DEFAULT_BUFFER_SIZE 1024
	char *buf = malloc(DEFAULT_BUFFER_SIZE);
	if (!buf) {
		fprintf(stderr, "Out of memory\n");
		return NULL;
	}

	printf(WLDBG_PROMPT " ");
	if (fgets(buf, DEFAULT_BUFFER_SIZE, stdin) == NULL) {
		free(buf);
		return NULL;
	}

	return remove_newline(buf);
}
#endif /* HAVE_LIBREADLINE */

char *
wldbgi_get_last_command(struct wldbg_interactive *wldbgi)
{
#if HAVE_READLINE_HISTORY
	(void) wldbgi;

	HIST_ENTRY *ent = history_get(where_history());
	if (ent)
		return ent->line;

	return NULL;
#else
	return wldbgi->last_command;
#endif /* HAVE_READLINE_HISTORY */
}

void
wldbgi_add_history(struct wldbg_interactive *wldbgi, const char *cmd)
{
	(void) wldbgi;
#if HAVE_READLINE_HISTORY
	add_history(cmd);
#else
	free(wldbgi->last_command);
	wldbgi->last_command = strdup(cmd);
#endif
}

void
wldbgi_clear_history(struct wldbg_interactive *wldbgi)
{
	(void) wldbgi;
#if HAVE_READLINE_HISTORY
	clear_history();
#else
	free(wldbgi->last_command);
	wldbgi->last_command = NULL;
#endif
}

