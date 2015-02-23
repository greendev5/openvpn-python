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

#include <stddef.h>
#include <stdio.h>

#include "openvpn-plugin.h"

#define PLUGIN_LOG(format, ...) {                                                     \
    __lock_plugin_logging__();                                                        \
    fprintf(stderr, "OPENVPN_PYTHON_PLUGIN: [INFO]: " format "\n", ##__VA_ARGS__);    \
    __unlock_plugin_logging__();                                                      \
}

#define PLUGIN_ERROR(format, ...) {                                                    \
    __lock_plugin_logging__();                                                         \
    fprintf(stderr, "OPENVPN_PYTHON_PLUGIN: [ERROR]: " format "\n", ##__VA_ARGS__);    \
    __unlock_plugin_logging__();                                                       \
}

#define PLUGIN_DEBUG(format, ...) {                                                        \
    int verb = __lock_plugin_logging__();                                                  \
    if (verb > 3)                                                                          \
        fprintf(stderr, "OPENVPN_PYTHON_PLUGIN: [DEBUG]: " format "\n", ##__VA_ARGS__);    \
    __unlock_plugin_logging__();                                                           \
}


struct plugin_config
{
    char *virtualenv;
    char *pythonpath;
    
    char *mod_plugin_up;
    char *func_plugin_up;
    
    char *mod_plugin_down;
    char *func_plugin_down;
    
    char *mod_auth_user_pass_verify;
    char *func_auth_user_pass_verify;
    
    char *mod_client_connect;
    char *func_client_connect;
    
    char *mod_client_disconnect;
    char *func_client_disconnect;
    
};

extern struct plugin_config * plugin_config_init(const char *filename);
extern void plugin_config_free(struct plugin_config *pcnf);

struct ovpn_envp_buf
{
    char **envpn;
    char *buf;
};

/*
 */
extern struct ovpn_envp_buf * ovpn_envp_buf_init(size_t length, size_t buf_size);

/*
 */
extern void ovpn_envp_buf_free(struct ovpn_envp_buf *envp_buf);

/**
 * @brief Return the length of a string array.
 *
 * @param Array
 * @return The length of the array.
 */
extern size_t string_array_len(const char *array[]);

/*
 */
extern size_t string_array_mem_size(const char *array[]);

/** @brief Search for env variable in openvpn format.
 *
 * Given an environmental variable name, search
 * the envp array for its value, returning it
 * if found or NULL otherwise.
 * A field in the envp-array looks like: name=user1
 *
 * @param The name of the variable.
 * @param The array with the enviromental variables.
 * @return A poniter to the variable value or NULL, if the varaible was not found.
 */
extern const char * get_openvpn_env(const char *name, const char *envp[]);

/**
 * This function just set verbosity. Use it if you want
 * to use logging from the single thread.
 * No need to clear logging which was init with this method.
 */
extern void init_plugin_logging(int verbosity);

/**
 * Set verbosity and init mutext. Use it if you want to
 * allow multiple thread use logging.
 * You should clear logging to release mutex.
 */
extern void init_plugin_logging_with_lock(int verbosity);

/**
 * Clear mutex from init_plugin_logging_with_lock.
 */
extern void clear_plugin_logging_with_lock(void);

extern int get_plugin_logging_verbosity();

int __lock_plugin_logging__(void);
void __unlock_plugin_logging__(void);

#endif /* _PLUGIN_UTILS_ */