/**
 * Shelldone
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

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <err.h>
#include <stdarg.h>

#include "xutils.h"

int
xmin (int a, int b)
{
    return a < b ? a : b;
}

int
xmax (int a, int b)
{
    return a > b ? a : b;
}

void
xfree_list (char **list, int size)
{
    if (list != NULL)
    {
        if (size < 0)
        {
            for (; *list != NULL; list++)
                xfree (*list);
        }
        else
        {
            int i;
            for (i = 0; i < size; i++)
                xfree (list[i]);
        }
        xfree (list);
    }
}

void
xfree (void *ptr)
{
    if (ptr != NULL)
    {
        free (ptr);
    }
}

size_t
xstrlen (const char *in)
{
    const char *s; 
    if (in == NULL)
    {
        return 0;
    }
    for (s = in; *s; ++s); 
    return (s - in); 
}

void *
xmalloc (size_t size)
{
    void *ret = malloc (size);
    if (ret == NULL)
        err (2, "xmalloc can not allocate %lu bytes", (u_long) size);
    return ret;
}

void *
xcalloc (size_t nmem, size_t size)
{
    void *ret = calloc (nmem, size);
    if (ret == NULL)
        err (2, "xcalloc can not allocate %lu bytes", (u_long) (size * nmem));
    return ret;
}

void *
xrealloc (void *src, size_t new_size)
{
    void *ret;
    if (src == NULL)
    {
        ret = xmalloc (new_size);
    }
    else
    {
        ret = realloc (src, new_size);
    }
    if (ret == NULL)
        err (2, "xrealloc can not reallocate %lu bytes", (u_long) new_size);
    return ret;
}

char *
xstrdup (const char *dup)
{
    size_t len;
    char *copy;

    len = xstrlen (dup);
    if (len == 0)
        return NULL;
    copy = xmalloc (len + 1);
    if (copy != NULL)
        strncpy (copy, dup, len + 1);
    return copy;
}

int
xstrcmp (const char *c1, const char *c2)
{
    size_t s1 = xstrlen (c1);
    size_t s2 = xstrlen (c2);

    if (s1 == 0 && s2 == 0)
        return 0;
    if (s1 == 0)
        return -1;
    if (s2 == 0)
        return +1;
    return strncmp (c1, c2, xmax (s1, s2));
}

char *
xstrcat (char *dest, const char *src)
{
    char *save = xstrdup (dest);
    size_t len = xstrlen (save) + xstrlen (src) + 1;
    xfree (dest);
    dest = xmalloc (len);
    if (dest == NULL)
    {
        xfree (save);
        return NULL;
    }
    snprintf (dest, len, "%s%s", save, src);
    xfree (save);
    return dest;
}

char **
xstrsplit (const char *src, const char *token, size_t *size)
{
    char **ret;
    int n = 1;
    char *save, *tmp, *init;
    init = xstrdup (src);
    tmp = strtok_r (init, token, &save);
    *size = 0;
    if (tmp == NULL)
    {
        xfree (init);
        return NULL;
    }
    ret = xcalloc (10, sizeof (char *));
    while (tmp != NULL)
    {
        if ((int) *size > n * 10)
        {
            n++;
            char **new = xrealloc (ret, n * 10 * sizeof (char *));
            if (new == NULL)
            {
                for (;*size > 0; (*size)--)
                    xfree (ret[*size-1]);
                xfree (ret);
                xfree (init);
                return NULL;
            }
            ret = new;
        }
        ret[*size] = xstrdup (tmp);
        tmp = strtok_r (NULL, token, &save);
        (*size)++;
    }
    if ((int) *size + 1 > n * 10)
    {
        ret = xrealloc (ret, (n * 10 + 1) * sizeof (char *));
    }
    ret[*size+1] = NULL;

    xfree (init);
    return ret;
}

char **
xstrsplitspace (const char *src, size_t *size)
{
    char **ret;
    int i, idx, len, j;
    size_t tot = xstrlen (src);
    unsigned int nospace = FALSE;
    *size = 0;
    for (i = 0; i < (int) tot; i++)
    {
        if (src[i] == ' ')
        {
            if (i == 0 || (i > 0 && src[i-1] != '\\' && src[i-1] != ' '))
                (*size)++;
        }
        else
            nospace = TRUE;
    }
    if (*size == 0 || !nospace)
    {
        *size = 1;
        ret = xcalloc (1, sizeof (char *));
        ret[0] = xstrdup (src);
        return ret;
    }
    if (i > 0 && src[i-1] != ' ')
        (*size)++;
    ret = xcalloc (*size, sizeof (char *));
    idx = 0;
    len = 0;
    j = 0;
    for (i = 0; i < (int) tot; i++)
    {
        if (src[i] == ' ')
        {
            if (i == 0)
            {
                while (src[i] == ' ' && i < (int) tot)
                    i++;
                idx = i;
            }
            else if (i > 0 && src[i-1] != '\\' && src[i-1] != ' ')
            {
                len = i - idx;
                ret[j] = xmalloc ((len + 1) * sizeof (char));
                snprintf (ret[j], len+1, "%s", src+idx);
                j++;
                while (src[i] == ' ' && i < (int) tot)
                    i++;
                idx = i;
            }
        }
    }
    if (i > 0 && src[i-1] != ' ')
    {
        len = i - idx;
        ret[j] = xmalloc ((len + 1) * sizeof (char));
        snprintf (ret[j], len+1, "%s", src+idx);
    }

    return ret;
}

char *
xstrjoin (char **tab, int size, const char *join)
{
    char *ret;
    size_t len = 0;
    int i, first = 1;
    if (size > 0)
    {
        for (i = 0; i < size; i++)
            len += xstrlen (tab[i]);
    }
    else
    {
        for (i = 0; tab[i] != NULL; i++)
            len += xstrlen (tab[i]);
    }
    len += (size > 0 ? size : i) * xstrlen (join);
    ret = xmalloc ((len + 1) * sizeof (char));
    --i; 
    for (; i >= 0; i--)
    {
        if (first)
        {
            snprintf (ret, 
                      xstrlen (tab[i]) + xstrlen (join) + 1, 
                      "%s%s",
                      join,
                      tab[i]);
            first = 0;
        }
        else
        {
            size_t s = xstrlen (tab[i]) + xstrlen (join) + xstrlen (ret) + 1;
            char *t = xmalloc (s * sizeof (char));
            snprintf (t, s, "%s%s%s", join, tab[i], ret);
            xfree (ret);
            ret = xstrdup (t);
            xfree (t);
        }
    }
    ret[len] = '\0';
    return ret;
}

void
syntax_error (const char input[], int size, int ind)
{
    int i;
    for (i = 0; i < size; i++)
        fprintf (stderr, "%c", input[i]);
    fprintf (stderr, "\n");
    for (i = 0; i < ind; i++)
        fprintf (stderr, " ");
    fprintf (stderr, "^\n");
    fprintf (stderr, "syntax error near '%c'\n", input[xmin (ind, size)]);
}

char *
xstrsub (const char *src, int begin, int len)
{
    if (src == NULL)
        return NULL;
    
    char *ret;
    size_t s_full = xstrlen (src);
    int l = len == -1 ? (int) s_full : len;
    int ind;

    ret = xmalloc ((xmin (s_full, l) + 1) * sizeof (char));
    ind = begin < 0 ? 
                xmax ((int) s_full + begin, 0) :
                xmin (s_full, begin);

    strncpy (ret, src+ind, xmin (s_full, l));
    ret[xmin (s_full, l)] = '\0';

    return ret;
}

/**
 * Prints a debug message if the DEBUG flag is enabled at the compilation time
 * @param file Which source-file calls the function
 * @param func In which function
 * @param line At which line
 * @param format The format message we'd like to print
 * @param ... optional arguments to pass to the format string
 */
void
xadebug (const char *file, const char *func, int line, const char *format, ...)
{
#ifdef DEBUG
    va_list args;
    fprintf (stdout, "[D] %s:%d    %s()", file, line, func);
    if (format != NULL)
    {
        va_start (args, format);
        fprintf (stdout, " => ");
        vfprintf (stdout, format, args);
        va_end (args);
    }
    fprintf (stdout, "\n");
#else
    (void) file;
    (void) func;
    (void) line;
    (void) format;
#endif

    return;
}
