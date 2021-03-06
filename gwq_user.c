#include <sqlite3.h>
#include <glib/gprintf.h>
#include <json-glib/json-glib.h>
#include "g_webqq.h"
#include "gwq_user.h"
static void _process_get_user_friends2_resp(SoupSession *ss, SoupMessage *msg,  gpointer user_data);
static void _add_category(JsonArray *array,
					 guint index,
					 JsonNode *element_node,
					 gpointer user_data);
static void _add_friend(JsonArray *array,
					 guint index,
					 JsonNode *element_node,
					 gpointer user_data);
static void _update_user_info(JsonArray *array,
					 guint index,
					 JsonNode *element_node,
					 gpointer user_data);
static void _update_user_markname(JsonArray *array,
					 guint index,
					 JsonNode *element_node,
					 gpointer user_data);
/*
 * [sqlite user info database]
 * 
 * friends table
 * =============
 * uin(int64)	qqNum(int64)	nick(string)	markname(string)    longNIck(string)	face(uint32)	category(uint32)	flag(uint32) online(uint32)
 * 
 * category table
 * ==============
 * index(uint32)	name(string)
 * 
 * 
*/
#define USERS_TBL_UIN_COL	0
#define USERS_TBL_QQ_NUM_COL	1
#define USERS_TBL_NICK_COL	2
#define USERS_TBL_MARKNAME_COL	3
#define USERS_TBL_FACE_COL	4
#define USERS_TBL_CATEGORY_COL	5
#define USERS_TBL_FLAG_COL	6
#define USERS_TBL_ONLINE_COL	7
#define USERS_TBL_LONG_NICK_COL	8
#define CREATE_USERS_TABLE_STMT	"create table if not exists users (\
        uin INTEGER PRIMARY KEY, qqNum INTEGER, \
        nick TEXT, markname TEXT, face INTEGER, category INTEGER, flag INTEGER, online INTEGER, longNick TEXT, \
        birth_year INTEGER, birth_month INTEGER, birth_day INTEGER, occupation TEXT, \
        phone TEXT, allow INTEGER, college TEXT, constel INTEGER, blood INTEGER, \
        homepage TEXT, stat INTEGER, vip_info INTEGER, country TEXT, city TEXT, \
        personal TEXT, shengxiao INTEGER, email TEXT, province TEXT, gender TEXT, mobile TEXT\
        )"
#define CATEGS_TBL_IDX_COL  0
#define CATEGS_TBL_NAME_COL 1
#define CREATE_CATOGERIES_TABLE_STMT "create table if not exists categories (idx INTEGER, name TEXT)"

void GWQUserInfoFree(GWQUserInfo* wui)
{
    GWQ_DBG("==>GWQUserInfoFree\n");
    if (wui->lnick)
        g_free(wui->lnick);
    if (wui->nick)
        g_free(wui->nick);
    if (wui->markname)
        g_free(wui->markname);
	g_slice_free(GWQUserInfo, wui);
}

int sqlite3_prepare_step_finalize(sqlite3* sql, const gchar* cmdStr)
{
	sqlite3_stmt *stmt;
	
	if (SQLITE_OK != sqlite3_prepare(sql, cmdStr, -1, &stmt, NULL)) {
		GWQ_ERR_OUT(ERR_OUT, "prepare <%s> failed: %s\n", cmdStr, sqlite3_errmsg(sql));
	}
	
	if (SQLITE_DONE != sqlite3_step(stmt)) {
		sqlite3_finalize(stmt);
		GWQ_ERR_OUT(ERR_OUT, "step <%s> failed: %s\n", cmdStr, sqlite3_errmsg(sql));
	}
	sqlite3_finalize(stmt);	
	return 0;
ERR_OUT:
	return -1;
}

static void _process_get_qq_num_resp(SoupSession *ss, SoupMessage *msg,  gpointer user_data);
int GWQSessionUpdateQQNumByUin(GWQSession* wqs, gint64 uin)
{
    SoupMessage *msg;
    GString *str;
    
    if (wqs->st != GWQS_ST_IDLE) {
        GWQ_ERR_OUT(ERR_OUT, "\n");
    }

    str = g_string_new("");
    g_string_printf(str, 
            "http://s.web2.qq.com/api/get_friend_uin2?tuin=%"G_GINT64_FORMAT
            "&verifysession=&type=1&code=&vfwebqq=%s&t=%"G_GINT32_FORMAT,
            uin,
            wqs->vfwebqq->str, 
            g_random_int());
    
    GWQ_DBG("GET %s\n", str->str);
    msg = soup_message_new("GET", str->str);
    soup_message_headers_append (msg->request_headers, "Referer", 
            "http://s.web2.qq.com/proxy.html?v=20101025002"); /* this is must */
    soup_session_queue_message (wqs->sps, msg, _process_get_qq_num_resp, wqs);
    g_string_free(str, TRUE);

    return 0;
ERR_OUT:
    return -1;
}


static void _process_get_qq_num_resp(SoupSession *ss, SoupMessage *msg,  gpointer user_data)
{
    GWQSession *wqs;
    const guint8* data;
    gsize size;
    SoupBuffer *sBuf;
    JsonParser *jParser;
    JsonNode *jn;
    JsonObject *jo;
    gint64 uin, qqNum;
    gchar *cmd;
    
    wqs = (GWQSession*)user_data;
    GWQ_DBG("GWQSessionUpdateQQNumByUin responsed, retcode=%d, reason:%s\n", msg->status_code, msg->reason_phrase);
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
    
    if ((qqNum = json_object_get_int_member(jo, "account")) <= 0) {
        GWQ_ERR_OUT(ERR_FREE_J_PARSER, "\n");
    }
    
    if ((uin = json_object_get_int_member(jo, "uin")) <= 0) {
        GWQ_ERR_OUT(ERR_FREE_J_PARSER, "\n");
    }
    
	cmd = g_strdup_printf("update users set qqNum=%"G_GINT64_FORMAT" "
			"where uin=%"G_GINT64_FORMAT,
			qqNum, uin);
	GWQ_DBG("%s\n", cmd);
	if (!cmd) {
		GWQ_ERR_OUT(ERR_FREE_J_PARSER, "\n");
	}
	sqlite3_prepare_step_finalize(wqs->pUserDb, cmd);
	g_free(cmd);
    
    g_object_unref(jParser);
    soup_buffer_free(sBuf);
    soup_session_cancel_message(ss, msg, SOUP_STATUS_CANCELLED);
    if (wqs->updateQQNumByUinCB) {
        wqs->updateQQNumByUinCB(wqs, uin, qqNum);
    }
    return;
ERR_FREE_J_PARSER:
    g_object_unref(jParser);
ERR_FREE_SBUF:
    soup_buffer_free(sBuf);
ERR_OUT:
    soup_session_cancel_message(ss, msg, SOUP_STATUS_CANCELLED);
}

static void _process_get_long_nick_resp(SoupSession *ss, SoupMessage *msg,  gpointer user_data);
int GWQSessionUpdateLongNickByUin(GWQSession* wqs, gint64 uin)
{
    SoupMessage *msg;
    GString *str;
    
    if (wqs->st != GWQS_ST_IDLE) {
        GWQ_ERR_OUT(ERR_OUT, "\n");
    }

    str = g_string_new("");
    g_string_printf(str, 
            "http://s.web2.qq.com//api/get_single_long_nick2?tuin=%"G_GINT64_FORMAT
            "&verifysession=&type=1&code=&vfwebqq=%s&t=%"G_GINT32_FORMAT,
            uin,
            wqs->vfwebqq->str, 
            g_random_int());
    
    GWQ_DBG("GET %s\n", str->str);
    msg = soup_message_new("GET", str->str);
    soup_message_headers_append (msg->request_headers, "Referer", 
            "http://s.web2.qq.com/proxy.html?v=20101025002"); /* this is must */
    soup_session_queue_message (wqs->sps, msg, _process_get_long_nick_resp, wqs);
    g_string_free(str, TRUE);

    return 0;
ERR_OUT:
    return -1;
}


static void _process_get_long_nick_resp(SoupSession *ss, SoupMessage *msg,  gpointer user_data)
{
    GWQSession *wqs;
    const guint8* data;
    gsize size;
    SoupBuffer *sBuf;
    JsonParser *jParser;
    JsonNode *jn;
    JsonObject *jo;
    gint64 uin;
    gchar *cmd;
    JsonArray *ja;
    const gchar *lnick;
    
    wqs = (GWQSession*)user_data;
    GWQ_DBG("GWQSessionUpdateLongNickByUin responsed, retcode=%d, reason:%s\n", msg->status_code, msg->reason_phrase);
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
    
    //if (!(jo = json_object_get_object_member(jo, "result"))) {
    //    GWQ_ERR_OUT(ERR_FREE_J_PARSER, "\n");
    //}
    
    if (!(ja = json_object_get_array_member(jo, "result"))) {
        GWQ_ERR_OUT(ERR_FREE_J_PARSER, "\n");
    }
    
    if (!(jo = json_array_get_object_element(ja, 0))) {
        GWQ_ERR_OUT(ERR_FREE_J_PARSER, "\n");
    }
    
    if ((lnick = json_object_get_string_member(jo, "lnick")) <= 0) {
        GWQ_ERR_OUT(ERR_FREE_J_PARSER, "\n");
    }
    
    if ((uin = json_object_get_int_member(jo, "uin")) <= 0) {
        GWQ_ERR_OUT(ERR_FREE_J_PARSER, "\n");
    }
    
	cmd = g_strdup_printf("update users set longNick=\"%s\" "
			"where uin=%"G_GINT64_FORMAT,
			lnick, uin);
	GWQ_DBG("%s\n", cmd);
	if (!cmd) {
		GWQ_ERR_OUT(ERR_FREE_J_PARSER, "\n");
	}
	sqlite3_prepare_step_finalize(wqs->pUserDb, cmd);
	g_free(cmd);
    if (wqs->updateLongNickByUinCB) {
        wqs->updateLongNickByUinCB(wqs, uin, lnick);
    }
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
}

int GWQSessionUpdateUsersInfo(GWQSession* wqs, GWQSessionCallback callback)
{
	gchar *tmpCStr = NULL;
	SoupMessage *msg;
	gchar *escaped;
	sqlite3_stmt *stmt;
	
	GWQ_DBG("==>GWQSessionUpdateUsersInfo()\n");
	
	if (wqs->st != GWQS_ST_IDLE) {
		GWQ_ERR_OUT(ERR_OUT, "Session busy\n");
	}
	
	/* create users table */
	if (SQLITE_OK != sqlite3_prepare(wqs->pUserDb, CREATE_USERS_TABLE_STMT, -1, &stmt, NULL)) {
		GWQ_ERR_OUT(ERR_OUT, "prepare users table failed: %s\n", sqlite3_errmsg(wqs->pUserDb));
	}
	
	if (SQLITE_DONE != sqlite3_step(stmt)) {
		sqlite3_finalize(stmt);
		GWQ_ERR_OUT(ERR_OUT, "Create users table failed: %s\n", sqlite3_errmsg(wqs->pUserDb));
	}
	
	sqlite3_finalize(stmt);
	
	/* create categories table */
	if (SQLITE_OK != sqlite3_prepare(wqs->pUserDb, CREATE_CATOGERIES_TABLE_STMT, -1, &stmt, NULL)) {
		GWQ_ERR_OUT(ERR_OUT, "prepare categories table failed: %s\n", sqlite3_errmsg(wqs->pUserDb));
	}
	
	if (SQLITE_DONE != sqlite3_step(stmt)) {
		sqlite3_finalize(stmt);
		GWQ_ERR_OUT(ERR_OUT, "Create categories table failed: %s\n", sqlite3_errmsg(wqs->pUserDb));
	}
	
	sqlite3_finalize(stmt);

	wqs->updatUserInfoCallBack = callback;
	tmpCStr = g_strdup_printf("{\"h\":\"hello\", \"vfwebqq\":\"%s\"}",
			wqs->vfwebqq->str);
    escaped = g_uri_escape_string(tmpCStr, NULL, FALSE);
    g_free(tmpCStr);
    tmpCStr = g_strdup_printf("r=%s", escaped);
    g_free(escaped);
	msg = soup_message_new("POST", "http://s.web2.qq.com/api/get_user_friends2");
	soup_message_set_request (msg, "application/x-www-form-urlencoded",
              SOUP_MEMORY_COPY, tmpCStr, strlen(tmpCStr));
    //soup_message_headers_append (msg->request_headers, "Cookie2", "$Version=1");  /* not must */
    soup_message_headers_append (msg->request_headers, "Referer", "http://s.web2.qq.com/proxy.html?v=20110412001&callback=1&id=3"); /* this is must */
    g_free(tmpCStr);
	soup_session_queue_message(wqs->sps, msg, _process_get_user_friends2_resp, wqs);
	return 0;
ERR_OUT:
	return -1;
}

static void _process_get_user_friends2_resp(SoupSession *ss, SoupMessage *msg,  gpointer user_data)
{
    GWQSession *wqs;
    const guint8* data;
    gsize size;
    SoupBuffer *sBuf;
    JsonParser *jParser;
    JsonNode *jn;
    JsonObject *jo;
    JsonArray *ja;
    
    wqs = (GWQSession*)user_data;
    
    GWQ_DBG("get_user_friends2 responsed, retcode=%d, reason:%s\n", msg->status_code, msg->reason_phrase);
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
    
    /* parse categories */
    if (!(jo = json_object_get_object_member(jo, "result"))
			|| !(ja=json_object_get_array_member(jo, "categories"))
			) {
        GWQ_ERR_OUT(ERR_FREE_J_PARSER, "\n");
    }
    
    json_array_foreach_element(ja, _add_category, wqs);

	/* parse friends */
	if (!(ja=json_object_get_array_member(jo, "friends"))) {
        GWQ_ERR_OUT(ERR_FREE_J_PARSER, "\n");
    }
    
    json_array_foreach_element(ja, _add_friend, wqs);

    /* parse info */
    if (!(ja=json_object_get_array_member(jo, "info"))) {
        GWQ_ERR_OUT(ERR_FREE_J_PARSER, "\n");
    }

    json_array_foreach_element(ja, _update_user_info, wqs);
    
    /* parse marknames */
    if (!(ja=json_object_get_array_member(jo, "marknames"))) {
        GWQ_ERR_OUT(ERR_FREE_J_PARSER, "\n");
    }

    json_array_foreach_element(ja, _update_user_markname, wqs);
    
    g_object_unref(jParser);
    soup_buffer_free(sBuf);
    soup_session_cancel_message(ss, msg, SOUP_STATUS_CANCELLED);
    wqs->updatUserInfoCallBack(wqs, wqs->context);
    return;
ERR_FREE_J_PARSER:
    g_object_unref(jParser);
ERR_FREE_SBUF:
    soup_buffer_free(sBuf);
ERR_OUT:
    soup_session_cancel_message(ss, msg, SOUP_STATUS_CANCELLED);
}

static void _add_category(JsonArray *array,
					 guint index,
					 JsonNode *element_node,
					 gpointer user_data)
{
	JsonObject *jo;
	GWQSession *wqs;
	const gchar *name;
	gchar *cmd;
	gint32 idx;
	
	wqs = (GWQSession*)user_data;
	
	if (!(jo = json_node_get_object(element_node))) {
		return;
	}
	if ((idx = json_object_get_int_member(jo, "index")) < 0) {
		return;
	}
	if (!(name = json_object_get_string_member(jo, "name"))) {
		return;
	}

	cmd = g_strdup_printf("insert into categories (idx, name) values (%d, \"%s\")", idx, name);

	if (!cmd) {
		return;
	}
	sqlite3_prepare_step_finalize(wqs->pUserDb, cmd);
	g_free(cmd);
}

static void _add_friend(JsonArray *array,
					 guint index,
					 JsonNode *element_node,
					 gpointer user_data)
{
	JsonObject *jo;
	GWQSession *wqs;
	gchar *cmd;
	gint32 flag, category;
	gint64 uin;
	
	wqs = (GWQSession*)user_data;
	
	/* {"flag":0,"uin":3935308378,"categories":0} */
	if (!(jo = json_node_get_object(element_node))) {
		GWQ_DBG("\n");
		return;
	}
	if ((flag = json_object_get_int_member(jo, "flag")) < 0) {
		GWQ_DBG("\n");
		return;
	}
	if ((uin = json_object_get_int_member(jo, "uin")) < 0) {
		GWQ_DBG("\n");
		return;
	}
	if ((category = json_object_get_int_member(jo, "categories")) < 0) {
		GWQ_DBG("\n");
		return;
	}
	cmd = g_strdup_printf("insert into users (uin, category, flag) values (%"G_GINT64_FORMAT", %d, %d)", uin, category, flag);
	GWQ_DBG("%s\n", cmd);
	if (!cmd) {
		return;
	}
	sqlite3_prepare_step_finalize(wqs->pUserDb, cmd);

	g_free(cmd);
}

static void _update_user_info(JsonArray *array,
					 guint index,
					 JsonNode *element_node,
					 gpointer user_data)
{
	JsonObject *jo;
	GWQSession *wqs;
	gchar *cmd;
	const gchar *nick;
	gint32 face, flag;
	gint64 uin;
	
	wqs = (GWQSession*)user_data;
	
	/* {"uin":2363643512,"nick":"Zakklars","face":414,"flag":13140550} */
	if (!(jo = json_node_get_object(element_node))) {
		return;
	}
	if ((flag = json_object_get_int_member(jo, "flag")) < 0) {
		flag = 0;
	}
	if ((uin = json_object_get_int_member(jo, "uin")) < 0) {
		return;
	}
	if ((face = json_object_get_int_member(jo, "face")) < 0) {
		face = 0;
	}
	if (!(nick = json_object_get_string_member(jo, "nick"))) {
		nick = "";
	}
	
	cmd = g_strdup_printf("update users set nick=\"%s\", flag=%d, face=%d "
			"where uin=%"G_GINT64_FORMAT,
			nick, flag, face, uin);
	GWQ_DBG("%s\n", cmd);
	if (!cmd) {
		return;
	}
	sqlite3_prepare_step_finalize(wqs->pUserDb, cmd);
	g_free(cmd);
}

static void _update_user_markname(JsonArray *array,
					 guint index,
					 JsonNode *element_node,
					 gpointer user_data)
{
	JsonObject *jo;
	GWQSession *wqs;
	gchar *cmd;
	const gchar *markname;
	gint64 uin;
	
	wqs = (GWQSession*)user_data;
	
	/* {"uin":2363643512,"nick":"Zakklars","face":414,"flag":13140550} */
	if (!(jo = json_node_get_object(element_node))) {
		return;
	}

	if ((uin = json_object_get_int_member(jo, "uin")) < 0) {
		return;
	}

	if (!(markname = json_object_get_string_member(jo, "markname"))) {
		markname = "";
	}
	
	cmd = g_strdup_printf("update users set markname=\"%s\""
			"where uin=%"G_GINT64_FORMAT,
			markname, uin);
	GWQ_DBG("%s\n", cmd);
	if (!cmd) {
		return;
	}
	sqlite3_prepare_step_finalize(wqs->pUserDb, cmd);
	g_free(cmd);
}


static void _process_get_friend_info2(SoupSession *ss, SoupMessage *msg,  gpointer user_data);
int GWQSessionUpdateUserDetailedInfoByUin(GWQSession* wqs, gint64 uin)
{
    SoupMessage *msg;
    GString *str;
    
    if (wqs->st != GWQS_ST_IDLE) {
        GWQ_ERR_OUT(ERR_OUT, "\n");
    }

    str = g_string_new("");
    g_string_printf(str, 
            "http://s.web2.qq.com/api/get_friend_info2?tuin=%"G_GINT64_FORMAT"&verifysession=&code=&vfwebqq=%s&t=%"G_GINT32_FORMAT,
            uin,
            wqs->vfwebqq->str, 
            g_random_int());
    
    GWQ_DBG("GET %s\n", str->str);
    msg = soup_message_new("GET", str->str);
    soup_message_headers_append (msg->request_headers, "Referer", 
            "http://s.web2.qq.com/proxy.html?v=20110412001&callback=1&id=3"); /* this is must */
    soup_session_queue_message (wqs->sps, msg, _process_get_friend_info2, wqs);
    g_string_free(str, TRUE);

    return 0;
ERR_OUT:
    return -1;
}

static void _process_get_friend_info2(SoupSession *ss, SoupMessage *msg,  gpointer user_data)
{
    GWQSession *wqs;
    const guint8* data;
    gsize size;
    SoupBuffer *sBuf;
    JsonParser *jParser;
    JsonNode *jn;
    JsonObject *jo;
    GWQUserDetailedInfo dui;
    char *cmd;
    
    wqs = (GWQSession*)user_data;
    GWQ_DBG("GWQSessionUpdateQQNumByUin responsed, retcode=%d, reason:%s\n", msg->status_code, msg->reason_phrase);
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
    
    if ((dui.allow = json_object_get_int_member(jo, "allow")) <= 0) {
        dui.allow = 0;
    }

    if ((dui.constel = json_object_get_int_member(jo, "constel")) <= 0) {
        dui.constel = 0;
    }

    if ((dui.blood = json_object_get_int_member(jo, "blood")) <= 0) {
        dui.blood = 0;
    }
    
    if ((dui.stat = json_object_get_int_member(jo, "stat")) <= 0) {
        dui.stat = 0;
    }
    
    if ((dui.vip_info = json_object_get_int_member(jo, "vip_info")) <= 0) {
        dui.vip_info = 0;
    }
    
    if ((dui.shengxiao = json_object_get_int_member(jo, "shengxiao")) <= 0) {
        dui.shengxiao = 0;
    }

    if ((dui.uin = json_object_get_int_member(jo, "uin")) <= 0) {
        dui.uin = 0;
    }

    if (!(dui.college = json_object_get_string_member(jo, "college"))) {
        dui.college = "";
    }
    
    if (!(dui.occupation = json_object_get_string_member(jo, "occupation"))) {
        dui.occupation = "";
    }
    
    if (!(dui.phone = json_object_get_string_member(jo, "phone"))) {
        dui.phone = "";
    }
    
    if (!(dui.homepage = json_object_get_string_member(jo, "homepage"))) {
        dui.homepage = "";
    }
    
    if (!(dui.country = json_object_get_string_member(jo, "country"))) {
        dui.country = "";
    }
    
    if (!(dui.city = json_object_get_string_member(jo, "city"))) {
        dui.city = "";
    }
    
    if (!(dui.personal = json_object_get_string_member(jo, "personal"))) {
        dui.personal = "";
    }
    
    if (!(dui.email = json_object_get_string_member(jo, "email"))) {
        dui.email = "";
    }
    
    if (!(dui.province = json_object_get_string_member(jo, "province"))) {
        dui.province = "";
    }
    
    if (!(dui.gender = json_object_get_string_member(jo, "gender"))) {
        dui.gender = "";
    }
    
    if (!(dui.mobile = json_object_get_string_member(jo, "mobile"))) {
        dui.mobile = "";
    }
    
    if (!(dui.nick = json_object_get_string_member(jo, "nick"))) {
        dui.nick = "";
    } 
       
    if ((jo = json_object_get_object_member(jo, "birthday"))) {
        if ((dui.birth_year = json_object_get_int_member(jo, "year")) <= 0) {
             dui.birth_year = 0;
        }
        if ((dui.birth_month = json_object_get_int_member(jo, "month")) <= 0) {
             dui.birth_month = 0;
        }
        if ((dui.birth_day = json_object_get_int_member(jo, "day")) <= 0) {
             dui.birth_day = 0;
        }
    }
    
	cmd = g_strdup_printf("update users set "
            "birth_year=%"G_GINT32_FORMAT", "
            "birth_month=%"G_GINT32_FORMAT", "
            "birth_day=%"G_GINT32_FORMAT", "
            "allow=%"G_GINT32_FORMAT", "
            "constel=%"G_GINT32_FORMAT", "
            "blood=%"G_GINT32_FORMAT", "
            "stat=%"G_GINT32_FORMAT", "
            "vip_info=%"G_GINT32_FORMAT","
            "shengxiao=%"G_GINT32_FORMAT", "
            "college=\"%s\""", "
            "occupation=\"%s\""", "
            "phone=\"%s\""", "
            "homepage=\"%s\""", "
            "country=\"%s\""", "
            "city=\"%s\""", "
            "personal=\"%s\""", "
            "email=\"%s\""", "
            "province=\"%s\""", "
            "gender=\"%s\""", "
            "mobile=\"%s\""", "
            "nick=\"%s\""" "
			"where uin=%"G_GINT64_FORMAT,
			dui.birth_year,
            dui.birth_month,
            dui.birth_day,
            dui.allow,
            dui.constel,
            dui.blood,
            dui.stat,
            dui.vip_info,
            dui.shengxiao,
            dui.college,
            dui.occupation,
            dui.phone,
            dui.homepage,
            dui.country,
            dui.city,
            dui.personal,
            dui.email,
            dui.province,
            dui.gender,
            dui.mobile,
            dui.nick,
            dui.uin);
	GWQ_DBG("%s\n", cmd);
	if (!cmd) {
		GWQ_ERR_OUT(ERR_FREE_J_PARSER, "\n");
	}
	sqlite3_prepare_step_finalize(wqs->pUserDb, cmd);
	g_free(cmd);
    if (wqs->updateUserDetailedInfoCB) {
        wqs->updateUserDetailedInfoCB(wqs, &dui);
    }
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
}

void GWQSessionUsersForeach(GWQSession* wqs, void(foreachFunc)(GWQSession* wqs, GWQUserInfo* info))
{
	sqlite3_stmt *stmt;
	gchar *cmdStr = "select * from users"; 
	const gchar *tmpCCStr;
    GWQUserInfo *ret;
    
	if (SQLITE_OK != sqlite3_prepare(wqs->pUserDb, cmdStr, -1, &stmt, NULL)) {
		GWQ_ERR_OUT(ERR_OUT, "prepare <%s> failed: %s\n", cmdStr, sqlite3_errmsg(wqs->pUserDb));
	}
	
	while (SQLITE_ROW == sqlite3_step(stmt)) {
        ret = g_slice_new0(GWQUserInfo);
        
        ret->uin =  sqlite3_column_int64(stmt, USERS_TBL_UIN_COL);
        
        ret->qqNum = sqlite3_column_int64(stmt, USERS_TBL_QQ_NUM_COL);
        
        tmpCCStr = (const gchar*)sqlite3_column_text(stmt, USERS_TBL_NICK_COL);
        if (tmpCCStr) {
            ret->nick = g_strdup(tmpCCStr);
        }
        
        tmpCCStr = (const gchar*)sqlite3_column_text(stmt, USERS_TBL_MARKNAME_COL);
        if (tmpCCStr) {
            ret->markname = g_strdup(tmpCCStr);
        }
        
        tmpCCStr = (const gchar*)sqlite3_column_text(stmt, USERS_TBL_LONG_NICK_COL);
        if (tmpCCStr) {
            ret->lnick = g_strdup(tmpCCStr);
        }
        
        ret->face = sqlite3_column_int(stmt, USERS_TBL_FACE_COL);
        ret->category = sqlite3_column_int(stmt, USERS_TBL_CATEGORY_COL);
        ret->flag = sqlite3_column_int(stmt, USERS_TBL_FLAG_COL);
        ret->online = sqlite3_column_int(stmt, USERS_TBL_ONLINE_COL);
        foreachFunc(wqs, ret);
        GWQUserInfoFree(ret);
    }
	sqlite3_finalize(stmt);
    return;
ERR_OUT:
    return;
}

GWQUserInfo* GWQSessionGetUserInfo(GWQSession* wqs, const gint64 qqNum, gint64 qqUin)
{
	GWQUserInfo *ret;
	sqlite3_stmt *stmt;
	gchar *cmdStr;
	const gchar *tmpCCStr;
	
	if (qqNum != -1) {
		cmdStr = g_strdup_printf("select * from users where qqNum=%"G_GINT64_FORMAT, qqNum);
	} else if (qqUin != -1) {
		cmdStr = g_strdup_printf("select * from users where uin=%"G_GINT64_FORMAT, qqUin);
	} else {
		GWQ_ERR_OUT(ERR_OUT, "\n");
	}
	
	if (SQLITE_OK != sqlite3_prepare(wqs->pUserDb, cmdStr, -1, &stmt, NULL)) {
		GWQ_ERR_OUT(ERR_FREE_CMD_STR, "prepare <%s> failed: %s\n", cmdStr, sqlite3_errmsg(wqs->pUserDb));
	}
	
	if (SQLITE_ROW != sqlite3_step(stmt)) {
		GWQ_ERR_OUT(ERR_FINALIZE_STMT, "step <%s> failed: %s\n", cmdStr, sqlite3_errmsg(wqs->pUserDb));
	}
	ret = g_slice_new0(GWQUserInfo);
	ret->uin =  sqlite3_column_int64(stmt, USERS_TBL_UIN_COL);
	
	ret->qqNum = sqlite3_column_int64(stmt, USERS_TBL_QQ_NUM_COL);
	
	tmpCCStr = (const gchar*)sqlite3_column_text(stmt, USERS_TBL_NICK_COL);
	if (tmpCCStr) {
		ret->nick = g_strdup(tmpCCStr);
	}
	
	tmpCCStr = (const gchar*)sqlite3_column_text(stmt, USERS_TBL_MARKNAME_COL);
	if (tmpCCStr) {
		ret->markname = g_strdup(tmpCCStr);
	}
	
    tmpCCStr = (const gchar*)sqlite3_column_text(stmt, USERS_TBL_LONG_NICK_COL);
	if (tmpCCStr) {
		ret->lnick = g_strdup(tmpCCStr);
	}
    
	ret->face = sqlite3_column_int(stmt, USERS_TBL_FACE_COL);
	ret->category = sqlite3_column_int(stmt, USERS_TBL_CATEGORY_COL);
	ret->flag = sqlite3_column_int(stmt, USERS_TBL_FLAG_COL);
	ret->online = sqlite3_column_int(stmt, USERS_TBL_ONLINE_COL);
	
	sqlite3_finalize(stmt);
    g_free(cmdStr);
	return ret;

ERR_FREE_RET:
	g_free(ret->markname);
	g_free(ret->nick);
	g_free(ret);
ERR_FINALIZE_STMT:
	sqlite3_finalize(stmt);
ERR_FREE_CMD_STR:
	g_free(cmdStr);
ERR_OUT:
	return NULL;
}

int GWQSessionGetUinByNum(GWQSession* wqs, gint64 num, gint64* uin)
{
	sqlite3_stmt *stmt;
	gchar *cmdStr;
	
    cmdStr = g_strdup_printf("select uin from users where qqNum=%"G_GINT64_FORMAT, num);

	if (SQLITE_OK != sqlite3_prepare(wqs->pUserDb, cmdStr, -1, &stmt, NULL)) {
		GWQ_ERR_OUT(ERR_FREE_CMD_STR, "prepare <%s> failed: %s\n", cmdStr, sqlite3_errmsg(wqs->pUserDb));
	}
	
	if (SQLITE_ROW != sqlite3_step(stmt)) {
		GWQ_ERR_OUT(ERR_FINALIZE_STMT, "step <%s> failed: %s\n", cmdStr, sqlite3_errmsg(wqs->pUserDb));
	}
	
	*uin =  sqlite3_column_int64(stmt, USERS_TBL_UIN_COL);

	sqlite3_finalize(stmt);
    g_free(cmdStr);
	return 0;

ERR_FINALIZE_STMT:
	sqlite3_finalize(stmt);
ERR_FREE_CMD_STR:
	g_free(cmdStr);
ERR_OUT:
	return -1;
}

int GWQSessionGetNumByUin(GWQSession* wqs, gint64 uin, gint64* num)
{
	sqlite3_stmt *stmt;
	gchar *cmdStr;
	
    cmdStr = g_strdup_printf("select qqNum from users where uin=%"G_GINT64_FORMAT, uin);

	if (SQLITE_OK != sqlite3_prepare(wqs->pUserDb, cmdStr, -1, &stmt, NULL)) {
		GWQ_ERR_OUT(ERR_FREE_CMD_STR, "prepare <%s> failed: %s\n", cmdStr, sqlite3_errmsg(wqs->pUserDb));
	}
	
	if (SQLITE_ROW != sqlite3_step(stmt)) {
		GWQ_ERR_OUT(ERR_FINALIZE_STMT, "step <%s> failed: %s\n", cmdStr, sqlite3_errmsg(wqs->pUserDb));
	}
	
	*num =  sqlite3_column_int64(stmt, USERS_TBL_UIN_COL);

	sqlite3_finalize(stmt);
    g_free(cmdStr);
	return 0;

ERR_FINALIZE_STMT:
	sqlite3_finalize(stmt);
ERR_FREE_CMD_STR:
	g_free(cmdStr);
ERR_OUT:
	return -1;
}

void GWQSessionCategoriesForeach(GWQSession* wqs, void(foreachFunc)(GWQSession* wqs, gint32 idx, const gchar* name))
{
	sqlite3_stmt *stmt;
	gchar *cmdStr = "select * from categories"; 
    gint32 idx;
	const gchar *tmpCCStr;
    
	if (SQLITE_OK != sqlite3_prepare(wqs->pUserDb, cmdStr, -1, &stmt, NULL)) {
		GWQ_ERR_OUT(ERR_OUT, "prepare <%s> failed: %s\n", cmdStr, sqlite3_errmsg(wqs->pUserDb));
	}
	
	while (SQLITE_ROW == sqlite3_step(stmt)) {
        idx =  sqlite3_column_int(stmt, CATEGS_TBL_IDX_COL);
        
        tmpCCStr = (const gchar*)sqlite3_column_text(stmt, CATEGS_TBL_NAME_COL);
        
        foreachFunc(wqs, idx, tmpCCStr);
    }
	sqlite3_finalize(stmt);
    return;
ERR_OUT:
    return;
}

static void _process_get_online_buddies2(SoupSession *ss, SoupMessage *msg,  gpointer user_data);
int GWQSessionUpdateOnlineBuddies(GWQSession* wqs)
{
    SoupMessage *msg;
    GString *str;
    
    if (wqs->st != GWQS_ST_IDLE) {
        GWQ_ERR_OUT(ERR_OUT, "\n");
    }

    str = g_string_new("");
    g_string_printf(str, 
            "http://d.web2.qq.com/channel/get_online_buddies2?clientid=%s&psessionid=%s&t=%"G_GINT32_FORMAT,
            wqs->clientId->str,
            wqs->psessionid->str, 
            g_random_int());
    
    GWQ_DBG("GET %s\n", str->str);
    msg = soup_message_new("GET", str->str);
    soup_message_headers_append (msg->request_headers, "Referer", 
            "http://d.web2.qq.com/proxy.html?v=20110331002&callback=1&id=2"); /* this is must */
    soup_session_queue_message (wqs->sps, msg, _process_get_online_buddies2, wqs);
    g_string_free(str, TRUE);

    return 0;
ERR_OUT:
    return -1;
}

static void _process_get_online_buddies2(SoupSession *ss, SoupMessage *msg,  gpointer user_data)
{
    GWQSession *wqs;
    const guint8* data;
    gsize size;
    SoupBuffer *sBuf;
    JsonParser *jParser;
    JsonNode *jn;
    JsonObject *jo;
    gint64 uin;
    gchar *cmd;
    JsonArray *ja;
    const gchar *status;
    gint32 client_type;
    gint32 len, i;
    
    wqs = (GWQSession*)user_data;
    GWQ_DBG("GWQSessionUpdateOnlineBuddies responsed, retcode=%d, reason:%s\n", msg->status_code, msg->reason_phrase);
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

    if (!(ja = json_object_get_array_member(jo, "result"))) {
        GWQ_ERR_OUT(ERR_FREE_J_PARSER, "\n");
    }
    
    len = json_array_get_length(ja);
    
    for (i=0; i<len; i++) {
        if (!(jo = json_array_get_object_element(ja, i))) {
            GWQ_ERR_OUT(ERR_FREE_J_PARSER, "\n");
        }
        
        if ((status = json_object_get_string_member(jo, "status")) <= 0) {
            status = GWQ_CHAT_ST_OFFLINE;
        }
        
        if ((uin = json_object_get_int_member(jo, "uin")) <= 0) {
            GWQ_ERR_OUT(ERR_FREE_J_PARSER, "\n");
        }
        
        if ((client_type = json_object_get_int_member(jo, "client_type")) <= 0) {
            client_type = 0;
        }
        
        cmd = g_strdup_printf("update users set online=%"G_GINT32_FORMAT", stat=\"%s\" "
                "where uin=%"G_GINT64_FORMAT,
                1, status, uin);
        GWQ_DBG("%s\n", cmd);
        if (!cmd) {
            GWQ_ERR_OUT(ERR_FREE_J_PARSER, "\n");
        }
        sqlite3_prepare_step_finalize(wqs->pUserDb, cmd);
        g_free(cmd);
        if (wqs->updateOnlinebuddiesCB) {
            wqs->updateOnlinebuddiesCB(wqs, uin, status, client_type);
        }
    }
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
}
