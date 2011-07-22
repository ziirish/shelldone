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
/*    return strlen (in); */
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
    if (len < 0)
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
