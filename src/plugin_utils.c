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

#include "plugin_utils.h"

#define PLUGIN_OPT_ALLOC_AND_COPY(x, y) \
    x= (char *)malloc(sizeof(char) * strlen(y) + 1); \
    strncpy(x, y, strlen(y) + 1);

#define PLUGIN_OPT_FREE_IF_NOT_NULL(a) if (a != NULL) free (a)

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
    
    while(!feof(ffd)) {
        
        if (feof(ffd)) break;
        
        if (fscanf(ffd, "%s %[^\n]\n", key, value) != 2) continue;
        
        if (!strcmp(key, "virtualenv")) {
            PLUGIN_OPT_ALLOC_AND_COPY(pcnf->virtualenv, value);
        
        } else if (!strcmp(key, "pythonpath")) {
            PLUGIN_OPT_ALLOC_AND_COPY(pcnf->pythonpath, value);

        } else if (!strcmp(key, "mod_init")) {
            PLUGIN_OPT_ALLOC_AND_COPY(pcnf->mod_init, value);
        
        } else if (!strcmp(key, "func_init")) {
            PLUGIN_OPT_ALLOC_AND_COPY(pcnf->func_init, value);
        
        } else if (!strcmp(key, "mod_term")) {
            PLUGIN_OPT_ALLOC_AND_COPY(pcnf->mod_term, value);
        
        } else if (!strcmp(key, "func_term")) {
            PLUGIN_OPT_ALLOC_AND_COPY(pcnf->func_term, value);
        
        } else if (!strcmp(key, "mod_auth_user_pass_verify")) {
            PLUGIN_OPT_ALLOC_AND_COPY(pcnf->mod_auth_user_pass_verify, value);
        
        } else if (!strcmp(key, "func_auth_user_pass_verify")) {
            PLUGIN_OPT_ALLOC_AND_COPY(pcnf->func_auth_user_pass_verify, value);
        
        } else if (!strcmp(key, "mod_client_connect")) {
            PLUGIN_OPT_ALLOC_AND_COPY(pcnf->func_auth_user_pass_verify, value);
        
        } else if (!strcmp(key, "func_client_connect")) {
            PLUGIN_OPT_ALLOC_AND_COPY(pcnf->func_auth_user_pass_verify, value);
        
        } else if (!strcmp(key, "mod_client_disconnect")) {
            PLUGIN_OPT_ALLOC_AND_COPY(pcnf->func_auth_user_pass_verify, value);
        
        } else if (!strcmp(key, "func_clinet_disconnect")) {
            PLUGIN_OPT_ALLOC_AND_COPY(pcnf->func_auth_user_pass_verify, value);
        }
    }

    fclose(fp);
    
    if (pcnf->mod_init == NULL || pcnf->func_init == NULL ||
        pcnf->mod_term == NULL || pcnf->func_term == NULL) {
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
    PLUGIN_OPT_FREE_IF_NOT_NULL(pcnf->mod_init);
    PLUGIN_OPT_FREE_IF_NOT_NULL(pcnf->func_init);
    PLUGIN_OPT_FREE_IF_NOT_NULL(pcnf->mod_term);
    PLUGIN_OPT_FREE_IF_NOT_NULL(pcnf->func_term);
    PLUGIN_OPT_FREE_IF_NOT_NULL(pcnf->mod_auth_user_pass_verify);
    PLUGIN_OPT_FREE_IF_NOT_NULL(pcnf->func_auth_user_pass_verify);
    PLUGIN_OPT_FREE_IF_NOT_NULL(pcnf->mod_client_connect);
    PLUGIN_OPT_FREE_IF_NOT_NULL(pcnf->func_client_connect);
    PLUGIN_OPT_FREE_IF_NOT_NULL(pcnf->mod_client_disconnect);
    PLUGIN_OPT_FREE_IF_NOT_NULL(pcnf->func_clinet_disconnect);
    
    fre(pcnf);
}


