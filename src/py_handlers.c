/* openvpn-python -- Plug-in embedding Python to OpenVPN.
 *
 * Copyright (C) ****** VPN Service 2015 <i.muzichuk@gmail.com>
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license.  See the LICENSE file for details.
 */
#include <Python.h>

#include <stdlib.h>
#include <string.h>

#include "py_handlers.h"

#define PY_KEY_BUF_SIZE 1024

#define OVPN_LOG_DEBUG 0
#define OVPN_LOG_INFO  1
#define OVPN_LOG_ERROR 2

struct py_function_def
{
    PyObject *module;
    PyObject *function;
};

struct py_context
{
    PyThreadState *main_thread_state;
    PyThreadState *worker_thread_state;
    
    PyObject *ovpnpy_module;
    
    struct py_function_def plugin_up_func;
    struct py_function_def plugin_down_func;
    struct py_function_def auth_user_pass_verify_func;
    struct py_function_def client_connect_func;
    struct py_function_def client_disconnect_func;
    
    struct py_function_def *handlers_mapper[OPENVPN_PLUGIN_N];
};

/*
 */
static void fill_py_context_mapper(struct py_context *context)
{
    context->handlers_mapper[OPENVPN_PLUGIN_UP] = &context->plugin_up_func;
    context->handlers_mapper[OPENVPN_PLUGIN_DOWN] = &context->plugin_down_func;
    
    /* context->handlers_mapper[OPENVPN_PLUGIN_ROUTE_UP]   = NULL; */
    /* context->handlers_mapper[OPENVPN_PLUGIN_IPCHANGE]   = NULL; */
    /* context->handlers_mapper[OPENVPN_PLUGIN_TLS_VERIFY] = NULL; */
    
    context->handlers_mapper[OPENVPN_PLUGIN_AUTH_USER_PASS_VERIFY] = &context->auth_user_pass_verify_func;
    context->handlers_mapper[OPENVPN_PLUGIN_CLIENT_CONNECT] = &context->client_connect_func;
    context->handlers_mapper[OPENVPN_PLUGIN_CLIENT_DISCONNECT] = &context->client_disconnect_func;
    
    /* context->handlers_mapper[OPENVPN_PLUGIN_LEARN_ADDRESS]     = NULL; */
    /* context->handlers_mapper[OPENVPN_PLUGIN_CLIENT_CONNECT_V2] = NULL; */
    /* context->handlers_mapper[OPENVPN_PLUGIN_TLS_FINAL]         = NULL; */
    /* context->handlers_mapper[OPENVPN_PLUGIN_ENABLE_PF]         = NULL; */
}

/*
 */
static struct
{
    const char *name;
    int  value;
} ovpnpy_constants[] = {
    {"OVPN_LOG_DEBUG", OVPN_LOG_DEBUG},
    {"OVPN_LOG_INFO",  OVPN_LOG_INFO},
    {"OVPN_LOG_ERROR", OVPN_LOG_ERROR},
    
    { NULL, 0 }
};

static PyObject * python_ovpn_log(PyObject *module, PyObject *args)
{
    int status;
    char *msg;
    
    if (!PyArg_ParseTuple(args, "is", &status, &msg)) {
        return NULL;
    }
    
    switch (status) {
            
        case OVPN_LOG_INFO:
            PLUGIN_LOG("%s", msg);
            break;
            
        case OVPN_LOG_ERROR:
            PLUGIN_ERROR("%s", msg);
            break;
            
        default:
            PLUGIN_DEBUG("%s", msg);
            break;
    }
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyMethodDef ovpnpy_methods[] = {
    {
        "ovpnlog",
        &python_ovpn_log,
        METH_VARARGS,
        "ovpnpy.ovpnlog(level, msg)\n\n" \
        "Print a message using openvpn logging system. level should be one of the\n" \
        "constants OVPN_LOG_DEBUG, OVPN_LOG_INFO, OVPN_LOG_ERROR.\n"
    },
    
    { NULL, NULL, 0, NULL },
};

static void python_error(void)
{
    /* This will be called with the GIL lock held */
    
    PyObject
    *pType = NULL,
    *pValue = NULL,
    *pTraceback = NULL,
    *pStr1 = NULL,
    *pStr2 = NULL;
    
    PyErr_Fetch(&pType, &pValue, &pTraceback);
    if (pType == NULL || pValue == NULL)
        goto failed;
    if (((pStr1 = PyObject_Str(pType)) == NULL) ||
        ((pStr2 = PyObject_Str(pValue)) == NULL))
        goto failed;
    
    PLUGIN_ERROR("PYTHON EXCEPT:%s: %s", PyString_AsString(pStr1), PyString_AsString(pStr2));
    
failed:
    Py_XDECREF(pStr1);
    Py_XDECREF(pStr2);
    Py_XDECREF(pType);
    Py_XDECREF(pValue);
    Py_XDECREF(pTraceback);
}

static PyObject * envp_to_py_dict(char **envp)
{
    /* This will be called with the GIL lock held */
    
    static char key_buff[PY_KEY_BUF_SIZE];
    size_t i, j;
    PyObject *py_dict;
    
    py_dict = PyDict_New();
    
    for (i = 0; envp[i] != NULL; ++i) {
        for (j = 0; j < strlen(envp[i]); ++j) {
            
            if ((envp[i][j] == '=') && (j < PY_KEY_BUF_SIZE - 1)) {
                
                memcpy(key_buff, envp[i], j);
                key_buff[j] = '\0';
                
                PyDict_SetItemString(py_dict, key_buff, PyString_FromString(envp[i] + j + 1));
                
                break;
            }
            
        }
    }
    
    return py_dict;
}

static int python_load_function(const char *module_name, const char *function_name, struct py_function_def *def)
{
    /* This will be called with the GIL lock held */
    
    const char *funcname = "python_load_function";
    
    if ((module_name != NULL) && (function_name != NULL)) {
        
        if ((def->module = PyImport_ImportModule(module_name)) == NULL) {
            PLUGIN_ERROR("PYTHON HANDLERS [%s]: module '%s' is not found", funcname, module_name);
            goto failed;
        }
        
        if ((def->function = PyObject_GetAttrString(def->module, function_name)) == NULL) {
            PLUGIN_ERROR("PYTHON HANDLERS [%s]: function '%s.%s' is not found", funcname, module_name, function_name);
            goto failed;
        }
        
        if (!PyCallable_Check(def->function)) {
            PLUGIN_ERROR("PYTHON HANDLERS [%s]: function '%s.%s' is not callable", funcname, module_name, function_name);
            goto failed;
        }
    }

    return 0;
    
failed:
    python_error();
    Py_XDECREF(def->function);
    def->function = NULL;
    Py_XDECREF(def->module);
    def->module = NULL;
    return -1;
}

static void python_clear_function(struct py_function_def *def)
{
    /* This will be called with the GIL lock held */
    
    Py_XDECREF(def->function);
    def->function = NULL;
    Py_XDECREF(def->module);
    def->module = NULL;
}

static void python_add_path(const char *pythonpath)
{
    /* This will be called with the GIL lock held */
    
    PyObject *sys_path;
    PyObject *path;
    size_t len, i, prev_i;
    
    if (pythonpath == NULL)
        return;
    
    if (!strlen(pythonpath))
        return;
    
    sys_path = PySys_GetObject("path");
    if (sys_path == NULL)
        return;
    
    for (i = prev_i = 0; i <= strlen(pythonpath); ++i) {
        if ((pythonpath[i] == ';') || (pythonpath[i] == '\0')) {
            len = i - prev_i;
            if (len > 0) {
                path = PyString_FromStringAndSize(pythonpath + prev_i, len);
                if (path)
                    PyList_Append(sys_path, path);
            }
            prev_i = i + 1;
        }
    }
}

struct py_context * py_context_init(struct plugin_config *pcnf)
{
    static char name[] = "openvpn";
    static char *argv[] = {"openvpn"};
    static char *module_descr = "OpenVPN python plugin loggin module.";
    struct py_context *pcnt = NULL;
    size_t i;
    
    Py_SetProgramName(name);
    
    /* Set virtual env */
    if (pcnf->virtualenv) {
        PLUGIN_LOG("Setting virtualenv to %s", pcnf->virtualenv);
        Py_SetPythonHome(pcnf->virtualenv);
        PLUGIN_LOG("Done setting virtualenv");
    } else {
        PLUGIN_LOG("No virtualenv in settings");
    }
    
    Py_InitializeEx(0);	/* Don't override signal handlers */
    
    PySys_SetArgv(1, argv);
    
    PyEval_InitThreads(); /* This also grabs a lock */
    
    python_add_path(pcnf->pythonpath);
    
    pcnt = (struct py_context*)malloc(sizeof(struct py_context));
    memset(pcnt, 0, sizeof(struct py_context));
    
    pcnt->ovpnpy_module = Py_InitModule3("ovpnpy", ovpnpy_methods, module_descr);
    if (pcnt->ovpnpy_module == NULL)
        goto failed;
    
    for (i = 0; ovpnpy_constants[i].name != NULL; ++i) {
        if ((PyModule_AddIntConstant(pcnt->ovpnpy_module,
                                     ovpnpy_constants[i].name,
                                     ovpnpy_constants[i].value)) < 0)
            goto failed;
    }
    
    if (python_load_function(pcnf->mod_plugin_up,
                             pcnf->func_plugin_up,
                             &pcnt->plugin_up_func) < 0)
        goto failed;

    if (python_load_function(pcnf->mod_plugin_down,
                             pcnf->func_plugin_down,
                             &pcnt->plugin_down_func) < 0)
        goto failed;
    
    if (python_load_function(pcnf->mod_auth_user_pass_verify,
                             pcnf->func_auth_user_pass_verify,
                             &pcnt->auth_user_pass_verify_func) < 0)
        goto failed;
    
    if (python_load_function(pcnf->mod_client_connect,
                             pcnf->func_client_connect,
                             &pcnt->client_connect_func) < 0)
        goto failed;
    
    if (python_load_function(pcnf->mod_client_disconnect,
                             pcnf->func_client_disconnect,
                             &pcnt->client_disconnect_func) < 0)
        goto failed;
    
    fill_py_context_mapper(pcnt);
    
    pcnt->main_thread_state = PyThreadState_Get(); /* We need this for setting up thread local stuff */;
    pcnt->worker_thread_state = PyThreadState_New(pcnt->main_thread_state->interp);
    
    PyEval_ReleaseLock();
    
    PLUGIN_LOG("Python process: Python context initialized. Handlers loaded.");
    
    return pcnt;

failed:
    if (pcnt != NULL) {
        /** @todo Need solve problem with module cleanup */
        /* Py_XDECREF(pcnt->ovpnpy_module); */
        free(pcnt);
    }
    python_error();
    PyEval_ReleaseLock();
    Py_Finalize();
    return NULL;
}

void py_context_free(struct py_context *context)
{
    PyThreadState *tmp_state;
    if (context == NULL)
        return;
    
    PyEval_AcquireLock();
    tmp_state = PyThreadState_Swap(context->main_thread_state);
    
    /** @todo Need solve problem with module cleanup */
    /* Py_XDECREF(context->ovpnpy_module); */
    
    python_clear_function(&context->plugin_up_func);
    python_clear_function(&context->plugin_down_func);
    python_clear_function(&context->auth_user_pass_verify_func);
    python_clear_function(&context->client_connect_func);
    python_clear_function(&context->client_disconnect_func);
    
    PyThreadState_Swap(tmp_state);
    PyEval_ReleaseLock();
    
    PyThreadState_Clear(context->worker_thread_state);
    PyThreadState_Delete(context->worker_thread_state);
    
    free(context);
    
    Py_Finalize();
    
    PLUGIN_LOG("Python process: Python context cleared. Handlers cleared.");
}

struct py_function_def * py_context_handler_func(struct py_context *context, int command)
{
    if ((command < 0) || (command >= OPENVPN_PLUGIN_N))
        return NULL;
    
    return context->handlers_mapper[command];
}

int py_context_exec_func(struct py_context *context, struct py_function_def *func, char **envp)
{
    PyThreadState *tmp_state;
    PyObject *py_dict, *py_args, *py_ret;
    int ret = -1;
    
    if (context == NULL)
        return ret;
    
    PyEval_AcquireLock();
    tmp_state = PyThreadState_Swap(context->worker_thread_state);
    
    py_dict = envp_to_py_dict(envp);
    
    py_args = PyTuple_New(1);
    PyTuple_SetItem(py_args, 0, py_dict);
    
    py_ret = PyObject_CallObject(func->function, py_args);
    
    if (py_ret == NULL) {
        python_error();
    } else {
        if (PyObject_IsTrue(py_ret))
            ret = 0;
        Py_XDECREF(py_ret);
    }
    
    Py_XDECREF(py_dict);
    Py_XDECREF(py_args);
    
    PyThreadState_Swap(tmp_state);
    PyEval_ReleaseLock();
    
    return ret;
}
