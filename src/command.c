/**
 * Shelldone
 *
 * vim:ts=4:sw=4:expandtab
 *
 * Copyright (c) 2011, Ziirish <mr.ziirish@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *    This product includes software developed by Ziirish.
 * 4. Neither the name of the author nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY Ziirish ''AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL Ziirish BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef _BSD_SOURCE
    #define _BSD_SOURCE
#endif

#ifndef _GNU_SOURCE
        #define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <err.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>
#include <setjmp.h>

#include "builtin.h"
#include "command.h"
#include "parser.h"
#include "xutils.h"
#include "jobs.h"
#include "modules.h"

static const builtin calls[] = {{"cd", (cmd_builtin) sd_cd},
                                {"bg", (cmd_builtin) sd_bg},
                                {"fg", (cmd_builtin) sd_fg},
                                {"pwd", (cmd_builtin) sd_pwd},
                                {"exec", (cmd_builtin) sd_exec},
                                {"exit", (cmd_builtin) sd_exit},
                                {"jobs", (cmd_builtin) sd_jobs},
                                {"module", (cmd_builtin) sd_module},
                                {"rehash", (cmd_builtin) sd_rehash},
/*                              {"echo", (cmd_builtin) sd_echo}, */
                                {NULL, NULL}};

extern pid_t shell_pgid;
extern int shell_is_interactive;
extern int shell_terminal;
extern unsigned int interrupted;
extern sigjmp_buf env;
extern int val;

command *curr = NULL;
int ret_code;

void
sigstophandler (int sig)
{
    fprintf (stdout, "\n");
/*    signal (SIGTSTP, SIG_DFL);*/
/*    kill (curr->pid, SIGTSTP);*/
    enqueue_job (curr, TRUE);
    interrupted = TRUE;
    if (curr->continued)
    {
        free_command (curr);
        curr = NULL;
        siglongjmp (env, val);
    }
    (void) sig;
}

command *
new_command (void)
{
    command *ret = xmalloc (sizeof (*ret));
    if (ret != NULL)
    {
        ret->cmd = NULL;
        ret->argv = NULL;
        ret->protected = NULL;
        ret->argvf = NULL;
        ret->argc = 0;
        ret->argcf = 0;
        ret->flag = END;
        ret->in = STDIN_FILENO;
        ret->out = STDOUT_FILENO;
        ret->err = STDERR_FILENO;
        ret->builtin = FALSE;
        ret->stopped = FALSE;
        ret->continued = FALSE;
        ret->pid = -1;
        ret->job = -1;
    }
    return ret;
}

command_line *
new_cmd_line (void)
{
    command_line *ret = xmalloc (sizeof (*ret));
    if (ret != NULL)
    {
        ret->content = new_command ();
        ret->next = NULL;
        ret->prev = NULL;
    }
    return ret;
}

command *
copy_command (const command *src)
{
    if (src == NULL)
        return NULL;
    command *ret = new_command ();
    if (ret == NULL)
        return NULL;
    ret->cmd = xstrdup (src->cmd);
    ret->flag = src->flag;
    ret->in = src->in;
    ret->out = src->out;
    ret->err = src->err;
    ret->builtin = src->builtin;
    ret->stopped = src->stopped;
    ret->continued = src->continued;
    ret->pid = src->pid;
    ret->job = src->job;
    if (src->argc > 0)
    {
        ret->argv = xcalloc (src->argc, sizeof (char *));
        ret->protected = xcalloc (src->argc, sizeof (Protection));
        if (ret->argv != NULL && ret->protected != NULL)
        {
            int i;
            for (i = 0; i < src->argc; i++)
            {
                ret->argv[i] = xstrdup (src->argv[i]);
                ret->protected[i] = src->protected[i];
            }
            ret->argc = src->argc;
        }
        else
        {
            free_command (ret);
            return NULL;
        }
    }
    else
    {
        ret->argv = NULL;
        ret->protected = NULL;
        ret->argc = 0;
    }
    if (src->argcf > 0)
    {
        ret->argvf = xcalloc (src->argcf, sizeof (char *));
        int i;
        for (i = 0; i < src->argcf; i++)
            ret->argvf[i] = xstrdup (src->argvf[i]);
        ret->argcf = src->argcf;
    }
    else
    {
        ret->argvf = NULL;
        ret->argcf = 0;
    }
    return ret;
}

command_line *
copy_cmd_line (const command_line *src)
{
    command_line *ret = new_cmd_line ();
    if (ret == NULL)
        return NULL;
    ret->prev = src->prev;
    ret->next = src->next;
    xfree (ret->content);
    ret->content = copy_command (src->content);
    return ret;
}

int
compare_command (command *c1, command *c2)
{
    int ret = 0;
    if (c1 != NULL && c2 != NULL)
    {
        int c, i;
        if ((c = xstrcmp (c1->cmd, c2->cmd)) != 0)
            return c;
        if (c1->argc != c2->argc)
            return (c1->argc - c2->argc);
        for (i = 0; i < c1->argc; i++)
        {
            if ((c = xstrcmp (c1->argv[i], c2->argv[i])) != 0)
                return c;
            if (c1->protected[i] != c2->protected[i])
                return (c1->protected[i] - c2->protected[i]);
        }
    }
    return ret;
}

void
free_command (command *ptr)
{
    if (ptr != NULL)
    {
        int cpt;
        unsigned int copy = (ptr->argv == ptr->argvf);
        for (cpt = 0; cpt < ptr->argc; cpt++)
        {
            xfree (ptr->argv[cpt]);
            ptr->argv[cpt] = NULL;
        }
        if (!copy)
        {
            for (cpt = 0; cpt < ptr->argcf; cpt++)
            {
                xfree (ptr->argvf[cpt]);
                ptr->argvf[cpt] = NULL;
            }
            xfree (ptr->argvf);
        }
        xfree (ptr->argv);
        ptr->argv = NULL;
        ptr->argvf = NULL;
        xfree (ptr->protected);
        xfree (ptr->cmd);
        xfree (ptr);
        ptr = NULL;
    }
}

void
free_cmd_line (command_line *ptr)
{
    if (ptr != NULL)
    {
        free_command (ptr->content);
        xfree (ptr);
    }
}

static unsigned int 
check_wildcard_match (const char *text, const char *pattern)
{
    const char *rPat;
    const char *rText;
    char ch;
    unsigned int found;
    unsigned int seq;
    size_t len;
    char first, last, prev, mem;

    (void) last;
    (void) first;
    (void) mem;
    (void) seq;
    (void) prev;

    rPat = NULL;
    rText = NULL;

    while (*text || *pattern) 
    {
        ch = *pattern++;

        switch (ch) 
        {
        case '*':
            rPat = pattern;
            rText = text;
            break;

        case '[':
            found = FALSE;

            while ((ch = *pattern++) != ']') 
            {
                if (ch == '\\')
                    ch = *pattern++;

                if (ch == '\0')
                    return FALSE;

                if (*text == ch)
                    found = TRUE;
                prev = ch;
            }
            len = xstrlen (text);
            if (found == FALSE && len != 0) 
            {
                return FALSE;
            }
            if (found == TRUE) 
            {
                if (xstrlen (pattern) == 0 && len == 1) 
                {
                    return TRUE;
                }
                if (len != 0) 
                {
                    text++;
                    continue;
                }
            }

            /* fall into next case */

        case '?':
            if (*text++ == '\0')
                return FALSE;

            break;

        case '\\':
            ch = *pattern++;

            if (ch == '\0')
                return FALSE;

            /* fall into next case */

        default:
            if (*text == ch) 
            {
                if (*text)
                    text++;
                break;
            }

            if (*text) 
            {
                pattern = rPat;
                text = ++rText;
                break;
            }

            return FALSE;
        }

        if (pattern == NULL)
            return FALSE;
    }

    return TRUE;
}

pid_t
run_command (command_line *ptrc)
{
    if (ptrc == NULL)
        return -1;
    command *ptr = ptrc->content;
    sdplist *modules = get_modules_list_by_type (PARSING);
    if (modules != NULL)
    {
        void *data[] = {(void *)&ptr};
        int r = launch_each_module (modules, data);
        free_sdplist (modules);
        if (r != 1)
            return -1;
    }
    else
    {
        ptr->argvf = ptr->argv;
        ptr->argcf = ptr->argc;
    }
    curr = ptr;
    pid_t r = -1;
    if (ptr != NULL)
    {
        size_t len = xstrlen (ptr->cmd);
        if (len >= 2 && ptr->cmd[len - 1] == 'h' && ptr->cmd[len - 2] == 's')
        {
            if (!(len > 3 && 
                ptr->cmd[len - 1] == 'h' && 
                ptr->cmd[len - 2] == 's' && 
                ptr->cmd[len - 3] == '.')) 
            {
                fprintf (stdout, 
                         "BAZINGA! I iz in ur term blocking ur Shell!\n");
                ptr->builtin = TRUE;
                return 0;
            }
        }
        cmd_builtin call = NULL;
        int i = 0;
        while (calls[i].key != NULL)
        {
            if (xstrcmp (ptr->cmd, calls[i].key) == 0)
            {
                call = calls[i].func;
                break;
            }
            i++;
        }
        if (call != NULL)
        {
            /**
             * FIXME: little hack to avoid compilation warning
             */
            for (i = 0; i < ptr->argc; i++)
                if (check_wildcard_match (ptr->argv[i], "toto"))
                {
/*                        argv[i] = xstrdup (ptr->argv[i]); */;
                }
            r = call (ptr->argcf, ptr->argvf, ptr->in, ptr->out, ptr->err);
            ret_code = r;
            ptr->builtin = TRUE;
        }
        else
        {
            signal (SIGTSTP, sigstophandler);
            r = fork ();
            if (r == 0)
            {
                pid_t pid, pgid;
                if (shell_is_interactive)
                {
                    pid = getpid ();
                    pgid = shell_pgid;
                    if (pgid == 0)
                        pgid = pid;
                    setpgid (pid, pgid);
                    if (ptr->flag == BG)
                        signal (SIGTSTP, SIG_IGN);
                    else
                    {
                        tcsetpgrp (shell_terminal, pgid);
                        signal (SIGTSTP, SIG_DFL);
                    }
                    signal (SIGTTIN, SIG_DFL);
                    signal (SIGTTOU, SIG_DFL);
                    signal (SIGCHLD, SIG_DFL);
                }
                /*
                {
                    struct sigaction sa;
                    sa.sa_handler = childstophandler;
                    sigemptyset(&sa.sa_mask);
                    sa.sa_flags = 0;
                    if (sigaction (SIGTSTP, &sa, NULL) != 0)
                        err (3, "sigaction");
                }
                */
                /*
                signal (SIGINT, SIG_DFL);
                signal (SIGTSTP, SIG_DFL);
                signal (SIGCHLD, sighandler);
                */
                /*
                signal (SIGTSTP, sighandler);
                signal (SIGSTOP, sighandler);
                */
                if (ptr->in != STDIN_FILENO)
                {
                    dup2 (ptr->in, STDIN_FILENO);
                }
                if (ptr->out != STDOUT_FILENO)
                {
                    if (ptr->out == STDERR_FILENO)
                        dup2 (ptr->err, STDOUT_FILENO);
                    else
                        dup2 (ptr->out, STDOUT_FILENO);
                }
                if (ptr->err != STDERR_FILENO)
                {
                    if (ptr->err == STDOUT_FILENO)
                        dup2 (ptr->out, STDERR_FILENO);
                    else
                        dup2 (ptr->err, STDERR_FILENO);
                }
                /* Here we add the argv[0] which is the program name */
                char ** argv;
                if (ptr->argcf > 0)
                {
                    int i;
                    argv = xcalloc (ptr->argcf + 2, sizeof (char *));
                    argv[0] = ptr->cmd;
                    for (i = 1; i - 1 < ptr->argcf; i++)
                    {
                        /*
                        if (xstrcmp ("$?", ptr->argv[i-1]) == 0 && 
                            ptr->protected[i-1] != SINGLE_QUOTE)
                        {
                            char buf[128];
                            snprintf (buf, 128, "%d", ret_code);
                            argv[i] = buf;
                        }
                        else
                        */
                            argv[i] = ptr->argvf[i-1];
                    }
                    argv[i] = NULL;
                }
                else
                    argv = (char *[]){ptr->cmd, NULL};
                execvp (ptr->cmd, argv);
                if (ptr->argcf > 0)
                    xfree (argv);
                err (1, "%s", ptr->cmd);
            }
        }
    }
    return r;
}

void
run_line (input_line *ptr)
{
    CmdFlag flag;
    int ret = 0;
    if (ptr != NULL)
    {
        command_line *cmd = ptr->head;
        while (cmd != NULL)
        {
            switch (cmd->content->flag)
            {
            /* 
             * launch a command in background is pretty much the same as
             * launch it in forground except in one case we wait until it's
             * done and in the other case we don't
             */
            case BG:
            case END:
            {
                pid_t p = run_command (cmd);
                cmd->content->pid = p;
                /* p should never be equal to -1 */
                if (p != -1 && !cmd->content->builtin)
                {
                    if (cmd->content->flag == BG)
                    {
                        ret_code = 0;
                        enqueue_job (cmd->content, FALSE);
                    }
                    else
                    {
                        waitpid (p, &ret, WUNTRACED);
                        ret_code = WEXITSTATUS(ret);
                    }
                }
                else if (p == -1)
                    ret_code = 254;

                break;
            }
            case OR:
            case AND:
            {
                int nb = 1, i;
                pid_t p;
                command_line *exec = cmd, *save;
                flag = cmd->content->flag;
                while (exec != NULL && exec->content->flag == flag)
                {
                    nb++;
                    save = exec;
                    exec = exec->next;
                }
                if (exec != NULL)
                    save = exec;
                exec = cmd;
                p = run_command (exec);
                exec->content->pid = p;
                if (p != -1 && !exec->content->builtin)
                {
                    waitpid (p, &ret, WUNTRACED);
                    ret_code = WEXITSTATUS(ret);
                }
                else if (p == -1)
                    ret_code = 254;
                i = 1;
                exec = exec->next;
                while (i < nb && ((flag == AND) ? 
                                        (ret_code == 0) : 
                                        (ret_code != 0)))
                {
                    p = run_command (exec);
                    exec->content->pid = p;
                    if (p != -1 && !exec->content->builtin)
                    {
                        waitpid (p, &ret, WUNTRACED);
                        ret_code = WEXITSTATUS(ret);
                    }
                    else if (p == -1)
                        ret_code = 254;
                    exec = exec->next;
                    i++;
                }
                cmd = save;
                break;
            }
            case PIPE:
            {
                int nb = 1, i, fd[2];
                pid_t *p;
                unsigned int *builtins;
                command_line *exec = cmd, *save = cmd;
                while (exec != NULL && exec->content->flag == PIPE)
                {
                    nb++;
                    exec = exec->next;
                }
                p = xmalloc (nb * sizeof (pid_t));
                builtins = xmalloc (nb * sizeof (unsigned int));
                exec = cmd;
                for (i = 0; i < nb; i++)
                {
                    pipe (fd);
                    if (i != nb - 1)
                        exec->content->out = fd[1];
                    p[i] = run_command (exec);
                    builtins[i] = exec->content->builtin;
                    exec->content->pid = p[i];
                    if (exec->content->out != STDOUT_FILENO && 
                        exec->content->out != STDERR_FILENO)
                        close (exec->content->out);
                    if (exec->content->err != STDERR_FILENO && 
                        exec->content->err != STDOUT_FILENO)
                        close (exec->content->err);
                    save = exec;
                    exec = exec->next;
                    if (exec != NULL)
                        exec->content->in = fd[0];
                }
                for (i = 0; i < nb; i++)
                {
                    if (p[i] != -1 && !builtins[i])
                    {
                        waitpid (p[i], &ret, WUNTRACED);
                        ret_code = WEXITSTATUS(ret);
                    }
                    else if (p[i] == -1)
                        ret_code = 254;
                }
                xfree (p);
                xfree (builtins);
                cmd = save;
                break;
            }
            }
            cmd = cmd->next;
        }
    }
/*    exit (ret_code);*/
}
