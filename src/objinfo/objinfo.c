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

/* for WL_SERVER_ID_START */
#include "wayland/wayland-private.h"

#include "wldbg.h"
#include "wldbg-private.h"
#include "wldbg-ids-map.h"


struct wldbg_objects_info *
create_objects_info(void)
{
	struct wldbg_objects_info *oi = malloc(sizeof *oi);
	if (!oi) {
		fprintf(stderr, "Out of memory\n");
		return NULL;
	}

	wldbg_ids_map_init(&oi->client_objects);
	wldbg_ids_map_init(&oi->server_objects);

	return oi;
}

void
destroy_objects_info(struct wldbg_objects_info *oi)
{
	unsigned int i;
	struct wldbg_object_info *info;

	for (i = 0; i < oi->client_objects.count; ++i) {
		info = wldbg_ids_map_get(&oi->client_objects, i);
		if (!info)
			continue;

		if (info->destroy)
			info->destroy(info->info);
		free(info);
	}

	for (i = 0; i < oi->server_objects.count; ++i) {
		info = wldbg_ids_map_get(&oi->server_objects, i);
		if (!info)
			continue;

		if (info->destroy)
			info->destroy(info->info);
		free(info);
	}

	wldbg_ids_map_release(&oi->client_objects);
	wldbg_ids_map_release(&oi->server_objects);

	free(oi);
}

static void *
wldbg_objects_info_get(struct wldbg_objects_info *oi, uint32_t id)
{
	if (id >= WL_SERVER_ID_START)
		return wldbg_ids_map_get(&oi->server_objects,
					 id - WL_SERVER_ID_START);
	else
		return wldbg_ids_map_get(&oi->client_objects, id);
}

struct wldbg_object_info *
wldbg_message_get_object_info(struct wldbg_message *msg, uint32_t id)
{
	struct wldbg_objects_info *oi = msg->connection->objects_info;
	if (!oi)
		return NULL;

	return wldbg_objects_info_get(oi, id);
}
