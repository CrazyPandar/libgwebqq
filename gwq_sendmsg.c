#include "gwq_sendmsg.h"
#include "gwq_user.h"
#define DEFAULT_FONT_NAME   "宋体"
#define DEFAULT_FONT_SIZE   "20"
#define DEFAULT_FONT_COLOR  "000000"
#define DEFAULT_FONT_STYLE  {0,0,0}

//
// QQMsgFont
//
QQMsgFont* qq_msgfont_new(const gchar *name, gint size, const gchar *color
                            , gint sa, gint sb, gint sc)
{
    QQMsgFont *font = g_slice_new0(QQMsgFont);
    if(font == NULL){
        g_warning("OOM...(%s, %d)", __FILE__, __LINE__);
        return NULL;
    }

    if(name == NULL){
        font -> name = g_string_new("Arial");
    }else{
        font -> name = g_string_new(name);
    }

    if(color == NULL){
        font -> color = g_string_new("000000");
    }else{
        font -> color = g_string_new(color);
    }

    font -> size = size;
    font -> style.a = sa;
    font -> style.b = sb;
    font -> style.c = sc;

    return font;
}

void qq_msgfont_free(QQMsgFont *font)
{
    if(font == NULL){
        return;
    }

    g_string_free(font -> name, TRUE);
    g_string_free(font -> color, TRUE);

    g_slice_free(QQMsgFont, font);
}

gboolean qq_msgfont_equal(QQMsgFont *a, QQMsgFont *b)
{
    if(a == b){
        return TRUE;
    }
    if(a == NULL || b == NULL){
        return FALSE;
    }

    if(!g_string_equal(a -> name, b -> name)){
        return FALSE;
    }
    if(!g_string_equal(a -> color, b -> color)){
        return FALSE;
    }
    if(a -> size != b -> size){
        return FALSE;
    }
    if(a -> style.a != b -> style.a){
        return FALSE;
    }
    if(a -> style.b != b -> style.b){
        return FALSE;
    }
    if(a -> style.c != b -> style.c){
        return FALSE;
    }
    return TRUE;
}

//
// QQMsgContent
//
QQMsgContent *qq_msgcontent_new(gint type, ...)
{
    QQMsgContent *cnt = g_slice_new0(QQMsgContent);
    if(cnt == NULL){
        g_warning("OOM...(%s, %d)", __FILE__, __LINE__);
        return NULL;
    }
    const gchar *name, *color;
    gint size, sa, sb, sc;
    cnt -> type = type;
    va_list ap;
    va_start(ap, type);
    switch(type)
    {
    case QQ_MSG_CONTENT_FACE_T:             //face
        cnt -> value.face = va_arg(ap, gint);
        break;
    case QQ_MSG_CONTENT_STRING_T:           //string
        cnt -> value.str = g_string_new(va_arg(ap, const gchar *));
        break;
    case QQ_MSG_CONTENT_FONT_T:             //font
        name = va_arg(ap, const gchar *);
        size = va_arg(ap, gint);
        color = va_arg(ap, const gchar *);
        sa= va_arg(ap, gint);
        sb= va_arg(ap, gint);
        sc = va_arg(ap, gint);
        cnt -> value.font = qq_msgfont_new(name, size, color, sa, sb, sc);
        break;
    default:
        g_warning("Unknown QQMsgContent type: %d! (%s, %d)"
                            , type, __FILE__, __LINE__);
        va_end(ap);
        g_slice_free(QQMsgContent, cnt);
        return NULL;
    }
    va_end(ap);
    return cnt;
}

void qq_msgcontent_free(QQMsgContent *cnt)
{
    if(cnt == NULL){
        return;
    }

    switch(cnt -> type)
    {
    case QQ_MSG_CONTENT_FACE_T:         //face
        // nothing to do...
        break;
    case QQ_MSG_CONTENT_STRING_T:       //string
        g_string_free(cnt -> value.str, TRUE);
        break;
    case QQ_MSG_CONTENT_FONT_T:         //font
        qq_msgfont_free(cnt -> value.font);
        break;
    default:
        g_warning("Unknown QQMsgContent type: %d! (%s, %d)"
                            , cnt -> type, __FILE__, __LINE__);
        break;
    }

    g_slice_free(QQMsgContent, cnt);
}

GString* qq_msgcontent_tostring(QQMsgContent *cnt)
{
    if(cnt == NULL){
        return g_string_new("");
    }

    GString * str = g_string_new("");
    gchar buf[500];
    gint i;
    switch(cnt -> type)
    {
    case QQ_MSG_CONTENT_FACE_T:             //face. [\"face\",110]
        g_snprintf(buf, 500, "[\\\"face\\\", %d]", cnt -> value.face);
        g_string_append(str, buf);
        break;
    case QQ_MSG_CONTENT_STRING_T:           //string, \"test\"
        g_string_append(str, "\\\"");
        for(i = 0; i < cnt -> value.str -> len; ++i){
            switch(cnt -> value.str -> str[i])
            {
            case '\\':
                g_string_append(str, "\\\\\\\\");
                break;
            case '"':\
                g_string_append(str, "\\\\\\\"");
                break;
            case '\n':
                g_string_append(str, "\\\\n");
                break;
            case '\r':
                g_string_append(str, "\\\\r");
                break;
            case '\t':
                g_string_append(str, "\\\\t");
                break;
            default:
                g_string_append_c(str, cnt -> value.str -> str[i]);
                break;
            }
        }
        g_string_append(str, "\\\"");
        break;
    case QQ_MSG_CONTENT_FONT_T:
        g_snprintf(buf, 500, "[\\\"font\\\", {\\\"name\\\": \\\"%s\\\", "
                        "\\\"size\\\": %d, "
                        "\\\"style\\\": [%d,%d,%d], "
                        "\\\"color\\\": \\\"%s\\\"}]"
                        , cnt -> value.font -> name -> str
                        , cnt -> value.font -> size
                        , cnt -> value.font -> style.a
                        , cnt -> value.font -> style.b
                        , cnt -> value.font -> style.c
                        , cnt -> value.font -> color -> str);
        g_string_append(str, buf);
        break;
    default:
        g_snprintf(buf, 500, "%s", "");
        g_string_append(str, buf);
        break;
    }
    return str;
}

QQSendMsg* qq_sendmsg_new(QQMsgType type, const gint64 to_uin)
{
    QQSendMsg *msg = g_slice_new0(QQSendMsg);

    if(msg == NULL){
        g_warning("OOM...(%s, %d)", __FILE__, __LINE__);
        return NULL;
    }
    
    msg -> contents = g_ptr_array_new();
    if(msg -> contents == NULL){
        g_warning("OOM...(%s, %d)", __FILE__, __LINE__);
        g_slice_free(QQSendMsg, msg);
        return NULL;
    }
    
    msg->to_uin = to_uin;
    
    msg->type = type;
    
    return msg;
}

void qq_sendmsg_free(QQSendMsg *msg)
{
    if(msg == NULL){
        return;
    }

    guint i;
    for(i = 0; i < msg -> contents -> len; ++i){
        qq_msgcontent_free(
                (QQMsgContent*)g_ptr_array_index(msg -> contents, i));
    }
    g_slice_free(QQSendMsg, msg);
}

void qq_sendmsg_add_content(QQSendMsg *msg, QQMsgContent *content)
{
    if(msg == NULL || msg -> contents == NULL){
        return;
    }

    if(content == NULL){
        return;
    }

    g_ptr_array_add(msg -> contents, content);
}



//
//"content":
//        "[\"test.\"
//          ,[\"font\",{\"name\":\"\\\"微软雅黑\\\"\",\"size\":\"11\"
//                    ,\"style\":[0,0,0],\"color\":\"000000\"}
//           ]
//          ,[\"face\", 20]
//        ]" 
//
GString * qq_sendmsg_contents_tostring(QQSendMsg *msg)
{
    if(msg == NULL || msg -> contents == NULL){
        return g_string_new("");
    }

    GString *str = g_string_new("\"content\":\"[");
    GString *tmp;
    guint i;
    for(i = 0; i < msg -> contents -> len; ++i){
        tmp = qq_msgcontent_tostring(
                    (QQMsgContent*)g_ptr_array_index(msg -> contents, i));
        g_string_append(str, tmp -> str);
        g_string_free(tmp, TRUE);
        if(i != msg -> contents -> len - 1){
            g_string_append(str, ",");
        }
    }
        
    g_string_append(str, "]\"");
    g_debug("contents_tostring: %s (%s, %d)", str -> str, __FILE__, __LINE__);
    return str;
}

/*
r={
	"to":1242066617,
	"face":81,
	"content":
		"[\"测试。test.\"
			,[\"font\"
				,{\"name\":\"\\\"微软雅黑\\\"\"
					,\"size\":\"11\"
					,\"style\":[0,0,0]
					,\"color\":\"000000\"
				}
			]
		]",
	"msg_id":57890008,
	"clientid":"30789207",
	"psessionid":"8368046764001e636f6e6e7365727..."
}
 */
static void _process_sendBuddyMsg_resp(SoupSession *ss, SoupMessage *msg,  gpointer user_data);
int GWQSessionSendBuddyMsg(GWQSession* wqs, gint64 toUin, QQSendMsg* qsm, MessageSentFunc msgSent)
{
    gchar *req;
    GWQUserInfo *wui;
    GString *cnts;
    static int msg_id;
    gchar *tmpCStr, *escaped;
    
    if (wqs->st != GWQS_ST_IDLE ||
        wqs->sendMsgSt != SEND_MSG_IDLE) {
        GWQ_ERR_OUT(ERR_OUT, "\n");
    }
    
    if (!(wui = GWQSessionGetUserInfo(wqs, NULL, toUin))) {
        GWQ_ERR_OUT(ERR_OUT, "user no found\n");
    }
    
    if (!(cnts = qq_sendmsg_contents_tostring(qsm))) {
        GWQ_ERR_OUT(ERR_FREE_WUI, "gen msg contents failed\n");
    }
    
    wqs->messageSent =msgSent;
    wqs->msgToSent = qsm;

    wqs->sendMsg = soup_message_new("POST", "http://d.web2.qq.com/channel/send_buddy_msg2");
    
    req = g_strdup_printf("{"
            "\"to\":%"G_GINT64_FORMAT","
            "\"face\":%d,"
            "%s,"
            "\"msg_id\":%d,"
            "\"clientid\":\"%s\","
            "\"psessionid\":\"%s\""
        "}&clientid=%s&psessionid=%s", 
        
        toUin, wui->face, cnts->str, 
        msg_id++, 
        wqs->clientId->str,
        wqs->psessionid->str,
        wqs->clientId->str,
        wqs->psessionid->str);
    
    escaped = g_uri_escape_string(req, NULL, FALSE);
    g_free(req);
    
    tmpCStr = g_strdup_printf("r=%s", escaped);
    g_free(escaped);
    
    soup_message_set_request (wqs->sendMsg, "application/x-www-form-urlencoded",
              SOUP_MEMORY_COPY, tmpCStr, strlen(tmpCStr));
    soup_message_headers_append (wqs->sendMsg->request_headers, "Referer", 
            " http://d.web2.qq.com/proxy.html?v=20110331002&callback=1&id=2"); /* this is must */
    g_free(tmpCStr);
	soup_session_queue_message(wqs->sps, wqs->sendMsg, _process_sendBuddyMsg_resp, wqs);
    //GWQUserInfoFree(wui);
    wqs->sendMsgSt = SEND_MSG_ING;
    return 0;
ERR_FREE_CNTS:
    g_string_free(cnts, TRUE);
ERR_FREE_WUI:
    GWQUserInfoFree(wui);
ERR_OUT:
    return -1;
}


static void _process_sendBuddyMsg_resp(SoupSession *ss, SoupMessage *msg,  gpointer user_data)
{
    GWQSession *wqs;
    const guint8* data;
    const gchar* tmpCStr;
    gsize size;
    SoupBuffer *sBuf;
    JsonParser *jParser;
    JsonNode *jn;
    JsonObject *jo;
    gint32 tmpInt;
    JsonArray *resJa;
    
    wqs = (GWQSession*)user_data;
    
    wqs->sendMsgSt = SEND_MSG_IDLE;
    GWQ_DBG("poll responsed, retcode=%d, reason:%s\n", msg->status_code, msg->reason_phrase);
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
    
    resJa = NULL;
    if (!json_parser_load_from_data(jParser, (const gchar*)data, size, NULL)) {
        GWQ_ERR("\n");
    } else if (!(jn = json_parser_get_root(jParser))
            || !(jo = json_node_get_object(jn))) {
        GWQ_ERR("\n");
    } else if ((tmpInt = json_object_get_int_member(jo, "retcode"))) {
        GWQ_ERR("sendMsg retcode=%d\n", tmpInt);
    }
    if (!(tmpCStr = json_object_get_string_member(jo, "result"))) {
        GWQ_ERR("sendMsg returned result: %s\n", tmpCStr);
    } 
    
    wqs->messageSent(wqs, wqs->msgToSent, tmpInt);
    
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
