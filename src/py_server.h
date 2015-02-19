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

struct py_server;

extern struct py_server * py_server_init();
extern void py_server_term(struct py_server *pser);

extern int py_auth_user_pass_verify(struct py_server *pser, const char *envp[]);
extern int py_client_connect(struct py_server *pser, const char *envp[]);
extern int py_client_disconnect(struct py_server *pser, const char *envp[]);

#endif /* _PY_SERVER_ */