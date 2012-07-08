#include <glib/gprintf.h>
#include <json-glib/json-glib.h>
#include "gwq_util.h"
#include "gwq_session.h"

int GWQSessionInit(GWQSession* wqs, const gchar* qqNum, const gchar* passwd, void* context)
{
    int s_errno;
    
	if (!qqNum || !passwd) {
		GWQ_ERR_OUT(ERR_OUT, "\n");
	}
	
    wqs->num = g_string_new(qqNum);
    wqs->passwd = g_string_new(passwd);
    
    if (!(wqs->sps = soup_session_async_new_with_options(NULL, NULL))) {
        GWQ_ERR_OUT(ERR_FREE_PASSWD, "\n");
    }
    
    if (!(wqs->scj = soup_cookie_jar_new())) {
        GWQ_ERR_OUT(ERR_UNREF_SPS, "\n");
    }
    
    soup_session_add_feature(wqs->sps, SOUP_SESSION_FEATURE(wqs->scj));
    
    s_errno = sqlite3_open(":memory:", &wqs->pUserDb);
    
    if (!(wqs->pUserDb)) {
        GWQ_ERR_OUT(ERR_UNREF_SCJ, "create user db in memory failed: no enough memory\n");
    }
    
    if (s_errno != SQLITE_OK) {
        GWQ_ERR_OUT(ERR_CLOSE_USER_DB, "open user db failed: %s\n", sqlite3_errmsg(wqs->pUserDb));
    }
    
    s_errno = sqlite3_open(":memory:", &wqs->pMsgDb);
    
    if (!(wqs->pMsgDb)) {
        GWQ_ERR_OUT(ERR_UNREF_SCJ, "create msg db in memory failed: no enough memory\n");
    }
    
    if (s_errno != SQLITE_OK) {
        GWQ_ERR_OUT(ERR_CLOSE_USER_DB, "open msg db failed: %s\n", sqlite3_errmsg(wqs->pMsgDb));
    }
    
    
    wqs->context = context;
    wqs->st = GWQS_ST_OFFLINE;
    wqs->sendMsgSt =SEND_MSG_IDLE;
    wqs->verifyCode = g_string_new("");
    wqs->vcUin = g_string_new("");
    wqs->clientId = g_string_new("");
    
    wqs->ptcz = g_string_new("");
	wqs->skey = g_string_new("");
	wqs->ptwebqq = g_string_new("");
	wqs->ptuserinfo = g_string_new("");
    wqs->uin = g_string_new("");
    wqs->ptisp = g_string_new("");
    wqs->pt2gguin = g_string_new("");
    
    wqs->vfwebqq = g_string_new("");
    wqs->psessionid = g_string_new("");
    wqs->index = -1;
    wqs->port = -1;
    
    wqs->updateQQNumCount = 0;
    return 0;
ERR_FREE_STRS:
    g_string_free(wqs->psessionid, TRUE);    
    g_string_free(wqs->vfwebqq, TRUE);
    
	g_string_free(wqs->pt2gguin, TRUE);
	g_string_free(wqs->ptisp, TRUE);
	g_string_free(wqs->uin, TRUE);
	g_string_free(wqs->ptuserinfo, TRUE);
	g_string_free(wqs->ptwebqq, TRUE);
	g_string_free(wqs->skey, TRUE);
	g_string_free(wqs->ptcz, TRUE);
	g_string_free(wqs->clientId, TRUE);
    g_string_free(wqs->vcUin, TRUE);
	g_string_free(wqs->verifyCode, TRUE);
//ERR_FREE_POLL_MSG:
//    soup_session_cancel_message(wqs->sps, wqs->pollMsg, SOUP_STATUS_CANCELLED);
ERR_CLOSE_MSG_DB:
    sqlite3_close(wqs->pMsgDb);
ERR_CLOSE_USER_DB:
    sqlite3_close(wqs->pUserDb);
ERR_UNREF_SCJ:
    g_object_unref(wqs->scj);
ERR_UNREF_SPS:
    g_object_unref(wqs->sps);
ERR_FREE_PASSWD:
	g_string_free(wqs->num, TRUE);
ERR_FREE_NUM:
	g_string_free(wqs->passwd, TRUE);
ERR_OUT:
    return -1;
}

gpointer GWQSessionGetUserData(GWQSession* wqs)
{
    return wqs->context;
    
}

int GWQSessionExit(GWQSession* wqs)
{
	g_string_free(wqs->pt2gguin, TRUE);
	g_string_free(wqs->ptisp, TRUE);
	g_string_free(wqs->uin, TRUE);
	g_string_free(wqs->ptuserinfo, TRUE);
	g_string_free(wqs->ptwebqq, TRUE);
	g_string_free(wqs->skey, TRUE);
	g_string_free(wqs->ptcz, TRUE);
    g_string_free(wqs->vcUin, TRUE);
	g_string_free(wqs->verifyCode, TRUE);
	
    sqlite3_close(wqs->pMsgDb);

    sqlite3_close(wqs->pUserDb);
    
    g_object_unref(wqs->scj);
    
    g_object_unref(wqs->sps);
    
	g_string_free(wqs->num, TRUE);

	g_string_free(wqs->passwd, TRUE);
    return 0;
}
