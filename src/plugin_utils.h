/* openvpn-python -- Plug-in embedding Python to OpenVPN.
 *
 * Copyright (C) ****** VPN Service 2015 <i.muzichuk@gmail.com>
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license.  See the LICENSE file for details.
 */

#ifndef _PLUGIN_UTILS_
#define _PLUGIN_UTILS_

struct plugin_config
{
    const char *virtualenv;
    const char *pythonpath;
    
    const char *mod_init;
    const char *func_init;
    
    const char *mod_term;
    const char *func_term;
    
    const char *mod_auth_user_pass_verify;
    const char *func_auth_user_pass_verify;
    
    const char *mod_client_connect;
    const char *func_client_connect;
    
    const char *mod_client_disconnect;
    const char *func_clinet_disconnect;
    
};

extern struct plugin_config * plugin_config_init(const char *filename);
extern void plugin_config_free(struct plugin_config *pcnf);

#endif /* _PLUGIN_UTILS_ */