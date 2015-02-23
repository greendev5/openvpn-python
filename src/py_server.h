/* openvpn-python -- Plug-in embedding Python to OpenVPN.
 *
 * Copyright (C) ****** VPN Service 2015 <i.muzichuk@gmail.com>
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license.  See the LICENSE file for details.
 */

#ifndef _PY_SERVER_
#define _PY_SERVER_

#include "plugin_utils.h"

struct py_server;

extern struct py_server * py_server_init(struct plugin_config *pcnf, const char *envp[]);
extern void py_server_term(struct py_server *pser);

extern int py_server_send_command(struct py_server *pser, int command, const char *envp[]);

#endif /* _PY_SERVER_ */