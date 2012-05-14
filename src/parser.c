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
#include <sys/ioctl.h>
#include <ctype.h>
#include <errno.h>
#include <termios.h>
#include <fnmatch.h>
#include <err.h>

#include "command.h"
#include "parser.h"
#include "xutils.h"
#include "structs.h"
#include "caps.h"
#include "list.h"
#include "modules.h"

#define reset_completion() completion (NULL, NULL, NULL)

#define sh_read(fileno,buf,size) do{\
interrupted = FALSE;\
read (fileno,buf,size);\
if (interrupted)\
{\
    goto exit;\
}\
} while (0);
/*    fprintf (stdout, "\nread returned line %d\n", __LINE__);\*/

static char *history[HISTORY];
static int last_history = 0, curr_history;
static char **command_list = NULL;
static int nb_commands = 0;
static struct termios in_save;

extern unsigned int interrupted;
extern pid_t shell_pgid;
extern int shell_terminal;
extern int shell_is_interactive;

/* static functions */
static void init_ioctl (void);
static void clear_ioctl (void);
static char *completion (const char *prompt, char *buf, int *ind);
static int cmpsort (const void *p1, const void *p2);
static int reg_filter (const struct dirent *p);
static char get_char (const char input[5],
                      const char *prompt,
                      char **ret,
                      int *cpt,
                      int *n,
                      unsigned int *squote,
                      unsigned int *dquote);
static char *parse_filepath (char **split,
                             size_t s_split,
                             char *tmp,
                             int curr,
                             const char *prompt,
                             int *cpt);
static char *parse_exe (char **split,
                        size_t s_split,
                        char *tmp,
                        int curr,
                        const char *prompt,
                        int *cpt);

static int
cmpsort (const void *p1, const void *p2)
{
    return xstrcmp (* (char * const *) p1, * (char * const *) p2);
}

static int
reg_filter (const struct dirent *p)
{
    return S_ISREG(DTTOIF(p->d_type));
}

void
init_history (void)
{
    xdebug (NULL);
    int i;
    for (i = 0; i < HISTORY; i++)
        history[i] = NULL;
}

void
insert_history (const char *cmd)
{
    unsigned int found = FALSE;
    /* do we save the command in the history? */
    /*
    for (i = 0; i < HISTORY && history[i] != NULL; i++)
    {
        if (xstrcmp (history[i], cmd) == 0)
        {
            found = TRUE;
            break;
        }
    }
    */
    if (!found && xstrlen (cmd) > 0)
    {
        if (last_history == 0 || xstrcmp (cmd, history[last_history-1]) != 0)
        {
            last_history = last_history < HISTORY ? last_history : 0;
            xfree (history[last_history]);
            history[last_history] = xstrdup (cmd);
            last_history++;
        }
    }
}

void 
init_command_list (void)
{
    xdebug (NULL);
    char *path = getenv ("PATH");
    size_t size;
    char **paths = xstrsplit (path, ":", &size);
    int i, j, k = 0, l, n = 1;
    char **tmp_command_list = xcalloc (n * 500, sizeof (char *));
    /* 
     * first of all, we store each executables files in each directories of the
     * PATH variable
     */
    for (i = 0; i < (int) size; i++)
    {
        struct dirent **files = NULL;
        int nbfiles = scandir (paths[i], &files, reg_filter, alphasort);
        for (j = 0; j < nbfiles; j++)
        {
            if (k >= n * 500)
            {
                n++;
                tmp_command_list = xrealloc (tmp_command_list, 
                                         n * 500 * sizeof (char *));
            }
            tmp_command_list[k] = xstrdup (files[j]->d_name);
            k++;
            xfree (files[j]);
        }
        xfree (files);
    }
    xfree_list (paths, size);
    /* then we sort the list */
    qsort (tmp_command_list, k, sizeof (char *), cmpsort);
    n = 1;
    command_list = xcalloc (n * 500, sizeof (char *));
    /* finally we remove each duplicates files */ 
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
clear_history (void)
{
    xdebug (NULL);
    int i;
    for (i = 0; i < HISTORY && history[i] != NULL; i++)
    {
/*        fprintf (stdout, "history[%d] = %s\n", i, history[i]); */
        xfree (history[i]);
    }
}

void
clear_command_list (void)
{
    xdebug (NULL);
    xfree_list (command_list, nb_commands);
    nb_commands = 0;
    command_list = NULL;
}

/* Input initialization function */
static void
init_ioctl (void)
{
    struct termios term;

    if (shell_is_interactive)
    {

        if (ioctl (shell_terminal, TCGETS, &term) != 0)
            err (3, "\nioctl G prob");
        in_save = term;
        term.c_lflag &= ~(ECHO | ICANON);
        term.c_cc[VMIN] = 1;
        term.c_cc[VTIME] = 0;
        if (ioctl (shell_terminal, TCSETS, &term) != 0)
            err (3, "\nioctl S prob");
    }
    else
    {
        xdebug ("noninteractive");
    }
}

static void
clear_ioctl (void)
{
    if (shell_is_interactive)
    {
        if (ioctl (shell_terminal, TCSETS, &in_save) != 0)
            err (3, "\nioctl S prob");
    }
}

static char *
parse_filepath (char **split,
                size_t s_split,
                char *tmp,
                int curr,
                const char *prompt,
                int *cpt)
{
    char *ret = NULL;
    char **path;
    char **files_list = NULL;
    char *clean_path = NULL;
    size_t s_path;
    struct dirent **files = NULL;
    int nbfiles, i, j, n = 1;
    unsigned int multifiles, redirect/*, found = FALSE*/;
    path = xstrsplit (split[s_split-1], "/", &s_path);
    multifiles = s_path > 0;
    if (s_path > 1 || ( s_path == 1 && ( 
        xstrcmp (path[0], ".") == 0 || 
        xstrcmp (path[0], "..") == 0)))
    {
        int size = 
            split[s_split-1][xstrlen(split[s_split-1])-1] == '/' ?
                s_path :
                s_path-1;
        multifiles = size != (int) s_path && multifiles;
        clean_path = xstrjoin (path, size, "/");
    }
    else if (*(split[s_split-1]) == '/')
    {
        if (split[s_split-1][xstrlen(split[s_split-1])-1] == '/')
        {
            multifiles = FALSE;
            clean_path = xstrdup (split[s_split-1]);
        }
        else
            clean_path = xstrdup ("/");
    }
    redirect = FALSE;
    if (clean_path != NULL)
    {
        char *red;
        red = strrchr (split[curr], '<');
        if (red != NULL)
        {
            redirect = TRUE;
            xfree (clean_path);
            char *sl = strrchr (split[curr], '/');
            clean_path = xstrsub (red, 
                                    red-split[curr]+1,
                                    sl ? 
                                    sl-split[curr] :
                                    (int) xstrlen (split[curr]));

        }
        else if ((red = strrchr (split[curr], '>')) != NULL)
        {
            redirect =TRUE;
            xfree (clean_path);
            char *sl = strrchr (split[curr], '/');
            clean_path = xstrsub (red, 
                                    red-split[curr]+1,
                                    sl ? 
                                    sl-split[curr] :
                                    (int) xstrlen (split[curr]));

        }
    }
    /* xstrjoin gives us a string starting with a '/' in every cases */
    if (*(split[s_split-1]) != '/' && clean_path != NULL && !redirect)
        clean_path++;
    nbfiles = scandir (clean_path != NULL ? clean_path : ".",
                        &files,
                        NULL,
                        alphasort);
    if (nbfiles > 0)
    {
        size_t s_tmp = 0;
        unsigned int sub = FALSE;
        if (multifiles)
        {
            s_tmp = xstrlen (path[s_path-1]);
            files_list = xcalloc (n * 10, sizeof (char *));
        }
        else
            fprintf (stdout, "\n");
        for (i = 0, j = 0; i < nbfiles; i++)
        {
            if (multifiles)
            {
                int ok = fnmatch (path[s_path-1], files[i]->d_name, 0);
                if (ok == FNM_NOMATCH)
                    ok = strncmp (path[s_path-1], 
                                    files[i]->d_name, 
                                    s_tmp);
                else
                    sub = TRUE;
                        /* FIXME: our list should be sorted... */
/*                        if (found && !ok) */
/*                            break; */
                if (ok == 0)
                {
                    if (j >= n * 10)
                    {
                        n++;
                        files_list = xrealloc (files_list, 
                                            n * 10 * sizeof (char *));
                    }
                    files_list[j] = xstrdup (files[i]->d_name);
                    j++;
                    /*found = TRUE;*/
                }
            }
            else
            {
                if (!S_ISREG(DTTOIF(files[i]->d_type)))
                    fprintf (stdout, "%s/\t", files[i]->d_name);
                else
                    fprintf (stdout, "%s\t", files[i]->d_name);
            }
            xfree (files[i]);
        }
        xfree (files);
        if (!multifiles)
        {
            fprintf (stdout, "\n%s%s", prompt, tmp);
            fflush (stdout);
        }
        if (j == 1)
        {
            size_t s = xstrlen (files_list[0]);
            char *f;
            struct stat sb;
            unsigned int space = FALSE;
            char *t = (s > xstrlen (path[s_path-1])) ?
                    xstrsub (files_list[0],      
                                xstrlen (path[s_path-1]),
                                s - xstrlen (path[s_path-1])) :
                    NULL;    
            if (clean_path != NULL)
            {
                size_t full = s + xstrlen (clean_path) + 2;
                f = xmalloc (full * sizeof (char));
                snprintf (f, full, "%s/%s", clean_path, files_list[0]);
            }
            else
                f = xstrdup (files_list[0]);
            stat (f, &sb);
            xfree (f);
            space = !S_ISDIR(sb.st_mode);
            if (t != NULL)
            {  
                size_t sum = xstrlen (tmp) + xstrlen (t) + 2;
                ret = xmalloc (sum * sizeof (char));
                snprintf (ret, sum, "%s%s%c", tmp, t, space ?
                                                        ' ' : '/');
            }
            xfree (t);

            if (sub)
            {
                size_t sum = xstrlen (files_list[0]) + 2;
                if (clean_path != NULL)
                    sum += xstrlen (clean_path) + 1;
                ret = xmalloc (sum * sizeof (char));
                if (clean_path != NULL)
                {
                    snprintf (ret,
                              sum,
                              "%s/%s%c",
                              clean_path,
                              files_list[0],
                              space ? ' ' : '/');
                }
                else
                {
                    snprintf (ret,
                              sum,
                              "%s%c",
                              files_list[0],
                              space ? ' ' : '/');
                }
            }

            for (i = 0; i < (int) s_tmp; i++)
            {
                fprintf (stdout, "\b");
                if (sub)
                    (*cpt)--;
            }
            fprintf (stdout, "%s%c", files_list[0], space ? ' ' : '/');
            fflush (stdout);
        }
        else if (j > 1)
        {
            size_t s_min = 10000, len = 0;
            int ind_min = -1;
            char *t = NULL;
            fprintf (stdout, "\n");
            for (i = 0; i < j; i++)
            {
                size_t s_tmp = xstrlen (files_list[i]);
                if (s_tmp < s_min)
                {
                    s_min = s_tmp;
                    ind_min = i;
                    len = s_tmp;
                }
                fprintf (stdout, "%s\t", files_list[i]);
            }
            for (i = 0; i < j; i++)
            {
                while (strncmp (files_list[ind_min],
                                files_list[i],
                                len) != 0 && len > 0)
                    len--;
            }
            t = (len > xstrlen (path[s_path-1]) && ind_min > -1) ?
                            xstrsub (files_list[ind_min],
                                        xstrlen (path[s_path-1]),
                                        len - xstrlen (path[s_path-1])) :
                            NULL;
            if (t != NULL)
            {
                size_t sum = xstrlen (tmp) + xstrlen (t) + 1;
                ret = xmalloc (sum * sizeof (char));
                if (sub)
                {
                    char *t_tmp = xstrsub (tmp, 0, xstrlen (tmp) - len + 1);
                    char *t_t = xstrsub (files_list[ind_min], 0, len);
                    snprintf (ret, sum, "%s%s", t_tmp, t_t);
                    xfree (t_tmp);
                    xfree (t_t);
                    for (i = 0; i < (int) len; i++, (*cpt)--);
                }
                else
                    snprintf (ret, sum, "%s%s", tmp, t);
            }

            xfree (t);

            fprintf (stdout,
                        "\n%s%s",
                        prompt,
                        ret != NULL ? ret : tmp);
            fflush (stdout);
        }
        if (multifiles)
            xfree_list (files_list, j);
    }
    if (*(split[s_split-1]) != '/' && clean_path != NULL && !redirect)
        clean_path--;
    xfree (clean_path);
    xfree_list (path, s_path);
    
    return ret;
}

static char *
parse_exe (char **split,
           size_t s_split,
           char *tmp,
           int curr,
           const char *prompt,
           int *cpt)
{
    char *ret = NULL;
    int i, j, n = 1;
    char **list = xcalloc (n * 10, sizeof (char *));
    size_t s_in = xstrlen (split[curr]);
    unsigned int found = FALSE, sub = FALSE;
    for (i = 0, j = 0; i < nb_commands; i++)
    {
        int ok = fnmatch (split[curr], command_list[i], 0);
        if (ok == FNM_NOMATCH)
            ok = strncmp (split[curr], command_list[i], s_in);
        else
            sub = TRUE;
        /* 
         * our list is sorted so once a command dismatch we are sure the 
         * rest won't match
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
        size_t s = xstrlen (list[0]);
        char *t = (s > xstrlen (split[curr])) ?
                        xstrsub (list[0], 
                                xstrlen (split[curr]), 
                                s - xstrlen (split[curr])) :
                        NULL;
        if (t != NULL)
        {    
            size_t sum = xstrlen (tmp) + xstrlen (t) + 2; 
            ret = xmalloc (sum * sizeof (char));
            snprintf (ret, sum, "%s%s ", tmp, t);
        }    
        xfree (t); 

        if (sub)
        {
            size_t sum = xstrlen (list[0]) + 2;
            ret = xmalloc (sum * sizeof (char));
            snprintf (ret, sum, "%s ", list[0]);
        }

        for (i = 0; i < (int) s_in; i++)
        {
            fprintf (stdout, "\b");
            if (sub)
                (*cpt)--;
        }
        fprintf (stdout, "%s ", list[0]);
        fflush (stdout);
    }
    else if (j > 1)
    {
        size_t s_min = 10000, len = 0;
        int ind_min = -1;
        char *t = NULL;
        fprintf (stdout, "\n");
        for (i = 0; i < j; i++)
        {
            size_t s_tmp = xstrlen (list[i]);
            if (s_tmp < s_min)
            {
                s_min = s_tmp;
                ind_min = i;
                len = s_tmp;
            } 
            fprintf (stdout, "%s\t", list[i]);
        }
        for (i = 0; i < j; i++)
        {
            while (strncmp (list[ind_min], list[i], len) != 0 && len > 0)
                len--;
        }
        t = (len > xstrlen (split[curr]) && ind_min > -1) ?
                        xstrsub (list[ind_min], 
                                xstrlen (split[curr]), 
                                len - xstrlen (split[curr])) :
                        NULL;
        if (t != NULL)
        {
            size_t sum = xstrlen (tmp) + xstrlen (t) + 1;
            ret = xmalloc (sum * sizeof (char));
            snprintf (ret, sum, "%s%s", tmp, t);
        }
        if (sub)
        {
            ret = xstrsub (list[ind_min], 0, len);
            for (i = 0; i < (int) xstrlen (tmp); i++, (*cpt)--);
                fprintf (stdout, "\b");
        }

        xfree (t);

        fprintf (stdout, 
                "\n%s%s", 
                prompt,
                ret != NULL ? ret : tmp);
        fflush (stdout);

    }
    else
        ret = parse_filepath (split, s_split, tmp, curr, prompt, cpt);
    
    xfree (list);
    
    return ret;
}

static char *
completion (const char *prompt, char *buf, int *ind)
{
    /* in order to avoid multiple [TAB] hits we store the last index */
    static int save = 0;
    if (prompt == NULL || buf == NULL)
    {
        save = 0;
        return NULL;
    }
    if (*ind < 1 || save == *ind)
        return NULL;
xdebug ("'%s' buf: '%s' %d", prompt, buf, *ind);
    size_t s_full = *ind + 1;
    char *tmp = xmalloc (s_full * sizeof (char)), *ret = NULL;
    char *to_split = xmalloc (s_full * sizeof (char));
    char **split;
    size_t s_split;
    int i, j, k, curr = 0;
    unsigned int squote = FALSE, dquote = FALSE, protected = FALSE;
    /* First: we prepare two strings to parse */
    for (i = 0, j = 0, k = 0; j < *ind; i++, j++, k++)
    {
        if (buf[j] == '\'' && !dquote)
            squote = !squote;
        if (buf[j] == '"' && !squote)
            dquote = !dquote;
        if (buf[j] == '\\' && !protected)
            protected = TRUE;
        else
            protected = FALSE;
        to_split[i] = buf[j];
        if (j+1 < *ind && !(dquote || squote) && !protected &&
            (buf[j+1] == '|' ||
            buf[j+1] == '&' ||
            buf[j+1] == ';'))
        {
            if (j > 0 && isalnum (buf[j-1]) && 
                buf[j] != '|' && buf[j] != '&' && buf[j] != ';')
            {
                s_full++;
                to_split = xrealloc (to_split, s_full * sizeof (char));
                i++;
                to_split[i] = ' ';
            }
        }
        if (j > 0 && !(dquote || squote) && !protected &&
            (buf[j-1] == '|' ||
            buf[j-1] == '&' ||
            buf[j-1] == ';'))
        {
            if (buf[j] != '|' && buf[j] != '&' && buf[j] != ';')
            {
                s_full++;
                to_split = xrealloc (to_split, s_full * sizeof (char));
                char c = to_split[i];
                to_split[i] = ' ';
                i++;
                to_split[i] = c;
            }
        }
        tmp[k] = buf[j];
    }
    tmp[k] = '\0';
    /* to_split is split ready meaning each commands are separated by spaces */
    to_split[i] = '\0';
    split = xstrsplitspace (to_split, &s_split);
    /*split = xstrsplit (to_split, " ", &s_split);*/
    xfree (to_split);
    if (s_split > 1 || (
        (j > 2 && tmp[j-1] == ' ' && tmp[j-2] != '\\') ||
        (j > 1 && tmp[j-1] == ' ')))
    {
        curr = s_split-1;
        /* if we have more than one word and the previous is a separator */
        if (s_split > 1 && (
            xstrcmp (split[s_split-2], "|") == 0 ||
            xstrcmp (split[s_split-2], "||") == 0 ||
            xstrcmp (split[s_split-2], "&") == 0 ||
            xstrcmp (split[s_split-2], "&&") == 0 ||
            xstrcmp (split[s_split-2], ";") == 0))
        {
            ret = parse_exe (split, s_split, tmp, curr, prompt, ind);
        }
        /* if we didn't type any character after a space, list all files */
        if (s_split == 1 || (
            (j > 2 && tmp[j-1] == ' ' && tmp[j-2] != '\\') &&
            (j > 1 && tmp[j-1] == ' ')))
        {
            struct dirent **files = NULL;
            int nbfiles = scandir (".", &files, NULL, alphasort);
            if (nbfiles > 0)
            {
                fprintf (stdout, "\n");
                for (i = 0; i < nbfiles; i++)
                {
                    fprintf (stdout, "%s\t", files[i]->d_name);
                    xfree (files[i]);
                }
                xfree (files);
                fprintf (stdout, "\n%s%s", prompt, tmp);
                fflush (stdout);
            }
        }
        /* TODO: remove \ from input + add \ to output + multi-completion */
        else
        {
            ret = parse_filepath (split, s_split, tmp, curr, prompt, ind);
        }
    }
    else
    {
        ret = parse_exe (split, s_split, tmp, curr, prompt, ind);
    }
    xfree_list (split, s_split);
    xfree (tmp);
    save = *ind;
    if (ret != NULL)
    {
        size_t len = xstrlen (ret);
        save = ret[len-1] == ' ' || ret[len-1] == '/' ? len-1 : len;
    }
xdebug ("'%s' buf: '%s' ret: '%s' %d", prompt, buf, ret, *ind);
    return ret;
}

void
free_line (input_line *ptr)
{
    if (ptr != NULL)
    {
        int cpt = 0;
        command_line *tmp = ptr->head;
        while (cpt < ptr->size && tmp != NULL)
        {
            command_line *tmp2 = tmp->next;
            free_cmd_line (tmp);
            tmp = tmp2;
            cpt++;
        }
        xfree (ptr);
    }
}

void
dump_cmd (command_line *ptrc)
{
    if (ptrc != NULL)
    {
        command *ptr = ptrc->content;
        if (ptr != NULL) {            
            int i;
            fprintf (stdout, "cmd: %s\n", ptr->cmd);
            fprintf (stdout, "argc: %d\n", ptr->argc);
            for (i = 0; i < ptr->argc; i++)
                fprintf (stdout, "argv[%d]: %s (%d)\n", i,
                                                    ptr->argv[i],
                                                    ptr->protected[i]);
        }
    }
}

void
dump_line (input_line *ptr)
{
    if (ptr != NULL)
    {
        command_line *tmp = ptr->head;
        int cpt = 0;
        if (ptr->size > 0)
            fprintf (stdout, "nb commands: %d\n", ptr->size);
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
        factor2 = 1, arg = 0, squote = 0, dquote = 0, bracket = 0,
        backquote = 0, setting = 0, backslash = 0, lastslash = 0;
    command_line *curr = NULL;
    i = 0;
    /* let's create the line container */
    ret = xmalloc (sizeof (*ret));
    if (ret == NULL)
        return NULL;
    ret->size = 0;
    ret->head = NULL;
    ret->tail = NULL;
    curr = new_cmd_line ();
    if (curr == NULL)
    {
        free_line (ret);
        return NULL;
    }
    /* the parsing begin here */
    while (cpt < size)
    {
        if (lastslash)
        {
            lastslash = 0;
            backslash = 1;
        }
        if (l[cpt] == '\\' && !backslash)
        {
            backslash = 1;
            lastslash = 1;
            if (!is_module_present ("wildcards"))
            {
                cpt++;
                continue;
            }
        }
        /* handle the quotes to protect some characters */
        if (l[cpt] == '\'' && !(dquote % 2 != 0 || backslash))
        {
            cpt++;
            squote++;
            continue;
        }
        if (l[cpt] == '"' && !(squote % 2 != 0 || backslash))
        {
            cpt++;
            dquote++;
            continue;
        }
        if (l[cpt] == '=' && !(dquote % 2 != 0 || squote % 2 != 0 || backslash))
            setting = 1;
        if (l[cpt] == '`' && !backslash)
            backquote = !backquote;
        if (l[cpt] == '(' && !backslash)
            bracket++;
        if (l[cpt] == ')' && !backslash)
        {
            if (bracket > 0)
                bracket--;
            else
            {
                syntax_error (l, size, cpt);
                curr->content->argv[curr->content->argc][i] = '\0';
                curr->content->argc++;
                free_cmd_line (curr);
                free_line (ret);
                return NULL;
            }
        }
        /* 
         * a 'space' found outside a quotation means we have done the current
         * command/argument
         */
        if (l[cpt] == ' ' &&
            !(squote % 2 != 0 || dquote % 2 != 0 || bracket > 0 ||
            backquote || backslash))
        {
            cpt++;
            /* 
             * of course if there are no non-space character before the space we
             * don't close any word
             */
            if (!first)
            {
                if (i != 0 && begin)
                {
                    curr->content->cmd[i] = '\0';
                    if (dquote % 2 == 0)
                        curr->content->protect = DOUBLE_QUOTE;
                    else if (squote % 2 == 0)
                        curr->content->protect = SINGLE_QUOTE;
                    else 
                        curr->content->protect = NONE;
                    if (setting != 0)
                        curr->content->protect |= SETTING;
                }
                else if (i != 0)
                {
                    curr->content->argv[curr->content->argc][i] = '\0';
                    curr->content->argc++;
                }
                new_word = 1;
                /*factor = 1;*/
                factor2 = 1;
                arg = 0;
                i = 0;
                begin = 0;
                setting = 0;
            }
            continue;
        }
        /* we found a comment */
        if (!(squote % 2 != 0 || dquote % 2 != 0) && l[cpt] == '#')
            break;
        /* handle the redirections (ie. cmd 2>&1 >/tmp/blah) */
        if (!(squote % 2 != 0|| dquote % 2 != 0) &&
            (l[cpt] == '<' || l[cpt] == '>'))
        {
            unsigned int read;
            int f = 1, zi = 0, fd = -1, flags;
            char *file;
            if (curr == NULL)
            {
                syntax_error (l, size, cpt);
                free_line (ret);
                return NULL;
            }
            switch (l[cpt])
            {
            /* redirecting the standard input is pretty simple */
            case '<':
                read = TRUE;
                flags = O_RDONLY;
                break;
            /* it's a bit more complicated for the standard/error output */
            case '>':
            {
                read = FALSE;
                flags = O_CREAT|O_WRONLY;
                if (cpt + 1 < size && l[cpt+1] != '>')
                {
                    flags |= O_TRUNC;
                }
                else
                {
                    cpt++;
                    flags |= O_APPEND;
                }

                /* 
                 * we can specify which file descriptor to redirect within the
                 * command-line (ie. cmd 2>/tmp/errors 1>/tmp/output)
                 */
                if ((cpt > 2 && isdigit (l[cpt-1]) && !isdigit (l[cpt-2])) ||
                    (cpt > 1 && isdigit (l[cpt-1])))
                {
                    char buf[10];
                    snprintf (buf, 10, "%c", l[cpt-1]);
                    fd = strtol (buf, NULL, 10);
                    i--;
                }

                if (fd != -1)
                {
                    if (cpt + 2 > size)
                    {
                        syntax_error (l, size, cpt);
                        free_cmd_line (curr);
                        free_line (ret);
                        return NULL;
                    }
                    /* do we redirect a descriptor to another one? */
                    if (l[cpt+1] == '&' && isdigit (l[cpt+2]))
                    {
                        if (cpt + 3 < size && isdigit (l[cpt+3]))
                        {
                            syntax_error (l, size, cpt+3);
                            free_cmd_line (curr);
                            free_line (ret);
                            return NULL;
                        }
                        char buf[10];
                        snprintf (buf, 10, "%c", l[cpt+2]);
                        if (fd == STDERR_FILENO)
                            curr->content->err = strtol (buf, NULL, 10);
                        else if (fd == STDOUT_FILENO)
                            curr->content->out = strtol (buf, NULL, 10);
                        cpt += 3;
                        continue;
                    }
                }
                break;
            }
            }
            /* get the filename */
            file = xmalloc (f * BUF * sizeof (char));
            cpt++;
            while (cpt < size && 
                   (isalnum (l[cpt]) || 
                    l[cpt] == '.' || 
                    l[cpt] == '-' || 
                    l[cpt] == '_' ||
                    l[cpt] == '/'))
            {
                if (zi >= f * BUF)
                {
                    f++;
                    file = xrealloc (file, f * BUF * sizeof (char));
                }
                file[zi] = l[cpt];
                zi++;
                cpt++;
            }
            if (zi == 0)
            {
                xfree (file);
                syntax_error (l, size, cpt);
                free_cmd_line (curr);
                free_line (ret);
                return NULL;
            }
            if (zi >= f * BUF)
            {
                file = xrealloc (file, (f * BUF + 1) * sizeof (char));
            }
            file[zi] = '\0';
            int desc;
            if (read)
                desc = open (file, flags);
            else
                desc = open (file, flags, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP);
            if (desc < 0)
            {
                fprintf (stderr, "%s: %s\n", file, strerror (errno));
                xfree (file);
                free_cmd_line (curr);
                free_line (ret);
                return NULL;
            }
            if (read)
                curr->content->in = desc;
            else
            {
                switch (fd)
                {
                case STDOUT_FILENO:
                    curr->content->out = desc;
                    break;
                case STDERR_FILENO:
                    curr->content->err = desc;
                    break;
                default:
                    curr->content->out = desc;
                }
            }
            xfree (file);
            continue;
        }
        /* end of command */
        if (!(squote % 2 != 0 || dquote % 2 != 0 || backquote || bracket > 0) &&
            (l[cpt] == '|' || l[cpt] == ';' || l[cpt] == '&'))
        {
            cpt++;
            if (i != 0 && begin)
            {
                curr->content->cmd[i] = '\0';
            }
            else if (i != 0)
            {
                curr->content->argv[curr->content->argc][i] = '\0';
                curr->content->argc++;
            }
            /* let's set the flag to know how to run the command */
            switch (l[cpt-1]) 
            {
            case '|':
                if (cpt < size && l[cpt] == '|')
                {
                    cpt++;
                    curr->content->flag = OR;
                }
                else
                    curr->content->flag = PIPE;
                break;
            case ';':
                curr->content->flag = END;
                break;
            case '&':
                if (cpt < size && l[cpt] == '&')
                {
                    cpt++;
                    curr->content->flag = AND;
                }
                else
                    curr->content->flag = BG;
                break;
            }
            new_command = 1;
            new_word = 0;
            begin = 1;
            factor = 1;
            factor2 = 1;
            setting = 0;
            arg = 0;
            first = 1;
            i = 0;
            continue;
        }
        first = 0;
        if (new_command)
        {
            new_command = 0;
            command_line *tmp = copy_cmd_line (curr);
            list_append ((sdlist **)&ret, (sddata *)tmp);
            free_cmd_line (curr);
            curr = new_cmd_line ();
            if (curr == NULL)
            {
                free_line (ret);
                return NULL;
            }
        }
        if (begin && !new_word)
        {
            if (curr->content->cmd == NULL)
            {
                curr->content->cmd = xmalloc (factor * BUF * sizeof (char));
                if (curr->content->cmd == NULL)
                {
                    free_cmd_line (curr);
                    free_line (ret);
                    return NULL;
                }
            }
            if (i >= factor * BUF)
            {
                factor++;
                char *tmp = xrealloc (curr->content->cmd, factor * BUF * sizeof (char));
                if (tmp == NULL)
                {
                    free_cmd_line (curr);
                    free_line (ret);
                    return NULL;
                }
                curr->content->cmd = tmp;
            }
            curr->content->cmd[i] = l[cpt];
        }
        else if (new_word)
        {
            if (curr->content->argc == 0 && !arg)
            {
                curr->content->argv = xcalloc (factor * ARGC, sizeof (char *));
                curr->content->protected = xcalloc (factor * ARGC,
                                                    sizeof (Protection));
                if (curr->content->argv == NULL ||
                    curr->content->protected == NULL)
                {
                    free_cmd_line (curr);
                    free_line (ret);
                    return NULL;
                }
                arg = 1;
            }
            else if (curr->content->argc >= factor * ARGC)
            {
                factor++;
                curr->content->argv = xrealloc (curr->content->argv,
                                       factor * ARGC * sizeof (char *));
                curr->content->protected = xrealloc (curr->content->protected,
                                            factor*ARGC * sizeof(Protection));
            }
            if (i == 0)
            {
                curr->content->argv[curr->content->argc] = 
                                                xmalloc (BUF * sizeof (char));
                if (curr->content->argv[curr->content->argc] == NULL)
                {
                    free_cmd_line (curr);
                    free_line (ret);
                    return NULL;
                }
            }
            else if (i >= factor2 * BUF)
            {
                factor2++;
                char *tmp = xrealloc (curr->content->argv[curr->content->argc],
                                      factor2 * BUF * sizeof (char));
                if (tmp == NULL)
                {
                    free_cmd_line (curr);
                    free_line (ret);
                    return NULL;
                }
                curr->content->argv[curr->content->argc] = tmp;
            }
            curr->content->argv[curr->content->argc][i] = l[cpt];
            if (dquote > 0 && dquote % 2 == 0)
                curr->content->protected[curr->content->argc] = DOUBLE_QUOTE;
            else if (squote > 0 && squote % 2 == 0)
                curr->content->protected[curr->content->argc] = SINGLE_QUOTE;
            else
                curr->content->protected[curr->content->argc] = NONE;
            if (setting != 0)
                curr->content->protected[curr->content->argc] |= SETTING;
        }
        i++;
        cpt++;
        backslash = 0;
    }
    if (i != 0 && begin)
    {   
        curr->content->cmd[i] = '\0';
        if (dquote > 0 && dquote % 2 == 0)
            curr->content->protect = DOUBLE_QUOTE;
        else if (squote > 0 && squote % 2 == 0)
            curr->content->protect = SINGLE_QUOTE;
        else 
            curr->content->protect = NONE;
        if (setting != 0)
            curr->content->protect |= SETTING;
    }   
    else if (i != 0)
    {   
        curr->content->argv[curr->content->argc][i] = '\0';
        if (dquote > 0 && dquote % 2 == 0)
            curr->content->protected[curr->content->argc] = DOUBLE_QUOTE;
        else if (squote > 0 && squote % 2 == 0)
            curr->content->protected[curr->content->argc] = SINGLE_QUOTE;
        else 
            curr->content->protected[curr->content->argc] = NONE;
        if (setting != 0)
            curr->content->protected[curr->content->argc] |= SETTING;
        curr->content->argc++;
        if (curr->content->argc >= factor * ARGC)
        {
            factor++;
            char **tmp = xrealloc (curr->content->argv,
                                   factor * ARGC * sizeof (char *));
            if (tmp == NULL)
            {
                free_cmd_line (curr);
                free_line (ret);
                return NULL;
            }
            curr->content->argv = tmp;
        }
        curr->content->argv[curr->content->argc] = NULL;
    }  
    if (curr->content->cmd != NULL)
        list_append ((sdlist **)&ret, (sddata *)curr);
    else
        free_cmd_line (curr);
    return ret;
}

static char
get_char (const char input[5], 
          const char *prompt, 
          char **ret, 
          int *cpt, 
          int *n,
          unsigned int *squote,
          unsigned int *dquote)
{
    size_t len;
    (void) prompt;
    if (input[4] == 0)
        len = xstrlen (input);
    else
        len = 5;
    if (interrupted)
        return '\n';
    if (len == 1)
    {
        /*fprintf (stdout, "\nc: %c d: %d\n", input[0], input[0]);*/
        switch (input[0])
        {
        /* backspace */
        case 127:
            if (*cpt > 0)
            {
                if ((*ret)[*cpt-1] == '"' && !*squote)
                    *dquote = !*dquote;
                if ((*ret)[*cpt-1] == '\'' && !*dquote)
                    *squote = !*squote;
                (*cpt)--;
                fprintf (stdout, "\b \b");
                fflush (stdout);
                reset_completion ();
            }
            return -1;
        case CTRL_D:
        case CTRL_L:
        case CTRL_R:
            return -1;
        default:
            return input[0];
        }
    }
    if (len >= 3 && input[0] == 27 && input[1] == '[')
    {
        switch (input[2])
        {
        /* arrow UP */
        case 'A':
            if (curr_history > 0 && history[curr_history-1] != NULL)
            {
                *dquote = FALSE;
                *squote = FALSE;
                for (; *cpt > 0; (*cpt)--)
                    fprintf (stdout, "\b \b");
                fprintf (stdout, "%s", history[curr_history-1]);
                fflush (stdout);
                curr_history = xmax (curr_history - 1, 0);
                for (; *cpt < (int) xstrlen (history[curr_history]); (*cpt)++)
                {
                    if (*cpt >= *n * BUF)
                    {
                        (*n)++;
                        *ret = xrealloc (*ret, *n * BUF * sizeof (char));
                    }
                    if (history[curr_history][*cpt] == '"' && !*squote)
                        *dquote = !*dquote;
                    if (history[curr_history][*cpt] == '\'' && !*dquote)
                        *squote = !*squote;
                    (*ret)[*cpt] = history[curr_history][*cpt];
                }
            }
            break;
        /* arrow DOWN */
        case 'B':
            for (; *cpt > 0; (*cpt)--)
                fprintf (stdout, "\b \b");
            *squote = FALSE;
            *dquote = FALSE;
            if (curr_history < last_history && history[curr_history+1] != NULL)
            {
                if (history[curr_history+1] != NULL)
                {
                    fprintf (stdout, "%s", history[curr_history+1]);
                    curr_history = xmin (curr_history + 1, HISTORY);
                    for (; *cpt < (int) xstrlen (history[curr_history]); (*cpt)++)
                    {
                        if (*cpt >= *n * BUF)
                        {
                            (*n)++;
                            *ret = xrealloc (*ret, *n * BUF * sizeof (char));
                        }
                        if (history[curr_history][*cpt] == '"' && !*squote)
                            *dquote = !*dquote;
                        if (history[curr_history][*cpt] == '\'' && !*dquote)
                            *squote = !*squote;
                        (*ret)[*cpt] = history[curr_history][*cpt];
                    }
                }
            }
            else
            {
                curr_history = xmin (curr_history + 1, last_history);
            }
            fflush (stdout);
            break;
        }
    } 
    else
    {
        int max = *cpt + len, i = 0;
        for (; *cpt < max; (*cpt)++)
        {
            if (*cpt >= *n * BUF)
            {
                (*n)++;
                *ret = xrealloc (*ret, *n * BUF * sizeof (char));
            }
            if (input[i] == '"' && !*squote)
                *dquote = !*dquote;
            if (input[i] == '\'' && !*dquote)
                *squote = !*squote;
            (*ret)[*cpt] = input[i];
            fprintf (stdout, "%c", input[i]);
            i++;
        }
        fflush (stdout);
    }
    return -1;
}

char *
read_line (const char *prompt)
{
    char *ret = xmalloc (BUF * sizeof (char));
    int cpt = 0, ind = 1, nb_lines = 0, antislashes = 0, read_tmp = 0,
        bracket = 0;
    unsigned int squote = FALSE, dquote = FALSE, backquote = FALSE;
    char in[5], tmp[5], c = -1;
    curr_history = last_history;
    reset_completion ();
    if (ret == NULL)
        return NULL;
    if (prompt != NULL && xstrlen (prompt) > 0)
    {
        fprintf (stdout, "%s", prompt);
        fflush (stdout);
    }
    init_ioctl ();
    do
    {
        memset (in, 0, 5);
        sh_read (shell_terminal, &in, 5);
        c = get_char (in, prompt, &ret, &cpt, &ind, &squote, &dquote);
    } while (c == -1);
    while (c != '\n' ||
           (c == '\n' && (squote || dquote || backquote || bracket > 0)) ||
           nb_lines != antislashes)
    {
        if (c == '\t')
        {
xdebug ("ret: '%s' (%d)", ret, cpt);
            char *comp = completion (prompt, ret, &cpt);
            if (comp != NULL)
            {
xdebug ("ret: '%s' (%d) -> comp: '%s'", ret, cpt, comp);
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
                xfree (comp);
            }
            do
            {
                memset (in, 0, 5);
                sh_read (shell_terminal, &in, 5);
                c = get_char (in, prompt, &ret, &cpt, &ind, &squote, &dquote);
            } while (c == -1);
            continue;
        }
        else if (c != '\n')
        {
            fprintf (stdout, "%c", c);
            fflush (stdout);
        }
        else if (c == '\n' && (squote || dquote || backquote || bracket > 0))
        {
            fprintf (stdout, "\n> ");
            fflush (stdout);
        }
        if (c == '\'' && !dquote)
            squote = !squote;
        if (c == '"' && !squote)
            dquote = !dquote;
        if (c == '(' && !squote)
            bracket++;
        if (c == ')' && !squote)
            bracket = xmax (bracket-1,0);
        if (c == '`' && !squote)
            backquote = !backquote;
        if (c == '\\')
        {
            char ctmp = -1;
            do
            {
                memset (tmp, 0, 5);
                sh_read (shell_terminal, &tmp, 5);
                ctmp = get_char (tmp, prompt, &ret, &cpt, &ind, &squote, &dquote);
            } while (ctmp == -1);
            if (ctmp == '\n')
            {
                c = ctmp;
                antislashes++;
                fprintf (stdout, "\n> ");
                fflush (stdout);
                continue;
            }
            read_tmp = 1;
        }
        if (c == '\n' && !(squote || dquote || backquote || bracket > 0))
        {
            do
            {
                memset (in, 0, 5);
                sh_read (shell_terminal, &in, 5);
                c = get_char (in, prompt, &ret, &cpt, &ind, &squote, &dquote);
            } while (c == -1);
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
            c = get_char (tmp, prompt, &ret, &cpt, &ind, &squote, &dquote);
            cpt++;
            fprintf (stdout, "%c", c);
            fflush (stdout);
            goto replay;
        }
        ret[cpt] = c;
        cpt++;
        do
        {
            memset (in, 0, 5);
            sh_read (shell_terminal, &in, 5);
            c = get_char (in, prompt, &ret, &cpt, &ind, &squote, &dquote);
        } while (c == -1);
    }
    fprintf (stdout, "\n");
exit:
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
    clear_ioctl ();
    if (interrupted) {
        xfree (ret);
        return NULL;
    }
    return ret;
}
