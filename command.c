/**
 * Shelldone
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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

#include "builtin.h"
#include "command.h"
#include "xutils.h"

#define BUF 128
#define ARGC 10

static builtin calls[] = {{"cd", (cmd_builtin) sd_cd},
                          {"pwd", (cmd_builtin) sd_pwd},
                          {"echo", (cmd_builtin) sd_echo},
                          {NULL, NULL}};

static char *
completion (char *buf, int ind)
{
    char *tmp = xmalloc (ind);
    char **split;
    size_t s_split;
    int i;
    for (i = 0; i < ind - 1; i++, tmp[i] = buf[i]);
    tmp[i] = '\0';
    split = xstrsplit (tmp, " ", &s_split);

    xfree_list (split, s_split);
    xfree (tmp);
    return NULL;
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
        ret->in = 0;
        ret->out = 1;
        ret->err = 2;
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

void
free_line (input_line *ptr)
{
    if (ptr != NULL)
    {
        int cpt = 0;
        command *tmp = ptr->head;
        while (cpt < ptr->nb && tmp != NULL)
        {
            command *tmp2 = tmp->next;
            free_cmd (tmp);
            tmp = tmp2;
            cpt++;
        }
        xfree (ptr);
    }
}

static void
line_append (input_line **ptr, command *data)
{
    if (*ptr != NULL && data != NULL)
    {
        if ((*ptr)->tail == NULL)
        {
            (*ptr)->head = data;
            (*ptr)->tail = data;
            data->next = NULL;
            data->prev = NULL;
        }
        else
        {
            (*ptr)->tail->next = data;
            data->prev = (*ptr)->tail;
            (*ptr)->tail = data;
            data->next = NULL;
        }
        (*ptr)->nb++;
    }
}

static command *
copy_cmd (const command *src)
{
    command *ret = new_cmd ();
    if (ret == NULL)
        return NULL;
    ret->cmd = xstrdup (src->cmd);
    ret->argv = xcalloc (src->argc, sizeof (char *));
    if (ret->argv != NULL)
    {
        int i;
        for (i = 0; i < src->argc; i++)
            ret->argv[i] = xstrdup (src->argv[i]);
        ret->argc = src->argc;
    }
    else
    {
        free_cmd (ret);
        return NULL;
    }
    return ret;
}

void
dump_cmd (command *ptr)
{
    if (ptr != NULL)
    {
        int i;
        fprintf (stdout, "cmd: %s\n", ptr->cmd);
        fprintf (stdout, "argc: %d\n", ptr->argc);
        for (i = 0; i < ptr->argc; i++)
            fprintf (stdout, "argv[%d]: %s\n", i, ptr->argv[i]);
    }
}

void
dump_line (input_line *ptr)
{
    if (ptr != NULL)
    {
        command *tmp = ptr->head;
        int cpt = 0;
        if (ptr->nb > 0)
            fprintf (stdout, "nb commands: %d\n", ptr->nb);
        while (tmp != NULL)
        {
            fprintf (stdout, "=== Dump cmd n°%d ===\n", ++cpt);
            dump_cmd (tmp);
            tmp = tmp->next;
        }
    }
}

input_line *
parse_line (const char *l)
{
    input_line *ret;
    size_t cpt = 0;
    size_t size = xstrlen (l);
    int new_word = 0, first = 1, new_command = 0, begin = 1, i = 0, factor = 1,
        factor2 = 1, arg = 0, squote = 0, dquote = 0;
    command *curr = NULL;
    ret = xmalloc (sizeof (*ret));
    if (ret == NULL)
        return NULL;
    ret->nb = 0;
    ret->head = NULL;
    ret->tail = NULL;
    curr = new_cmd ();
    if (curr == NULL)
    {
        free_line (ret);
        return NULL;
    }
    while (cpt < size)
    {
        if (l[cpt] == '\'' && !dquote)
        {
            cpt++;
            squote = !squote;
            continue;
        }
        if (l[cpt] == '"' && !squote)
        {
            cpt++;
            dquote = !dquote;
            continue;
        }
        if (l[cpt] == ' ' && !(squote || dquote))
        {
            cpt++;
            if (!first)
            {
                if (i != 0 && begin)
                {
                    curr->cmd[i] = '\0';
                }
                else if (i != 0)
                {
                    curr->argv[curr->argc][i] = '\0';
                    curr->argc++;
                }
                new_word = 1;
                factor = 1;
                factor2 = 1;
                arg = 0;
                i = 0;
                begin = 0;
            }
            continue;
        }
        if (!(squote || dquote) &&
            (l[cpt] == '|' || l[cpt] == ';' || l[cpt] == '&'))
        {
            cpt++;
            if (i != 0 && begin)
            {
                curr->cmd[i] = '\0';
            }
            else if (i != 0)
            {
                curr->argv[curr->argc][i] = '\0';
                curr->argc++;
            }
            switch (l[cpt-1]) 
            {
            case '|':
                if (cpt < size && l[cpt] == '|')
                {
                    cpt++;
                    curr->flag = OR;
                }
                else
                    curr->flag = PIPE;
                break;
            case ';':
                curr->flag = END;
                break;
            case '&':
                if (cpt < size && l[cpt] == '&')
                {
                    cpt++;
                    curr->flag = AND;
                }
                else
                    curr->flag = BG;
                break;
            }
            new_command = 1;
            new_word = 0;
            begin = 1;
            factor = 1;
            factor2 = 1;
            arg = 0;
            first = 1;
            i = 0;
            continue;
        }
        first = 0;
        if (new_command)
        {
            new_command = 0;
            command *tmp = copy_cmd (curr);
            line_append (&ret, tmp);
            free_cmd (curr);
            curr = new_cmd ();
            if (curr == NULL)
            {
                free_line (ret);
                return NULL;
            }
        }
        if (begin && !new_word)
        {
            if (curr->cmd == NULL)
            {
                curr->cmd = xmalloc (factor * BUF * sizeof (char));
                if (curr->cmd == NULL)
                {
                    free_cmd (curr);
                    free_line (ret);
                    return NULL;
                }
            }
            if (i >= factor * BUF)
            {
                factor++;
                char *tmp = xrealloc (curr->cmd, factor * BUF * sizeof (char));
                if (tmp == NULL)
                {
                    free_cmd (curr);
                    free_line (ret);
                    return NULL;
                }
                curr->cmd = tmp;
            }
            curr->cmd[i] = l[cpt];
        }
        else if (new_word)
        {
            if (curr->argc == 0 && !arg)
            {
                curr->argv = xcalloc (factor * ARGC, sizeof (char *));
                if (curr->argv == NULL)
                {
                    free_cmd (curr);
                    free_line (ret);
                    return NULL;
                }
                arg = 1;
            }
            else if (curr->argc >= factor * ARGC)
            {
                factor++;
                char **tmp = xrealloc (curr->argv, 
                                       factor * ARGC * sizeof (char *));
                if (tmp == NULL)
                {
                    free_cmd (curr);
                    free_line (ret);
                    return NULL;
                }
                curr->argv = tmp;
            }
            if (i == 0)
            {
                curr->argv[curr->argc] = xmalloc (BUF * sizeof (char));
                if (curr->argv[curr->argc] == NULL)
                {
                    free_cmd (curr);
                    free_line (ret);
                    return NULL;
                }
            }
            else if (i >= factor2 * BUF)
            {
                factor2++;
                char *tmp = xrealloc (curr->argv[curr->argc], 
                                      factor2 * BUF * sizeof (char));
                if (tmp == NULL)
                {
                    free_cmd (curr);
                    free_line (ret);
                    return NULL;
                }
                curr->argv[curr->argc] = tmp;
            }
            curr->argv[curr->argc][i] = l[cpt];
        }
        i++;
        cpt++;
    }
    if (i != 0 && begin)
    {   
        curr->cmd[i] = '\0';
    }   
    else if (i != 0)
    {   
        curr->argv[curr->argc][i] = '\0';
        curr->argc++;
        if (curr->argc >= factor * ARGC)
        {
            factor++;
            char **tmp = xrealloc (curr->argv, factor * ARGC * sizeof (char *));
            if (tmp == NULL)
            {
                free_cmd (curr);
                free_line (ret);
                return NULL;
            }
            curr->argv = tmp;
        }
        curr->argv[curr->argc] = NULL;
    }  
    if (curr->cmd != NULL)
        line_append (&ret, curr);
    else
        free_cmd (curr);
    return ret;
}

char *
read_line (const char *prompt)
{
    char *ret = xmalloc (BUF * sizeof (char));
    int cpt = 0, ind = 1, nb_lines = 0, antislashes = 0, read_tmp = 0, 
        for_open = 0;
    char c, tmp;
    if (ret == NULL)
        return NULL;
    if (prompt != NULL && xstrlen (prompt) > 0)
    {
        fprintf (stdout, "%s", prompt);
        fflush (stdout);
    }
/*    c = getchar (); */
    read (0, &c, 1);
    while (c != '\n' || nb_lines != antislashes || !for_open)
    {
        if (c == '\t')
        {
            /*
            fprintf (stdout, "[TAB]");
            fflush (stdout);
            */
            completion (ret, cpt);
            read (0, &c, 1);
            continue;
        }
        else if (c != '\n')
        {
            fprintf (stdout, "%c", c);
            fflush (stdout);
        }
        if (c == '\\')
        {
/*            tmp = getchar (); */
            read (0, &tmp, 1);
            if (tmp == '\n')
            {
                c = tmp;
                antislashes++;
                fprintf (stdout, "\n> ");
                fflush (stdout);
                continue;
            }
            read_tmp = 1;
        }
        if (c == '\n')
        {
/*            c = getchar (); */
            read (0, &c, 1);
            nb_lines++;
            continue;
        }
replay:
        if (cpt >= ind * BUF)
        {
            ind++;
            char *new = xrealloc (ret, ind * BUF * sizeof (char));
            if (new == NULL)
            {
                xfree (ret);
                return NULL;
            }
            ret = new;
        }
        if (read_tmp)
        {
            read_tmp = 0;
            ret[cpt] = c;
            c = tmp;
            cpt++;
            goto replay;
        }
        ret[cpt] = c;
        cpt++;
/*        c = getchar (); */
        read (0, &c, 1);
    }
    fprintf (stdout, "\n");
    if (cpt >= ind * BUF)
    {
        char *new = xrealloc (ret, (ind * BUF * sizeof (char)) + 1);
        if (new == NULL)
        {
            xfree (ret);
            return NULL;
        }
        ret = new;
    }
    ret[cpt] = '\0';
    return ret;
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
        r = fork ();
        if (r == 0)
        {
            if (ptr->in != 0)
            {
                close (0);
                dup (0);
            }
            if (ptr->out != 1)
            {
                close (1);
                dup (1);
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
                call (ptr->argc, ptr->argv);
            }
            else
            {
                execvp (ptr->cmd, ptr->argv);
                perror ("execvp");
                r = -1;
            }
        }
    }
    return r;
}

void
run_line (input_line *ptr)
{
    if (ptr != NULL)
    {
        command *cmd = ptr->head;
        while (cmd != NULL)
        {
            switch (cmd->flag)
            {
            case BG:
            case OR:
            case AND:
            case END:
            {
                pid_t p = run_command (cmd);
                waitpid (p, NULL, cmd->flag == BG ? WNOHANG : 0);
                break;
            }
            case PIPE:
            {
                int nb = 0, i, fd[2];
                pid_t *p;
                command *exec = cmd->next;
                while (exec != NULL && exec->flag == OR)
                {
                    nb++;
                    exec = exec->next;
                }
                p = xmalloc (nb * sizeof (pid_t));
                exec = cmd;
                for (i = 0; i < nb; i++)
                {
                    pipe (fd);
                    if (i != nb - 1)
                        exec->out = fd[1];
                    p[i] = run_command (exec);
                    if (exec->out != 1)
                        close (exec->out);
                    exec = exec->next;
                    exec->in = fd[0];
                }
                for (i = 0; i < nb; i++)
                {
                    if (p[i] != -1)
                        waitpid (p[i], NULL, 0);
                }
                cmd = exec;
                break;
            }
            }
            cmd = cmd->next;
        }
    }
}
