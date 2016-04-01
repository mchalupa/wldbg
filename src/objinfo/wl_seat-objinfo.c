/*
 * Copyright (c) 2016 Marek Chalupa
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
#include <assert.h>

#include "wldbg.h"
#include "wldbg-private.h"
#include "wldbg-parse-message.h"
#include "wldbg-objects-info.h"

#include "objinfo-private.h"

static void
free_seat(void *ptr)
{
	struct wldbg_wl_seat_info *info = ptr;
	free(info->name);
	free(info);
}

struct wldbg_object_info *
create_wl_seat_info(struct wldbg_resolved_message *rm)
{
	struct wldbg_resolved_arg *arg;

	struct wldbg_object_info *oi = malloc(sizeof *oi);
	if (!oi)
		return NULL;

	struct wldbg_wl_seat_info *info = malloc(sizeof *info);
	if (!info) {
		free(oi);
		return NULL;
	}

	/* XXX: is this always valid */
	extern const struct wl_interface wl_seat_interface;
	oi->wl_interface = &wl_seat_interface;
	oi->info = info;
	oi->destroy = free_seat;

	info->capabilities = 0;

	/* get version of the seat */
	arg = wldbg_resolved_message_next_argument(rm);
	oi->version = *arg->data;

	/* get id of the seat */
	arg = wldbg_resolved_message_next_argument(rm);
	oi->id = *arg->data;

	return oi;
}

void
handle_wl_seat_message(struct wldbg_objects_info *oi,
		       struct wldbg_resolved_message *rm, int from)
{
	struct wldbg_object_info *i;
	struct wldbg_resolved_arg *arg;
	struct wldbg_wl_seat_info *info;

	i = objects_info_get(oi, rm->base.id);
	if (!i) {
		fprintf(stderr, "ERROR: no wl_seat with id %d\n", rm->base.id);
		return;
	}

	info = i->info;

	if (from == SERVER) {
		if (strcmp(rm->wl_message->name, "name") == 0) {
			arg = wldbg_resolved_message_next_argument(rm);
			if (arg->data)
				info->name = strdup((const char *) arg->data);
		} else if (strcmp(rm->wl_message->name, "capabilities") == 0) {
			arg = wldbg_resolved_message_next_argument(rm);
			info->capabilities = *arg->data;
		}
	}
}

