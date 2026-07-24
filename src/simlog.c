/*  simlog.c  --  SDL-free logging for the sim library  */

#include "simlog.h"
#include <stdarg.h>
#include <stdio.h>

static int g_enabled = 1;

int sim_log_set_enabled(int enabled)
{
    int prev  = g_enabled;
    g_enabled = enabled ? 1 : 0;
    return prev;
}

void sim_log(const char *fmt, ...)
{
    va_list ap;

    if (!g_enabled) return;

    /* stderr, unbuffered by default, so the last line before a crash is
     * not lost in a pipe's buffer — the same property SDL_Log has and
     * the smoke test relies on. */
    fputs("INFO: ", stderr);
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
}
