/**
 * Shelldone plugin 'env'
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
#ifndef _XOPEN_SOURCE
    #define _XOPEN_SOURCE
#endif
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sdlib/plugin.h>
#include <xutils.h>
#include <structs.h>
#include "list.h"

static sdlist *env;

static void
init_env (void)
{
    xdebug (NULL);
    env = xmalloc (sizeof (*env));
    if (env != NULL)
    {
        env->head = NULL;
        env->tail = NULL;
        env->size = 0;
    }
}

static void
clear_env (void)
{
    xdebug (NULL);
    sddata *tmp = env->head;
    while (tmp != NULL)
    {
        sddata *tmp2 = tmp->next;
        xfree (tmp->content);
        xfree (tmp);
        tmp = tmp2;
    }
    xfree (env);
}

static int
sd_putenv (const char *set)
{
    sddata *tmp = xmalloc (sizeof (*tmp));
    tmp->content = xstrdup (set);
    list_append (&env, tmp);
    return putenv (tmp->content);
}


void
sd_plugin_init (sdplugindata *plugin)
{
    plugin->name = "env";
    plugin->type = PARSING;
    plugin->prio = -2;
    init_env ();
}

int
sd_plugin_main (void **data)
{
    void *tmp = data[0];
    command **tmpc = (command **)tmp;
    command *cmd = *tmpc;
    if (cmd->cmd != NULL &&
        (cmd->protect & SETTING) != 0 && cmd->argc < 1)
    {
        sd_debug ("setting: '%s'\n", cmd->cmd);
        sd_putenv (cmd->cmd);
        xfree (cmd->cmd);
        cmd->cmd = NULL;
    }
    if (cmd->argc > 0)
    {
        while ((cmd->protect & SETTING) != 0 && cmd->argc > 0)
        {
            int i;
            char **argv = xcalloc (cmd->argc-1, sizeof (char *));
            Protection *prot = xcalloc (cmd->argc-1, sizeof (Protection));
            sd_debug ("setting: '%s'\n", cmd->cmd);
            sd_putenv (cmd->cmd);
            xfree (cmd->cmd);
            cmd->cmd = cmd->argv[0];
            cmd->protect = cmd->protected[0];
            for (i = 1; i < cmd->argc; i++)
            {
                argv[i-1] = cmd->argv[i];
                prot[i-1] = cmd->protected[i];
            }
            xfree (cmd->argv);
            xfree (cmd->protected);
            cmd->argv = argv;
            cmd->protected = prot;
            (cmd->argc)--;
        }
        if ((cmd->protect & SETTING) != 0 && cmd->argc == 0)
        {
            sd_debug ("setting: '%s'\n", cmd->cmd);
            sd_putenv (cmd->cmd);
            xfree (cmd->cmd);
            cmd->cmd = NULL;
        }
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
            if ((cmd->protected[i] & SETTING) != 0)
            {
                sd_debug ("setting: '%s'\n", cmd->argv[i]);
                sd_putenv (cmd->argv[i]);
            }
            else
            {
                cmd->argvf[k] = xstrdup (cmd->argv[i]);
                k++;
            }
        }
        cmd->argcf = k;
    }

    return 1;
}

void
sd_plugin_clean (void)
{
    clear_env ();
    return;
}
