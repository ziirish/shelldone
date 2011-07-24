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
