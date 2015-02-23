#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <unistd.h>
#include <errno.h>

#include "../src/plugin_utils.h"
#include "../src/py_server.h"
#include "../src/py_handlers.h"

const char *python_module_code = "\
import time\n\
import ovpnpy\n\
\n\
def plugin_up(envp):\n\
    s = str(envp)\n\
    ovpnpy.ovpnlog(ovpnpy.OVPN_LOG_INFO, 'start plugin_up for 2 sec: ' + s)\n\
    time.sleep(2)\n\
    ovpnpy.ovpnlog(ovpnpy.OVPN_LOG_INFO, 'end plugin_up: ' + s)\n\
    return True\n\
\n\
def plugin_down(envp):\n\
    s = str(envp)\n\
    ovpnpy.ovpnlog(ovpnpy.OVPN_LOG_INFO, 'plugin_down: ' + s)\n\
    return True\n\
\n\
def auth_user_pass_verify(envp):\n\
    s = str(envp)\n\
    ovpnpy.ovpnlog(ovpnpy.OVPN_LOG_INFO, 'auth_user_pass_verify: ' + s)\n\
    return True\n\
\n\
def client_connect(envp):\n\
    s = str(envp)\n\
    ovpnpy.ovpnlog(ovpnpy.OVPN_LOG_INFO, 'client_connect: ' + s)\n\
    return True\n\
\n\
def client_disconnect(envp):\n\
    s = str(envp)\n\
    ovpnpy.ovpnlog(ovpnpy.OVPN_LOG_INFO, 'client_disconnect: ' + s)\n\
    return True\n\
\n";

int main (void)
{
    const char *filename = "settings_test.conf";
    const char *pyfilename = "ovpn_test_module.py";
    FILE *fp;
    char cwd[1024];
    struct plugin_config *pcnf;
    struct py_server *pser;
    int count = 0;
    
    const char *init_envp[] = {
        "daemon=1",
        "daemon_log_redirect=1",
        NULL
    };
    
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

    
    memset(cwd, 0, sizeof(cwd));
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        fprintf(stdout, "Current working dir: %s\n", cwd);
    } else {
        perror("getcwd() error");
        return 1;
    }
    
    fp = fopen(filename, "w");
    /* fprintf(fp, "%s %s\n", "virtualenv", "/Users/green/Development/Wasel/experimental/env/wasel_sevices"); */
    fprintf(fp, "%s %s\n", "pythonpath", cwd);
    fprintf(fp, "%s %s\n", "mod_plugin_up", "ovpn_test_module");
    fprintf(fp, "%s %s\n", "func_plugin_up", "plugin_up");
    fprintf(fp, "%s %s\n", "mod_plugin_down", "ovpn_test_module");
    fprintf(fp, "%s %s\n", "func_plugin_down", "plugin_down");
    fprintf(fp, "%s %s\n", "mod_auth_user_pass_verify", "ovpn_test_module");
    fprintf(fp, "%s %s\n", "func_auth_user_pass_verify", "auth_user_pass_verify");
    fprintf(fp, "%s %s\n", "mod_client_connect", "ovpn_test_module");
    fprintf(fp, "%s %s\n", "func_client_connect", "client_connect");
    fprintf(fp, "%s %s\n", "mod_client_disconnect", "ovpn_test_module");
    fprintf(fp, "%s %s\n", "func_client_disconnect", "client_disconnect");
    fclose(fp);
    
    fp = fopen(pyfilename, "w");
    fprintf(fp, "%s", python_module_code);
    fclose(fp);
    
    init_plugin_logging(1);
    pcnf = plugin_config_init(filename);
    
    pser = py_server_init(pcnf, init_envp);
    if (!pser) {
        printf("Failed to runserver\n");
        return 0;
    }
    
    while (count < 5) {
        py_server_send_command(pser, OPENVPN_PLUGIN_UP, envp);
        py_server_send_command(pser, OPENVPN_PLUGIN_DOWN, envp);
        py_server_send_command(pser, OPENVPN_PLUGIN_AUTH_USER_PASS_VERIFY, envp);
        py_server_send_command(pser, OPENVPN_PLUGIN_CLIENT_CONNECT, envp1);
        py_server_send_command(pser, OPENVPN_PLUGIN_CLIENT_DISCONNECT, envp2);
        py_server_send_command(pser, OPENVPN_PLUGIN_TLS_FINAL, envp2);
    
        sleep(4);
        count++;
    }
    
    py_server_term(pser);
    
    return 0;
}