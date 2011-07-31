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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <termios.h>


#include "xutils.h"
#include "parser.h"
#include "command.h"

static char *prompt = NULL;
static char *host = NULL;
static const char *fpwd = NULL;

void init_ioctl (void);

void
shelldone_init (void)
{
    host = malloc (30);

    gethostname (host, 30);

    init_ioctl ();
}

void
shelldone_clean (void)
{
    xfree (host);
    xfree (prompt);
}

void
init_ioctl (void)
{
    struct termios   term;

    if (ioctl(0, TCGETS, &term) != 0)
        printf("ioctl G prob\n");
    term.c_lflag &= ~(ECHO | ICANON);
    term.c_cc[VMIN] = 1;
    term.c_cc[VTIME] = 0;
    if (ioctl(0, TCSETS, &term) != 0)
        printf("ioctl S prob\n");
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
    size_t full;

    if (fpwd != NULL && xstrcmp (full_pwd, fpwd) == 0)
    {
        return xstrdup (prompt);
    }

    if (strncmp (full_pwd, home, s_home) == 0)
    {
        size_t size = s_pwd - s_home;
        int cpt = 1;
        pwd = xmalloc (size + 2);
        *pwd = '~';
        while (cpt < (int) size + 2)
        {
            pwd[cpt] = full_pwd[s_home+cpt-1];
            cpt++;
        }
        pwd[cpt-1] = '\0';
    }
    else
        pwd = xstrdup (full_pwd);

    xfree (prompt);
    full = xstrlen (user) + xstrlen (host) + xstrlen (pwd) + 4;
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
/*        dump_line (l); */
        run_line (l);
        free_line (l);
        xfree (li);
    }

/*    fprintf (stdout, "entered: %s\n", line); */

    xfree (li);
    shelldone_clean ();

/* avoid the 'unused variables' warning */

    (void) argc;
    (void) argv;

    return 0;
}
