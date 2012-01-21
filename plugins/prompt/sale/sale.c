/**
 * Shelldone plugin 'sale'
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
#include <xutils.h>

static char *prompt = NULL;
static const char *fpwd = NULL;

/**
 * Gets the prompt in a hard-coded pattern like:
 * C:\path>
 * @return The prompt
 */
static const char *
get_prompt (void)
{
    const char *full_pwd = getenv ("PWD");
    char *pwd = NULL;
    char **tmp = NULL;
    size_t size;
    size_t full;

    if (fpwd != NULL && xstrcmp (full_pwd, fpwd) == 0)
    {
        return prompt;
    }

    tmp = xstrsplit (full_pwd, "/", &size);
    if (size > 0)
        pwd = xstrjoin (tmp, size, "\\");
    else
        pwd = xstrdup ("\\");
    xfree_list (tmp, size);

    xfree (prompt);
    full = xstrlen (pwd) + 4;
    prompt = xmalloc (full + 1);
    snprintf (prompt, full + 1, "C:%s> ", pwd);
    fpwd = full_pwd;

    xfree (pwd);
    return prompt;
}

void
sd_plugin_init (sdplugindata *plugin)
{
    plugin->name = "sale";
    plugin->type = PROMPT;
    plugin->prio = 1;
}

void
sd_plugin_main (void **data)
{
    void *tmp = data[0];
    const char **stmp = (const char **)tmp;
    *stmp = get_prompt ();

    return;
}

void
sd_plugin_clean (void)
{
    xfree (prompt);
}
