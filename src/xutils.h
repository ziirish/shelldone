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
#ifndef _XUTILS_H_
#define _XUTILS_H_

/* define a few useful MACRO */
#define TRUE      1
#define FALSE     0
#define ARGC     50
#define BUF     256
#define HISTORY 100

/**
 * Returns the smallest value of the given parameters
 * @param a Value to compare
 * @param b Value to compare
 * @return Smallest value between a and b
 */
int xmin (int a, int b);

/**
 * Returns the biggest value of the given parameters
 * @param a Value to compare
 * @param b Value to compare
 * @return Biggest value between a and b
 */
int xmax (int a, int b);

/**
 * Frees the memory used by the given list
 * @param list List to free
 * @param size Size of the list (-1 to try to guess the size)
 */
void xfree_list (char **list, int size);
/* Macro to simplify the above command */
#define xafree_list(list) xfree_list (list, -1)

/**
 * Frees the memory used by the given pointer
 * @param ptr Pointer to free. If NULL the function returns
 */
void xfree (void *ptr);

/**
 * Allocates memory or exit and display an error
 * @param size Requested size to allocate
 * @return The address pointing to the memory-space
 */
void *xmalloc (size_t size);

/* Same as above for the calloc function */
void *xcalloc (size_t nmem, size_t size);

/* Same as abov for the realloc function */
void *xrealloc (void *src, size_t new_size);

/**
 * Duplicates a string
 * @param dup String to copy
 * @return A pointer to a copy of the given string
 */
char *xstrdup (const char *dup);

/**
 * Gives the size of a string
 * @param src String we want the size of
 * @return Size of src, 0 if NULL
 */
size_t xstrlen (const char *src);

/**
 * Compares two strings
 * @param c1 String to compare
 * @param c2 String to compare
 * @return <0 if c1 is lesser than c2, >0 if c2 is lesser than c1, 0 if c1
 * equals c2
 */
int xstrcmp (const char *c1, const char *c2);

/**
 * Concats two strings and increases dest if not large enough
 * @param dest String we concat with the other
 * @param src String to concat
 * @return dest
 */
char *xstrcat (char *dest, const char *src);

/**
 * Splits a string into an array of strings using the given tokenizer
 * @param src String to split
 * @param token On which token we split the string
 * @param size Pointer that will contains the size of the returned array
 * @return Array of strings
 */
char **xstrsplit (const char *src, const char *token, size_t *size);

/**
 * Joins an array of strings into a string using the given join
 * @param tab Array to join
 * @param size Size of the array (if -1 joins until NULL is found)
 * @param join String to add between the strings in the array
 * @return A string
 */
char *xstrjoin (char **tab, int size, const char *join);
/* Macro to simplify the above command */
#define xastrjoin(tab, join) xstrjoin (tab, -1, join);

/**
 * Shows where there is an error while parsing the user input
 * @param input The full input
 * @param size The full size of the input
 * @param ind Indice of the error
 */
void syntax_error (const char input[], int size, int ind);

/**
 * Extracts a substring out of the given string
 * @param src Input string
 * @param begin Index where to start the substring. If <0 starts from the end
 * @param len size of the substring
 * @return an allocated substring or NULL
 */
char *xstrsub (const char *src, int begin, int len);

#define xdebug(...) xadebug(__FILE__,__PRETTY_FUNCTION__,__LINE__,__VA_ARGS__)

void xadebug (const char *file,
              const char *func,
              int line,
              const char *format,
              ...);

#endif
