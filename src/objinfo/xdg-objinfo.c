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
destroy_xdg_surface_info(void *info)
{
	struct wldbg_xdg_surface_info *i = info;
	free(i->title);
	free(info);
}

static struct wldbg_object_info *
create_xdg_surface_info(struct wldbg_resolved_message *rm)
{
	struct wldbg_object_info *oi = malloc(sizeof *oi);
	if (!oi)
		return NULL;

	struct wldbg_xdg_surface_info *info = calloc(1, sizeof *info);
	if (!info) {
		free(oi);
		return NULL;
	}

	/* types[0] should be xdg_surface_interface */
	oi->wl_interface = rm->wl_message->types[0];
	oi->info = info;
	oi->destroy = destroy_xdg_surface_info;

	return oi;
}

void
handle_xdg_shell_message(struct wldbg_objects_info *oi,
			 struct wldbg_resolved_message *rm, int from)
{
	struct wldbg_object_info *info;
	struct wldbg_resolved_arg *arg;

	if (from == CLIENT) {
		 if (strcmp(rm->wl_message->name, "get_xdg_surface") == 0) {
			info = create_xdg_surface_info(rm);
			if (!info) {
				fprintf(stderr, "Out of memory, loosing informaiton\n");
				return;
			}

			/* new xdg_surface id */
			arg = wldbg_resolved_message_next_argument(rm);
			/* FIXME check for NULL */
			info->id = *arg->data;

			/* just check -> do it an assertion */
			if (objects_info_get(oi, info->id))
				fprintf(stderr, "Already got an object on id %d, "
					"but now I'm creating xgd_surface\n", info->id);

			/* wl_surface id */
			arg = wldbg_resolved_message_next_argument(rm);
			((struct wldbg_xdg_surface_info *) info->info)->wl_surface_id = *arg->data;

			objects_info_put(oi, info->id, info);
			vdbg("Created xdg_surface, id %u\n", info->id);
		}
	}
}

void
handle_xdg_surface_message(struct wldbg_objects_info *oi,
			   struct wldbg_resolved_message *rm, int from)
{
	struct wldbg_xdg_surface_info *xdg_info;
	struct wldbg_resolved_arg *arg;
	struct wldbg_object_info *info = objects_info_get(oi, rm->base.id);
	if (!info) {
		fprintf(stderr, "ERROR: no xdg_surface with id %d\n", rm->base.id);
		return;
	}

	xdg_info = (struct wldbg_xdg_surface_info *) info->info;
	if (from == SERVER) {
		 if (strcmp(rm->wl_message->name, "configure") == 0) {
			uint8_t idx = xdg_info->configures_num % 10;
			struct xdg_configure *c = &xdg_info->configures[idx];

			/* width */
			arg = wldbg_resolved_message_next_argument(rm);
			c->width = *arg->data;
			/* height */
			arg = wldbg_resolved_message_next_argument(rm);
			c->height = *arg->data;
			/* flags */
			arg = wldbg_resolved_message_next_argument(rm);
			/* serial */
			arg = wldbg_resolved_message_next_argument(rm);
			c->serial = *arg->data;

			/* reset acked flag with this configure,
			 * we didn't get it yet */
			c->acked = 0;

			++xdg_info->configures_num;
		}
	} else {
		if (strcmp(rm->wl_message->name, "set_title") == 0) {
			arg = wldbg_resolved_message_next_argument(rm);
			if (arg && arg->data) {
				/* free on NULL is no-op */
				free(xdg_info->title);
				xdg_info->title = strdup((const char *) arg->data);
			}
		} else if (strcmp(rm->wl_message->name, "ack_configure") == 0) {
			arg = wldbg_resolved_message_next_argument(rm);
			uint32_t serial = *arg->data;
			uint8_t idx;

			/* find serial */
			for (idx = 0; idx < 10; ++idx) {
				struct xdg_configure *c = &xdg_info->configures[idx];
				if (c->serial == serial) {
					c->acked = 1;
					break;
				}
			}
		} else if (strcmp(rm->wl_message->name, "destroy") == 0) {
			wldbg_object_info_free(oi, info);
		}

	}
}


