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
#ifndef _XOPEN_SOURCE
    #define _XOPEN_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <wordexp.h>

#include <sdlib/plugin.h>
#include <command.h>
#include <structs.h>
#include <xutils.h>

void
sd_plugin_init (sdplugindata *plugin)
{
    plugin->name = "wildcards";
    plugin->type = PARSING;
    plugin->prio = -1;
}

int
sd_plugin_main (void **data)
{
    int i, k;
    void *tmp = data[0];
    command **tmpc = (command **)tmp;
    command *cmd = *tmpc;
/*
    if ((cmd->protect & SETTING) != 0)
    {
        xputenv (cmd->cmd);
    }
*/
    if (cmd->argc == 0)
    {
        /*
        if ((cmd->protect & SETTING) != 0)
            return 0;
        */
        return 1;
    }

    unsigned int first = (cmd->argcf == 0);
    if (first)
    {
        cmd->argvf = xcalloc (cmd->argc, sizeof (char *));
        cmd->argcf = cmd->argc;
    }

    for (i = 0, k = 0; i < (first ? (cmd->argc) : (cmd->argcf)); i++)
    {
        char *tmp = (first ? cmd->argv[i] : cmd->argvf[i]);
        if (((cmd->protected[i] & NONE) != 0 &&
            strpbrk (tmp,"*?[]$`") != NULL))
        {
            wordexp_t p;
            int j;
            int r = wordexp (tmp, &p, 0);
            if (r != 0)
            {
                fprintf (stderr, "shelldone: syntax error near '%s'\n",
                                 tmp);
                return -1;
            }
            if (p.we_wordc == 0 || (p.we_wordc == 1 &&
                xstrcmp (p.we_wordv[0],tmp) == 0))
            {
                fprintf (stderr, "shelldone: '%s': no match found!\n",
                                 tmp);
                if (r == 0/* && p.we_wordc > 0*/)
                    /* why did I do that p.we_wordc check? */
                    wordfree (&p);
                return -1;
            }
            if (r == 0)
            {
                if (p.we_wordc > 1)
                {
                    cmd->argcf += p.we_wordc - 1;
                    cmd->argvf = xrealloc (cmd->argvf,cmd->argcf * sizeof(char *));
                }

                for (j = 0; j < (int) p.we_wordc; j++)
                {
                    char *t = xstrdup (p.we_wordv[j]);
                    if (cmd->argvf[k+j] != NULL)
                        xfree (cmd->argvf[k+j]);
                    cmd->argvf[k+j] = t;
                }
                k += j;
                wordfree (&p);
            }
        }
        else if ((cmd->protected[i] & SINGLE_QUOTE) == 0 &&
                 strpbrk (tmp,"$`") != NULL)
        {
            size_t s1, s2;
            /*char **ttmp = xstrsplitspace (tmp, &s1);*/
            char **ttmp = xstrsplit (tmp, " ", &s1);
            int j,z;
            if (s1 > 0)
            {
                char **ret = xcalloc (s1, sizeof (char *));
                s2 = s1;
                for (z = 0; z < (int) s1; z++)
                    ret[z] = NULL;
                for (z = 0; z < (int) s1; z++)
                {
                    wordexp_t p;
                    int r = wordexp (ttmp[z], &p, 0);
                    if (r != 0)
                    {
                        fprintf (stderr, "shelldone: syntax error near '%s'\n",
                                         ttmp[z]);
                        return -1;
                    }
                    if (p.we_wordc == 0 || (p.we_wordc == 1 &&
                        xstrcmp (p.we_wordv[0],ttmp[z]) == 0))
                    {
                        if (r == 0 && p.we_wordc > 0)
                            wordfree (&p);
                        if (ret[k] != NULL)
                            xfree (ret[k]);
                        ret[k] = xstrdup (ttmp[z]);
                        k++;
                        continue;
                    }
                    if (r == 0)
                    {
                        if (p.we_wordc > 1)
                        {
                            s2 += p.we_wordc - 1;
                            ret = xrealloc (ret, s2 * sizeof(char *));
                        }

                        for (j = 0; j < (int) p.we_wordc; j++)
                        {
                            char *t = xstrdup (p.we_wordv[j]);
                            if (ret[z+j] != NULL)
                                xfree (ret[z+j]);
                            ret[z+j] = t;
                        }
                        z += j;
                        wordfree (&p);
                    }
                }
                if (cmd->argvf[k] != NULL)
                    xfree (cmd->argvf[k]);
                cmd->argvf[k] = xstrjoin (ret, s2, " ");
                k++;
                xfree_list (ret, s2);
                xfree_list (ttmp, s1);
            }
        }
        else
        {
            /*
            if ((cmd->protected[i] & SETTING) != 0)
            {
                xputenv (cmd->argv[i]);
                (cmd->argcf)--;
                continue;
            }
            */

            if (cmd->argvf[k] != NULL)
                xfree (cmd->argvf[k]);
            cmd->argvf[k] = xstrdup (cmd->argv[i]);
            k++;
        }
    }

    return 1;
}

void
sd_plugin_clean (void)
{
    return;
}
