/***************************************************************************
 * Copyright (c) 2024 Microsoft Corporation
 * Copyright (c) 2024 Intel Corporation
 *
 * This program and the accompanying materials are made available under the
 * terms of the MIT License which is available at
 * https://opensource.org/licenses/MIT.
 *
 * SPDX-License-Identifier: MIT
 **************************************************************************/

/**************************************************************************/
/**************************************************************************/
/**                                                                       */
/** Thread-Metric Component                                               */
/**                                                                       */
/**   Porting Layer for RT-Thread RTOS                                    */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/

/* Include necessary files.  */
#include "tm_api.h"
#include <rtthread.h>

/* Define constants for the test suite */
#define TM_TEST_NUM_THREADS        10
#define TM_TEST_STACK_SIZE         1024
#define TM_TEST_NUM_SEMAPHORES     4
#define TM_TEST_NUM_MESSAGE_QUEUES 4
#define TM_TEST_NUM_SLABS          4

/* Ensure single processor system */
#if defined(RT_USING_SMP)
#error "*** Tests are only designed for single processor systems! ***"
#endif

/* Define thread control blocks and stacks */
static struct rt_thread test_thread[TM_TEST_NUM_THREADS];
static rt_uint8_t test_stack[TM_TEST_NUM_THREADS][TM_TEST_STACK_SIZE];

/* Define semaphores */
static rt_sem_t test_sem[TM_TEST_NUM_SEMAPHORES];

/* Define message queues and buffers */
static rt_mq_t test_msgq[TM_TEST_NUM_MESSAGE_QUEUES];
static char test_msgq_buffer[TM_TEST_NUM_MESSAGE_QUEUES][8][16];

/* Define memory pools and buffers */
static rt_mp_t test_slab[TM_TEST_NUM_SLABS];
static char test_slab_buffer[TM_TEST_NUM_SLABS][8 * 128];

/*
 * This function performs basic RTOS initialization,
 * calls the test initialization function, and then starts the RTOS.
 */
void tm_initialize(void (*test_initialization_function)(void))
{
    test_initialization_function();
}

/*
 * This function creates a thread with the specified ID and priority.
 * Priorities range from 1 (highest) to 31 (lowest).
 * Returns TM_SUCCESS on success, TM_ERROR on failure.
 */
int tm_thread_create(int thread_id, int priority, void (*entry_function)(void *, void *, void *))
{
    rt_err_t result;

    result = rt_thread_init(&test_thread[thread_id], "test_thread", (void (*)(void *))entry_function,
                            RT_NULL, test_stack[thread_id], TM_TEST_STACK_SIZE, priority, 20);

    if (result == RT_EOK)
    {
        /* Start and immediately suspend the thread to match Thread-Metric requirements */
        rt_thread_startup(&test_thread[thread_id]);
        rt_thread_suspend(&test_thread[thread_id]);
        return TM_SUCCESS;
    }
    return TM_ERROR;
}

/*
 * This function resumes the specified thread.
 * Returns TM_SUCCESS on success, TM_ERROR on failure.
 */
int tm_thread_resume(int thread_id)
{
    rt_err_t result = rt_thread_resume(&test_thread[thread_id]);
    return (result == RT_EOK) ? TM_SUCCESS : TM_ERROR;
}

/*
 * This function suspends the specified thread.
 * Returns TM_SUCCESS on success, TM_ERROR on failure.
 */
int tm_thread_suspend(int thread_id)
{
    rt_err_t result = rt_thread_suspend(&test_thread[thread_id]);
    return (result == RT_EOK) ? TM_SUCCESS : TM_ERROR;
}

/*
 * This function relinquishes control to other ready threads of the same priority.
 */
void tm_thread_relinquish(void)
{
    rt_thread_yield();
}

/*
 * This function suspends the calling thread for the specified number of seconds.
 */
void tm_thread_sleep(int seconds)
{
    rt_thread_mdelay(seconds * 1000); /* Convert seconds to milliseconds */
}

/*
 * This function creates a message queue with the specified ID.
 * The queue should hold at least one 16-byte message.
 * Returns TM_SUCCESS on success, TM_ERROR on failure.
 */
int tm_queue_create(int queue_id)
{
    rt_err_t result = rt_mq_init(&test_msgq[queue_id], "test_mq", &test_msgq_buffer[queue_id][0][0],
                                 16, sizeof(test_msgq_buffer[queue_id]), RT_IPC_FLAG_FIFO);
    return (result == RT_EOK) ? TM_SUCCESS : TM_ERROR;
}

/*
 * This function sends a 16-byte message to the specified queue.
 * Returns TM_SUCCESS on success, TM_ERROR on failure.
 */
int tm_queue_send(int queue_id, unsigned long *message_ptr)
{
    rt_err_t result = rt_mq_send(&test_msgq[queue_id], message_ptr, 16);
    return (result == RT_EOK) ? TM_SUCCESS : TM_ERROR;
}

/*
 * This function receives a 16-byte message from the specified queue.
 * Returns TM_SUCCESS on success, TM_ERROR on failure.
 */
int tm_queue_receive(int queue_id, unsigned long *message_ptr)
{
    rt_err_t result = rt_mq_recv(&test_msgq[queue_id], message_ptr, 16, RT_WAITING_NO);
    return (result == RT_EOK) ? TM_SUCCESS : TM_ERROR;
}

/*
 * This function creates a binary semaphore with the specified ID.
 * Returns TM_SUCCESS on success, TM_ERROR on failure.
 */
int tm_semaphore_create(int semaphore_id)
{
    test_sem[semaphore_id] = rt_sem_create("test_sem", 1, RT_IPC_FLAG_FIFO);
    return (test_sem[semaphore_id] != RT_NULL) ? TM_SUCCESS : TM_ERROR;
}

/*
 * This function gets the specified semaphore.
 * Returns TM_SUCCESS on success, TM_ERROR on failure.
 */
int tm_semaphore_get(int semaphore_id)
{
    rt_err_t result = rt_sem_take(&test_sem[semaphore_id], RT_WAITING_NO);
    return (result == RT_EOK) ? TM_SUCCESS : TM_ERROR;
}

/*
 * This function releases the specified semaphore.
 * Returns TM_SUCCESS on success, TM_ERROR on failure.
 */
int tm_semaphore_put(int semaphore_id)
{
    rt_err_t result = rt_sem_release(&test_sem[semaphore_id]);
    return (result == RT_EOK) ? TM_SUCCESS : TM_ERROR;
}

/*
 * This function triggers an interrupt for the benchmark.
 * Note: User must ensure SVC #255 handler calls tm_interrupt_handler.
 */
void tm_cause_interrupt(void)
{
    /* Trigger SVC interrupt with a unique number not used by RT-Thread */
    asm("SVC #255");
}

/*
 * This function creates a memory pool that supports 128-byte block allocations.
 * Returns TM_SUCCESS on success, TM_ERROR on failure.
 */
int tm_memory_pool_create(int pool_id)
{
    rt_err_t result = rt_mp_init(&test_slab[pool_id], "test_mp", &test_slab_buffer[pool_id][0],
                                 sizeof(test_slab_buffer[pool_id]), 128);
    return (result == RT_EOK) ? TM_SUCCESS : TM_ERROR;
}

/*
 * This function allocates a 128-byte block from the specified memory pool.
 * Returns TM_SUCCESS on success, TM_ERROR on failure.
 */
int tm_memory_pool_allocate(int pool_id, unsigned char **memory_ptr)
{
    *memory_ptr = (unsigned char *)rt_mp_alloc(&test_slab[pool_id], RT_WAITING_NO);
    return (*memory_ptr != RT_NULL) ? TM_SUCCESS : TM_ERROR;
}

/*
 * This function releases a 128-byte block back to the specified memory pool.
 * Returns TM_SUCCESS on success, TM_ERROR on failure.
 */
int tm_memory_pool_deallocate(int pool_id, unsigned char *memory_ptr)
{
    rt_mp_free(memory_ptr);
    return TM_SUCCESS;
}
