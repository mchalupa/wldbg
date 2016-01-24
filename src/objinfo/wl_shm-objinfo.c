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
#include <assert.h>

#include "wldbg.h"
#include "wldbg-private.h"
#include "wldbg-parse-message.h"
#include "wldbg-objects-info.h"

#include "objinfo-private.h"

static struct wldbg_object_info *
create_wl_buffer_info(struct wldbg_resolved_message *rm)
{
	struct wldbg_object_info *oi = malloc(sizeof *oi);
	if (!oi)
		return NULL;

	struct wldbg_wl_buffer_info *info = calloc(1, sizeof *info);
	if (!info) {
		free(oi);
		return NULL;
	}

	/* types[0] should be wl_buffer_interface */
	oi->wl_interface = rm->wl_message->types[0];
	oi->info = info;
	oi->destroy = free;

	return oi;
}

void
handle_shm_pool_message(struct wldbg_objects_info *oi,
			struct wldbg_resolved_message *rm, int from)
{
	struct wldbg_object_info *info;
	struct wldbg_resolved_arg *arg;
	struct wldbg_wl_buffer_info *buff_info;

	if (from == CLIENT) {
		 if (strcmp(rm->wl_message->name, "create_buffer") == 0) {
			info = create_wl_buffer_info(rm);
			if (!info) {
				fprintf(stderr, "Out of memory, loosing informaiton\n");
				return;
			}

			buff_info = (struct wldbg_wl_buffer_info *) info->info;

			/* new wl_buffer id */
			arg = wldbg_resolved_message_next_argument(rm);
			info->id = *arg->data;

			/* offset */
			arg = wldbg_resolved_message_next_argument(rm);
			buff_info->offset = (int32_t) *arg->data;

			/* width */
			arg = wldbg_resolved_message_next_argument(rm);
			buff_info->width = (int32_t) *arg->data;

			/* height */
			arg = wldbg_resolved_message_next_argument(rm);
			buff_info->height = (int32_t) *arg->data;

			/* stride */
			arg = wldbg_resolved_message_next_argument(rm);
			buff_info->stride = (int32_t) *arg->data;

			/* format */
			arg = wldbg_resolved_message_next_argument(rm);
			buff_info->format = *arg->data;

			objects_info_put(oi, info->id, info);
			dbg("Created wl_buffer, id %u\n", info->id);
		}
	}
}

void
handle_wl_buffer_message(struct wldbg_objects_info *oi,
			 struct wldbg_resolved_message *rm, int from)
{
	struct wldbg_object_info *info = objects_info_get(oi, rm->base.id);
	if (!info) {
		fprintf(stderr, "ERROR: no wl_buffer with id %d\n", rm->base.id);
		return;
	}

	if (from == SERVER) {
		if (strcmp(rm->wl_message->name, "release") == 0) {
			struct wldbg_wl_buffer_info *buff_info = info->info;
			buff_info->released = 1;
		}
	} else {
		if (strcmp(rm->wl_message->name, "destroy") == 0) {
			wldbg_object_info_free(oi, info);
		}
	}
}

