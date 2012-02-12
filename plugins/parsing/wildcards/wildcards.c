/**
 * Shelldone plugin 'wildcards'
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
#include <string.h>
#include <fnmatch.h>
#include <dirent.h>

#include <sdlib/plugin.h>
#include <structs.h>
#include <xutils.h>

void
sd_plugin_init (sdplugindata *plugin)
{
    plugin->name = "wildcards";
    plugin->type = PARSING;
    plugin->prio = -1;
}

static char **
get_subfiles (char *dir, char *pattern, size_t *nb_files)
{
    int factor = 1;
    char **ret = NULL;
    struct dirent **files = NULL;
    unsigned int first = TRUE;
    int nb = scandir (dir, &files, NULL, NULL);
    int i;
    *nb_files = 0;
    if (nb == 0)
        return NULL;

    for (i = 0; i < nb; i++)
    {
        if (xstrcmp (files[i]->d_name, ".") != 0 &&
            xstrcmp (files[i]->d_name, "..") != 0 &&
            fnmatch (pattern, files[i]->d_name, 0) == 0)
        {
            if (first)
            {
                ret = xcalloc (factor * 50, sizeof (char *));
                first = FALSE;
            }
            if ((int) *nb_files > factor * 50)
            {
                factor++;
                ret = xrealloc (ret, factor * 50 * sizeof (char *));
            }
            ret[*nb_files] = xstrdup (files[i]->d_name);
            (*nb_files)++;
        }
        xfree (files[i]);
    }
    xfree (files);
    
    if (*nb_files == 0)
    {
        xfree (ret);
        ret = NULL;
    }

    return ret;
}

static char **
parse_wildcard (char *match, size_t *size)
{
    char **ret = NULL;
    *size = 0;
    if (strchr (match, '/') != NULL)
    {
        size_t tmp = 0;
        char **ltmp = xstrsplit (match, "/", &tmp);
        int i;
        for (i = 0; i < (int) tmp; i++)
        {
            if (strpbrk (ltmp[i], "*?[]") != NULL)
            {
                size_t nb = 0;
                size_t save = *size;
                size_t cpt;
                char *tmp_path = i > 0 ? xstrjoin (ltmp, i-1, "/") : ".";
                int ind = (*match == '/' || i == 0) ? 0 : 1;
                char **res = get_subfiles (tmp_path+ind, ltmp[i], &nb);
                if (i > 0)
                    xfree (tmp_path);
                if (nb == 0)
                    continue;
                if (*size == 0)
                {
                    *size = nb;
                    ret = xcalloc (nb, sizeof (char *));
                }
                else
                {
                    *size += nb;
                    ret = xrealloc (ret, *size * sizeof (char *));
                }
                for (cpt = 0; cpt < nb; cpt++)
                {
                    ret[save+cpt] = res[cpt];
                }
                xfree (res);
            }
        }
        xfree_list (ltmp, tmp);
    }
    else
    {
        size_t nb = 0;
        char **res = get_subfiles (".", match, &nb);
        if (nb > 0)
        {
            size_t cpt;
            size_t save = *size;
            if (*size == 0)
            {
                *size = nb;
                ret = xcalloc (nb, sizeof (char *));
            }
            else
            {
                *size += nb;
                ret = xrealloc (ret, *size * sizeof (char *));
            }
            for (cpt = 0; cpt < nb; cpt++)
            {
                ret[save+cpt] = res[cpt];
            }
            xfree (res);
        }
    }

    return ret;
}

void
sd_plugin_main (void **data)
{
    int i, k;
    void *tmp = data[0];
    command **tmpc = (command **)tmp;
    command *cmd = *tmpc;

    if (cmd->argc == 0)
        return;

    if (cmd->argcf == 0)
    {
        cmd->argvf = xcalloc (cmd->argc, sizeof (char *));
        cmd->argcf = cmd->argc;
    }

    for (i = 0, k = 0; i < cmd->argc; i++)
    {
        if (cmd->protected[i] == NONE && strpbrk (cmd->argv[i],"*?[]") != NULL)
        {
            size_t size = 0;
            char **tmp_list = parse_wildcard (cmd->argv[i], &size);
            if (size > 0)
            {
                int j;
                cmd->argcf += (int) size - 1;
                cmd->argvf = xrealloc (cmd->argvf,cmd->argcf * sizeof(char *));
                for (j = 0; j < (int) size; j++)
                {
                    cmd->argvf[k+j] = tmp_list[j];
                }
                k += j;
                xfree (tmp_list);
            }
        }
        else
        {
            if (cmd->argvf[k] != NULL)
                xfree (cmd->argvf[k]);
            cmd->argvf[k] = xstrdup (cmd->argv[i]);
            k++;
        }
    }
}

void
sd_plugin_clean (void)
{
    return;
}
