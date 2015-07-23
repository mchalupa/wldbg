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
#include <assert.h>
#include <ctype.h>
#include <regex.h>

#include <linux/input.h>

#include "wldbg.h"
#include "wayland/wayland-util.h"
#include "util.h"
#include "print.h"
#include "interactive/interactive.h"
#include "wldbg-private.h"
#include "resolve.h"
#include "wldbg-parse-message.h"

size_t
wldbg_get_message_name(struct message *message, char *buf, size_t maxsize)
{
	struct resolved_objects *ro = message->connection->resolved_objects;
	struct wldbg_parsed_message pm;
	const struct wl_interface *interface;
	const struct wl_message *wl_message = NULL;
	int ret;
	size_t written;

	wldbg_parse_message(message, &pm);
	interface = resolved_objects_get(ro, pm.id);

	/* put the interface name into the buffer */
	ret = snprintf(buf, maxsize, "%s",
		       interface ? interface->name : "unknown");
	written = ret;
	if (ret >= (int) maxsize)
		return written;

	/* create name of the message we got */
	if (wl_message) {
		ret = snprintf(buf + ret, maxsize - written,
			       "@%d.%s", pm.id, wl_message->name);

	} else {
		ret = snprintf(buf + ret, maxsize - written,
			       "@%d.%d", pm.id, pm.opcode);
	}

	written += ret;

	// return how many characters we wrote or how
	// many characters we'd wrote in the case of overflow
	return written;
}

/* return 1 if some of filters matches - thus hide the message */
static int
filter_match(struct wl_list *filters, struct message *message)
{
	struct print_filter *pf;
	char buf[128];
	int ret, has_show_only = 0;

	wl_list_for_each(pf, filters, link) {
		ret = wldbg_get_message_name(message, buf, sizeof buf);
		if (ret >= (int) sizeof buf) {
			fprintf(stderr, "BUG: buffer too small for message name\n");
			continue;
		}

		/* run regular expression */
		ret = regexec(&pf->regex, buf, 0, NULL, 0);

		/* got we match? */
		if (ret == 0) {
			/* If this filter is show_only,
			 * we must return 0, because we'd like to show this message */
			if (pf->show_only)
				return 0;

			return 1;
		} else if (ret != REG_NOMATCH) {
			fprintf(stderr, "Executing regexp failed!\n");
			return 0;
		}

		if (pf->show_only)
			has_show_only = 1;
	}

	/* if we haven't found filter match and we have some show_only filters,
	 * we must return 1 so that this message will get hidden */
	if (has_show_only)
		return 1;

	return 0;
}

static void
print_key(uint32_t p)
{
#define CASE(k) case KEY_##k: printf("'%s'", #k); break;

	switch (p) {
		CASE(RESERVED)
		CASE(ESC)
		CASE(1)
		CASE(2)
		CASE(3)
		CASE(4)
		CASE(5)
		CASE(6)
		CASE(7)
		CASE(8)
		CASE(9)
		CASE(0)
		CASE(MINUS)
		CASE(EQUAL)
		CASE(BACKSPACE)
		CASE(TAB)
		CASE(Q)
		CASE(W)
		CASE(E)
		CASE(R)
		CASE(T)
		CASE(Y)
		CASE(U)
		CASE(I)
		CASE(O)
		CASE(P)
		CASE(LEFTBRACE)
		CASE(RIGHTBRACE)
		CASE(ENTER)
		CASE(LEFTCTRL)
		CASE(A)
		CASE(S)
		CASE(D)
		CASE(F)
		CASE(G)
		CASE(H)
		CASE(J)
		CASE(K)
		CASE(L)
		CASE(SEMICOLON)
		CASE(APOSTROPHE)
		CASE(GRAVE)
		CASE(LEFTSHIFT)
		CASE(BACKSLASH)
		CASE(Z)
		CASE(X)
		CASE(C)
		CASE(V)
		CASE(B)
		CASE(N)
		CASE(M)
		CASE(COMMA)
		CASE(DOT)
		CASE(SLASH)
		CASE(RIGHTSHIFT)
		CASE(KPASTERISK)
		CASE(LEFTALT)
		CASE(SPACE)
		CASE(CAPSLOCK)
		CASE(F1)
		CASE(F2)
		CASE(F3)
		CASE(F4)
		CASE(F5)
		CASE(F6)
		CASE(F7)
		CASE(F8)
		CASE(F9)
		CASE(F10)
		CASE(NUMLOCK)
		CASE(SCROLLLOCK)
		CASE(KP7)
		CASE(KP8)
		CASE(KP9)
		CASE(KPMINUS)
		CASE(KP4)
		CASE(KP5)
		CASE(KP6)
		CASE(KPPLUS)
		CASE(KP1)
		CASE(KP2)
		CASE(KP3)
		CASE(KP0)
		CASE(KPDOT)

		CASE(ZENKAKUHANKAKU)
		CASE(102ND)
		CASE(F11)
		CASE(F12)
		CASE(RO)
		CASE(KATAKANA)
		CASE(HIRAGANA)
		CASE(HENKAN)
		CASE(KATAKANAHIRAGANA)
		CASE(MUHENKAN)
		CASE(KPJPCOMMA)
		CASE(KPENTER)
		CASE(RIGHTCTRL)
		CASE(KPSLASH)
		CASE(SYSRQ)
		CASE(RIGHTALT)
		CASE(LINEFEED)
		CASE(HOME)
		CASE(UP)
		CASE(PAGEUP)
		CASE(LEFT)
		CASE(RIGHT)
		CASE(END)
		CASE(DOWN)
		CASE(PAGEDOWN)
		CASE(INSERT)
		CASE(DELETE)
		CASE(MACRO)
		CASE(MUTE)
		CASE(VOLUMEDOWN)
		CASE(VOLUMEUP)
		CASE(POWER)
		CASE(KPEQUAL)
		CASE(KPPLUSMINUS)
		CASE(PAUSE)
		CASE(SCALE)

		CASE(LEFTMETA)
		CASE(RIGHTMETA)
		CASE(COMPOSE)

		CASE(STOP)
		CASE(AGAIN)
		CASE(PROPS)
		CASE(UNDO)
		CASE(FRONT)
		CASE(COPY)
		CASE(OPEN)
		CASE(PASTE)
		CASE(FIND)
		CASE(CUT)
		CASE(HELP)
		CASE(MENU)
		CASE(CALC)
		CASE(SETUP)
		CASE(SLEEP)
		CASE(WAKEUP)
		CASE(FILE)
		CASE(SENDFILE)
		CASE(DELETEFILE)
		CASE(XFER)
		CASE(PROG1)
		CASE(PROG2)
		CASE(WWW)
		CASE(MSDOS)
		CASE(SCREENLOCK)
		CASE(DIRECTION)
		CASE(CYCLEWINDOWS)
		CASE(MAIL)
		CASE(BOOKMARKS)
		CASE(COMPUTER)
		CASE(BACK)
		CASE(FORWARD)
		CASE(CLOSECD)
		CASE(EJECTCD)
		CASE(EJECTCLOSECD)
		CASE(NEXTSONG)
		CASE(PLAYPAUSE)
		CASE(PREVIOUSSONG)
		CASE(STOPCD)
		CASE(RECORD)
		CASE(REWIND)
		CASE(PHONE)
		CASE(ISO)
		CASE(CONFIG)
		CASE(HOMEPAGE)
		CASE(REFRESH)
		CASE(EXIT)
		CASE(MOVE)
		CASE(EDIT)
		CASE(SCROLLUP)
		CASE(SCROLLDOWN)
		CASE(KPLEFTPAREN)
		CASE(KPRIGHTPAREN)
		CASE(NEW)
		CASE(REDO)

		CASE(F13)
		CASE(F14)
		CASE(F15)
		CASE(F16)
		CASE(F17)
		CASE(F18)
		CASE(F19)
		CASE(F20)
		CASE(F21)
		CASE(F22)
		CASE(F23)
		CASE(F24)

		CASE(PLAYCD)
		CASE(PAUSECD)
		CASE(PROG3)
		CASE(PROG4)
		CASE(DASHBOARD)
		CASE(SUSPEND)
		CASE(CLOSE)
		CASE(PLAY)
		CASE(FASTFORWARD)
		CASE(BASSBOOST)
		CASE(PRINT)
		CASE(HP)
		CASE(CAMERA)
		CASE(SOUND)
		CASE(QUESTION)
		CASE(EMAIL)
		CASE(CHAT)
		CASE(SEARCH)
		CASE(CONNECT)
		CASE(FINANCE)
		CASE(SPORT)
		CASE(SHOP)
		CASE(ALTERASE)
		CASE(CANCEL)
		CASE(BRIGHTNESSDOWN)
		CASE(BRIGHTNESSUP)
		CASE(MEDIA)

		CASE(SWITCHVIDEOMODE)
		CASE(KBDILLUMTOGGLE)
		CASE(KBDILLUMDOWN)
		CASE(KBDILLUMUP)

		CASE(SEND)
		CASE(REPLY)
		CASE(FORWARDMAIL)
		CASE(SAVE)
		CASE(DOCUMENTS)

		CASE(BATTERY)

		CASE(BLUETOOTH)
		CASE(WLAN)
		CASE(UWB)

		CASE(UNKNOWN)
		default: printf("%u", p);
	}

#undef CASE
}

static const char *MODIFIERS[] =
{
	"SHIFT",
	"CAPS",
	"CTRL",
	"ALT",
	"MOD2",
	"MOD3",
	"MOD4",
	"MOD5"
};

static void
print_modifiers(uint32_t p)
{
	unsigned int i, printed = 0;
	for (i = 0; i < (8 * sizeof p); ++i) {
		if (p & (1U << i)) {
			if (i < (sizeof MODIFIERS / sizeof *MODIFIERS))
				printf("%s%s", printed++ ? "|" : "", MODIFIERS[i]);
			else
				printf("%s0x%x", printed++ ? "|" : "", 1U << i);
		}
	}
}

static int
print_wl_keyboard_message(const struct wl_interface *wl_interface,
			  const struct wl_message *wl_message, int pos, uint32_t p)
{
	if (strcmp(wl_interface->name, "wl_keyboard") != 0)
		return 0;

	if (strcmp(wl_message->name, "modifiers") == 0) {
		if (pos == 2 || p == 0)
			return 0;

		print_modifiers(p);
		return 1;
	} else if (strcmp(wl_message->name, "key") == 0) {
		/* state */
		if (pos == 5) {
			printf("%s", p == 1 ? "press" : "release");
			return 1;
		/* key */
		} else if (pos == 4) {
			print_key(p);
			return 1;
		}
	}

	return 0;
}

void
print_array(uint32_t *p, size_t len, size_t howmany)
{
	size_t j;

	if (len == 0)
		printf("(nil)");
	else {
		putchar('[');

		/* print max first howmany elements from array */
		for (j = 0; j < howmany && j < len; ++j) {
			if (j > 0)
				putchar(' ');

			printf("%04x", *(p + j));
		}

		if (len > j)
			printf(" ...");

		putchar(']');
	}
}

/* roughly based on wl_closure_print from connection.c */
void
print_bare_message(struct message *message, struct wl_list *filters)
{
	int i, is_buggy = 0;
	uint32_t pos, len, *p;
	const struct wl_interface *interface, *obj;
	const char *signature;
	const struct wl_message *wl_message = NULL;
	struct wldbg_connection *conn = message->connection;
	struct wldbg_parsed_message pm;

	assert(conn->wldbg->flags.one_by_one
		&& "This function is meant to be used in one-by-one mode");

	wldbg_parse_message(message, &pm);
	p = message->data;
	interface = resolved_objects_get(conn->resolved_objects, pm.id);

	if (filters && filter_match(filters, message))
		return;

	if (conn->wldbg->flags.server_mode) {
		if (conn->client.program)
			printf("[%-15s] ", conn->client.program);
		else
			printf("[%-5d] ", conn->client.pid);
	}

	printf("%c: ", message->from == SERVER ? 'S' : 'C');

	/* if we do not know interface or the interface is
	 * unknown_interface or free_entry */
	if (!interface
		|| (message->from == SERVER && !interface->event_count)
		|| (message->from == CLIENT && !interface->method_count)) {
		/* print at least fall-back description */
		printf("unknown@%u.[opcode %u][size %uB]\n",
		       pm.id, pm.opcode, pm.size);
		return;
	}

	if (message->from == SERVER) {
		if ((uint32_t) interface->event_count <= pm.opcode)
			is_buggy = 1;

		wl_message = &interface->events[pm.opcode];
	} else {
		if ((uint32_t) interface->method_count <= pm.opcode)
			is_buggy = 1;

		wl_message = &interface->methods[pm.opcode];
	}

	printf("%s@%u.", interface->name, pm.id);

	/* catch buggy events/requests. We don't want them to make
	 * wldbg crash */
	if (!wl_message || !wl_message->signature || is_buggy) {
		printf("_buggy %s_",
			message->from == SERVER ? "event" : "request");
		printf("[opcode %u][size %uB]\n", pm.opcode, pm.size);
		return;
	} else {
		printf("%s(", wl_message->name);
	}


	/* 2 is position of the first argument */
	pos = 2;
	signature = wl_message->signature;
	for (i = 0; signature[i] != '\0'; ++i) {
		if (signature[i] == '?' || isdigit(signature[i]))
			continue;

		if (pos > 2)
			printf(", ");

		/* buffer has 4096 bytes and position jumps over 4 bytes */
		if (pos >= 1024) {
			/* be kind to user... for now */
			fflush(stdout);
			fprintf(stderr, "Probably wrong %s, expect crash\n",
				message->from == SERVER ? "event" : "request");
			break;
		}

		switch (signature[i]) {
		case 'u':
			if (!print_wl_keyboard_message(interface,
						       wl_message, pos, p[pos]))
				printf("%u", p[pos]);
			break;
		case 'i':
			printf("%d", p[pos]);
			break;
		case 'f':
			printf("%f", wl_fixed_to_double(p[pos]));
			break;
		case 's':
			if (p[pos] > 0)
				printf("%u:\"%s\"", p[pos],
				       (const char *) (p + pos + 1));
			else
				printf("%u:\"\"", p[pos]);

			pos += DIV_ROUNDUP(p[pos], sizeof(uint32_t));
			break;
		case 'o':
			obj = resolved_objects_get(conn->resolved_objects,
						   p[pos]);
			if (obj)
				printf("%s@%u", obj->name, p[pos]);
			else
				printf("nil");
			break;
		case 'n':
			printf("new id %s@",
				(wl_message->types[i]) ?
					wl_message->types[i]->name :
					"[unknown]");
			if (p[pos] != 0)
				printf("%u", p[pos]);
			else
				printf("nil");
			break;
		case 'a':
			len = DIV_ROUNDUP(p[pos], sizeof(uint32_t));

			printf("array:");
			print_array(p + pos, len, 8);

			pos += len;
			break;
		case 'h':
			printf("fd");
			break;
		}

		++pos;
	}

	printf(")\n");
}

void
wldbgi_print_message(struct wldbg_interactive *wldbgi, struct message *message, int force)
{
	if (force)
		print_bare_message(message, NULL);
	else
		print_bare_message(message, &wldbgi->print_filters);
}
