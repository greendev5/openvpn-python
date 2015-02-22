/* openvpn-python -- Plug-in embedding Python to OpenVPN.
 *
 * Copyright (C) ****** VPN Service 2015 <i.muzichuk@gmail.com>
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license.  See the LICENSE file for details.
 */
#include <stdlib.h>
#include <string.h>

#include <Python.h>

#include "py_handlers.h"

#define PY_KEY_BUF_SIZE 1024

#define PY_FUNCTION_DEF_OR_NULL(x) \
    struct py_function_def *func_def = &context->x; \
    if ((func_def->module == NULL) || (func_def->module == NULL)) \
        return NULL; \
    return func_def;

struct py_function_def
{
    PyObject *module;
    PyObject *function;
};

struct py_context
{
    PyThreadState *main_thread_state;
    PyThreadState *worker_thread_state;
    
    struct py_function_def plugin_up_func;
    struct py_function_def plugin_down_func;
    struct py_function_def auth_user_pass_verify_func;
    struct py_function_def client_connect_func;
    struct py_function_def client_disconnect_func;
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

struct py_context * py_context_init(struct plugin_config *pcnf)
{
    static char name[] = "openvpn";
    static char *argv[] = {"openvpn"};
    struct py_context *pcnt = NULL;
    
    Py_SetProgramName(name);
    
    Py_InitializeEx(0);	/* Don't override signal handlers */
    
    PySys_SetArgv(1, argv);
    
    PyEval_InitThreads(); /* This also grabs a lock */
    
    pcnt = (struct py_context*)malloc(sizeof(struct py_context));
    memset(pcnt, 0, sizeof(struct py_context));
    
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
    
    pcnt->main_thread_state = PyThreadState_Get(); /* We need this for setting up thread local stuff */;
    pcnt->worker_thread_state = PyThreadState_New(pcnt->main_thread_state->interp);
    
    PyEval_ReleaseLock();
    
    PLUGIN_LOG("Python process: Python context initialized. Handlers loaded.");
    
    return pcnt;

failed:
    if (pcnt != NULL)
        free(pcnt);
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

struct py_function_def * plugin_up_func(struct py_context *context)
{
    PY_FUNCTION_DEF_OR_NULL(plugin_up_func);
}

struct py_function_def * plugin_down_func(struct py_context *context)
{
    PY_FUNCTION_DEF_OR_NULL(plugin_up_func);
}

struct py_function_def * auth_user_pass_verify_func(struct py_context *context)
{
    PY_FUNCTION_DEF_OR_NULL(auth_user_pass_verify_func);
}

struct py_function_def * client_connect_func(struct py_context *context)
{
    PY_FUNCTION_DEF_OR_NULL(client_connect_func);
}

struct py_function_def * client_disconnect_func(struct py_context *context)
{
    PY_FUNCTION_DEF_OR_NULL(client_disconnect_func);
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
