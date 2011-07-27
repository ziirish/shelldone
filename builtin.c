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
#include "builtin.h"

void 
sd_pwd (int argc, char **argv)
{
    if (argc > 0)
    {
        fprintf (stderr, "ERROR: too many arguments\n");
        fprintf (stderr, "usage: pwd\n");
        _exit (1);
    }

    fprintf (stdout, "%s\n", getenv ("PWD"));

    (void) argv;

    _exit (0);
}

void 
sd_cd (int argc, char **argv)
{
    char *target;
    if (argc > 1)
    {
        fprintf (stderr, "ERROR: too many arguments\n");
        fprintf (stderr, "usage: cd [directory]\n");
/*        _exit (1); */
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
/*        _exit (2); */
        return;
    }
    else
    {
        if (argc != 0 && xstrcmp (argv[0], "-") == 0)
        {
            char *tmp = getenv ("OLDPWD");
            setenv ("OLDPWD", getenv ("PWD"), 1);
            setenv ("PWD", tmp, 1);
            xfree (target);
            _exit (0);
        }
        setenv ("OLDPWD", getenv ("PWD"), 1);
        if (*target == '/')
        {
            setenv ("PWD", target, 1);
        }
        else if (xstrcmp (target, ".") != 0)
        {
            char *oldpwd = xstrdup (getenv ("OLDPWD")), *res;
            size_t s_old, s_tar;
            char **old_tab, **tar_tab, **full;
            int i, j;
            old_tab = xstrsplit (oldpwd, "/", &s_old);
            tar_tab = xstrsplit (target, "/", &s_tar);
            full = xcalloc ((s_old + s_tar), sizeof (char *));
            for (i = 0, j = 0; i < (int) s_old; i++, j++)
            {
                full[j] = xstrdup (old_tab[i]);
            }
            for (i = 0; i < (int) s_tar; i++, j++)
            {
                full[j] = xstrdup (tar_tab[i]);
            }
            xfree_list (old_tab, s_old);
            xfree_list (tar_tab, s_tar);
            xfree (oldpwd);
            i = 0;
            j = 0;
            while (i < (int) (s_old + s_tar))
            {
                if (xstrcmp (full[i], "..") == 0)
                {
                    j = xmax (j-1, 0);
                    xfree (full[j]);
                    full[j] = NULL;
                    xfree (full[i]);
                    full[i] = NULL;
                    i++;
                    continue;
                }
                full[j] = full[i];
                j++;
                i++;
            }

            res = xstrjoin (full, j, "/");
            if (xstrcmp (res, "") != 0)
                setenv ("PWD", res, 1);
            else
                setenv ("PWD", "/", 1);

            xfree_list (full, j);
            xfree (res); 
        }
    }
    xfree (target);
/*    _exit (0); */
    return;
}

void
sd_echo (int argc, char **argv)
{
    int nflag;
    int i;

    if (argc < 1)
        _exit (0);

    if (xstrcmp (argv[0], "-n") == 0) 
    {
        nflag = 1;
    }
    else
    {
        nflag = 0;
    }

    for (i = nflag ? 1 : 0; i < argc; i++) 
    {
        fprintf (stdout, "%s", argv[i]);
        if (i < argc - 1)
            fprintf (stdout, " ");
    }
    if (!nflag)
        fprintf (stdout, "\n");
    else
        fflush (stdout);

    _exit (0);
}
