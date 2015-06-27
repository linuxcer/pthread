/*
 * Network server main loop -
 * serves IPC requests from other environments.
 */

#include "thread.h"
#include <stdio.h>
static void
tmain(uint32_t arg) {
    printf("RUN... ...");
}

void
main(int argc, char **argv)
{
	thread_init();
	thread_create(0, "main", tmain, 0);
    printf("tmain = %p\n", tmain);
    thread_yield();
	// never coming here!
}
