#include <stdio.h>
#include "gwq_util.h"

glong GetNowMillisecond()
{
    glong re;
    GTimeVal now;
    g_get_current_time(&now);
    re = now.tv_usec / 1000;
    re += now.tv_sec;
    return re;
}

void GWQAsciiToUtf8(const gchar* from, GString* to)
{
    const gchar *c;
    gunichar uc;
    int len;
    
    gchar tmpCStr[8];
    GWQ_DBG("==>GWQAsciiToUtf8()\n");
    for(c = from; *c != '\0'; ++c){
        if(*c == '\\' && *(c + 1) == 'u') {
            sscanf(c, "\\u%"G_GINT32_FORMAT"X", &uc);
            len = g_unichar_to_utf8(uc, tmpCStr);
            GWQ_DBG("unicode:%lld\n", (long long int)uc);
            g_string_append_len(to, tmpCStr, len);
        } else {
            g_string_append_c(to, *c);
            continue;
        }
    }
}

JsonObject* json_object_get_object(JsonObject* jo, const gchar* key)
{
    JsonNode *jn;
    
    if ((jn = json_object_get_member(jo, key))) {
        return json_node_get_object(jn);
    }
    return NULL;
}
