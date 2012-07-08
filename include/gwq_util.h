#ifndef _GWQ_UTIL_H_
#define _GWQ_UTIL_H_
#include <string.h>
#include <sqlite3.h>
#include <glib.h>
#include <glib/gprintf.h>
#include <libsoup/soup.h>
#include <json-glib/json-glib.h>

//#define _DEBUG_
#ifdef _DEBUG_
#define GWQ_DBG(...) {g_printf("[D][%s:%d]", __FILE__, __LINE__);g_printf(__VA_ARGS__);}
#else
#define GWQ_DBG(...)
#endif
#define GWQ_MSG(...)	{g_printf("[M][%s:%d]", __FILE__, __LINE__);g_printf(__VA_ARGS__);}
#define GWQ_WARN(...)	{g_printf("[W][%s:%d]", __FILE__, __LINE__);g_printf(__VA_ARGS__);}
#define GWQ_ERR(...) {g_printf("[E][%s:%d]", __FILE__, __LINE__);g_printf(__VA_ARGS__);}
#define GWQ_ERR_OUT(__label, ...)	{g_printf("[E][%s:%d]", __FILE__, __LINE__);g_printf(__VA_ARGS__);goto __label;}


glong GetNowMillisecond();
void GWQAsciiToUtf8(const gchar* from, GString* to);
JsonObject* json_object_get_object(JsonObject* jo, const gchar* key);

#endif
