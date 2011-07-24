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
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

#include "xutils.h"

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
        perror ("xmalloc");
    return ret;
}

void *
xcalloc (size_t nmem, size_t size)
{
    void *ret = calloc (nmem, size);
    if (ret == NULL)
        perror ("xcalloc");
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
        perror ("xrealloc");
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

    if (s1 != s2)
        return s1 - s2;
    if (s1 == 0 && s2 == 0)
        return 0;
    if (s1 == 0)
        return -1;
    if (s2 == 0)
        return +1;
    return strncmp (c1, c2, s1);
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
        return NULL;
    }
    snprintf (dest, len, "%s%s", save, src);
    xfree (save);
    return dest;
}

