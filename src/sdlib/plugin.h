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
#ifndef _LIB_PLUGIN_H_
#define _LIB_PLUGIN_H_

#define SDPLDIR "../plugins"

typedef struct _sdplugin sdplugin;
typedef struct _sdplist sdplist;
typedef struct _sdplugindata sdplugindata;

typedef enum 
{
    UNKNOWN = 0x00,
    PROMPT  = 0x01,
    PARSING = 0x02,
    BUILTIN = 0x04
} sdplugin_type;

typedef enum
{
    MAIN   = 0x01,
    INIT   = 0x02,
    CLEAN  = 0x04,
    CONFIG = 0x08
} function;

struct _sdplugindata
{
    const char *name;
    int prio;
    unsigned int loaded;
    sdplugin_type type;

    void (*init)   (sdplugindata *plugin);
    void (*clean)  (void);
    int  (*main)   (void **data);
    void (*config) (void **data);

    void *lib;
};

struct _sdplugin
{
    sdplugindata *content;

    sdplugin *next;
    sdplugin *prev;
};

struct _sdplist
{
    sdplugin *head;
    sdplugin *tail;
    int size;
};

#endif
