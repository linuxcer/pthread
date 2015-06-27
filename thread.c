//#include <inc/lib.h> del
// add 
//#include <inc/malloc.h>
//#include "string.h"
#include <string.h>
#include <stdlib.h>
#include "thread.h"
#include "threadq.h"
#include <stdio.h>
//#include "setjmp.h"
#include <setjmp.h>
static thread_id_t max_tid;
static struct thread_context *cur_tc;

static struct thread_queue thread_queue;
static struct thread_queue kill_queue;

void
thread_init(void) {
    threadq_init(&thread_queue);
    max_tid = 0;
    printf("thread init successful\n");
}

uint32_t
thread_id(void) {
    return cur_tc->tc_tid;
}

void
thread_wakeup(volatile uint32_t *addr) {
    struct thread_context *tc = thread_queue.tq_first;
    while (tc) {
	if (tc->tc_wait_addr == addr)
	    tc->tc_wakeup = 1;
	tc = tc->tc_queue_link;
    }
}

void
thread_wait(volatile uint32_t *addr, uint32_t val, uint32_t msec) {
    uint32_t s = 0;  //sys_time_msec();返回系统当前时间del 
    uint32_t p = s;

    cur_tc->tc_wait_addr = addr;
    cur_tc->tc_wakeup = 0;

    while (p < msec) {
	if (p < s)
	    break;
	if (addr && *addr != val)
	    break;
	if (cur_tc->tc_wakeup)
	    break;

	thread_yield();
	p = 0;//sys_time_msec();返回系统当前时间del
    }

    cur_tc->tc_wait_addr = 0;
    cur_tc->tc_wakeup = 0;
}

int
thread_wakeups_pending(void)
{
    struct thread_context *tc = thread_queue.tq_first;
    int n = 0;
    while (tc) {
	if (tc->tc_wakeup)
	    ++n;
	tc = tc->tc_queue_link;
    }
    return n;
}

int
thread_onhalt(void (*fun)(thread_id_t)) {
    if (cur_tc->tc_nonhalt >= THREAD_NUM_ONHALT)
	return -4; //-E_NO_MEM;del

    cur_tc->tc_onhalt[cur_tc->tc_nonhalt++] = fun;
    return 0;
}

static thread_id_t
alloc_tid(void) {
    int tid = max_tid++;
    if (max_tid == (uint32_t)~0)
        ; // add
	//panic("alloc_tid: no more thread ids");
    return tid;
}

static void
thread_set_name(struct thread_context *tc, const char *name)
{
    strncpy(tc->tc_name, name, name_size - 1);
    tc->tc_name[name_size - 1] = 0;
}

static void
thread_entry(void) {
    cur_tc->tc_entry(cur_tc->tc_arg);
    thread_halt();
}



int
thread_create(thread_id_t *tid, const char *name, 
		void (*entry)(uint32_t), uint32_t arg) {
    struct thread_context *tc = malloc(sizeof(struct thread_context));
    if (!tc) {
        return -4;// -E_NO_MEM;del
    }
    memset(tc, 0, sizeof(struct thread_context));
    
    thread_set_name(tc, name);
    tc->tc_tid = alloc_tid();

    tc->tc_stack_bottom = malloc(stack_size);
    if (!tc->tc_stack_bottom) {
        free(tc); 
	    return -4; //-E_NO_MEM; del
    }

    void *stacktop = tc->tc_stack_bottom + stack_size;
    // Terminate stack unwinding
    stacktop = stacktop - 4;
    memset(stacktop, 0, 4);
    
    memset(&tc->tc_jb, 0, sizeof(tc->tc_jb));
    //tc->tc_jb.jb_esp = (uint32_t)stacktop;
    //tc->tc_jb.jb_eip = (uint32_t)&thread_entry;
    tc->tc_entry = entry;
    tc->tc_arg = arg;

    threadq_push(&thread_queue, tc);

    if (tid)
    	*tid = tc->tc_tid;
    return 0;
}

static void
thread_clean(struct thread_context *tc) {
    if (!tc) return;

    int i;
    for (i = 0; i < tc->tc_nonhalt; i++)
	tc->tc_onhalt[i](tc->tc_tid);
    free(tc->tc_stack_bottom);
    free(tc);
}

void
thread_halt() {
    // right now the kill_queue will never be more than one
    // clean up a thread if one is on the queue
    thread_clean(threadq_pop(&kill_queue));

    threadq_push(&kill_queue, cur_tc);
    cur_tc = NULL;
    thread_yield();
    // WHAT IF THERE ARE NO MORE THREADS? HOW DO WE STOP?
    // when yield has no thread to run, it will return here!
    exit(0); 
}

void
thread_yield(void) {
    struct thread_context *next_tc = threadq_pop(&thread_queue);

    if (!next_tc)
	return;

    if (cur_tc) {
	    if (setjmp(cur_tc->tc_jb) != 0)
	        return;
	    threadq_push(&thread_queue, cur_tc);
    }
    printf("1 \n");

    cur_tc = next_tc;
    longjmp(cur_tc->tc_jb, 1);
    printf("2\n");
}

static void
print_jb(struct thread_context *tc) {
    //cprintf("jump buffer for thread %s:\n", tc->tc_name);del 
    //cprintf("\teip: %x\n", tc->tc_jb.jb_eip);del
    //cprintf("\tesp: %x\n", tc->tc_jb.jb_esp);del
    //cprintf("\tebp: %x\n", tc->tc_jb.jb_ebp);del
    //cprintf("\tebx: %x\n", tc->tc_jb.jb_ebx);del
    //cprintf("\tesi: %x\n", tc->tc_jb.jb_esi);del
    //cprintf("\tedi: %x\n", tc->tc_jb.jb_edi);del
}
