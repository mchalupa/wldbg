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

#include <stdio.h>
#include <assert.h>

#include "wayland/wayland-private.h"

#include "interactive.h"
#include "wldbg-private.h"
#include "objinfo/objinfo-private.h"
#include "wldbg-objects-info.h"
#include "util.h"

static void
print_wl_buffer_info(struct wldbg_wl_buffer_info *info, int ind)
{
	printf("%*s-- wl_buffer --\n", ind, "");
	printf("%*s  offset: %d\n", ind, "", info->offset);
	printf("%*s  size: %dx%d\n", ind, "", info->width, info->height);
	printf("%*s  stride: %d\n", ind, "", info->stride);
	printf("%*s  format: %u\n", ind, "", info->format);
}

static void
print_wl_surface_info(struct wldbg_objects_info *oi,
		      struct wldbg_wl_surface_info *info,
		      int ind)
{
	printf("%*s-- wl_surface --\n", ind, "");
	printf("%*sCurrent / pending ->\n\n", ind, "");
	printf("%*s  wl_buffer: %u\n", ind, "", info->wl_buffer_id);
	printf("%*s  attached to: %ux%u\n", ind, "",
					    info->attached_x, info->attached_y);
	printf("%*s  buffer scale: %u\n", ind, "", info->buffer_scale);
	printf("%*s  last frame id: %u\n", ind, "", info->last_frame_id);

	if (info->wl_buffer_id != 0) {
		printf("\n%*sAttached buffer ->\n", ind, "");
		struct wldbg_object_info *bi
			= objects_info_get(oi, info->wl_buffer_id);
		if (!bi)
			printf("%*sNO BUFFER!\n", ind, "");
		else
			print_wl_buffer_info(bi->info, ind + 2);
	}

	if (info->commited) {
		printf("\n%*sCommited ->\n", ind, "");
		print_wl_surface_info(oi, info->commited, ind + 2);
	}
}


static void
print_xdg_surface_info(struct wldbg_objects_info *oi,
		       struct wldbg_xdg_surface_info *xdg_info)
{
	uint8_t i = xdg_info->configures_num % 10;
	uint8_t ord = 0;

	printf(" -- xdg_surface --\n");
	printf("Tile: '%s'\n", xdg_info->title);
	printf("App id: '%s'\n", xdg_info->app_id);
	printf("Last 10 configures (out of %lu):\n", xdg_info->configures_num);
	for (; ord < 10; ++i, ++ord) {
		i = i % 10;
		printf("  [%-2u]: width: %5u, height: %5u,"
		       " state: -- , serial: %5u, acked: %u\n",
		       ord + 1, xdg_info->configures[i].width,
		       xdg_info->configures[i].height,
		       xdg_info->configures[i].serial,
		       xdg_info->configures[i].acked);
	}

	struct wldbg_object_info *info
		= objects_info_get(oi, xdg_info->wl_surface_id);
	assert (info && "has no wl_surface info in xdg_surface");

	print_wl_surface_info(oi, info->info, 2);
}

static void
print_objinfo(struct wldbg_objects_info *oi, struct wldbg_object_info *info)
{
	const char *name = info->wl_interface->name;
	if (strcmp(name, "xdg_surface") == 0) {
		print_xdg_surface_info(oi, info->info);
	} else if (strcmp(name, "wl_buffer") == 0) {
		print_wl_buffer_info(info->info, 0);
	} else if (strcmp(name, "wl_surface") == 0) {
		print_wl_surface_info(oi, info->info, 0);
	} else {
		fprintf(stderr, "Unhandled objinfo: %u\n", info->id);
	}
}

static void
print_all_objinfo(struct wldbg_objects_info *oi)
{

	unsigned int i;
	struct wldbg_object_info *info;

	for (i = 0; i < oi->client_objects.count; ++i) {
		info = wldbg_ids_map_get(&oi->client_objects, i);
		if (info)
			print_objinfo(oi, info);
	}

	for (i = 0; i < oi->server_objects.count; ++i) {
		info = wldbg_ids_map_get(&oi->server_objects, i);
		if (info)
			print_objinfo(oi, info);
	}
}

void
print_object_info(struct wldbg_message *msg, char *buf)
{
	struct wldbg_object_info *info;
	struct wldbg_objects_info *oi = msg->connection->objects_info;
	int id;

	if (!msg->connection->wldbg->gathering_info) {
		printf("Not gathering information about objects, "
		       "run wldbg with -g or -objinfo option ;)\n");
		return;
	}

	if (strncmp(buf, "all", 4) == 0) {
		print_all_objinfo(oi);
		return;
	}

	id = str_to_uint(buf);
	if (id == -1) {
		fprintf(stderr, "Wrong id given\n");
		return;
	}

	info = wldbg_message_get_object_info(msg, (uint32_t) id);
	if (!info) {
		printf("No information about %u\n", id);
		return;
	}

	print_objinfo(oi, info);
}
