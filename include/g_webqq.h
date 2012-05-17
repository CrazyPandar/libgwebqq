#include <string.h>
#include <glib.h>
#include <libsoup/soup.h>
#include "qqhosts.h"

#define _DEBUG_
#ifdef _DEBUG_
#define GWQ_DBG(...) {g_print("[D][%s:%d]", __FILE__, __LINE__);g_print(__VA_ARGS__);}
#else
#define GWQ_DBG(...)
#endif
#define GWQ_MSG(...)	{g_print("[M][%s:%d]", __FILE__, __LINE__);g_print(__VA_ARGS__);}
#define GWQ_WARN(...)	{g_print("[W][%s:%d]", __FILE__, __LINE__);g_print(__VA_ARGS__);}
#define GWQ_ERR(...) {g_print("[E][%s:%d]", __FILE__, __LINE__);g_print(__VA_ARGS__);}
#define GWQ_ERR_OUT(__label, ...)	{g_print("[E][%s:%d]", __FILE__, __LINE__);g_print(__VA_ARGS__);goto __label;}

#define WQ_QQ_NUM_MAX_LEN   20
#define WQ_QQ_PASSWORD_MAX_LEN  50
typedef struct g_webqq_session GWQSession;

typedef void(*GWQSessionCallback)(GWQSession* wqs, void* context);

struct g_webqq_session {
    SoupSession *sps;
    SoupCookieJar *scj;
    enum {
        GWQS_ST_OFFLINE,
        GWQS_ST_LOGIN,
        GWQS_ST_IDLE
    }st;
    GString* num;
    GString* passwd;
    void* context;
    GWQSessionCallback loginCallback;
    /* get from check body */
    GString* verifyCode;
    /* get from login cookies */
    GString* ptcz;
	GString* skey;
	GString* ptwebqq;
	GString* ptuserinfo;
    GString* uin;
    GString* ptisp;
    GString* pt2gguin;
    
};

int GWQSessionInit(GWQSession* wqs, const gchar* qqNum, const gchar* passwd);

int GWQSessionExit(GWQSession* wqs);

int GWQSessionLogin(GWQSession* wqs, GWQSessionCallback callback, gpointer callbackCtx);

