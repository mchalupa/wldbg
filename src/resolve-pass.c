/*
 * Copyright (c) 2014 - 2015 Marek Chalupa
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
#include "wldbg-pass.h"
#include "wldbg-private.h"
#include "wldbg-ids-map.h"
#include "passes.h"
#include "util.h"
#include "resolve.h"

static struct wl_list shared_interfaces;

static void
resolved_objects_put(struct resolved_objects *ro,
		     uint32_t id, const struct wl_interface *intf)
{
	if (id >= WL_SERVER_ID_START)
		wldbg_ids_map_insert(&ro->objects.server_objects,
				     id - WL_SERVER_ID_START, (void *) intf);
	else
		wldbg_ids_map_insert(&ro->objects.client_objects, id, (void *) intf);
}

/* this pass analyze the connection and translates object id
 * to human-readable names */

struct interface
{
	const struct wl_interface *interface;
	struct wl_list link;
};

static void
add_interface(struct wl_list *intfs, struct interface *intf)
{
	struct interface *i;

	wl_list_for_each(i, intfs, link) {
		if (strcmp(i->interface->name, intf->interface->name) == 0) {
			dbg("Already got '%s' interface, discarding\n",
				intf->interface->name);
			free(intf);
			return;
		}
	}

	dbg("Adding interface '%s'\n", intf->interface->name);
	wl_list_insert(intfs->next, &intf->link);
}

static void
add_shared_interface(struct interface *intf)
{
	add_interface(&shared_interfaces, intf);
}

static const struct wl_interface *
get_interface(struct resolved_objects *ro, const char *name)
{
	struct interface *i;

	wl_list_for_each(i, ro->interfaces, link) {
		if (strcmp(i->interface->name, name) == 0) {
			return i->interface;
		}
	}

	wl_list_for_each(i, &ro->additional_interfaces, link) {
		if (strcmp(i->interface->name, name) == 0) {
			return i->interface;
		}
	}

	dbg("RESOLVE: Didn't find '%s' interface\n", name);
	return NULL;
}

static struct interface *
create_interface(const struct wl_interface *intf)
{
	struct interface *ret = malloc(sizeof *ret);
	if (!ret)
		return NULL;

	ret->interface = intf;

	return ret;
}

static struct interface *
libwayland_get_interface(void *handle, const char *intf)
{
	const struct wl_interface *interface;

	dbg("Loading interface '%s' from libwayland\n", intf);

	interface = dlsym(handle, intf);
	if (!interface) {
		dbg("Failed loading interface '%s' from libwayland: %s\n",
			intf, dlerror());
		return NULL;
	}

	return create_interface(interface);
}

static void
parse_libwayland(void)
{
	struct interface *intf;
	void *handle;

	handle = dlopen("libwayland-client.so", RTLD_NOW);
	if (!handle) {
		fprintf(stderr, "Loading pass: %s\n", dlerror());
		return;
	}

	if ((intf = libwayland_get_interface(handle, "wl_display_interface")))
		add_shared_interface(intf);
	if ((intf = libwayland_get_interface(handle, "wl_registry_interface")))
		add_shared_interface(intf);
	if ((intf = libwayland_get_interface(handle, "wl_callback_interface")))
		add_shared_interface(intf);
	if ((intf = libwayland_get_interface(handle, "wl_compositor_interface")))
		add_shared_interface(intf);
	if ((intf = libwayland_get_interface(handle, "wl_shm_pool_interface")))
		add_shared_interface(intf);
	if ((intf = libwayland_get_interface(handle, "wl_shm_interface")))
		add_shared_interface(intf);
	if ((intf = libwayland_get_interface(handle, "wl_buffer_interface")))
		add_shared_interface(intf);
	if ((intf = libwayland_get_interface(handle, "wl_data_offer_interface")))
		add_shared_interface(intf);
	if ((intf = libwayland_get_interface(handle, "wl_data_source_interface")))
		add_shared_interface(intf);
	if ((intf = libwayland_get_interface(handle, "wl_data_device_interface")))
		add_shared_interface(intf);
	if ((intf = libwayland_get_interface(handle, "wl_data_device_manager_interface")))
		add_shared_interface(intf);
	if ((intf = libwayland_get_interface(handle, "wl_shell_interface")))
		add_shared_interface(intf);
	if ((intf = libwayland_get_interface(handle, "wl_shell_surface_interface")))
		add_shared_interface(intf);
	if ((intf = libwayland_get_interface(handle, "wl_surface_interface")))
		add_shared_interface(intf);
	if ((intf = libwayland_get_interface(handle, "wl_seat_interface")))
		add_shared_interface(intf);
	if ((intf = libwayland_get_interface(handle, "wl_pointer_interface")))
		add_shared_interface(intf);
	if ((intf = libwayland_get_interface(handle, "wl_keyboard_interface")))
		add_shared_interface(intf);
	if ((intf = libwayland_get_interface(handle, "wl_touch_interface")))
		add_shared_interface(intf);
	if ((intf = libwayland_get_interface(handle, "wl_output_interface")))
		add_shared_interface(intf);
	if ((intf = libwayland_get_interface(handle, "wl_region_interface")))
		add_shared_interface(intf);
	if ((intf = libwayland_get_interface(handle, "wl_subcompositor_interface")))
		add_shared_interface(intf);
	if ((intf = libwayland_get_interface(handle, "wl_subsurface_interface")))
		add_shared_interface(intf);

	dlclose(handle);
}

extern const struct wl_interface xdg_shell_interface;
extern const struct wl_interface xdg_surface_interface;
extern const struct wl_interface xdg_popup_interface;

static void
add_hardcoded_xdg_shell(void)
{
	struct interface *intf;

	intf = create_interface(&xdg_shell_interface);
	if (!intf)
		return;

	add_shared_interface(intf);

	intf = create_interface(&xdg_surface_interface);
	if (!intf)
		return;

	add_shared_interface(intf);

	intf = create_interface(&xdg_popup_interface);
	if (!intf)
		return;

	add_shared_interface(intf);
}

extern const struct wl_interface wl_drm_interface;

static void
add_hardcoded_drm_interface(void)
{
	struct interface *intf;

	intf = create_interface(&wl_drm_interface);
	if (!intf)
		return;

	add_shared_interface(intf);
}

static int
wl_message_new_id_pos(const char *signature)
{
	int n = 0;

	while (signature[n] != '\0') {
		if (signature[n] == 'n')
			return n;

		++n;
	}

	return -1;
}

static uint32_t
get_new_id(const char *signature, uint32_t *data)
{
	int n = 0, pos = 2;
	uint32_t size;

	while (signature[n] != '\0') {
		switch(signature[n]) {
		case 'n':
			vdbg("Found new_id: %u\n", data[pos]);
			return data[pos];
		case 's':
		case 'a':
			size = DIV_ROUNDUP(data[pos], sizeof(uint32_t));
			pos += size + 1;
			break;
		default:
			++pos;
		}

		++n;
	}

	assert(0 && "No new id in message");
}

static void
get_new_ids(struct resolved_objects *ro, uint32_t *data,
	    const struct wl_message *wl_message, const char *guess_type)
{
	uint32_t new_id;
	const struct wl_interface *new_intf;
	int id_pos, pos = 0;

	assert(wl_message->signature && "BUG: no wl_message->signature");
	assert(wl_message->types && "BUG: no wl_message->types");

	/* search for new_id's in loop in the case there
	 * are more of them in a event/request */
	while ((id_pos = wl_message_new_id_pos(
			wl_message->signature + pos)) >= 0) {

		new_id = get_new_id(wl_message->signature + pos, data);
		new_intf = (void *) wl_message->types[id_pos];

		/* if the type is unknown, we guessed it is
		 * this type (usualy from bind request) */
		if (!new_intf && guess_type){
			dbg("RESOLVE: Guessing unknown type is '%s'\n",
				guess_type);
			new_intf = get_interface(ro, guess_type);
		}

		if (!new_intf)
			new_intf = &unknown_interface;

		resolved_objects_put(ro, new_id, new_intf);

		dbg("RESOLVE: Got new id %u (%s)\n", new_id, new_intf->name);

		pos = id_pos + 1;
	}
}

static int
resolve_in(void *user_data, struct wldbg_message *message)
{
	uint32_t id, opcode;
	uint32_t *data = message->data;
	const struct wl_interface *intf;
	const struct wl_message *wl_message;
	struct resolved_objects *ro = message->connection->resolved_objects;

	(void) user_data;

	id = data[0];
	opcode = data[1] & 0xffff;

	intf = wldbg_message_get_object(message, id);
	if (intf) {
		if (intf == &unknown_interface || intf == &free_entry)
			return PASS_NEXT;

		if (((uint32_t) intf->event_count) <= opcode) {
			fprintf(stderr,
				"Invalid opcode in event, maybe protocol"
				" version mismatch? opcode: %u ", opcode);
			if (intf->event_count == 1)
				fprintf(stderr, "available opcodes: 0\n");
			else
				fprintf(stderr, "available opcodes: 0 - %d\n",
					intf->event_count - 1);
			return PASS_NEXT;
		}


		wl_message = &intf->events[opcode];

		/* handle delete_id event */
		if (id == 1 /* wl_display */
			&& opcode == WL_DISPLAY_DELETE_ID) {
			resolved_objects_put(ro, data[2], &free_entry);
			dbg("RESOLVE: Freed id %u\n", data[2]);
		} else
			get_new_ids(ro, data, wl_message, NULL);
	}

	return PASS_NEXT;
}

static int
resolve_out(void *user_data, struct wldbg_message *message)
{
	uint32_t id, opcode;
	uint32_t *data = message->data;
	const struct wl_interface *intf;
	const struct wl_message *wl_message;
	const char *guess_type = NULL;
	struct resolved_objects *ro = message->connection->resolved_objects;

	(void) user_data;

	id = data[0];
	opcode = data[1] & 0xffff;

	intf = wldbg_message_get_object(message, id);
	if (intf) {
		/* unknown interface */
		if (intf == &unknown_interface || intf == &free_entry)
			return PASS_NEXT;

		if (((uint32_t) intf->method_count) <= opcode) {
			fprintf(stderr,
				"Invalid opcode in request, maybe protocol"
				" version mismatch? opcode: %u ", opcode);
			if (intf->method_count == 1)
				fprintf(stderr, "available opcodes: 0\n");
			else
				fprintf(stderr, "available opcodes: 0 - %d\n",
					intf->method_count - 1);
			return PASS_NEXT;
		}

		wl_message = &intf->methods[opcode];

		/* data + 4 is the string with the name
		 * of interface in bind request. Use it
		 * to guess the interface of new id */
		if (strcmp(intf->name, "wl_registry") == 0
			&& opcode == WL_REGISTRY_BIND)
				guess_type = (const char *) (data + 4);

		get_new_ids(ro, data, wl_message, guess_type);
	}

	return PASS_NEXT;
}

struct resolved_objects *
create_resolved_objects(void)
{
	struct resolved_objects *ro = malloc(sizeof *ro);
	if (!ro) {
		fprintf(stderr, "Out of memory\n");
		return NULL;
	}

	wldbg_ids_map_init(&ro->objects.client_objects);
	wldbg_ids_map_init(&ro->objects.server_objects);
	wl_list_init(&ro->additional_interfaces);

	/* these are shared between connection */
	/* and contain at least libwayland interfaces */
	assert(!wl_list_empty(&shared_interfaces));
	ro->interfaces = &shared_interfaces;

	/* id 0 is always empty and 1 is always display */
	resolved_objects_put(ro, 0, NULL);
	resolved_objects_put(ro, 1, get_interface(ro, "wl_display"));

	return ro;
}

void
destroy_resolved_objects(struct resolved_objects *ro)
{
	struct interface *intf, *tmp;

	wldbg_ids_map_release(&ro->objects.client_objects);
	wldbg_ids_map_release(&ro->objects.server_objects);

	wl_list_for_each_safe(intf, tmp, &ro->additional_interfaces, link) {
		free(intf);
	}

	free(ro);
}

static int
resolve_init(struct wldbg *wldbg, struct wldbg_pass *pass,
	     int argc, const char *argv[])
{
	(void) argc;
	(void) argv;
	(void) wldbg;
	(void) pass;

	wl_list_init(&shared_interfaces);
	struct interface *i;
	int count = 0;
	wl_list_for_each(i, &shared_interfaces, link)
		count++;

	/* get interfaces from libwayland.so */
	parse_libwayland();

	/* this is a workaround until we have parser for binary */
	add_hardcoded_xdg_shell();
	add_hardcoded_drm_interface();

	/* XXX
	if (path_to_binary)
		parse_binary(... bfd

	*/

	dbg("Resolving objects inited\n");

	return 0;
}

static void
resolve_destroy(void *data)
{
	struct interface *intf, *tmp;

	(void) data;

	wl_list_for_each_safe(intf, tmp, &shared_interfaces, link) {
		free(intf);
	}
}

static struct pass *
create_resolve_pass(void)
{
	struct pass *pass;

	pass = alloc_pass("resolve");
	if (!pass)
		return NULL;

	pass->wldbg_pass.init = resolve_init;
	pass->wldbg_pass.destroy = resolve_destroy;
	pass->wldbg_pass.server_pass = resolve_in;
	pass->wldbg_pass.client_pass = resolve_out;
	pass->wldbg_pass.description = "Assign interfaces to objects";

	return pass;
}

int
wldbg_add_resolve_pass(struct wldbg *wldbg)
{
	/* do not add this pass more times */
	if (wldbg->resolving_objects)
		return 0;

	struct pass *pass = create_resolve_pass();
	if (!pass)
		return -1;

	if (resolve_init(wldbg, &pass->wldbg_pass, 0, NULL) < 0) {
		dealloc_pass(pass);
		return -1;
	}

	/* insert always at the begining */
	wl_list_insert(&wldbg->passes, &pass->link);
	wldbg->resolving_objects = 1;

	return 0;
}
