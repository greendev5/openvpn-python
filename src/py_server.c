/* openvpn-python -- Plug-in embedding Python to OpenVPN.
 *
 * Copyright (C) ****** VPN Service 2015 <i.muzichuk@gmail.com>
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license.  See the LICENSE file for details.
 */

#include "py_server.h"

/* Command codes for foreground -> background communication */
#define PYOVPN_MSG_AUTH_USER_PASS_VERIFY 0
#define PYOVPN_MSG_CLIENT_CONNECT        1
#define PYOVPN_MSG_CLIENT_DISCONNECT     2

/* Response codes for background -> foreground communication */
#define PYOVPN_RESPONSE_SUCCEEDED 10
#define PYOVPN_RESPONSE_FAILED    11

struct py_server
{
};