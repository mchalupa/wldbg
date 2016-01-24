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

static void
destroy_wl_surface_info(void *data)
{
	struct wldbg_wl_surface_info *info = data;
	if (!info)
		return;

	destroy_wl_surface_info(info->commited);
	free(info);
}

static struct wldbg_object_info *
create_wl_surface_info(struct wldbg_resolved_message *rm)
{
	struct wldbg_object_info *oi = malloc(sizeof *oi);
	if (!oi)
		return NULL;

	struct wldbg_wl_surface_info *info = calloc(1, sizeof *info);
	if (!info) {
		free(oi);
		return NULL;
	}

	/* type of new id - wl_surface_interface */
	oi->wl_interface = rm->wl_message->types[0];
	oi->info = info;
	oi->destroy = destroy_wl_surface_info;

	return oi;
}

static void
make_wl_buffer_unreleased(struct wldbg_objects_info *oi, uint32_t id)
{
	struct wldbg_object_info *info = objects_info_get(oi, id);
	if (!info) {
		fprintf(stderr, "ERROR: no wl_buffer with id %d\n", id);
		return;
	}

	((struct wldbg_wl_buffer_info *) info->info)->released = 0;
}

void
handle_wl_surface_message(struct wldbg_objects_info *oi,
			  struct wldbg_resolved_message *rm, int from)
{
	struct wldbg_wl_surface_info *surf_info;
	struct wldbg_resolved_arg *arg;
	struct wldbg_object_info *info = objects_info_get(oi, rm->base.id);
	if (!info) {
		fprintf(stderr, "ERROR: no wl_surface that with id %d\n", rm->base.id);
		return;
	}

	surf_info = (struct wldbg_wl_surface_info *) info->info;
	if (from == CLIENT) {
		if (strcmp(rm->wl_message->name, "frame") == 0) {
			arg = wldbg_resolved_message_next_argument(rm);
			surf_info->last_frame_id = *arg->data;
		} else if (strcmp(rm->wl_message->name, "attach") == 0) {
			/* wl_buffer */
			arg = wldbg_resolved_message_next_argument(rm);
			surf_info->wl_buffer_id = *arg->data;
			/* attached x, y */
			arg = wldbg_resolved_message_next_argument(rm);
			surf_info->attached_x = *arg->data;
			arg = wldbg_resolved_message_next_argument(rm);
			surf_info->attached_y = *arg->data;

			make_wl_buffer_unreleased(oi, surf_info->wl_buffer_id);

		} else if (strcmp(rm->wl_message->name, "commit") == 0) {
			if (!surf_info->commited) {
				surf_info->commited = malloc(sizeof *surf_info);
				if (!surf_info->commited) {
					fprintf(stderr, "Out of memory, loosing info\n");
					return;
				}
			}

			/* commit the state */
			memcpy(surf_info->commited, surf_info, sizeof *surf_info);
			surf_info->commited->commited = NULL;

			/* reset current state */
			void *tmp = surf_info->commited;
			memset(surf_info, 0, sizeof *surf_info);
			surf_info->commited = tmp;
		} else if (strcmp(rm->wl_message->name, "destroy") == 0) {
			free(surf_info->commited);
			wldbg_object_info_free(oi, info);
		}
		/* TODO damage */
	}
}

void
handle_wl_compositor_message(struct wldbg_objects_info *oi,
			  struct wldbg_resolved_message *rm, int from)
{
	struct wldbg_object_info *info;
	struct wldbg_resolved_arg *arg;

	if (from == CLIENT) {
		if (strcmp(rm->wl_message->name, "create_surface") == 0) {
		       info = create_wl_surface_info(rm);
			if (!info) {
				fprintf(stderr,
					"Out of memory, loosing information\n");
				return;
			}

			/* new wl_surface id */
			arg = wldbg_resolved_message_next_argument(rm);
			info->id = *arg->data;

			objects_info_put(oi, info->id, info);
			dbg("Created wl_surface, id %u\n", info->id);
		}
	}
}

