/* openvpn-auth-http -- An OpenVPN plugin for do accounting.
 *
 * Copyright (C) ****** VPN Service 2014 <i.muzichuk@gmail.com>
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license.  See the LICENSE file for details.
 */

#ifndef _DEFERRED_QUEUE_
#define _DEFERRED_QUEUE_

#define DEFERRED_QUEUE_ADD_SUCCESS 0
#define DEFERRED_QUEUE_ADD_FULL -1

#define DEFERRED_QUEUE_MAX_SIZE 1024

#include "py_handlers.h"
#include "plugin_utils.h"

struct deferred_queue_item;

/*
 */
extern struct deferred_queue_item * create_deferred_queue_item(
    struct py_context *context, struct py_function_def *func, struct ovpn_envp_buf *envp_buf);

/*
 */
extern void free_deferred_queue_item(struct deferred_queue_item *item);

struct deferred_queue;

/*
 */
extern struct deferred_queue * create_deferred_queue(void);

/*
 */
extern void free_deferred_queue(struct deferred_queue *queue);

/*
 */
extern int add_item_to_deferred_queue(struct deferred_queue *queue, struct deferred_queue_item *item);

#endif /* _DEFERRED_QUEUE_ */