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
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "command.h"
#include "parser.h"
#include "xutils.h"

static char **command_list = NULL;
static int nb_commands = 0;

static int
cmpsort (const void *p1, const void *p2)
{
    return xstrcmp(* (char * const *) p1, * (char * const *) p2);
}

void 
init_command_list (void)
{
    char *path = xstrdup (getenv ("PATH"));
    size_t size;
    char **paths = xstrsplit (path, ":", &size);
    int i, j, k = 0, l, n = 1;
    char **tmp_command_list = xcalloc (n * 500, sizeof (char *));
    xfree (path);
    for (i = 0; i < (int) size; i++)
    {
        struct dirent **files;
        int nbfiles = scandir (paths[i], &files, NULL, alphasort);
        for (j = 0; j < nbfiles; j++)
        {
            if (k >= n * 500)
            {
                n++;
                tmp_command_list = xrealloc (tmp_command_list, 
                                         n * 500 * sizeof (char *));
            }
            if (S_ISREG(DTTOIF(files[j]->d_type)) && 
                access(files[j]->d_name, X_OK))
            {
                tmp_command_list[k] = xstrdup (files[j]->d_name);
                k++;
            }
            xfree (files[j]);
        }
        xfree (files);
    }
    xfree_list (paths, size);
    qsort (tmp_command_list, k, sizeof (char *), cmpsort);
    n = 1;
    command_list = xcalloc (n * 500, sizeof (char *));
    for (i = 0, j = 1, l = 0; j < k; i++)
    {
        if (i >= n * 500)
        {
            n++;
            command_list = xrealloc (command_list, n * 500 * sizeof (char *));
        }
        command_list[i] = xstrdup (tmp_command_list[l]);
        while (j <= k - 1 && 
               xstrcmp (command_list[i], tmp_command_list[j]) == 0)
            j++;
        l = (j <= k - 1) ? j : k - 1;
        j++;
    }
    nb_commands = i;
    xfree_list (tmp_command_list, k);
}

void
clear_command_list (void)
{
    xfree_list (command_list, nb_commands);
    nb_commands = 0;
}

static const char *
completion (const char *prompt, char *buf, int ind)
{
    char *tmp = xmalloc (ind + 1), *ret = NULL;
    char **split, **list;
    size_t s_split, s_in;
    int i, j, n = 1;
    unsigned int found = FALSE;
    for (i = 0; i < ind; i++)
        tmp[i] = buf[i];
    tmp[i] = '\0';
    split = xstrsplit (tmp, " ", &s_split);
    if (s_split > 1)
    {
        xfree_list (split, s_split);
        xfree (tmp);
    }

    list = xcalloc (n * 10, sizeof (char *));
    s_in = xstrlen (split[0]);
    for (i = 0, j = 0; i < nb_commands; i++)
    {
        int ok = strncmp (split[0], command_list[i], s_in);
        /* 
         * our list is sorted so once a command dismatch we are sure the rest
         * won't match
         */
        if (found && ok != 0)
            break;
        if (ok == 0)
        {
            if (j >= n * 10)
            {
                n++;
                list = xrealloc (list, n * 10 * sizeof (char *));
            }
            list[j] = command_list[i];
            j++;
            found = TRUE;
        }
    }
    if (j == 1)
    {
        for (i = 0; i < (int) s_in; i++)
            fprintf (stdout, "\b");
        fprintf (stdout, "%s", list[0]);
        fflush (stdout);
        ret = list[0];
    }
    else if (j > 1)
    {
        fprintf (stdout, "\n");
        for (i = 0; i < j; i++)
            fprintf (stdout, "%s\t", list[i]);
        fprintf (stdout, "\n%s%s", prompt, tmp);
        fflush (stdout);
    }
    xfree (list);
    xfree_list (split, s_split);
    xfree (tmp);
    return ret;
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
get_char_full (const char input[5], const char *prompt, const char *ret, int *cpt)
{
    size_t len = xstrlen (input);
    int i;
    if (len == 1)
    {
        if (input[0] == 127)
        {
            if (*cpt > 0)
            {
                (*cpt)--;
                fprintf (stdout, "\b \b");
                fflush (stdout);
            }
            return -1;
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
    for (i = 0; i < *cpt; i++)
        fprintf (stdout, "%c", ret[i]);
    fflush (stdout);
    return -1;
}

#define get_char(in) get_char_full (in, prompt, ret, &cpt)

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
    while (c == -1)
    {
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
            const char *comp = completion (prompt, ret, cpt);
            if (comp != NULL)
            {
                size_t s_comp = xstrlen (comp);
                int iz;
                for (iz = cpt; iz < (int) s_comp; iz++, cpt++)
                {
                    if (cpt >= ind * BUF)
                    {
                        ind++;
                        ret = xrealloc (ret, ind * BUF * sizeof (char));
                    }
                    ret[cpt] = comp[iz];
                }
            }
            c = -1;
            while (c == -1)
            {
                memset (in, 0, 5);
                read (0, &in, 5);
                c = get_char (in);
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
            char ctmp = -1;
            while (ctmp == -1)
            {
                memset (tmp, 0, 5);
                read (0, &tmp, 5);
                ctmp = get_char (tmp);
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
            c = -1;
            while (c == -1)
            {
                memset (in, 0, 5);
                read (0, &in, 5);
                c = get_char (in);
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
        c = -1;
        while (c == -1)
        {
            memset (in, 0, 5);
            read (0, &in, 5);
            c = get_char (in);
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
