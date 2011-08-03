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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include "command.h"
#include "parser.h"
#include "xutils.h"

static char *
completion (char *buf, int ind)
{
    char *tmp = xmalloc (ind + 1);
    char **split;
    size_t s_split;
    int i;
    for (i = 0; i < ind; i++)
        tmp[i] = buf[i];
    tmp[i] = '\0';
    split = xstrsplit (tmp, " ", &s_split);

    xfree_list (split, s_split);
    xfree (tmp);
    return NULL;
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
    ret->flag = src->flag;
    ret->in = src->in;
    ret->out = src->out;
    ret->err = src->err;
    ret->prev = src->prev;
    ret->next = src->next;
    ret->builtin = src->builtin;
    if (src->argc > 0)
    {
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
    }
    else
    {
        ret->argv = NULL;
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

static char
get_char_full (const char input[5], const char *prompt, const char *ret, int cpt)
{
    size_t len = xstrlen (input);
    int i;
    if (len == 1)
    {
        if (input[0] == 127)
        {
            if (cpt > 0)
            {
                fprintf (stdout, "\b \b");
                fflush (stdout);
            }
            return -2;
        }
        return input[0];
    }
/*    fprintf (stdout, "size: %d, in: %s\n", len, input);*/
    fprintf (stdout, "\nsize: %d, in: ", len);
    for (i = 0; i < (int) len; i++)
    {
        fprintf (stdout, "%c", input[i]);
    }
    fprintf (stdout, "\n%s", prompt);
    for (i = 0; i < cpt; i++)
        fprintf (stdout, "%c", ret[i]);
    fflush (stdout);
    return -1;
}

#define get_char(in) get_char_full (in, prompt, ret, cpt)

char *
read_line (const char *prompt)
{
    char *ret = xmalloc (BUF * sizeof (char));
    int cpt = 0, ind = 1, nb_lines = 0, antislashes = 0, read_tmp = 0,
        squote = 0, dquote = 0;
    char in[5], tmp[5], c = -1;
    if (ret == NULL)
        return NULL;
    if (prompt != NULL && xstrlen (prompt) > 0)
    {
        fprintf (stdout, "%s", prompt);
        fflush (stdout);
    }
/*    c = getchar (); */
    while (c == -1 || c == -2)
    {
        if (c == -2)
        {
            cpt = xmax (cpt - 1, 0);
        }
        memset (in, 0, 5);
        read (0, &in, 5);
        c = get_char (in);
    }
    while (c != '\n' || (c == '\n' && (squote || dquote)) || nb_lines != antislashes)
    {
        if (c == '\t')
        {
            /*
            fprintf (stdout, "[TAB]");
            fflush (stdout);
            */
            completion (ret, cpt);
read1:
            memset (in, 0, 5);
            read (0, &in, 5);
            c = get_char (in);
            if (c == -1)
                goto read1;
            if (c == -2)
            {
                cpt = xmax (cpt - 1, 0);
                goto read1;
            }
            continue;
        }
        else if (c != '\n')
        {
            fprintf (stdout, "%c", c);
            fflush (stdout);
        }
        else if (c == '\n' && (squote || dquote))
        {
            fprintf (stdout, "\n> ");
            fflush (stdout);
        }
        if (c == '\'' && !dquote)
            squote = !squote;
        if (c == '"' && !squote)
            dquote = !dquote;
        if (c == '\\')
        {
/*            tmp = getchar (); */
read2:
            memset (tmp, 0, 5);
            read (0, &tmp, 5);
            if (get_char (tmp) == -1)
                goto read2;
            if (get_char (tmp) == -2)
            {
                cpt = xmax (cpt - 1, 0);
                goto read2;
            }
            if (get_char (tmp) == '\n')
            {
                c = get_char (tmp);
                antislashes++;
                fprintf (stdout, "\n> ");
                fflush (stdout);
                continue;
            }
            read_tmp = 1;
        }
        if (c == '\n' && !(squote || dquote))
        {
/*            c = getchar (); */
read3:
            memset (in, 0, 5);
            read (0, &in, 5);
            c = get_char (in);
            if (c == -1)
                goto read3;
            if (c == -2)
            {
                cpt = xmax (cpt - 1, 0);
                goto read3;
            }
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
            c = get_char (tmp);
            cpt++;
            goto replay;
        }
        ret[cpt] = c;
        cpt++;
/*        c = getchar (); */
read4:
        memset (in, 0, 5);
        read (0, &in, 5);
        c = get_char (in);
        if (c == -1)
            goto read4;
        if (c == -2)
        {
            cpt = xmax (cpt - 1, 0);
            goto read4;
        }
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
