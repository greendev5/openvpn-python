/* openvpn-python -- Plug-in embedding Python to OpenVPN.
 *
 * Copyright (C) ****** VPN Service 2015 <i.muzichuk@gmail.com>
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license.  See the LICENSE file for details.
 */

#ifndef _PY_HANDLERS_
#define _PY_HANDLERS_

#include "plugin_utils.h"

struct py_function_def;

struct py_context;

/*
 */
extern struct py_context * py_context_init(struct plugin_config *pcnf);

/*
 */
extern void py_context_free(struct py_context *context);

/*
 */
extern int py_context_exec_func(struct py_context *context, struct py_function_def *func, char **envp);

extern struct py_function_def * py_context_handler_func(struct py_context *context, int command);

#endif /* _PY_HANDLERS_ */