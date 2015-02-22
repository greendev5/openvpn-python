#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/plugin_utils.h"

int test_plugin_config()
{
    const char *filename = "file.txt";
    FILE *fp;
    
    printf("%s\n", filename);
    remove(filename);
    
    fp = fopen(filename, "w");
    fprintf(fp, "%s %s\n", "virtualenv", "/home/admin/env");
    fprintf(fp, "%s %s\n", "pythonpath", "/srv/wasel/wasel-services/packages;/srv/wasel/wasel-services/waselpro");
    fprintf(fp, "%s %s\n", "mod_plugin_up", "waselradius.rmodule");
    fprintf(fp, "%s %s\n", "func_plugin_up", "instantiate");
    fprintf(fp, "%s %s\n", "mod_plugin_down", "waselradius.rmodule");
    fprintf(fp, "%s %s\n", "func_plugin_down", "term");
    fprintf(fp, "%s %s\n", "mod_auth_user_pass_verify", "waselradius.rmodule");
    fprintf(fp, "%s %s\n", "func_auth_user_pass_verify", "auth_user_pass_verify");
    fprintf(fp, "%s %s\n", "mod_client_connect", "waselradius.rmodule");
    fprintf(fp, "%s %s\n", "func_client_connect", "client_connect");
    fprintf(fp, "%s %s\n", "mod_client_disconnect", "waselradius.rmodule");
    fprintf(fp, "%s %s\n", "func_client_disconnect", "client_disconnect");
    fclose(fp);
    
    struct plugin_config *pcnf = plugin_config_init(filename);
    
    printf("%s\n", pcnf->mod_auth_user_pass_verify);
    printf("%s\n", pcnf->func_client_disconnect);
    printf("%s\n", pcnf->pythonpath);
    
    if (strcmp(pcnf->pythonpath, "/srv/wasel/wasel-services/packages;/srv/wasel/wasel-services/waselpro"))
        return 1;
    if (strcmp(pcnf->virtualenv, "/home/admin/env"))
        return 1;
    if (strcmp(pcnf->mod_plugin_up, "waselradius.rmodule"))
        return 1;
    if (strcmp(pcnf->func_plugin_up, "instantiate"))
        return 1;
    if (strcmp(pcnf->mod_plugin_down, "waselradius.rmodule"))
        return 1;
    if (strcmp(pcnf->func_plugin_down, "term"))
        return 1;
    if (strcmp(pcnf->mod_auth_user_pass_verify, "waselradius.rmodule"))
        return 1;
    if (strcmp(pcnf->func_auth_user_pass_verify, "auth_user_pass_verify"))
        return 1;
    if (strcmp(pcnf->mod_client_connect, "waselradius.rmodule"))
        return 1;
    if (strcmp(pcnf->func_client_connect, "client_connect"))
        return 1;
    if (strcmp(pcnf->mod_client_disconnect, "waselradius.rmodule"))
        return 1;
    if (strcmp(pcnf->func_client_disconnect, "client_disconnect"))
        return 1;
    
    plugin_config_free(pcnf);
    
    return 0;
}

void test_logging()
{
    init_plugin_logging(5);
    PLUGIN_DEBUG("%s", "DEBUG");
    PLUGIN_LOG("%s", "LOG");
    PLUGIN_ERROR("%s", "ERROR");
    
    init_plugin_logging_with_lock(5);
    PLUGIN_DEBUG("%s", "DEBUG PTHREAD");
    PLUGIN_LOG("%s", "LOG PTHREAD");
    PLUGIN_ERROR("%s", "ERROR PTHREAD");
    clear_plugin_logging_with_lock();
}

int test_ovpn_envp_buf()
{
    const char *envp[] = {
        "username=hello",
        "password=123",
        "trusted_ip=123.3.3.3",
        NULL
    };
    
    size_t length = string_array_len(envp);
    size_t buf_size = string_array_mem_size(envp);
    
    printf("envp len: %zu\n", length);
    printf("envp mem size: %zu\n", buf_size);
    
    struct ovpn_envp_buf * envp_buf = ovpn_envp_buf_init(length, buf_size);
    
    ovpn_envp_buf_free(envp_buf);
    
    return 0;
}

int main (void)
{
    if (test_plugin_config())
        return 1;
    
    test_logging();
    
    if (test_ovpn_envp_buf())
        return 1;
    
    return 0;
}
