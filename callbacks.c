/**
 * Shelldone
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
#ifndef _GNU_SOURCE
    #define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "xutils.h"
#include "callbacks.h"

void 
sd_cd (int argc, char **argv)
{
    char *target;
    if (argc > 1)
    {
        fprintf (stderr, "ERROR: too many arguments\n");
        fprintf (stderr, "usage: cd [directory]\n");
        return;
    }
    if (argc == 0 || xstrcmp (argv[0], "~") == 0)
    {
        target = xstrdup (getenv ("HOME"));
    }
    else
    {
        if (xstrcmp (argv[0], "-") == 0)
        {
            target = xstrdup (getenv ("OLDPWD"));
        }
        else if (*argv[0] == '~')
        {
            char *home = getenv ("HOME");
            char *tmp = xmalloc (xstrlen (argv[0]));
            size_t len;
            int i = 0, j = 1;
            for (; argv[0][j] != '\0'; tmp[i] = argv[0][j],i++,j++);
            tmp[i] = '\0';
            len = xstrlen (home) + xstrlen (tmp) + 1;
            target = xmalloc (len);
            snprintf (target, len, "%s%s", home, tmp);
            xfree (tmp);
        }
        else
        {
            target = xstrdup (argv[0]);
        }
    }
    if (chdir (target) == -1)
    {
        xfree (target);
        perror ("cd");
        return;
    }
    else
    {
        setenv ("OLDPWD", getenv ("PWD"), 1);
        if (*target == '/')
        {
            setenv ("PWD", target, 1);
        }
        else if (xstrcmp (target, ".") != 0)
        {
            char *oldpwd = getenv ("OLDPWD");
            size_t len = xstrlen (oldpwd) + xstrlen (target) + 2;
            char *tmp = xmalloc (len);
            snprintf (tmp, len, "%s/%s", oldpwd, target);
            setenv ("PWD", tmp, 1);
            xfree (tmp);
        }
    }
    xfree (target);
}
