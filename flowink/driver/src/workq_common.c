#include <zephyr/kernel.h>

#define WORK_QUEUE_STACK_SIZE 512
#define WORK_QUEUE_PRIORITY 5

K_THREAD_STACK_DEFINE(work_queue_stack, WORK_QUEUE_STACK_SIZE);

struct k_work_q work_queue;

void workq_init(void)
{
    k_work_queue_init(&work_queue);
    k_work_queue_start(&work_queue, work_queue_stack, K_THREAD_STACK_SIZEOF(work_queue_stack),
                       WORK_QUEUE_PRIORITY, NULL);
}