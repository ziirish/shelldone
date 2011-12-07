#ifndef __PLUGIN_H__
#define __PLUGIN_H__

typedef struct _sdplugin sdplugin;
typedef struct _sdplist sdplist;

typedef enum 
{
    PROMPT = 1,
    PARSING
} sdplugin_type;

struct _sdplugin
{
    const char *name;
    int prio;
    unsigned int loaded;
    sdplugin_type type;

    void (*init_plugin) (sdplugin *plugin);
    void (*clean_plugin) (sdplugin *plugin);
    void (*main_plugin) (void);

    sdplugin *next;
    sdplugin *prev;

    void *lib;
};

struct _sdplist
{
    sdplugin *head;
    sdplugin *tail;
    int size;
}

#endif
