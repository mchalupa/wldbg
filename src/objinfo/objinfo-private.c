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

#include "wldbg.h"
#include "wldbg-private.h"
#include "wldbg-ids-map.h"
#include "wldbg-objects-info.h"

/* for WL_SERVER_ID_START */
#include "wayland/wayland-private.h"

#include "objinfo-private.h"

void
objects_info_put(struct wldbg_objects_info *oi,
		 uint32_t id, struct wldbg_object_info *info)
{
	if (id >= WL_SERVER_ID_START)
		wldbg_ids_map_insert(&oi->server_objects,
				     id - WL_SERVER_ID_START, info);
	else
		wldbg_ids_map_insert(&oi->client_objects, id, info);
}

void *
objects_info_get(struct wldbg_objects_info *oi, uint32_t id)
{
	if (id >= WL_SERVER_ID_START)
		return wldbg_ids_map_get(&oi->server_objects,
					 id - WL_SERVER_ID_START);
	else
		return wldbg_ids_map_get(&oi->client_objects, id);
}

void
wldbg_object_info_free(struct wldbg_objects_info *oi, struct wldbg_object_info *info)
{
	objects_info_put(oi, info->id, NULL);

	if (info->destroy)
		info->destroy(info->info);
	free(info);
}

