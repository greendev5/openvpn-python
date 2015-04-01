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
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <signal.h>
#include <syslog.h>
#include <fcntl.h>
#include <errno.h>
#include <pwd.h>

#include "plugin_utils.h"
#include "deferred_queue.h"
#include "py_server.h"

/* Response codes for background -> foreground communication */
#define PYOVPN_RESPONSE_SUCCEEDED   10
#define PYOVPN_RESPONSE_FAILED      11

struct py_server
{
    /* Foreground's socket to background process */
    int foreground_fd;
    
    /* Process ID of background process */
    pid_t background_pid;
    
    /* Is backgroud process daemon process */
    unsigned char is_daemon;
};

/*
 * Close most of parent's fds.
 * Keep stdin/stdout/stderr, plus one other fd which 
 * is presumed to be our pipe back to parent.
 * Admittedly, a bit of a kludge, but posix doesn't 
 * give us a kind of FD_CLOEXEC which will stop
 * fds from crossing a fork().
 */
static void close_fds_except(int keep)
{
    int i;
    closelog();
    for (i = 3; i <= 100; ++i) {
        if (i != keep)
            close(i);
    }
}

/*
 * Usually we ignore signals, because our parent will
 * deal with them.
 */
static void set_signals (void)
{
    signal(SIGTERM, SIG_DFL);
    signal(SIGINT,  SIG_IGN);
    signal(SIGHUP,  SIG_IGN);
    signal(SIGUSR1, SIG_IGN);
    signal(SIGUSR2, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);
}

/**
 * @return 0 when all buf was sent, -1 in other case
 */
static int recv_buf(int fd, void *buf, size_t buf_size, size_t *bytes_read);

/**
 * @return 0 when all buf was received, -1 in other case
 */
static int send_buf(int fd, const void *buf, size_t buf_size, size_t *bytes_written);

/**
 * @return Response codes for background or -1 when communication was broken
 */
static int recv_pyovpn_resp(int fd, int *code)
{
    unsigned char c;
    const ssize_t size = recv(fd, &c, sizeof(c), 0);
    if (size == sizeof(c)) {
        if (c == PYOVPN_RESPONSE_SUCCEEDED)
            *code = PYOVPN_RESPONSE_SUCCEEDED;
        else if (c == PYOVPN_RESPONSE_FAILED)
            *code = PYOVPN_RESPONSE_FAILED;
        else
            return -1;
        return 0;
    }
    return -1;
}

/**
 * @return O - success, -1 - failed.
 */
static int send_pyovpn_resp(int fd, int code)
{
    unsigned char c = (unsigned char)code;
    const ssize_t size = send(fd, &c, sizeof(c), 0);
    if (size == sizeof(c))
        return 0;
    else
        return -1;
}

/**
 * @return O - success, -1 - failed.
 */
static int send_command(int fd, int command, const char *envp[])
{
    int ret, i;
    
    size_t len = string_array_len(envp);
    size_t mem_size = string_array_mem_size(envp);
    
    ret = send_buf(fd, &command, sizeof(int), NULL);
    if (ret != 0)
        return -1;
    
    ret = send_buf(fd, &len, sizeof(size_t), NULL);
    if (ret != 0)
        return -1;
    
    ret = send_buf(fd, &mem_size, sizeof(size_t), NULL);
    if (ret != 0)
        return -1;
    
    for (i = 0; i < len; ++i) {
        ret = send_buf(fd, envp[i], strlen(envp[i]) + 1, NULL);
        if (ret != 0)
            return -1;
    }
    
    return 0;
}

/**
 * @return O - success, -1 - failed.
 */
static int recv_command(int fd, int *command, struct ovpn_envp_buf **envp_buf)
{
    int ret;
    size_t i, j;
    size_t len;
    size_t mem_size;
    
    ret = recv_buf(fd, command, sizeof(int), NULL);
    if (ret != 0)
        return -1;
    
    ret = recv_buf(fd, &len, sizeof(size_t), NULL);
    if (ret != 0)
        return -1;
    
    ret = recv_buf(fd, &mem_size, sizeof(size_t), NULL);
    if (ret != 0)
        return -1;
    
    *envp_buf = ovpn_envp_buf_init(len, mem_size);
    
    ret = recv_buf(fd, (*envp_buf)->buf, mem_size, NULL);
    if (ret != 0) {
        ovpn_envp_buf_free(*envp_buf);
        return -1;
    }
    
    (*envp_buf)->envpn[0] = (*envp_buf)->buf;
    j = 1;
    if (j < len) {
        for (i = 0; i < mem_size; ++i) {
            if ((*envp_buf)->buf[i] == '\0') {
                (*envp_buf)->envpn[j] = &((*envp_buf)->buf[i + 1]);
                j++;
                if (j == len)
                    break;
            }
        }
    }
    (*envp_buf)->envpn[len] = NULL;
    
    return 0;
}

/*
 */
static void py_server_loop(int fd, struct py_context *context, struct deferred_queue *queue)
{
    int command;
    struct ovpn_envp_buf *envp_buf;
    struct deferred_queue_item *item;
    struct py_function_def *handler;
    
    while (1) {
        
        item = NULL;
        
        if (recv_command(fd, &command, &envp_buf) < 0) {
            PLUGIN_DEBUG("Python process: Connection with OpenVPN process interupted. Break listening loop");
            break;
        }
        
        handler = py_context_handler_func(context, command);
        if (handler != NULL) {
            item = create_deferred_queue_item(context, handler, envp_buf);
            add_item_to_deferred_queue(queue, item);
        } else {
            ovpn_envp_buf_free(envp_buf);
        }
        
        if (send_pyovpn_resp(fd, PYOVPN_RESPONSE_SUCCEEDED) < 0) {
            PLUGIN_DEBUG("Python process: Connection with OpenVPN process interupted. Break listening loop");
            break;
        }
    
    }
}

/*
 * Daemonize if "daemon" env var is true.
 * Preserve stderr across daemonization if
 * "daemon_log_redirect" env var is true.
 */
static void daemonize(const char *envp[])
{
    const char *daemon_string = get_openvpn_env("daemon", envp);
    const char *log_redirect  = get_openvpn_env("daemon_log_redirect", envp);
    int fd = -1;
    
    if (envp == NULL)
        return;
    
    if ((daemon_string != NULL) && (daemon_string[0] == '1')) {
        
        if (log_redirect && log_redirect[0] == '1')
            fd = dup(2);
        
        if (daemon (0, 0) < 0) {
            PLUGIN_ERROR("Python process: daemonization failed");
        
        } else if (fd >= 3) {
            dup2 (fd, 2);
            close (fd);
        
        }
    }
}

struct py_server * py_server_init(struct plugin_config *pcnf, const char *envp[])
{
    pid_t pid;
    int fd[2];
    struct py_server *pser;
    int verb;
    int ret, resp;
    struct passwd *pwd_data = NULL;
    struct py_context *context = NULL;
    struct deferred_queue *queue = NULL;
    const char *daemon_string = get_openvpn_env("daemon", envp);
    
    /*
     * Make a socket for foreground and background processes
     * to communicate.
     */
    if (socketpair(PF_UNIX, SOCK_STREAM, 0, fd) == -1) {
        PLUGIN_ERROR("OpenVPN Process: socketpair call failed");
        goto error;
    }
    
    /*
     * Fork off the privileged process.  It will remain privileged
     * even after the foreground process drops its privileges.
     */
    pid = fork();
    if (pid) {
        /* Foreground Process */
        
        /* close our copy of child's socket */
        close(fd[1]);
        
        /* don't let future subprocesses inherit child socket */
        if (fcntl(fd[0], F_SETFD, FD_CLOEXEC) < 0)
            PLUGIN_LOG("OpenVPN Process: Set FD_CLOEXEC flag on socket file descriptor failed");
        
        ret = recv_pyovpn_resp(fd[0], &resp);
        if ((ret < 0) || (resp != PYOVPN_RESPONSE_SUCCEEDED)) {
            PLUGIN_ERROR("OpenVPN Process: Python server process init failed");
            close(fd[0]);
            goto error;
        }
        
        pser = (struct py_server *)malloc(sizeof(struct py_server));
        pser->background_pid = pid;
        pser->foreground_fd = fd[0];
        if ((daemon_string != NULL) && (daemon_string[0] == '1'))
            pser->is_daemon = 1;
        else
            pser->is_daemon = 0;
        PLUGIN_LOG("OpenVPN Process: Python server process started with pid: %d", pid);
        return pser;
        
    } else {
        /* Background Process */
        
        ret = -1;

        /* close all parent fds except our socket back to parent */
        close_fds_except(fd[1]);
        
        /* Ignore most signals (the parent will receive them) */
        set_signals();
        
        /* Daemonize if --daemon option is set. */
        daemonize(envp);
        
        /* Init plugin loggin for multimple threads. */
        verb = get_plugin_logging_verbosity();
        init_plugin_logging_with_lock(verb);
        
        /* Set uid */
        if (pcnf->uid) {
            pwd_data = getpwnam(pcnf->uid);
            if (pwd_data)
                ret = setuid(pwd_data->pw_uid);
            if ((pwd_data == NULL) || (ret == -1)) {
                PLUGIN_ERROR("Python process: Could not set uid to %s.", pcnf->uid);
                PLUGIN_DEBUG("Python process: Sending PYOVPN_RESPONSE_FAILED from Python process to OpenVPN process");
                send_pyovpn_resp(fd[1], PYOVPN_RESPONSE_FAILED);
                PLUGIN_LOG("Python process: Python process is finished.");
                clear_plugin_logging_with_lock();
                close(fd[1]);
                exit(0);
            }
        }
        
        context = py_context_init(pcnf);
        if (context == NULL) {
            PLUGIN_DEBUG("Python process: Sending PYOVPN_RESPONSE_FAILED from Python process to OpenVPN process");
            ret = send_pyovpn_resp(fd[1], PYOVPN_RESPONSE_FAILED);
        } else {
            queue = create_deferred_queue();
            if (queue == NULL) {
                PLUGIN_DEBUG("Python process: Sending PYOVPN_RESPONSE_FAILED from Python process to OpenVPN process");
                ret = send_pyovpn_resp(fd[1], PYOVPN_RESPONSE_FAILED);
            } else {
                PLUGIN_DEBUG("Python process: Sending PYOVPN_RESPONSE_SUCCEEDED from Python process to OpenVPN process");
                ret = send_pyovpn_resp(fd[1], PYOVPN_RESPONSE_SUCCEEDED);
            }
        }
        
        if ((ret != -1) && (context != NULL) && (queue != NULL)) {
            py_server_loop(fd[1], context, queue);
        }
        
        if (queue != NULL)
            free_deferred_queue(queue);
        if (context != NULL)
            py_context_free(context);
        PLUGIN_LOG("Python process: Python process is finished.");
        
        clear_plugin_logging_with_lock();
        
        close(fd[1]);
        
        exit(0);
        return NULL; /* NOTREACHED */
    }
    
error:
    return NULL;
}

void py_server_term(struct py_server *pser)
{
    if (pser == NULL)
        return;
    
    close(pser->foreground_fd);
    
    /* Wait for child prcess */
    if (!pser->is_daemon)
        waitpid(pser->background_pid, NULL, 0);
    
    free(pser);
}

int py_server_send_command(struct py_server *pser, int command, const char *envp[])
{
    int resp;
    int ret = send_command(pser->foreground_fd, command, envp);
    if (ret < 0)
        return -1;
    
    ret = recv_pyovpn_resp(pser->foreground_fd, &resp);
    if ((ret < 0) || (resp != PYOVPN_RESPONSE_SUCCEEDED))
        return -1;
    return 0;
}

/*
 * A wrapper around <x-man-page://2/read> that keeps reading until either
 * buf_size bytes are read or until EOF is encountered, in which case you get
 * EPIPE.
 *
 * If bytes_read is not NULL, *bytes_read will be set to the number
 * of bytes successfully read.  On success, this will always be equal to
 * buf_size.  On error, it indicates how much was read before the error
 * occurred (which could be zero).
 */
static int recv_buf(int fd, void *buf, size_t buf_size, size_t *bytes_read)
{
    int 	err;
    char *	cursor;
    size_t	bytes_left;
    ssize_t bytes_this_time;
    
    err = 0;
    bytes_left = buf_size;
    cursor = (char *)buf;
    
    while ((err == 0) && (bytes_left != 0)) {
        bytes_this_time = read(fd, cursor, bytes_left);
        if (bytes_this_time > 0) {
            cursor     += bytes_this_time;
            bytes_left -= bytes_this_time;
        } else if (bytes_this_time == 0) {
            err = EPIPE;
        } else {
            err = errno;
            if (err == EINTR) {
                err = 0; /* let's loop again */
            }
        }
    }
    if (bytes_read != NULL) {
        *bytes_read = buf_size - bytes_left;
    }
    
    if (err != 0)
        return -1;
    return err;
}
/*
 * A wrapper around <x-man-page://2/write> that keeps writing until either
 * all the data is written or an error occurs, in which case
 * you get EPIPE.
 *
 * If bytes_written is not NULL, *bytes_written will be set to the number
 * of bytes successfully written.  On success, this will always be equal to
 * buf_size.  On error, it indicates how much was written before the error
 * occurred (which could be zero).
 */
static int send_buf(int fd, const void *buf, size_t buf_size, size_t *bytes_written)
{
    int 	err;
    char *	cursor;
    size_t	bytes_left;
    ssize_t bytes_this_time;
    
    err = 0;
    bytes_left = buf_size;
    cursor = (char *) buf;
    
    while ((err == 0) && (bytes_left != 0)) {
        bytes_this_time = write(fd, cursor, bytes_left);
        if (bytes_this_time > 0) {
            cursor     += bytes_this_time;
            bytes_left -= bytes_this_time;
        } else if (bytes_this_time == 0) {
            err = EPIPE;
        } else {
            err = errno;
            if (err == EINTR) {
                err = 0; /* let's loop again */
            }
        }
    }
    if (bytes_written != NULL) {
        *bytes_written = buf_size - bytes_left;
    }
    
    if (err != 0)
        return -1;
    return err;
}

