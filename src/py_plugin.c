/* openvpn-python -- Plug-in embedding Python to OpenVPN.
 *
 * Copyright (C) ****** VPN Service 2015 <i.muzichuk@gmail.com>
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license.  See the LICENSE file for details.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "plugin_utils.h"
#include "py_server.h"
#include "py_plugin.h"

struct py_plugin_context
{
    struct py_server *pser;
    struct plugin_config *pcnf;
};


struct py_plugin_context * py_plugin_context_init(const char *path_to_opt_file, const char *envp[])
{
    struct py_plugin_context *context = (struct py_plugin_context *)malloc(sizeof(struct py_plugin_context));
    memset(context, 0, sizeof(struct py_plugin_context));
    
    context->pcnf = plugin_config_init(path_to_opt_file);
    if (!context->pcnf) {
        PLUGIN_ERROR("Can not open plugin options file: %s.", path_to_opt_file);
        goto error;
    }
    
    context->pser = py_server_init(context->pcnf, envp);
    if (!context->pser) {
        PLUGIN_ERROR("Failed to run python server");
        goto error;
    }
    
    return context;
    
error:
    if (context->pcnf)
        plugin_config_free(context->pcnf);
    free(context);
    return NULL;
}

void py_plugin_context_free(struct py_plugin_context *context)
{
    if (!context)
        return;
    
    py_server_term(context->pser);
    plugin_config_free(context->pcnf);
    free(context);
}


OPENVPN_PLUGIN_DEF openvpn_plugin_handle_t OPENVPN_PLUGIN_FUNC(openvpn_plugin_open_v2)(
     unsigned int *type_mask,
     const char *argv[],
     const char *envp[],
     struct openvpn_plugin_string_list **return_list)
{
    const char *verb_str = NULL;
    struct py_plugin_context *context = NULL;
    
    /* Init logginig */
    verb_str = get_openvpn_env("verb", envp);
    if (verb_str) {
        init_plugin_logging(atoi(verb_str));
    } else {
        init_plugin_logging(3);
    }
    
    if (string_array_len(argv) < 2) {
        PLUGIN_ERROR("Missed path to file with settings.");
        goto error;
    }
    
    context = py_plugin_context_init(argv[1], envp);
    if (!context)
        goto error;
    
    /* Intercept the --auth-user-pass-verify, --client-connect and --client-disconnect callback. */
    *type_mask = OPENVPN_PLUGIN_MASK(OPENVPN_PLUGIN_UP)
               | OPENVPN_PLUGIN_MASK(OPENVPN_PLUGIN_DOWN)
               | OPENVPN_PLUGIN_MASK(OPENVPN_PLUGIN_AUTH_USER_PASS_VERIFY)
               | OPENVPN_PLUGIN_MASK(OPENVPN_PLUGIN_CLIENT_CONNECT)
               | OPENVPN_PLUGIN_MASK(OPENVPN_PLUGIN_CLIENT_DISCONNECT);
    
    return (openvpn_plugin_handle_t)context;

error:
    return NULL;
}

OPENVPN_PLUGIN_DEF int OPENVPN_PLUGIN_FUNC(openvpn_plugin_func_v2)(
    openvpn_plugin_handle_t handle,
    const int type,
    const char *argv[],
    const char *envp[],
    void *per_client_context,
    struct openvpn_plugin_string_list **return_list)
{
    struct py_plugin_context *context = (struct py_plugin_context *)handle;
    
    py_server_send_command(context->pser, type, envp);
    
    return OPENVPN_PLUGIN_FUNC_SUCCESS;
}

OPENVPN_PLUGIN_DEF void OPENVPN_PLUGIN_FUNC(openvpn_plugin_close_v1)(
    openvpn_plugin_handle_t handle)
{
    struct py_plugin_context *context = (struct py_plugin_context *)handle;
    py_plugin_context_free(context);
}