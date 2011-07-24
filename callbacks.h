#ifndef _CALLBACKS_H_
#define _CALLBACKS_H_

typedef struct _callback callback;

typedef void (* cmd_callback) (int argc, char **argv);

struct _callback {
    /* key */
    char *key;
    /* callback to execute */
    cmd_callback func;
};

void sd_cd (int argc, char **argv);

#endif
