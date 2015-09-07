#include "wldbg-private.h"

void
wldbg_exit(struct wldbg *wldbg)
{
	wldbg->flags.exit = 1;
}
