#ifndef _GWQ_SESSION_H_
#define _GWQ_SESSION_H_
#include <string.h>
#include <sqlite3.h>
#include <glib.h>
#include <libsoup/soup.h>
#include <json-glib/json-glib.h>
#include "g_qqhosts.h"
#include "gwq_recvmsg.h"
#include "gwq_sendmsg.h"

#define WQ_QQ_NUM_MAX_LEN   20
#define WQ_QQ_PASSWORD_MAX_LEN  50
typedef struct g_webqq_session GWQSession;

typedef void(*GWQSessionCallback)(GWQSession* wqs, void* context);
typedef void(*MessageRecievedCallBack)(GWQSession* wqs, QQRecvMsg* msg);
typedef void(*MessageSentCallBack)(GWQSession* wqs, QQSendMsg* msg, gint32 retCode);
typedef void(*UpdateQQNumByUinCallBack)(GWQSession* wqs, gint64 uin, gint64 qqNum);
typedef void(*UpdateLongNickByUinCallBack)(GWQSession* wqs, gint64 uin, const gchar* lnick);

struct g_webqq_session {
    SoupSession *sps;
    SoupCookieJar *scj;
    enum {
        GWQS_ST_OFFLINE,
        GWQS_ST_LOGIN,
        GWQS_ST_IDLE,
        GWQS_ST_LOGOUT,
    }st;
    GString* num;
    GString* passwd;
    void* context;
    GWQSessionCallback loginCallBack;
    GWQSessionCallback logoutCallBack;
    GWQSessionCallback updatUserInfoCallBack;
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
    
	sqlite3* pUserDb;	/* user info, in memory */
	sqlite3* pMsgDb;	/* message database */
    
    /* poll implementation */
    SoupMessage* pollMsg;
    MessageRecievedCallBack messageRecieved;
    
    /* send msg */
    enum {
        SEND_MSG_IDLE,
        SEND_MSG_ING
    }sendMsgSt;
    SoupMessage* sendMsg;
    QQSendMsg* msgToSent;
    MessageSentCallBack messageSent;
    UpdateQQNumByUinCallBack updateQQNumByUinCB;
    UpdateLongNickByUinCallBack updateLongNickByUinCB;
};

#define GWQ_CHAT_ST_HIDDEN "hidden"
#define GWQ_CHAT_ST_ONLINE "online"

int GWQSessionInit(GWQSession* wqs, const gchar* qqNum, const gchar* passwd, void* context);

int GWQSessionExit(GWQSession* wqs);

GWQSession* GWQSessionNew(const gchar* qqNum, const gchar* passwd, void* context);

void GWQSessionDestroy(GWQSession* wqs);

gpointer GWQSessionGetUserData(GWQSession* wqs);

int GWQSessionLogin(GWQSession* wqs, const gchar* chatStatus);

int GWQSessionLogOut(GWQSession* wqs);

int GWQSessionDoPoll(GWQSession* wqs);

int GWQSessionSendBuddyMsg(GWQSession* wqs, gint64 qqNum, gint64 toUin, QQSendMsg* qsm);

void GWQSessionSetCallBack(GWQSession* wqs, 
        GWQSessionCallback loginCallBack,
        GWQSessionCallback  logoutCallBack,
        MessageRecievedCallBack messageRecieved,
        MessageSentCallBack messageSent,
        UpdateQQNumByUinCallBack updateQQNumByUinCB,
        UpdateLongNickByUinCallBack updateLongNickByUinCB);

#endif
