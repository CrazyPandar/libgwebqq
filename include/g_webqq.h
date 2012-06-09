#ifndef _G_WEBQQ_H_
#define _G_WEBQQ_H_
#include <string.h>
#include <glib.h>
#include <libsoup/soup.h>
#include "g_qqhosts.h"

//#define _DEBUG_
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
        GWQS_ST_IDLE,
        GWQS_ST_LOGOUT,
        GWQS_ST_RECV_MSG,
        
    }st;
    GString* num;
    GString* passwd;
    void* context;
    GWQSessionCallback callBack;
    enum {
        GWQ_ERR_NO_ERROR,
        GWQ_ERR_SERVER_BUSY,
        GWQ_ERR_QQ_NUM_EXPIRED,
        GWQ_ERR_WRONG_PASSWORD,
        GWQ_ERR_WRONG_VERIFY_CODE,
        GWQ_ERR_VERIFY_FAIL,
        GWQ_ERR_NEED_TRY_AGAIN,
        GWQ_ERR_WRONG_INPUT,
        GWQ_ERR_SO_MANY_LOGIN_SAME_IP,
        GWQ_ERR_UNKNOWN,
    }errCode;
    const gchar* chatSt;
    GString* clientId;
    
    /* get from check body */
    GString* verifyCode;
    GString* vcUin;
    unsigned char vcUinAry[20];
    int vcUinAryLen;
    
    /* get from login cookies */
    GString* ptcz;
	GString* skey;
	GString* ptwebqq;
	GString* ptuserinfo;
    GString* uin;
    GString* ptisp;
    GString* pt2gguin;
    
    /*get from login2 result*/
    gint64 cip;
    gint index;
    gint port;
    GString* vfwebqq;
    GString* psessionid;
    
};

#define GWQS_CHAT_ST_HIDDEN "hidden"

int GWQSessionInit(GWQSession* wqs, const gchar* qqNum, const gchar* passwd);

int GWQSessionExit(GWQSession* wqs);

int GWQSessionLogin(GWQSession* wqs, GWQSessionCallback callback, gpointer callbackCtx);
int GWQSessionLogOut(GWQSession* wqs, GWQSessionCallback callback, gpointer callbackCtx);


glong GetNowMillisecond();
#endif


