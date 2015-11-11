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
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <assert.h>

/* for WL_SERVER_ID_START */
#include "wayland/wayland-private.h"

#include <wayland-server-protocol.h>
#include <wayland-client-protocol.h>

#include "wldbg.h"
#include "wldbg-private.h"
#include "wldbg-parse-message.h"
#include "wldbg-ids-map.h"
#include "wldbg-objects-info.h"
#include "passes.h"
#include "util.h"
#include "resolve.h"

/* FIXME move to info.c
static void
print_wl_buffer_info(struct wldbg_wl_buffer_info *info)
{
	printf("  offset: %d\n", info->offset);
	printf("  size: %dx%d\n", info->width, info->height);
	printf("  stride: %d\n", info->stride);
	printf("  format: %u\n", info->format);
}

static void
print_wl_surface_info(struct wldbg_objects_info *oi, struct wldbg_wl_surface_info *info)
{
	if (info->commited) {
		printf(" Commited ->\n");
		print_wl_surface_info(oi, info->commited);
		printf(" ---\n");
	}

	printf("  wl_buffer: %u\n", info->wl_buffer_id);
	printf("  attached to: %ux%u\n", info->attached_x, info->attached_y);
	printf("  buffer scale: %u\n", info->buffer_scale);
	printf("  last frame id: %u\n", info->last_frame_id);

	if (info->wl_buffer_id != 0) {
		printf(" --- buffer:\n");
		struct wldbg_object_info *bi = objects_info_get(oi, info->wl_buffer_id);
		if (!bi)
			printf("NO BUFFER!\n");
		else
			print_wl_buffer_info(bi->info);
	}
}


static void
print_xdg_surface_info(struct wldbg_objects_info *oi, struct wldbg_xdg_surface_info *xdg_info)
{
	uint8_t i = xdg_info->configures_num % 10;
	uint8_t ord = 0;

	printf("  Tile: '%s'\n", xdg_info->title);
	printf("  App id: '%s'\n", xdg_info->app_id);
	printf("  Last 10 configures (out of %lu):\n", xdg_info->configures_num);
	for (; ord < 10; ++i, ++ord) {
		i = i % 10;
		printf("    [%-2u]: width: %5u, height: %5u, state: -- , serial: %5u, acked: %u\n",
		       ord + 1, xdg_info->configures[i].width,
		       xdg_info->configures[i].height,
		       xdg_info->configures[i].serial,
		       xdg_info->configures[i].acked);
	}

	struct wldbg_object_info *info = objects_info_get(oi, xdg_info->wl_surface_id);
	assert (info && "has no wl_surface info in xdg_surface");
	printf(" -- wl_surface:\n");
	print_wl_surface_info(oi, info->info);
}


*/

void
handle_shm_pool_message(struct wldbg_objects_info *oi,
			struct wldbg_resolved_message *rm, int from);

void
handle_wl_buffer_message(struct wldbg_objects_info *oi,
			 struct wldbg_resolved_message *rm, int from);
void
handle_wl_compositor_message(struct wldbg_objects_info *oi,
			  struct wldbg_resolved_message *rm, int from);

void
handle_wl_surface_message(struct wldbg_objects_info *oi,
			  struct wldbg_resolved_message *rm, int from);
void
handle_xdg_shell_message(struct wldbg_objects_info *oi,
			 struct wldbg_resolved_message *rm, int from);

void
handle_xdg_surface_message(struct wldbg_objects_info *oi,
			   struct wldbg_resolved_message *rm, int from);

static int
gather_info(void *user_data, struct wldbg_message *message)
{
	(void) user_data;
	struct wldbg_objects_info *oinf = message->connection->objects_info;
	struct wldbg_resolved_message rm;

	if (!wldbg_resolve_message(message, &rm)) {
		fprintf(stderr, "Failed resolving message, loosing info\n");
		return PASS_NEXT;
	}

	if (strcmp(rm.wl_interface->name, "wl_surface") == 0)
		handle_wl_surface_message(oinf, &rm, message->from);
	else if (strcmp(rm.wl_interface->name, "xdg_surface") == 0)
		handle_xdg_surface_message(oinf, &rm, message->from);
	else if (strcmp(rm.wl_interface->name, "wl_buffer") == 0)
		handle_wl_buffer_message(oinf, &rm, message->from);
	else if (strcmp(rm.wl_interface->name, "wl_compositor") == 0)
		handle_wl_compositor_message(oinf, &rm, message->from);
	else if (strcmp(rm.wl_interface->name, "wl_shm_pool") == 0)
		handle_shm_pool_message(oinf, &rm, message->from);
	else if (strcmp(rm.wl_interface->name, "xdg_shell") == 0)
		handle_xdg_shell_message(oinf, &rm, message->from);

	return PASS_NEXT;
}

static struct pass *
create_objinfo_pass(void)
{
	struct pass *pass;

	pass = alloc_pass("objinfo");
	if (!pass)
		return NULL;

	pass->wldbg_pass.init = NULL;
	pass->wldbg_pass.destroy = NULL;
	pass->wldbg_pass.server_pass = gather_info;
	pass->wldbg_pass.client_pass = gather_info;
	pass->wldbg_pass.description
		= "Gather additional information about objects";

	return pass;
}

int
wldbg_add_objinfo_pass(struct wldbg *wldbg)
{
	if (wldbg->gathering_info)
		return 0;

	struct pass *pass = create_objinfo_pass();
	if (!pass)
		return -1;

	wl_list_insert(wldbg->passes.next, &pass->link);
	wldbg->gathering_info = 1;

	return 0;
}
