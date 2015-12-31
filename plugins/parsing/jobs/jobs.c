/**
 * Shelldone plugin 'jobs'
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

#include <sdlib/plugin.h>
#include <structs.h>
#include <jobs.h>
#include <xutils.h>

void
sd_plugin_init (sdplugindata *plugin)
{
    plugin->name = "jobs";
    plugin->type = PARSING;
    plugin->prio = 2;
}

int
sd_plugin_main (void **data)
{
    void *tmp = data[0];
    command **tmpc = (command **)tmp;
    command *cmd = *tmpc;
    if (cmd->argc > 0 && cmd->cmd &&
            (xstrcmp(cmd->cmd, "kill") == 0 ||
             xstrcmp(cmd->cmd, "bg") == 0 ||
             xstrcmp(cmd->cmd, "fg") == 0))
    {
        unsigned int first = (cmd->argcf == 0);
        if (first)
        {
            cmd->argvf = xcalloc (cmd->argc, sizeof (char *));
            cmd->argcf = cmd->argc;
        }
        int i,k;
        for (i = 0, k = 0; i < (first ? (cmd->argc) : (cmd->argcf)); i++)
        {
            if (!first && k > cmd->argcf)
            {
                cmd->argvf = xrealloc (cmd->argvf, cmd->argc * sizeof(char *));
                cmd->argcf = cmd->argc;
            }
            if (first ?
                (xstrlen (cmd->argv[i]) > 0 && *(cmd->argv[i]) == '%') :
                (xstrlen (cmd->argvf[i]) > 0 && *(cmd->argvf[i]) == '%'))
            {
                int j = strtol (first ? (cmd->argv[i]+1) : (cmd->argvf[i]+1),
                                NULL,
                                10);
                job *jb = get_job_by_job_id (j);
                if (jb != NULL)
                {
                    if (!first && cmd->argvf[k] != NULL)
                        xfree (cmd->argvf[k]);
                    cmd->argvf[k] = xmalloc (64 * sizeof (char));
                    snprintf (cmd->argvf[k], 64, "%d", jb->content->pid);
                }
                else
                {
                    fprintf (stderr, "'%s': no such job\n", cmd->argv[i]);
                    return -1;
                }
            }
            else if (first)
            {
                if (cmd->argvf[k] != NULL)
                    xfree (cmd->argvf[k]);
                cmd->argvf[k] = xstrdup (cmd->argv[i]);
            }
            k++;
        }
        if (first)
            cmd->argcf = k;
    }

    return 1;
}
