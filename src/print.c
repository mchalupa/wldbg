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
#include <wayland-client-protocol.h>
#include <wayland-version.h>

#include "wldbg.h"
#include "wayland/wayland-util.h"
#include "wayland/wayland-private.h"
#include "interactive/interactive.h"
#include "wldbg-private.h"
#include "wldbg-parse-message.h"
#include "resolve.h"
#include "util.h"

/* is wayland version greater or equal to given version? */
#define WAYLAND_VERSION_GE(maj, min, mic)\
    (((WAYLAND_VERSION_MAJOR > (maj))) ||\
     ((WAYLAND_VERSION_MAJOR == (maj)) &&\
      (WAYLAND_VERSION_MINOR > (min))) ||\
     ((WAYLAND_VERSION_MAJOR == (maj)) &&\
      (WAYLAND_VERSION_MINOR == (min)) &&\
      (WAYLAND_VERSION_MICRO >= (mic))))

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
		/* pos == 2 is serial, pos == 6 is group (layout) */
		if (pos == 2 || pos == 6 || p == 0)
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

static int
print_wl_seat_message(const struct wl_interface *wl_interface,
		      const struct wl_message *wl_message, uint32_t p)
{
	int n = 0;
	if (strcmp(wl_interface->name, "wl_seat") != 0)
		return 0;

	if (strcmp(wl_message->name, "capabilities") == 0) {
		if (p & WL_SEAT_CAPABILITY_KEYBOARD) {
			printf("keyboard");
			n = 1;
		}

		if (p & WL_SEAT_CAPABILITY_POINTER) {
			printf("%spointer", n ? " | " : "");
			n = 1;
		}

		if (p & WL_SEAT_CAPABILITY_TOUCH) {
			printf("%stouch", n ? " | " : "");
			n = 1;
		}

		if (!n)
			printf("none");

		return 1;
	}

	return 0;
}

/* The actions were introduced in 1.9.91 */
#if WAYLAND_VERSION_GE(1, 9, 91)
static void
print_actions(uint32_t act)
{
	if (act == WL_DATA_DEVICE_MANAGER_DND_ACTION_NONE) {
		printf("none");
		return;
	}

	int n = 0;
	if (act & WL_DATA_DEVICE_MANAGER_DND_ACTION_COPY) {
		printf("copy");
		n = 1;
	}

	if (act & WL_DATA_DEVICE_MANAGER_DND_ACTION_MOVE) {
		printf("%smove", n ? "|" : "");
		n = 1;
	}

	if (act & WL_DATA_DEVICE_MANAGER_DND_ACTION_ASK) {
		printf("%sask", n ? "|" : "");
	}
}

static int
print_wl_data_source_message(const struct wl_interface *wl_interface,
			     const struct wl_message *wl_message,
			     int pos, uint32_t p)
{
	if (strcmp(wl_interface->name, "wl_data_source") != 0)
		return 0;

	if (strcmp(wl_message->name, "action") == 0) {
		assert(pos == 0 && "action event has only one argument");

		print_actions(p);
		return 1;
	} else if (strcmp(wl_message->name, "set_actions") == 0) {
		assert(pos == 0 && "set_actions has only one argument");

		print_actions(p);
		return 1;
	}

	return 0;
}

static int
print_wl_data_offer_message(const struct wl_interface *wl_interface,
			    const struct wl_message *wl_message,
			    int pos, uint32_t p)
{
	if (strcmp(wl_interface->name, "wl_data_offer") != 0)
		return 0;

	if (strcmp(wl_message->name, "action") == 0) {
		assert(pos == 0 && "action event has only one argument");

		print_actions(p);
		return 1;
	} else if (strcmp(wl_message->name, "set_actions") == 0) {
		assert(pos <= 1 && "set_actions has max two arguments");

		print_actions(p);
		return 1;
	} else if (strcmp(wl_message->name, "source_actions") == 0) {
		assert(pos == 0 && "this event has only one argument");

		print_actions(p);
		return 1;
	}

	return 0;
}
#endif /* WAYLAND_VERSION >= 1.9.91 */


/* we have whole xdg-surface hardcoded now...
 * FIXME */
#ifndef XDG_SURFACE_STATE_ENUM
#define XDG_SURFACE_STATE_ENUM
enum xdg_surface_state {
	XDG_SURFACE_STATE_MAXIMIZED = 1,
	XDG_SURFACE_STATE_FULLSCREEN = 2,
	XDG_SURFACE_STATE_RESIZING = 3,
	XDG_SURFACE_STATE_ACTIVATED = 4,
};
#endif /* XDG_SURFACE_STATE_ENUM */

static int
print_xdg_surface_message(const struct wl_interface *wl_interface,
			  const struct wl_message *wl_message, int pos,
			  uint32_t *data, size_t len)
{
	int n;
	uint32_t i;

	if (strcmp(wl_interface->name, "xdg_surface") != 0)
		return 0;

	if (pos == 2 && strcmp(wl_message->name, "configure") == 0) {
		/* print human readable configure states */
		n = 0;
		for (i = 0; i < len; ++i) {

			switch(data[i]) {
			case XDG_SURFACE_STATE_MAXIMIZED:
				printf("%smaximized", n++ ? "|" : "");
				break;
			case XDG_SURFACE_STATE_FULLSCREEN:
				printf("%sfullscreen", n++ ? "|" : "");
				break;
			case XDG_SURFACE_STATE_RESIZING:
				printf("%sresizing", n++ ? "|" : "");
				break;
			case XDG_SURFACE_STATE_ACTIVATED:
				printf("%sactivated", n++ ? "|" : "");
				break;
			default:
				printf("%sunknown", n++ ? "|" : "");
			}
		}

		if (n == 0)
			printf("none");

		return 1;
	}

	return 0;
}

static void
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

static inline void
print_id(uint32_t id)
{
	if (id >= WL_SERVER_ID_START)
		printf("SRV%d", id - WL_SERVER_ID_START);
	else
		printf("%d", id);
}

static void
print_arg(struct wldbg_resolved_arg *arg, struct wldbg_resolved_message *rm,
	  uint32_t pos, struct wldbg_message *message)
{
	const struct wl_interface *obj;
	size_t len;

	switch (arg->type) {
	case 'u':
		/* FIXME: do this more efficient. We're also
		 * comparing strings in these functions */
		if (print_wl_keyboard_message(rm->wl_interface,
					      rm->wl_message, pos + 2,
					      *arg->data))
			break;

		if (print_wl_seat_message(rm->wl_interface,
					  rm->wl_message, *arg->data))
			break;

#if WAYLAND_VERSION_GE(1, 9, 91)
		if (print_wl_data_source_message(rm->wl_interface,
						 rm->wl_message, pos,
						 *arg->data))
			break;

		if (print_wl_data_offer_message(rm->wl_interface,
						rm->wl_message, pos,
						*arg->data))
			break;
#endif /* WAYLAND_VERSION >= 1.9.91 */

		/* nothing worked? Then it is just a number */
		printf("%u", *arg->data);
		break;
	case 'i':
		printf("%d", (int32_t) *arg->data);
		break;
	case 'f':
		printf("%f", wl_fixed_to_double(*arg->data));
		break;
	case 's':
		if (arg->data)
			printf("%u:\"%s\"", *(arg->data - 1),
			       (const char *) (arg->data));
		else
			printf("0:\"\"");
		break;
	case 'o':
		obj = wldbg_message_get_object(message, *arg->data);
		if (obj) {
			printf("%s@", obj->name);
			print_id(*arg->data);
		} else
			printf("nil");
		break;
	case 'n':
		printf("new id %s@", rm->wl_message->types[pos] ?
			rm->wl_message->types[pos]->name : "[unknown]");

		if (*arg->data != 0)
			print_id(*arg->data);
		else
			printf("nil");
		break;
	case 'a':
		if (arg->data)
			len = DIV_ROUNDUP(*(arg->data - 1), sizeof(uint32_t));
		else
			len = 0;

		if (len && print_xdg_surface_message(rm->wl_interface,
						     rm->wl_message, pos,
						     arg->data, len))
			break;

		printf("array:");
		print_array(arg->data, len, 8);
		break;
	case 'h':
		printf("fd");
		break;
	}
}

void
wldbg_message_print(struct wldbg_message *message)
{
	int is_buggy = 0;
	uint32_t pos;
	struct wldbg_connection *conn = message->connection;
	struct wldbg_resolved_message rm;
	struct wldbg_resolved_arg *arg;

	if (conn->wldbg->flags.server_mode) {
		if (conn->client.program)
			printf("[%-*s |%-5d] ",
			       conn->wldbg->server_mode.client_name_width,
			       conn->client.program,
			       conn->client.pid);
		else
			printf("[? |%-5d] ", conn->client.pid);
	}

	printf("%c: ", message->from == SERVER ? 'S' : 'C');

	if (!wldbg_resolve_message(message, &rm)) {
		if (!wldbg_parse_message(message, &rm.base)) {
			printf("_failed_parsing_message_\n");
			return;
		}

		printf("unknown@");
		print_id(rm.base.id);
		printf(".[opcode %u][size %uB]\n", rm.base.opcode, rm.base.size);
		return;
	}

	if (message->from == SERVER) {
		if ((uint32_t) rm.wl_interface->event_count <= rm.base.opcode)
			is_buggy = 1;
	} else {
		if ((uint32_t) rm.wl_interface->method_count <= rm.base.opcode)
			is_buggy = 1;
	}

	printf("%s@", rm.wl_interface->name);
	print_id(rm.base.id);
	putchar('.');

	/* catch buggy events/requests. We don't want them to make
	 * wldbg crash. This means probably protocol versions mismatch */
	if (is_buggy) {
		printf("_buggy %s_",
			message->from == SERVER ? "event" : "request");
		printf("[opcode %u][size %uB]\n", rm.base.opcode, rm.base.size);
		return;
	} else {
		printf("%s(", rm.wl_message->name);
	}


	pos = 0;
	while((arg = wldbg_resolved_message_next_argument(&rm))) {
		if (pos > 0)
			printf(", ");

		/* currently we can have up to WL_CLOSURE_MAX_ARGS,
		 * but if that changes we must update wayland-private.h */
		if (pos >= WL_CLOSURE_MAX_ARGS) {
			/* be kind to user... for now */
			fprintf(stderr, "Probably wrong %s, too much arguments\n",
				message->from == SERVER ? "event" : "request");
			fflush(stdout);
			break;
		}

		print_arg(arg, &rm, pos, message);
		++pos;
	}

	printf(")\n");
}

