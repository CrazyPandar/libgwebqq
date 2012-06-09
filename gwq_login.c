#include <glib/gprintf.h>
#include <json-glib/json-glib.h>
#include "g_webqq.h"

static void _process_check_resp(SoupSession *session, SoupMessage *msg,  gpointer user_data);
static int _GWQSessionDoLogin(GWQSession* wqs);
static void _process_login_resp(SoupSession *ss, SoupMessage *msg,  gpointer user_data);
static int _GWQSessionDoLogin2(GWQSession* wqs);
static void _process_login2_resp(SoupSession *ss, SoupMessage *msg,  gpointer user_data);

static void _process_logout_resp(SoupSession *ss, SoupMessage *msg,  gpointer user_data);

static void soup_cookie_jar_get_cookie_value(SoupCookieJar* scj, 
        const gchar* domain, const gchar* path, const gchar* name, GString* value);
static void _GWQGenClientId(GWQSession* wqs);
static int _EncPwdVc(const gchar* passwd, const gchar* vcode, GString *ret);

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
    
    if (wqs->st != GWQS_ST_OFFLINE) {
        GWQ_ERR_OUT(ERR_OUT, "\n");
    }
    str = g_string_new("");
    g_string_printf(str, 
            "http://"LOGINHOST""VCCHECKPATH"?uin=%s&appid="APPID"&r=%.16f",
            wqs->num->str, g_random_double());
    msg = soup_message_new("GET", str->str);
    GWQ_DBG("do check\n");
    soup_session_queue_message (wqs->sps, msg, _process_check_resp, wqs);
    g_string_free(str, TRUE);
    wqs->context = callbackCtx;
    wqs->callBack = callback;
    wqs->st = GWQS_ST_LOGIN;
    return 0;
ERR_OUT:
    return -1;
}

static void _process_check_resp(SoupSession *ss, SoupMessage *msg,  gpointer user_data)
{
    GWQSession *wqs;
    const guint8* data;
    gsize size;
    SoupBuffer *sBuf;
    
    wqs = (GWQSession*)user_data;
    sBuf = soup_message_body_flatten(msg->response_body);
    GWQ_DBG("check responsed, retcode=%d, reason:%s\n", msg->status_code, msg->reason_phrase);
    if (sBuf) {
        soup_buffer_get_data(sBuf, &data, &size);
        GWQ_DBG("bodySize=%d\nbody:%s\n", size, data);
        if (size>0) {
        	const gchar *tmpCStr, *vc, *end;
        	
        	tmpCStr = (const gchar*)data;
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
        		g_string_assign(wqs->verifyCode, "");
        		g_string_append_len(wqs->verifyCode, vc, tmpCStr-vc);
        	}
        }
        soup_buffer_free(sBuf);
    }
    soup_session_cancel_message(ss, msg, SOUP_STATUS_CANCELLED);
    if (wqs->verifyCode->len) {
        if (_GWQSessionDoLogin(wqs)) {
            wqs->st = GWQS_ST_OFFLINE;
            wqs->callBack(wqs, wqs->context);
        }
    } else {
        wqs->st = GWQS_ST_OFFLINE;
        wqs->callBack(wqs, wqs->context);
    }
}

static int _GWQSessionDoLogin(GWQSession* wqs)
{
	gchar *tmpCStr = NULL;
	GString *tmpStr = NULL;
	SoupMessage *msg;
	
	GWQ_DBG("==>_GWQSessionDoLogin()\n");
	
	if (wqs->verifyCode->len > 0) {
		tmpStr = g_string_new("");
		if (_EncPwdVc(wqs->passwd->str, wqs->verifyCode->str, tmpStr)) {
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
	    return -1;
	}
}

static void _process_login_resp(SoupSession *ss, SoupMessage *msg,  gpointer user_data)
{
    GWQSession *wqs;
    const guint8* data;
    gsize size;
    SoupBuffer *sBuf;
    const gchar *tmpCStr;
    const gchar *retStr;
    gint retCode;
    
    wqs = (GWQSession*)user_data;
    sBuf = soup_message_body_flatten(msg->response_body);
    GWQ_DBG("login responsed, retcode=%d, reason:%s\n", msg->status_code, msg->reason_phrase);
    
    if (!sBuf) {
        GWQ_ERR_OUT(ERR_OUT, "\n");
    }

    soup_buffer_get_data(sBuf, &data, &size);
    
    if (!data || size <=0 ) {
        GWQ_ERR_OUT(ERR_FREE_SBUF, "\n");
    }
    

	GWQ_DBG("bodySize=%d\nbody:%s\n", size, data);
	/*
        ptuiCB('0','0','http://web.qq.com/loginproxy.html?login2qq=1&webqq_type=10','0','...............', '............');
    */
	tmpCStr = (const gchar*)data;
	tmpCStr = g_strstr_len(tmpCStr, size, "'");
	if (!tmpCStr 
	        || !(retStr=(tmpCStr+1))
	        || !(tmpCStr=g_strstr_len(retStr, size, "'"))) {
	    GWQ_ERR_OUT(ERR_FREE_SBUF, "\n");
	}
    retCode = g_ascii_strtoll(retStr, NULL, 10);
    switch (retCode) {
        case 0:
            wqs->errCode = GWQ_ERR_NO_ERROR;
            break;
        case 1:
            wqs->errCode = GWQ_ERR_SERVER_BUSY;
            break;
        case 2:
            wqs->errCode = GWQ_ERR_QQ_NUM_EXPIRED;
            break;
        case 3:
            wqs->errCode = GWQ_ERR_WRONG_PASSWORD;
            break;
        case 4:
            wqs->errCode = GWQ_ERR_WRONG_VERIFY_CODE;
            break;
        case 5:
            wqs->errCode = GWQ_ERR_VERIFY_FAIL;
            break;
        case 6:
            wqs->errCode = GWQ_ERR_NEED_TRY_AGAIN;
            break;
        case 7:
            wqs->errCode = GWQ_ERR_WRONG_INPUT;
            break;
        case 8:
            wqs->errCode = GWQ_ERR_SO_MANY_LOGIN_SAME_IP;
            break;
        default:
            wqs->errCode = GWQ_ERR_UNKNOWN;
            break;
    }
    if (wqs->errCode != GWQ_ERR_NO_ERROR) {
        GWQ_ERR_OUT(ERR_FREE_SBUF, "retCode=%d\n", retCode);
    }
	soup_cookie_jar_get_cookie_value(wqs->scj, "ptlogin2.qq.com", "/", "ptcz", wqs->ptcz);
	soup_cookie_jar_get_cookie_value(wqs->scj, "qq.com", "/", "skey", wqs->skey);
	soup_cookie_jar_get_cookie_value(wqs->scj, "qq.com", "/", "ptwebqq", wqs->ptwebqq);
	soup_cookie_jar_get_cookie_value(wqs->scj, "ptlogin2.qq.com", "/", "ptuserinfo", wqs->ptuserinfo);
	soup_cookie_jar_get_cookie_value(wqs->scj, "qq.com", "/", "uin", wqs->uin);
	soup_cookie_jar_get_cookie_value(wqs->scj, "qq.com", "/", "ptisp", wqs->ptisp);
	soup_cookie_jar_get_cookie_value(wqs->scj, "qq.com", "/", "pt2gguin", wqs->pt2gguin);
    if (_GWQSessionDoLogin2(wqs)) {
        GWQ_ERR_OUT(ERR_FREE_SBUF, "\n");
    }
    soup_buffer_free(sBuf);
    soup_session_cancel_message(ss, msg, SOUP_STATUS_CANCELLED);
    return;
ERR_FREE_SBUF:
    soup_buffer_free(sBuf);
ERR_OUT:
    soup_session_cancel_message(ss, msg, SOUP_STATUS_CANCELLED);
    wqs->st = GWQS_ST_OFFLINE;
    wqs->callBack(wqs, wqs->context);
}

static int _GWQSessionDoLogin2(GWQSession* wqs)
{
	gchar *tmpCStr = NULL;
	SoupMessage *msg;
	gchar *escaped;
	
	GWQ_DBG("==>_GWQSessionDoLogin2()\n");
	
	_GWQGenClientId(wqs);
	tmpCStr = g_strdup_printf("{\"status\":\"%s\",\"ptwebqq\":\"%s\","
			"\"passwd_sig\":""\"\",\"clientid\":\"%s\""
			", \"psessionid\":null}",
			GWQS_CHAT_ST_HIDDEN, wqs->ptwebqq->str,
			wqs->clientId->str);
    escaped = g_uri_escape_string(tmpCStr, NULL, FALSE);
    g_free(tmpCStr);
    tmpCStr = g_strdup_printf("r=%s", escaped);
    g_free(escaped);
	msg = soup_message_new("POST", "http://"PSIDHOST""PSIDPATH);
	soup_message_set_request (msg, "application/x-www-form-urlencoded",
              SOUP_MEMORY_COPY, tmpCStr, strlen(tmpCStr));
    //soup_message_headers_append (msg->request_headers, "Cookie2", "$Version=1");  /* not must */
    soup_message_headers_append (msg->request_headers, "Referer", "http://d.web2.qq.com/proxy.html?v=20101025002"); /* this is must */
    g_free(tmpCStr);
	soup_session_queue_message(wqs->sps, msg, _process_login2_resp, wqs);
	return 0;
}

static void _process_login2_resp(SoupSession *ss, SoupMessage *msg,  gpointer user_data)
{
    GWQSession *wqs;
    const guint8* data;
    gsize size;
    SoupBuffer *sBuf;
    const gchar *tmpCStr;
    JsonParser *jParser;
    JsonNode *jn;
    JsonObject *jo;
    
    wqs = (GWQSession*)user_data;
    
    GWQ_DBG("login2 responsed, retcode=%d, reason:%s\n", msg->status_code, msg->reason_phrase);
    if (msg->status_code != 200) {
        GWQ_ERR_OUT(ERR_OUT, "\n");
    }
    
    sBuf = soup_message_body_flatten(msg->response_body);
    if (!sBuf) {
        GWQ_ERR_OUT(ERR_OUT, "\n");
    }

    soup_buffer_get_data(sBuf, &data, &size);
    if (!data || size <=0 ) {
        GWQ_ERR_OUT(ERR_FREE_SBUF, "\n");
    }
    GWQ_DBG("bodySize=%d\nbody:%s\n", size, data);
    
    if (!(jParser = json_parser_new())) {
        GWQ_ERR_OUT(ERR_FREE_SBUF, "\n");
    }
    
    if (!json_parser_load_from_data(jParser, (const gchar*)data, size, NULL)) {
        GWQ_ERR_OUT(ERR_FREE_J_PARSER, "\n");
    }
    
    if (!(jn = json_parser_get_root(jParser))
            || !(jo = json_node_get_object(jn))) {
        GWQ_ERR_OUT(ERR_FREE_J_PARSER, "\n");
    }
    
    if (0 != json_object_get_int_member(jo, "retcode")) {
        GWQ_ERR_OUT(ERR_FREE_J_PARSER, "\n");
    }
    
    if (!(jo = json_object_get_object_member(jo, "result"))) {
        GWQ_ERR_OUT(ERR_FREE_J_PARSER, "\n");
    }
    
    if (!(tmpCStr = json_object_get_string_member(jo, "vfwebqq"))) {
        GWQ_ERR_OUT(ERR_FREE_J_PARSER, "\n");
    }
    g_string_assign(wqs->vfwebqq, tmpCStr);
    
    if (!(tmpCStr = json_object_get_string_member(jo, "psessionid"))) {
        GWQ_ERR_OUT(ERR_FREE_J_PARSER, "\n");
    }
    g_string_assign(wqs->psessionid, tmpCStr);
    
    wqs->cip = json_object_get_int_member(jo, "cip");
    
    wqs->index = json_object_get_int_member(jo, "index");
    
    wqs->port = json_object_get_int_member(jo, "port");
    
    wqs->st = GWQS_ST_IDLE;
    wqs->callBack(wqs, wqs->context);
    g_object_unref(jParser);
    soup_buffer_free(sBuf);
    soup_session_cancel_message(ss, msg, SOUP_STATUS_CANCELLED);
    return;
ERR_FREE_J_PARSER:
    g_object_unref(jParser);
ERR_FREE_SBUF:
    soup_buffer_free(sBuf);
ERR_OUT:
    soup_session_cancel_message(ss, msg, SOUP_STATUS_CANCELLED);
    wqs->st = GWQS_ST_OFFLINE;
    wqs->callBack(wqs, wqs->context);
}

int GWQSessionLogOut(GWQSession* wqs, GWQSessionCallback callback, gpointer callbackCtx)
{
    SoupMessage *msg;
    GString *str;
    
    if (wqs->st != GWQS_ST_IDLE) {
        GWQ_ERR_OUT(ERR_OUT, "\n");
    }
    str = g_string_new("");
    g_string_printf(str, 
            "http://"LOGOUTHOST""LOGOUTPATH"?clientid=%s&psessionid=%s&t=%ld",
            wqs->clientId->str, wqs->psessionid->str, GetNowMillisecond());
    msg = soup_message_new("GET", str->str);
    soup_message_headers_append (msg->request_headers, "Referer", LOGOUT_REFERER); /* this is must */
    GWQ_DBG("do logout\n");
    soup_session_queue_message (wqs->sps, msg, _process_logout_resp, wqs);
    g_string_free(str, TRUE);
    wqs->context = callbackCtx;
    wqs->callBack = callback;
    wqs->st = GWQS_ST_LOGOUT;
    return 0;
ERR_OUT:
    return -1;
}

static void _process_logout_resp(SoupSession *ss, SoupMessage *msg,  gpointer user_data)
{
    GWQSession *wqs;
    const guint8* data;
    gsize size;
    SoupBuffer *sBuf;
    const gchar *tmpCStr;
    JsonParser *jParser;
    JsonNode *jn;
    JsonObject *jo;
    
    wqs = (GWQSession*)user_data;
    
    GWQ_DBG("logout responsed, retcode=%d, reason:%s\n", msg->status_code, msg->reason_phrase);
    if (msg->status_code != 200) {
        GWQ_ERR_OUT(ERR_OUT, "\n");
    }
    
    sBuf = soup_message_body_flatten(msg->response_body);
    if (!sBuf) {
        GWQ_ERR_OUT(ERR_OUT, "\n");
    }

    soup_buffer_get_data(sBuf, &data, &size);
    if (!data || size <=0 ) {
        GWQ_ERR_OUT(ERR_FREE_SBUF, "\n");
    }
    GWQ_DBG("bodySize=%d\nbody:%s\n", size, data);
    
    if (!(jParser = json_parser_new())) {
        GWQ_ERR_OUT(ERR_FREE_SBUF, "\n");
    }
    
    if (!json_parser_load_from_data(jParser, (const gchar*)data, size, NULL)) {
        GWQ_ERR_OUT(ERR_FREE_J_PARSER, "\n");
    }
    
    if (!(jn = json_parser_get_root(jParser))
            || !(jo = json_node_get_object(jn))) {
        GWQ_ERR_OUT(ERR_FREE_J_PARSER, "\n");
    }
    
    if (0 != json_object_get_int_member(jo, "retcode")) {
        GWQ_ERR_OUT(ERR_FREE_J_PARSER, "\n");
    }
    
    if (!(tmpCStr = json_object_get_string_member(jo, "result"))
        || strcmp(tmpCStr, "ok")) {
        GWQ_ERR_OUT(ERR_FREE_J_PARSER, "\n");
    }
    
    wqs->st = GWQS_ST_OFFLINE;
    wqs->callBack(wqs, wqs->context);
    g_object_unref(jParser);
    soup_buffer_free(sBuf);
    soup_session_cancel_message(ss, msg, SOUP_STATUS_CANCELLED);
    return;
ERR_FREE_J_PARSER:
    g_object_unref(jParser);
ERR_FREE_SBUF:
    soup_buffer_free(sBuf);
ERR_OUT:
    soup_session_cancel_message(ss, msg, SOUP_STATUS_CANCELLED);
    wqs->st = GWQS_ST_IDLE;
    wqs->callBack(wqs, wqs->context);
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
/*
    GWQ_DBG("find cookie:[%s%s] %s = %s\n", 
            soup_cookie_get_domain(sc),
            soup_cookie_get_path(sc),
            soup_cookie_get_name(sc),
            soup_cookie_get_value(sc));
*/            
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

void _GWQGenClientId(GWQSession* wqs)
{
	gint32 r = g_random_int_range(10, 99);
	GTimeVal now;
	g_get_current_time(&now);
	glong t = now.tv_usec % 1000000;

	gchar buf[20] = {0};
	g_snprintf(buf, sizeof(buf), "%d%ld", (int)r, t);

	g_string_assign(wqs->clientId, buf);
}

static int _EncPwdVc(const gchar* passwd, const gchar* vcode, GString *ret)
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


