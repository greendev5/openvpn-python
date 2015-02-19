/* openvpn-python -- Plug-in embedding Python to OpenVPN.
 *
 * Copyright (C) ****** VPN Service 2015 <i.muzichuk@gmail.com>
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license.  See the LICENSE file for details.
 */

#ifndef _PY_PLUGIN_
#define _PY_PLUGIN_

#include "openvpn-plugin.h"

struct py_plugin_context;

extern struct py_plugin_context * py_plugin_context_init(const char *path_to_opt_file);
extern void py_plugin_context_free(struct py_plugin_context *context);

#endif /* _PY_PLUGIN_ */