#include <glib/gprintf.h>
#include <json-glib/json-glib.h>
#include "g_webqq.h"

glong GetNowMillisecond()
{
    glong re;
    GTimeVal now;
    g_get_current_time(&now);
    re = now.tv_usec / 1000;
    re += now.tv_sec;
    return re;
}
