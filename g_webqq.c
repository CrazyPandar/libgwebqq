#include <glib/gprintf.h>
#include "g_webqq.h"

static int GWQEncPwdVc(const gchar* passwd, const gchar* vcode, GString *ret)
{
	GChecksum *cs;
	gint pwdLen;
	guchar sumBuf[2048];
	gsize sumBufSize;
	GString *tmpStr;
	const gchar *tmpCStr;
	
	if (!passwd || !vcode || !ret) {
		GWQ_ERR_OUT(ERR_OUT, "\n");
	}
	pwdLen = strlen(passwd);
	cs = g_checksum_new(G_CHECKSUM_MD5);
	if (!cs) {
		GWQ_ERR_OUT(ERR_OUT, "\n");
	}

	g_checksum_update(cs, (const guchar*)passwd, pwdLen);
	g_checksum_get_digest(cs, sumBuf, &sumBufSize);

	g_checksum_reset(cs);
	g_checksum_update(cs, sumBuf, sumBufSize);
	g_checksum_get_digest(cs, sumBuf, &sumBufSize);
	
	g_checksum_reset(cs);
	g_checksum_update(cs, sumBuf, sumBufSize);
	tmpCStr = g_checksum_get_string(cs);
	tmpCStr = g_ascii_strup(tmpCStr, strlen(tmpCStr));
	
	tmpStr = g_string_new(tmpCStr);
	g_string_append(tmpStr, vcode);
	
	g_checksum_reset(cs);
	g_checksum_update(cs, (const guchar*)tmpStr->str, tmpStr->len);
	tmpCStr = g_checksum_get_string(cs);
	tmpCStr = g_ascii_strup(tmpCStr, strlen(tmpCStr));
	
	g_string_assign(ret, tmpCStr);
	
	g_string_free(tmpStr, TRUE);
	
	g_checksum_free(cs);
	return 0;
ERR_FREE_CS:
	g_checksum_free(cs);
ERR_OUT:
	return -1;
}


static int _GWQSessionDoLogin(GWQSession* wqs);
static void _process_check_resp(SoupSession *session, SoupMessage *msg,  gpointer user_data);
static void _process_login_resp(SoupSession *ss, SoupMessage *msg,  gpointer user_data);
static void soup_cookie_jar_get_cookie_value(SoupCookieJar* scj, 
        const gchar* domain, const gchar* path, const gchar* name, GString* value);
int GWQSessionInit(GWQSession* wqs, const gchar* qqNum, const gchar* passwd)
{
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
    
    wqs->st = GWQS_ST_OFFLINE;
    wqs->verifyCode = g_string_new("");
    
    wqs->ptcz = g_string_new("");
	wqs->skey = g_string_new("");
	wqs->ptwebqq = g_string_new("");
	wqs->ptuserinfo = g_string_new("");
    wqs->uin = g_string_new("");
    wqs->ptisp = g_string_new("");
    wqs->pt2gguin = g_string_new("");
    return 0;
ERR_FREE_STRS:
	g_string_free(wqs->pt2gguin, TRUE);
	g_string_free(wqs->ptisp, TRUE);
	g_string_free(wqs->uin, TRUE);
	g_string_free(wqs->ptuserinfo, TRUE);
	g_string_free(wqs->ptwebqq, TRUE);
	g_string_free(wqs->skey, TRUE);
	g_string_free(wqs->ptcz, TRUE);
	g_string_free(wqs->verifyCode, TRUE);
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

int GWQSessionExit(GWQSession* wqs)
{
	g_string_free(wqs->pt2gguin, TRUE);
	g_string_free(wqs->ptisp, TRUE);
	g_string_free(wqs->uin, TRUE);
	g_string_free(wqs->ptuserinfo, TRUE);
	g_string_free(wqs->ptwebqq, TRUE);
	g_string_free(wqs->skey, TRUE);
	g_string_free(wqs->ptcz, TRUE);
	g_string_free(wqs->verifyCode, TRUE);
	
    g_object_unref(wqs->scj);
    
    g_object_unref(wqs->sps);
    
	g_string_free(wqs->num, TRUE);

	g_string_free(wqs->passwd, TRUE);
    return 0;
}

int GWQSessionLogin(GWQSession* wqs, GWQSessionCallback callback, gpointer callbackCtx)
{
    SoupMessage *msg;
    GString *str;
    
    str = g_string_new("");
    g_string_printf(str, 
            "http://"LOGINHOST""VCCHECKPATH"?uin=%s&appid="APPID"&r=%.16f",
            wqs->num->str, g_random_double());
    msg = soup_message_new("GET", str->str);
    GWQ_DBG("do check\n");
    soup_session_queue_message (wqs->sps, msg, _process_check_resp, wqs);
    g_string_free(str, TRUE);
    wqs->context = callbackCtx;
    wqs->loginCallback = callback;
    return 0;
}

static void _process_check_resp(SoupSession *ss, SoupMessage *msg,  gpointer user_data)
{
    GWQSession *wqs;
    const guint8* data;
    gsize size;
    SoupBuffer *sBuf;
    
    wqs = (GWQSession*)user_data;
    sBuf = soup_message_body_flatten(msg->response_body);
    GWQ_DBG("check responsed, retcode=%d\nreason:%s\n", msg->status_code, msg->reason_phrase);
    if (sBuf) {
        soup_buffer_get_data(sBuf, &data, &size);
        GWQ_DBG("bodySize=%d\nbody:%s\n", size, data);
        if (size>0) {
        	gchar *tmpCStr, *vc, *end;
        	
        	tmpCStr = (gchar*)data;
        	end = (gchar*)(data+size-1);
        	if (tmpCStr 
        			&& (tmpCStr = g_strstr_len(tmpCStr, size, ","))
        			&& tmpCStr < end
        			&& *(++tmpCStr)
        			&& (tmpCStr = g_strstr_len(tmpCStr, size, "'"))
        			&& tmpCStr < end
        			&& *(vc = ++tmpCStr)
        			&& (tmpCStr = g_strstr_len(tmpCStr, size, "'"))
        			&& tmpCStr < end) {
        		*tmpCStr = '\0';
        		g_string_assign(wqs->verifyCode, vc);
        	}
        }
        soup_buffer_free(sBuf);
    }
    soup_session_cancel_message(ss, msg, SOUP_STATUS_CANCELLED);
    _GWQSessionDoLogin(wqs);
}

static int _GWQSessionDoLogin(GWQSession* wqs)
{
	gchar *tmpCStr = NULL;
	GString *tmpStr = NULL;
	SoupMessage *msg;
	
	GWQ_DBG("==>_GWQSessionDoLogin()\n");
	
	if (wqs->verifyCode->len > 0) {
		tmpStr = g_string_new("");
		if (GWQEncPwdVc(wqs->passwd->str, wqs->verifyCode->str, tmpStr)) {
			GWQ_ERR("\n");
			g_string_free(tmpStr, TRUE);
			return -1;
		}
		tmpCStr = g_strdup_printf("http://"LOGINHOST""LOGINPATH"?u=%s&p=%s&verifycode=%s&webqq_type=40&"
				 "remember_uin=0&aid="APPID"&login2qq=1&u1=%s&h=1&"
				 "ptredirect=0&ptlang=2052&from_ui=1&pttype=1"
				 "&dumy=&fp=loginerroralert&action=4-30-764935&mibao_css=m_webqq"
				 , wqs->num->str, tmpStr->str, wqs->verifyCode->str
				 , LOGIN_S_URL);
		msg = soup_message_new("GET", tmpCStr);
		soup_session_queue_message(wqs->sps, msg, _process_login_resp, wqs);
		g_free(tmpCStr);
		g_string_free(tmpStr, TRUE);
		return 0;
	} else {
		wqs->st = GWQS_ST_OFFLINE;
		wqs->loginCallback(wqs, wqs->context);
		return 0;
	}
}

static void _process_login_resp(SoupSession *ss, SoupMessage *msg,  gpointer user_data)
{
    GWQSession *wqs;
    const guint8* data;
    gsize size;
    SoupBuffer *sBuf;
    
    wqs = (GWQSession*)user_data;
    sBuf = soup_message_body_flatten(msg->response_body);
    GWQ_DBG("login responsed, retcode=%d\nreason:%s\n", msg->status_code, msg->reason_phrase);
    if (sBuf) {
        soup_buffer_get_data(sBuf, &data, &size);

        if (data && size>0) {
        	GWQ_DBG("bodySize=%d\nbody:%s\n", size, data);
        	soup_cookie_jar_get_cookie_value(wqs->scj, "ptlogin2.qq.com", "/", "ptcz", wqs->ptcz);
        	soup_cookie_jar_get_cookie_value(wqs->scj, "qq.com", "/", "skey", wqs->ptcz);
        	soup_cookie_jar_get_cookie_value(wqs->scj, "qq.com", "/", "ptwebqq", wqs->ptcz);
        	soup_cookie_jar_get_cookie_value(wqs->scj, "ptlogin2.qq.com", "/", "ptuserinfo", wqs->ptcz);
        	soup_cookie_jar_get_cookie_value(wqs->scj, "qq.com", "/", "uin", wqs->ptcz);
        	soup_cookie_jar_get_cookie_value(wqs->scj, "qq.com", "/", "ptisp", wqs->ptcz);
        	soup_cookie_jar_get_cookie_value(wqs->scj, "qq.com", "/", "pt2gguin", wqs->ptcz);
        	GWQ_DBG("ptcz: %s\n", wqs->ptcz->str);
        	wqs->st = GWQS_ST_IDLE;
        	wqs->loginCallback(wqs, wqs->context);
        } else {
        	wqs->st = GWQS_ST_OFFLINE;
        	wqs->loginCallback(wqs, wqs->context);
        }
        soup_buffer_free(sBuf);
    }
    soup_session_cancel_message(ss, msg, SOUP_STATUS_CANCELLED);
    
}

struct cookie_key {
    const gchar* domain;
    const gchar* path;
    const gchar* name;
};

static gint _find_cookie(gconstpointer data, gconstpointer userData)
{
    struct cookie_key *ck;
    SoupCookie *sc;
    
    ck = (struct cookie_key*)userData;
    sc = (SoupCookie*)data;
    
    GWQ_DBG("find cookie:[%s%s] %s = %s\n", 
            soup_cookie_get_domain(sc),
            soup_cookie_get_path(sc),
            soup_cookie_get_name(sc),
            soup_cookie_get_value(sc));
    if (soup_cookie_domain_matches(sc, ck->domain) 
            && !strcmp(ck->name, soup_cookie_get_name(sc))
            && !strcmp(ck->path, soup_cookie_get_path(sc))) {
        return 0;
    }
    return -1;
}

static void soup_cookie_jar_get_cookie_value(SoupCookieJar* scj, 
        const gchar* domain, const gchar* path, const gchar* name, GString* value)
{
    GSList *sl;
    GSList *tmpSl;
    
    struct cookie_key ck = {
        .domain = domain,
        .path = path,
        .name = name
    };
    sl = soup_cookie_jar_all_cookies(scj);
    if (sl) {
        tmpSl = g_slist_find_custom(sl, &ck, _find_cookie);
        if (tmpSl) {
            g_string_assign(value, soup_cookie_get_value((SoupCookie*)tmpSl->data));
        }
        g_slist_free_full(sl, (GDestroyNotify)soup_cookie_free);
    }
}





