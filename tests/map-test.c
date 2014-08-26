#include <assert.h>
#include "util.h"
#include "test-runner.h"

TEST(map_create)
{
	struct wldbg_ids_map m;

	wldbg_ids_map_init(&m);
	wldbg_ids_map_release(&m);
	/* leaks? */
}

TEST(map_insert)
{
	struct wldbg_ids_map m;

	wldbg_ids_map_init(&m);
	assert(wldbg_ids_map_get(&m, 0) == NULL);

	wldbg_ids_map_insert(&m, 0, (void *) 0x1);
	assert(wldbg_ids_map_get(&m, 0) == (void *) 0x1);
	wldbg_ids_map_insert(&m, 1, (void *) 0x2);
	assert(wldbg_ids_map_get(&m, 1) == (void *) 0x2);

	/* rewrite first value */
	wldbg_ids_map_insert(&m, 0, (void *) 0x3);
	assert(wldbg_ids_map_get(&m, 0) == (void *) 0x3);
	assert(wldbg_ids_map_get(&m, 1) == (void *) 0x2);

	wldbg_ids_map_insert(&m, 10, (void *) 0x10);
	assert(m.count == 11);
	assert(wldbg_ids_map_get(&m, 11) == NULL);
	assert(wldbg_ids_map_get(&m, 10) == (void *) 0x10);

	wldbg_ids_map_release(&m);
}
