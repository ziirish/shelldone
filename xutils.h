#ifndef _XUTILS_H_
#define _XUTILS_H_

void xfree (void *ptr);

void *xmalloc (size_t size);

void *xcalloc (size_t nmem, size_t size);

void *xrealloc (void *src, size_t new_size);

char *xstrdup (const char *dup);

size_t xstrlen (const char *src);

int xstrcmp (const char *c1, const char *c2);

char *xstrcat (char *dest, const char *src);

#endif
