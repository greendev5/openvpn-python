/* openvpn-auth-http -- An OpenVPN plugin for do accounting.
 *
 * Copyright (C) ****** VPN Service 2014 <i.muzichuk@gmail.com>
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license.  See the LICENSE file for details.
 */

#include <stdlib.h>
#include <string.h>

#include <pthread.h>

#include "deferred_queue.h"

struct deferred_queue_item
{
    struct deferred_queue_item *next_node;

    struct py_context      *context;
    struct py_function_def *func;
    struct ovpn_envp_buf   *envp_buf;
};

struct deferred_queue
{
    pthread_mutex_t q_mutex;
    pthread_cond_t  q_condv;
    pthread_t       q_pthrd;

    unsigned char   q_sflag;

    size_t q_size;

    struct deferred_queue_item *q_head;
    struct deferred_queue_item *q_tail;
};

struct deferred_queue_item * create_deferred_queue_item(struct py_context *context, struct py_function_def *func, struct ovpn_envp_buf *envp_buf)
{
    struct deferred_queue_item *item = (struct deferred_queue_item*)malloc(sizeof(struct deferred_queue_item));
    item->next_node = NULL;
    item->context = context;
    item->func = func;
    item->envp_buf = envp_buf;
    return item;
}

extern void free_deferred_queue_item(struct deferred_queue_item *item)
{
    ovpn_envp_buf_free(item->envp_buf);
    free(item);
}

static struct deferred_queue_item * remove_item_from_deferred_queue(struct deferred_queue *queue)
{
    struct deferred_queue_item *item = NULL;

    pthread_mutex_lock(&queue->q_mutex);

    for (;;) {
        if (queue->q_sflag) {
            PLUGIN_DEBUG("Python process: Second thread got stop flag. Exiting...");
            item = NULL;
            goto out;
        } else if (queue->q_head) {
            item = queue->q_head;
            queue->q_head = item->next_node;
            if (!queue->q_head)
                queue->q_tail = NULL;
            queue->q_size--;
            goto out;
        }
        PLUGIN_DEBUG("Python process: Wait for deferred handler in queue...");
        pthread_cond_wait(&queue->q_condv, &queue->q_mutex);
    }

out:
    pthread_cond_signal(&queue->q_condv);
    pthread_mutex_unlock(&queue->q_mutex);
    return item;
}


static void * consumer_func(void *arg)
{
    int r = 0;
    struct deferred_queue *queue = (struct deferred_queue*)arg;
    struct deferred_queue_item *item = NULL;
    for (;;) {
        item = remove_item_from_deferred_queue(queue);
        if (item) {
            PLUGIN_DEBUG("Perform handler from queue...");
            r = py_context_exec_func(item->context, item->func, item->envp_buf->envpn);
            free_deferred_queue_item(item);
            if (r < 0) {
                PLUGIN_ERROR("Plugin function failed.");
            }
        }
        else {
            break;
        }
    }
    return NULL;
}

struct deferred_queue * create_deferred_queue(void)
{
    int r;
    struct deferred_queue *queue = (struct deferred_queue*)malloc(sizeof(struct deferred_queue));
    memset(queue, 0, sizeof(struct deferred_queue));

    r = pthread_mutex_init(&queue->q_mutex, NULL);
    if (r) {
        goto error;
    }

    r = pthread_cond_init(&queue->q_condv, NULL);
    if (r) {
        pthread_mutex_destroy(&queue->q_mutex);
        goto error;
    }

    r = pthread_create(&queue->q_pthrd, NULL, consumer_func, queue);
    if (r) {
        pthread_mutex_destroy(&queue->q_mutex);
        pthread_cond_destroy(&queue->q_condv);
        goto error;
    }

    return queue;

error:
    free(queue);
    return NULL;
}

extern void free_deferred_queue(struct deferred_queue *queue)
{
    struct deferred_queue_item *item;
    
    PLUGIN_DEBUG("Stoping second thread from manin thread...");
    pthread_mutex_lock(&queue->q_mutex);
    queue->q_sflag = 1;
    pthread_cond_signal(&queue->q_condv);
    pthread_mutex_unlock(&queue->q_mutex);

    pthread_join(queue->q_pthrd, NULL);

    PLUGIN_DEBUG("Destroing mutex and condition...");
    pthread_cond_destroy(&queue->q_condv);
    pthread_mutex_destroy(&queue->q_mutex);

    PLUGIN_DEBUG("Free queue items mem...");
    while (queue->q_head) {
        item = queue->q_head;
        queue->q_head = queue->q_head->next_node;
        free_deferred_queue_item(item);
    }

    PLUGIN_DEBUG("Free queue mem...");
    free(queue);
}

extern int add_item_to_deferred_queue(struct deferred_queue *queue, struct deferred_queue_item *item)
{
    int r = DEFERRED_QUEUE_ADD_SUCCESS;
   
    PLUGIN_DEBUG("Start - add_item_to_deferred_queue"); 
    pthread_mutex_lock(&queue->q_mutex);
    
    if (queue->q_size > DEFERRED_QUEUE_MAX_SIZE) {
        PLUGIN_ERROR("Deferred QUEUE is FULL!!!");
        r = DEFERRED_QUEUE_ADD_FULL;
        goto out;
    }
    
    if (!queue->q_head) {
        queue->q_head = item;
    } else {
        queue->q_tail->next_node = item;
    }
    queue->q_tail = item;
    queue->q_size++;
    pthread_cond_signal(&queue->q_condv);

out:
    pthread_mutex_unlock(&queue->q_mutex);
    PLUGIN_DEBUG("END - add_item_to_deferred_queue");
    return r;
}
