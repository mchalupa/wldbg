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


struct wldbg_object_info *
create_wl_seat_info(struct wldbg_resolved_message *rm);

static void
handle_bind(struct wldbg_objects_info *oi,
	    struct wldbg_resolved_message *rm, const char *obj)
{
	struct wldbg_object_info *info;

	if (strcmp(obj, "wl_seat") == 0) {
		info = create_wl_seat_info(rm);
		if (!info) {
			fprintf(stderr, "Out of memory, loosing informaiton\n");
			return;
		}

		assert(info->id > 0);
		objects_info_put(oi, info->id, info);
		dbg("Created wl_seat info, id %u\n", info->id);
	}
}

void
handle_wl_registry_message(struct wldbg_objects_info *oi,
			   struct wldbg_resolved_message *rm, int from)
{
	struct wldbg_resolved_arg *arg;

	if (from == CLIENT) {
		 if (strcmp(rm->wl_message->name, "bind") == 0) {
			/* skip the global id */
			wldbg_resolved_message_next_argument(rm);
			/* get the global name */
			arg = wldbg_resolved_message_next_argument(rm);

			handle_bind(oi, rm, (const char *) arg->data);
		}
	}
}

