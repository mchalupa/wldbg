#include <assert.h>
#include "test-runner.h"

#include "wldbg-parse-message.h"
#include "wldbg.h"
#include "wayland/wayland-util.h"

TEST(parse_base_message)
{
	int ret;
	uint32_t data[] = { 123, 0x0040feef, 0xd00d, 0xdeed };
	struct message msg = {
		.data = data
	};

	struct wldbg_parsed_message pm;
	ret = wldbg_parse_message(&msg, &pm);
	assert(ret != 0 && "failed parsing message");
	assert(pm.id == 123 && "wrong id");
	assert(pm.opcode = 0xfeef && "wrong opcode");
	assert(pm.size = 0x0040 && "wrong opcode");
	assert(*pm.data == 0xd00d);
	assert(*(pm.data + 1) == 0xdeed);
}

TEST(parse_base_message_fail)
{
	int ret;
	uint32_t data[] = { 123, 0x0002feef, 0xd00d, 0xdeed };
	struct message msg = {
		.data = data
	};

	struct wldbg_parsed_message pm;
	ret = wldbg_parse_message(&msg, &pm);
	/* size is less than 8, we should fail */
	assert(ret == 0 && "succeed parsing message");

	data[1] = 0x0041feef;
	ret = wldbg_parse_message(&msg, &pm);
	/* size is not divisible by 4 (sizeof uint32_t),
	 * we should fail */
	assert(ret == 0 && "succeed parsing message");
}

static const struct wl_message dummy_requests[] = {
	{ "foo", "4i?o2ih", NULL }
};

static const struct wl_message dummy_events[] = {
	{ "foo", "1s?a?sa?2s", NULL }
};

static const struct wl_interface dummy_interface = {
	.name = "dummy_interface", 1,
	1, dummy_requests,
	1, dummy_events,
};

TEST(resolved_message_iterator_types)
{
	struct wldbg_resolved_message rm = {
		.wl_interface = &dummy_interface,
		.wl_message = &dummy_requests[0],
	};

	wldbg_resolved_message_reset_iterator(&rm);
	struct wldbg_resolved_arg *arg;

	arg = wldbg_resolved_message_next_argument(&rm);
	assert(arg != NULL);
	assert(arg->type == 'i');
	assert(!arg->nullable);

	arg = wldbg_resolved_message_next_argument(&rm);
	assert(arg != NULL);
	assert(arg->type == 'o');
	assert(arg->nullable);

	arg = wldbg_resolved_message_next_argument(&rm);
	assert(arg != NULL);
	assert(arg->type == 'i');
	assert(!arg->nullable);

	arg = wldbg_resolved_message_next_argument(&rm);
	assert(arg != NULL);
	assert(arg->type == 'h');
	assert(!arg->nullable);

	arg = wldbg_resolved_message_next_argument(&rm);
	assert(arg == NULL);
}

TEST(resolved_iterator_test)
{
	/* { "foo", "1s?a?sa?2s", NULL } */
	uint32_t data[] = { 16 /* string size */,
			    0xdee1, 0xdee2, 0xdee3, 0xdee4,
			    0 /* array size */,
			    0 /* string size */,
			    12 /* array size */,
			    0xaaa1, 0xaaa2, 0xaaa3,
			    0};
	struct wldbg_resolved_message rm = {
		.wl_interface = &dummy_interface,
		.wl_message = &dummy_events[0],
		.base.data = data
	};

	wldbg_resolved_message_reset_iterator(&rm);
	struct wldbg_resolved_arg *arg;

	arg = wldbg_resolved_message_next_argument(&rm);
	assert(arg != NULL);
	assert(arg->type == 's');
	assert(!arg->nullable);
	assert(arg->ptr == data + 1);
	assert(*arg->ptr == 0xdee1);

	arg = wldbg_resolved_message_next_argument(&rm);
	assert(arg != NULL);
	assert(arg->type == 'a');
	assert(arg->nullable);
	assert(arg->ptr == NULL);

	arg = wldbg_resolved_message_next_argument(&rm);
	assert(arg != NULL);
	assert(arg->type == 's');
	assert(arg->nullable);
	assert(arg->ptr == NULL);

	arg = wldbg_resolved_message_next_argument(&rm);
	assert(arg != NULL);
	assert(arg->type == 'a');
	assert(!arg->nullable);
	assert(arg->ptr == data + 8);
	assert(*arg->ptr == 0xaaa1);

	arg = wldbg_resolved_message_next_argument(&rm);
	assert(arg != NULL);
	assert(arg->type == 's');
	assert(arg->nullable);
	assert(arg->ptr == NULL);

	arg = wldbg_resolved_message_next_argument(&rm);
	assert(arg == NULL);
	arg = wldbg_resolved_message_next_argument(&rm);
	assert(arg == NULL);

	/* try reset iterator, we should be at the begginging again */
	wldbg_resolved_message_reset_iterator(&rm);
	arg = wldbg_resolved_message_next_argument(&rm);
	assert(arg != NULL);
	assert(arg->type == 's');
	assert(!arg->nullable);
	assert(arg->ptr == data + 1);
	assert(*arg->ptr == 0xdee1);

	arg = wldbg_resolved_message_next_argument(&rm);
	assert(arg != NULL);
	assert(arg->type == 'a');
	assert(arg->nullable);
	assert(arg->ptr == NULL);

	arg = wldbg_resolved_message_next_argument(&rm);
	assert(arg != NULL);
	assert(arg->type == 's');
	assert(arg->nullable);
	assert(arg->ptr == NULL);

	arg = wldbg_resolved_message_next_argument(&rm);
	assert(arg != NULL);
	assert(arg->type == 'a');
	assert(!arg->nullable);
	assert(arg->ptr == data + 8);
	assert(*arg->ptr == 0xaaa1);

	arg = wldbg_resolved_message_next_argument(&rm);
	assert(arg != NULL);
	assert(arg->type == 's');
	assert(arg->nullable);
	assert(arg->ptr == NULL);

	arg = wldbg_resolved_message_next_argument(&rm);
	assert(arg == NULL);
	arg = wldbg_resolved_message_next_argument(&rm);
	assert(arg == NULL);
}
