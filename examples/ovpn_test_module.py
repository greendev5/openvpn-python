import ovpnpy

def plugin_up(envp):
    s = str(envp)
    ovpnpy.ovpnlog(ovpnpy.OVPN_LOG_INFO, 'start plugin_up for 2 sec: ' + s)
    return True

def plugin_down(envp):
    s = str(envp)
    ovpnpy.ovpnlog(ovpnpy.OVPN_LOG_INFO, 'plugin_down: ' + s)
    return True

def auth_user_pass_verify(envp):
    s = str(envp)
    ovpnpy.ovpnlog(ovpnpy.OVPN_LOG_INFO, 'auth_user_pass_verify: ' + s)
    return True

def client_connect(envp):
    s = str(envp)
    ovpnpy.ovpnlog(ovpnpy.OVPN_LOG_INFO, 'client_connect: ' + s)
    return True

def client_disconnect(envp):
    s = str(envp)
    ovpnpy.ovpnlog(ovpnpy.OVPN_LOG_INFO, 'client_disconnect: ' + s)
    return True

