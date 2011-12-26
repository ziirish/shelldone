#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sdlib/plugin.h>

void
sd_plugin_init (sdplugindata *plugin)
{
    plugin->name = "dummy";
    plugin->type = PROMPT;
    plugin->prio = 1;
}

void
sd_plugin_main (void)
{
    fprintf (stdout, "$ ");
    fflush (stdout);
}
