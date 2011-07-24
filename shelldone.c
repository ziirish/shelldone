#ifndef _GNU_SOURCE
    #define _GNU_SOURCE
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>


#include "xutils.h"
#include "command.h"

static char *prompt = NULL;
static char *host = NULL;
static const char *fpwd = NULL;

void
shelldone_init (void)
{
    host = malloc (30);

    gethostname (host, 30);
}

void
shelldone_clean (void)
{
    xfree (host);
    xfree (prompt);
}

char *
get_prompt (void)
{
    const char *user = getenv ("USER");
    const char *full_pwd = getenv ("PWD");
    const char *home = getenv ("HOME");
    char *pwd = NULL;

    size_t s_home = xstrlen (home);
    size_t s_pwd = xstrlen (full_pwd);

    if (fpwd != NULL && xstrcmp (full_pwd, fpwd) == 0)
    {
        return xstrdup (prompt);
    }

    if (s_pwd < s_home)
    {
        pwd = strndup (full_pwd, s_pwd);
    } 
    else if (strncmp (full_pwd, home, s_home) == 0)
    {
        size_t size = s_pwd - s_home;
        int cpt = 1;
        pwd = malloc (size + 2);
        if (pwd == NULL)
        {
            perror ("Memory");
            exit (1);
        }
        *pwd = '~';
        while (cpt < size + 2)
        {
            pwd[cpt] = full_pwd[s_home+cpt-1];
            cpt++;
        }
        pwd[cpt-1] = '\0';
    }

    xfree (prompt);
    size_t full = xstrlen (user) + xstrlen (host) + xstrlen (pwd) + 4;
    prompt = malloc (full + 1);
    snprintf (prompt, full + 1, "%s@%s:%s$ ", user, host, pwd);
    fpwd = full_pwd;

    xfree (pwd);
    return xstrdup (prompt);
}

int
main (int argc, char **argv)
{
    char *li = NULL;
    char *pt;

    shelldone_init ();

    while (1)
    {
        pt = get_prompt ();
        li = read_line (pt);
        xfree (pt);
        if (xstrcmp ("quit", li) == 0)
            break;
        input_line *l = parse_line (li);
        dump_line (l);
        run_line (l);
        free_line (l);
        xfree (li);
    }
    
/*    fprintf (stdout, "entered: %s\n", line); */

    xfree (li);
    shelldone_clean ();

    return 0;
}
