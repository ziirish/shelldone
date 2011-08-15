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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <err.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>

#include "builtin.h"
#include "command.h"
#include "parser.h"
#include "xutils.h"

static const builtin calls[] = {{"cd", (cmd_builtin) sd_cd},
                                {"pwd", (cmd_builtin) sd_pwd},
                                {"exec", (cmd_builtin) sd_exec},
                                {"exit", (cmd_builtin) sd_exit},
                                {"rehash", (cmd_builtin) sd_rehash},
/*                              {"echo", (cmd_builtin) sd_echo}, */
                                {NULL, NULL}};

int ret_code = 0;
static char *lastcmd = NULL;

static void
sighandler (int sig)
{
    fprintf (stdout, "\nprogram %s exited\n", lastcmd);
    xfree (lastcmd);
    lastcmd = NULL;
    signal (SIGCHLD, SIG_DFL);
    (void) sig;
}

command *
new_cmd (void)
{
    command *ret = xmalloc (sizeof (*ret));
    if (ret != NULL)
    {
        ret->cmd = NULL;
        ret->argv = NULL;
        ret->argc = 0;
        ret->next = NULL;
        ret->prev = NULL;
        ret->flag = END;
        ret->in = STDIN_FILENO;
        ret->out = STDOUT_FILENO;
        ret->err = STDERR_FILENO;
        ret->builtin = FALSE;
        ret->pid = -1;
    }
    return ret;
}

void
free_cmd (command *ptr)
{
    if (ptr != NULL)
    {
        int cpt;
        for (cpt = 0; cpt < ptr->argc; cpt++)
            xfree (ptr->argv[cpt]);
        xfree (ptr->argv);
        xfree (ptr->cmd);
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
run_command (command *ptr)
{
    pid_t r = -1;
    if (ptr != NULL)
    {
        size_t len = xstrlen (ptr->cmd);
        if (len >= 2 && ptr->cmd[len - 1] == 'h' && ptr->cmd[len - 2] == 's')
        {
            if (len > 3 && 
                ptr->cmd[len - 1] == 'h' && 
                ptr->cmd[len - 2] == 's' && 
                ptr->cmd[len - 3] == '.') {
                ;
            }
            else
            {
                fprintf (stdout, "BAZINGA! I iz in ur term blocking ur Shell!\n");
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
            r = call (ptr->argc, ptr->argv, ptr->in, ptr->out, ptr->err);
            ret_code = r;
            ptr->builtin = TRUE;
        }
        else
        {
            r = fork ();
            if (r == 0)
            {
                signal (SIGINT, SIG_DFL);
                if (ptr->in != STDIN_FILENO)
                {
                    dup2 (ptr->in, STDIN_FILENO);
                }
                if (ptr->out != STDOUT_FILENO)
                {
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
                if (ptr->argc > 0)
                {
                    int i;
                    argv = xcalloc (ptr->argc + 2, sizeof (char *));
                    argv[0] = ptr->cmd;
                    for (i = 1; i - 1 < ptr->argc; i++)
                    {
                        if (xstrcmp ("$?", ptr->argv[i-1]) == 0)
                        {
                            char buf[128];
                            snprintf (buf, 128, "%d", ret_code);
                            argv[i] = buf;
                        }
                        else
                            argv[i] = ptr->argv[i-1];
                    }
                    argv[i] = NULL;
                }
                else
                    argv = (char *[]){ptr->cmd, NULL};
                execvp (ptr->cmd, argv);
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
    int ret;
    if (ptr != NULL)
    {
        command *cmd = ptr->head;
        while (cmd != NULL)
        {
            switch (cmd->flag)
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
                cmd->pid = p;
                /* p should never be equal to -1 */
                if (p != -1 && !cmd->builtin)
                {
                    waitpid (p, &ret, cmd->flag == BG ? WNOHANG : 0);
                    ret_code = WEXITSTATUS(ret);
                    if (cmd->flag == BG)
                    {
                        lastcmd = xstrdup (cmd->cmd);
                        signal (SIGCHLD, sighandler);
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
                command *exec = cmd, *save;
                flag = cmd->flag;
                while (exec != NULL && exec->flag == flag)
                {
                    nb++;
                    save = exec;
                    exec = exec->next;
                }
                if (exec != NULL)
                    save = exec;
                exec = cmd;
                p = run_command (exec);
                exec->pid = p;
                if (p != -1 && !exec->builtin)
                {
                    waitpid (p, &ret, 0);
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
                    exec->pid = p;
                    if (p != -1 && !exec->builtin)
                    {
                        waitpid (p, &ret, 0);
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
                command *exec = cmd, *save = cmd;
                while (exec != NULL && exec->flag == PIPE)
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
                        exec->out = fd[1];
                    p[i] = run_command (exec);
                    builtins[i] = exec->builtin;
                    exec->pid = p[i];
                    if (exec->out != STDOUT_FILENO)
                        close (exec->out);
                    if (exec->err != STDERR_FILENO && exec->err != STDOUT_FILENO)
                        close (exec->err);
                    save = exec;
                    exec = exec->next;
                    if (exec != NULL)
                        exec->in = fd[0];
                }
                for (i = 0; i < nb; i++)
                {
                    if (p[i] != -1 && !builtins[i])
                    {
                        waitpid (p[i], &ret, 0);
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
}
