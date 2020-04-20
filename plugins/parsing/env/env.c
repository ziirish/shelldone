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
#ifndef _DEFAULT_SOURCE
    #define _DEFAULT_SOURCE
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
#include <xutils.h>
#include <structs.h>
#include <list.h>
#include <modules.h>

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
sd_putenv (const char *set, Protection prot)
{
    sddata *tmp = xmalloc (sizeof (*tmp));
    tmp->content = xstrdup (set);
    if (is_module_present ("wildcards") && strpbrk (set,"*?[]$`") != NULL &&
        !(prot & SINGLE_QUOTE))
    {
        wordexp_t p;
        int r, i;
        char *param = strstr (tmp->content, "=");
        char *ini = xstrsub (tmp->content,
                             0,
                             (int)(xstrlen (tmp->content) - xstrlen (param)));
        ++param;
        r = wordexp (param, &p, 0);
        if (p.we_wordc == 0 || (p.we_wordc == 1 &&
            xstrcmp (p.we_wordv[0], param) == 0))
        {
            if (r == 0)
                wordfree (&p);

            sd_info ("'%s' is not set, using empty value\n", param);
            xfree (tmp->content);
            xfree (ini);
            xfree (tmp);

            return 0;
        }
        else if (r == 0)
        {
            if (p.we_wordc > 1)
            {
                char *res = NULL;
                size_t l = 0;
                for (i = 0; i < (int) p.we_wordc; i++)
                    l += xstrlen (p.we_wordv[i]) + 1;
                res = xmalloc (l * sizeof (char));
                memset (res, '\0', l);
                for (i = 0; i < (int) p.we_wordc; i++)
                {
                    size_t s = xstrlen (res) + xstrlen (p.we_wordv[i]) + 2;
                    char *tmp = NULL;
                    xalloca(tmp, s * sizeof (char));
                    if (res != NULL)
                    {
                        snprintf (tmp,
                                  s,
                                  "%s%s%c",
                                  res,
                                  p.we_wordv[i],
                                  i == (int) p.we_wordc ? '\0' : ' ');
                    }
                    else
                    {
                        snprintf (tmp,
                                  s,
                                  "%s%c",
                                  p.we_wordv[i],
                                  i == (int) p.we_wordc ? '\0' : ' ');
                    }
                    xfree (res);
                    res = xstrdup (tmp);
                }

                wordfree (&p);
                xfree (tmp->content);
                l = xstrlen (ini) + xstrlen (res) + 2;
                tmp->content = xmalloc (l * sizeof (char));
                snprintf (tmp->content, l, "%s=%s", ini, res);
                xfree (res);
            }
            else
            {
                size_t l = xstrlen (ini) + xstrlen (p.we_wordv[0]) + 2;
                xfree (tmp->content);
                tmp->content = xmalloc (l * sizeof (char));
                snprintf (tmp->content, l, "%s=%s", ini, p.we_wordv[0]);
                wordfree (&p);
            }
        }
        xfree (ini);
    }
    list_append (&env, tmp);

    return putenv (tmp->content);
}


void
sd_plugin_init (sdplugindata *plugin)
{
    plugin->name = "env";
    plugin->type = PARSING;
    plugin->prio = -5;
    init_env ();
}

int
sd_plugin_main (void **data)
{
    void *tmp = data[0];
    command **tmpc = (command **)tmp;
    command *cmd = *tmpc;
    if (cmd->cmd != NULL &&
        (cmd->protect & SETTING) && cmd->argc < 1)
    {
        sd_debug ("setting: '%s'\n", cmd->cmd);
        sd_putenv (cmd->cmd, cmd->protect);
        xfree (cmd->cmd);
        cmd->cmd = NULL;
    }
    if (cmd->argc > 0)
    {
        while ((cmd->protect & SETTING) && cmd->argc > 0)
        {
            int i;
            char **argv = xcalloc (cmd->argc-1, sizeof (char *));
            Protection *prot = xcalloc (cmd->argc-1, sizeof (Protection));
            sd_debug ("setting: '%s'\n", cmd->cmd);
            sd_putenv (cmd->cmd, cmd->protect);
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
        if ((cmd->protect & SETTING) && cmd->argc == 0)
        {
            sd_debug ("setting: '%s'\n", cmd->cmd);
            sd_putenv (cmd->cmd, cmd->protect);
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
            if (cmd->protected[i] & SETTING)
            {
                sd_debug ("setting: '%s'\n", cmd->argv[i]);
                sd_putenv (cmd->argv[i], cmd->protected[i]);
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
