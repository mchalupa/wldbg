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

void
handle_wl_registry_message(struct wldbg_objects_info *oi,
			   struct wldbg_resolved_message *rm, int from);

void
handle_wl_seat_message(struct wldbg_objects_info *oi,
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
	else if (strcmp(rm.wl_interface->name, "wl_registry") == 0)
		handle_wl_registry_message(oinf, &rm, message->from);
	else if (strcmp(rm.wl_interface->name, "wl_seat") == 0)
		handle_wl_seat_message(oinf, &rm, message->from);

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
