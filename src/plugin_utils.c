/* openvpn-python -- Plug-in embedding Python to OpenVPN.
 *
 * Copyright (C) ****** VPN Service 2015 <i.muzichuk@gmail.com>
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license.  See the LICENSE file for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "plugin_utils.h"

#define PLUGIN_OPT_ALLOC_AND_COPY(x, y) \
    x= (char *)malloc(sizeof(char) * strlen(y) + 1); \
    strncpy(x, y, strlen(y) + 1);

#define PLUGIN_OPT_FREE_IF_NOT_NULL(a) if (a != NULL) free((void *)a)

struct plugin_config * plugin_config_init(const char *filename)
{
    static char key[BUFSIZ];
    static char value[BUFSIZ];
    struct plugin_config *pcnf;
    FILE *fp;
    
    if ((fp = fopen(filename, "r")) == NULL)
        return NULL;
        
    pcnf = (struct plugin_config *)malloc(sizeof(struct plugin_config));
    memset(pcnf, 0, sizeof(struct plugin_config));
    
    while(!feof(fp)) {
        
        if (feof(fp)) break;
        
        if (fscanf(fp, "%s %[^\n]\n", key, value) != 2) continue;
        
        if (!strcmp(key, "virtualenv")) {
            PLUGIN_OPT_ALLOC_AND_COPY(pcnf->virtualenv, value);
        
        } else if (!strcmp(key, "pythonpath")) {
            PLUGIN_OPT_ALLOC_AND_COPY(pcnf->pythonpath, value);

        } else if (!strcmp(key, "uid")) {
            PLUGIN_OPT_ALLOC_AND_COPY(pcnf->uid, value);
            
        } else if (!strcmp(key, "mod_plugin_up")) {
            PLUGIN_OPT_ALLOC_AND_COPY(pcnf->mod_plugin_up, value);
        
        } else if (!strcmp(key, "func_plugin_up")) {
            PLUGIN_OPT_ALLOC_AND_COPY(pcnf->func_plugin_up, value);
        
        } else if (!strcmp(key, "mod_plugin_down")) {
            PLUGIN_OPT_ALLOC_AND_COPY(pcnf->mod_plugin_down, value);
        
        } else if (!strcmp(key, "func_plugin_down")) {
            PLUGIN_OPT_ALLOC_AND_COPY(pcnf->func_plugin_down, value);
        
        } else if (!strcmp(key, "mod_auth_user_pass_verify")) {
            PLUGIN_OPT_ALLOC_AND_COPY(pcnf->mod_auth_user_pass_verify, value);
        
        } else if (!strcmp(key, "func_auth_user_pass_verify")) {
            PLUGIN_OPT_ALLOC_AND_COPY(pcnf->func_auth_user_pass_verify, value);
        
        } else if (!strcmp(key, "mod_client_connect")) {
            PLUGIN_OPT_ALLOC_AND_COPY(pcnf->mod_client_connect, value);
        
        } else if (!strcmp(key, "func_client_connect")) {
            PLUGIN_OPT_ALLOC_AND_COPY(pcnf->func_client_connect, value);
        
        } else if (!strcmp(key, "mod_client_disconnect")) {
            PLUGIN_OPT_ALLOC_AND_COPY(pcnf->mod_client_disconnect, value);
        
        } else if (!strcmp(key, "func_client_disconnect")) {
            PLUGIN_OPT_ALLOC_AND_COPY(pcnf->func_client_disconnect, value);
        }
    }

    fclose(fp);
    
    if (pcnf->mod_plugin_up == NULL || pcnf->func_plugin_up == NULL ||
        pcnf->mod_plugin_down == NULL || pcnf->func_plugin_down == NULL) {
        /* Error in your config file. */
        plugin_config_free(pcnf);
        pcnf = NULL;
    }

    return pcnf;
}

void plugin_config_free(struct plugin_config *pcnf)
{
    if (pcnf == NULL)
        return;
    
    PLUGIN_OPT_FREE_IF_NOT_NULL(pcnf->virtualenv);
    PLUGIN_OPT_FREE_IF_NOT_NULL(pcnf->pythonpath);
    PLUGIN_OPT_FREE_IF_NOT_NULL(pcnf->uid);
    PLUGIN_OPT_FREE_IF_NOT_NULL(pcnf->mod_plugin_up);
    PLUGIN_OPT_FREE_IF_NOT_NULL(pcnf->func_plugin_up);
    PLUGIN_OPT_FREE_IF_NOT_NULL(pcnf->mod_plugin_down);
    PLUGIN_OPT_FREE_IF_NOT_NULL(pcnf->func_plugin_down);
    PLUGIN_OPT_FREE_IF_NOT_NULL(pcnf->mod_auth_user_pass_verify);
    PLUGIN_OPT_FREE_IF_NOT_NULL(pcnf->func_auth_user_pass_verify);
    PLUGIN_OPT_FREE_IF_NOT_NULL(pcnf->mod_client_connect);
    PLUGIN_OPT_FREE_IF_NOT_NULL(pcnf->func_client_connect);
    PLUGIN_OPT_FREE_IF_NOT_NULL(pcnf->mod_client_disconnect);
    PLUGIN_OPT_FREE_IF_NOT_NULL(pcnf->func_client_disconnect);
    
    free(pcnf);
}

struct ovpn_envp_buf * ovpn_envp_buf_init(size_t length, size_t buf_size)
{
    struct ovpn_envp_buf * envp_buf = (struct ovpn_envp_buf *)malloc(sizeof(struct ovpn_envp_buf));
    
    envp_buf->envpn = (char **)malloc(sizeof(char*) * (length + 1));
    envp_buf->envpn[length] = NULL;
    envp_buf->buf = (char *)malloc(sizeof(char) * buf_size);
    
    return envp_buf;
}

void ovpn_envp_buf_free(struct ovpn_envp_buf *envp_buf)
{
    if (envp_buf == NULL)
        return;
    
    free(envp_buf->envpn);
    free(envp_buf->buf);
    free(envp_buf);
}

size_t string_array_len(const char *array[])
{
    size_t i = 0;
    if (array) {
        while (array[i])
            ++i;
    }
    return i;
}

size_t string_array_mem_size(const char *array[])
{
    size_t mem_size = 0 ;
    size_t i = 0;
    
    if (array) {
        while (array[i]) {
            mem_size += strlen(array[i]) + 1;
            ++i;
        }
    }
    return mem_size;
}

const char * get_openvpn_env(const char *name, const char *envp[])
{
    size_t i;
    const size_t namelen = strlen(name);
    const char *cp = NULL;
    
    if (envp) {
        
        for (i = 0; envp[i]; i++) {
            if(!strncmp(envp[i], name, namelen)) {
                cp = envp[i] + namelen;
                if ( *cp == '=' )
                    return cp + 1;
            }
        }
    }
    return NULL;
}

/* Some tricks for thread safe logging */

typedef struct __plugin_logging__ {
    int verbosity;
    pthread_mutex_t *mutex_ptr;
} __plugin_logging_t__;

static __plugin_logging_t__ __plugin_loging_data__ = {0, NULL};

int __lock_plugin_logging__(void)
{
    if (__plugin_loging_data__.mutex_ptr != NULL)
        pthread_mutex_lock(__plugin_loging_data__.mutex_ptr);
    return __plugin_loging_data__.verbosity;
}

void __unlock_plugin_logging__(void)
{
    if (__plugin_loging_data__.mutex_ptr != NULL)
        pthread_mutex_unlock(__plugin_loging_data__.mutex_ptr);
}

void init_plugin_logging(int verbosity)
{
    __plugin_loging_data__.verbosity = verbosity;
}

void init_plugin_logging_with_lock(int verbosity)
{
    __plugin_loging_data__.verbosity = verbosity;
    
    __plugin_loging_data__.mutex_ptr = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(__plugin_loging_data__.mutex_ptr, NULL);
}

void clear_plugin_logging_with_lock(void)
{
    if (__plugin_loging_data__.mutex_ptr != NULL) {
        pthread_mutex_destroy(__plugin_loging_data__.mutex_ptr);
        __plugin_loging_data__.mutex_ptr = NULL;
    }
}

int get_plugin_logging_verbosity(void)
{
    return __plugin_loging_data__.verbosity;
}

/***************************************/



