#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../src/plugin_utils.h"
#include "../src/py_server.h"
#include "../src/py_handlers.h"

int main (void)
{
    const char *filename = "file.txt";
    FILE *fp;
    
    printf("%s\n", filename);
    remove(filename);
    
    fp = fopen(filename, "w");
    fprintf(fp, "%s %s\n", "virtualenv", "/home/admin/env");
    fprintf(fp, "%s %s\n", "pythonpath", "/srv/wasel/wasel-services/packages;/srv/wasel/wasel-services/waselpro");
    fprintf(fp, "%s %s\n", "mod_plugin_up", "some");
    fprintf(fp, "%s %s\n", "func_plugin_up", "instantiate");
    fprintf(fp, "%s %s\n", "mod_plugin_down", "some");
    fprintf(fp, "%s %s\n", "func_plugin_down", "term");
    fprintf(fp, "%s %s\n", "mod_auth_user_pass_verify", "some");
    fprintf(fp, "%s %s\n", "func_auth_user_pass_verify", "auth_user_pass_verify");
    fprintf(fp, "%s %s\n", "mod_client_connect", "some");
    fprintf(fp, "%s %s\n", "func_client_connect", "client_connect");
    fprintf(fp, "%s %s\n", "mod_client_disconnect", "some");
    fprintf(fp, "%s %s\n", "func_client_disconnect", "client_disconnect");
    fclose(fp);
    
    init_plugin_logging(100);
    
    struct plugin_config *pcnf = plugin_config_init(filename);
    
    
    /*struct py_context *context = py_context_init(pcnf);
    py_context_free(context);
    
    return 0;*/
    
    struct py_server *pser = py_server_init(pcnf);
    if (!pser) {
        printf("Failed to runserver\n");
        return 0;
    }
    
    const char *envp[] = {
        "username=hello",
        "password=123",
        "trusted_ip=123.3.3.3",
        NULL
    };
    
    const char *envp1[] = {
        "username=hello",
        "password=123",
        "trusted_ip=123.3.3.3",
        "dfsdfs=123.3.3.3",
        "ddsfsdf=123.3.3.3",
        "trusted_ip=123.3.3.3",
        "trsfdsfsusted_ip=123.3dfsdfsdf.3.3",
        "fsdfsdfdsfsdfsdfsdfsdf=123.sdfsdfsdf3.3.3",
        NULL
    };
    
    const char *envp2[] = {
        "usernadfasfasdfame=hello",
        "pasdafdasfadsfsword=123",
        "truasdfadsfadsfsted_ip=123.3fadsfasfadsfasdfas.3.3",
        "truasdfadsfaddddsfsted_ip=123.3fadsfasfadsfasdfas.3.3",
        "truasdfadsfaddddsfsted_ip=123.3fadsfasfadsfasdfas.3.3",
        NULL
    };
    
    while (1) {
        py_auth_user_pass_verify(pser, envp);
        py_client_connect(pser, envp1);
        py_client_disconnect(pser, envp2);
    
        printf("GOGOGOGOGOGO\n");
        sleep(4);
        printf("Stop\n");
    }
    
    py_server_term(pser);
    
    return 0;
}