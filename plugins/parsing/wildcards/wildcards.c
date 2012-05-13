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
    int i, k, argc;
    char **argv = NULL;
    void *tmp = data[0];
    command **tmpc = (command **)tmp;
    command *cmd = *tmpc;

    if (cmd->argc == 0)
        return 1;

    /* check if we are the first parser */
    unsigned int first = (cmd->argcf == 0);
    if (first)
    {
        cmd->argvf = xcalloc (cmd->argc, sizeof (char *));
        cmd->argcf = cmd->argc;
    }
    else
    {
        /* if not, copy the args because we are gonna change it a lot */
        argv = xcalloc (cmd->argcf, sizeof (char *));
        argc = cmd->argcf;
        for (i = 0; i < argc; i++)
            argv[i] = xstrdup (cmd->argvf[i]);
    }

    for (i = 0, k = 0; i < (first ? (cmd->argc) : (argc)); i++)
    {
        char *tmp = (first ? cmd->argv[i] : argv[i]);

        if (!(cmd->protected[i] & SINGLE_QUOTE) &&
            strpbrk (tmp,"*?[]$`") != NULL)
        {
            wordexp_t p;
            int j;
            int r = wordexp (tmp, &p, 0);
            if (r != 0)
            {
                fprintf (stderr, "shelldone: syntax error near '%s'\n",
                                 tmp);

                if (!first)
                {
                    int z;
                    for (z = 0; z < argc; z++)
                        xfree (argv[z]);
                    xfree (argv);
                }

                return -1;
            }
            if (p.we_wordc == 0 || (p.we_wordc == 1 &&
                xstrcmp (p.we_wordv[0],tmp) == 0))
            {
                /*
                fprintf (stderr, "shelldone: '%s' => '%s', k: %d %d\n",
                                 tmp,
                                 cmd->argvf[k],
                                 k,
                                 cmd->argcf);
                */
                if (r == 0)
                    wordfree (&p);

                if (cmd->argvf[k] != NULL)
                    xfree (cmd->argvf[k]);

                cmd->argvf[k] = xmalloc (1 * sizeof (char));
                *(cmd->argvf[k]) = '\0';

                k++;

/*                return -1;*/
            }
            else if (r == 0)
            {
                int argcf = cmd->argcf;
                if (p.we_wordc > 1)
                {
                    cmd->argcf += p.we_wordc - 1;
                    cmd->argvf = xrealloc (cmd->argvf,cmd->argcf * sizeof(char *));
                }

                for (j = 0; j < (int) p.we_wordc; j++)
                {
                    char *t = xstrdup (p.we_wordv[j]);
                    if (k+j < argcf && cmd->argvf[k+j] != NULL)
                        xfree (cmd->argvf[k+j]);
                    cmd->argvf[k+j] = t;
                }
                k += j;
                wordfree (&p);
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
    if (!first)
    {
        for (i = 0; i < argc; i++)
            xfree (argv[i]);
        xfree (argv);
    }

    return 1;
}

void
sd_plugin_clean (void)
{
    return;
}
