#include "g_webqq.h"
#include "gwq_recvmsg.h"

QQRecvMsg* qq_recvmsg_new(QQMsgType type)
{
    QQRecvMsg *msg = g_slice_new0(QQRecvMsg);
    if(msg == NULL){
        g_warning("OOM...(%s, %d)", __FILE__, __LINE__); 
        return NULL;
    }

    msg -> msg_type = type;

#define NEW_STR(x, y) msg -> x = g_string_new(y)
    NEW_STR(raw_content, "");
    NEW_STR(status, "");
#undef NEW_STR

    msg -> contents = g_ptr_array_new();
    return msg;
}

void qq_recvmsg_add_content(QQRecvMsg *msg, QQMsgContent *content)
{
    if(msg == NULL || msg -> contents == NULL){
        return;
    }

    if(content == NULL){
        return;
    }

    g_ptr_array_add(msg -> contents, content);
}

void qq_recvmsg_set(QQRecvMsg *msg, const gchar *name, void *value)
{
    if(msg == NULL || name == NULL){
        return;
    }
#define SET_INT(xx) msg->xx = *((gint64*)value);
    if(g_strcmp0(name, "msg_id") == 0){
        SET_INT(msg_id);
    }else if(g_strcmp0(name, "msg_id2") == 0){
        SET_INT(msg_id2);
    }else if(g_strcmp0(name, "from_uin") == 0){
        SET_INT(from_uin);
    }else if(g_strcmp0(name, "to_uin") == 0){
        SET_INT(to_uin);
    }else if(g_strcmp0(name, "reply_ip") == 0){
        SET_INT(reply_ip);
    }else if(g_strcmp0(name, "group_code") == 0){
        SET_INT(group_code);
    }else if(g_strcmp0(name, "send_uin") == 0){
        SET_INT(send_uin);
    }else if(g_strcmp0(name, "time") == 0){
        SET_INT(time);
    }else if(g_strcmp0(name, "raw_content") == 0){
        g_string_truncate(msg->raw_content, 0);
        g_string_append(msg->raw_content, *((gchar**)value));
    }else if(g_strcmp0(name, "uin") == 0){
        SET_INT(uin);
    }else if(g_strcmp0(name, "status") == 0){
        g_string_assign(msg->status, *((gchar**)value));
    }else if(g_strcmp0(name, "client_type") == 0){
        SET_INT(client_type);
    }else{
        GWQ_ERR("Unknown QQRecvMsg memeber: %s (%s, %d)", name
                                    , __FILE__, __LINE__);
    }
#undef SET_INT
}

void qq_recvmsg_free(QQRecvMsg *msg)
{
    if(msg == NULL){
        return;
    }

#define FREE_STR(x) g_string_free(msg -> x, TRUE)
    FREE_STR(status);
    FREE_STR(raw_content);
#undef FREE_STR

    guint i;
    for(i = 0; i < msg -> contents -> len; ++i){
        qq_msgcontent_free(
                (QQMsgContent*)g_ptr_array_index(msg -> contents, i));
    }
    g_slice_free(QQRecvMsg, msg);
}

static void parase_content(QQRecvMsg *msg, JsonNode *jn)
{
    GString *utf8 = g_string_new("");
    GString *tmpstring = g_string_new("");
    gint i;
    JsonNodeType jnt;
    QQMsgContent *ct = NULL;
    
    jnt = json_node_get_node_type(jn);
        if(jnt == JSON_NODE_VALUE){
            //String
            g_string_truncate(utf8, 0);
            g_string_truncate(tmpstring, 0);
            
            GWQAsciiToUtf8(json_node_get_string(jn), utf8);

            for(i = 0; i < utf8 -> len; ++i){
                if(utf8 -> str[i] == '\\' && i + 1 < utf8 -> len){
                    switch(utf8 -> str[i + 1])
                    {
                    case '\\':
                        g_string_append_c(tmpstring, '\\');
                        break;
                    case 'n':
                        g_string_append_c(tmpstring, '\n');
                        break;
                    case 'r':
                        g_string_append_c(tmpstring, '\r');
                        break;
                    case 't':
                        g_string_append_c(tmpstring, '\t');
                        break;
                    case '"':
                        g_string_append_c(tmpstring, '"');
                        break;
                    default:
                        break;
                    }
                    ++i;
                }else{
                    g_string_append_c(tmpstring, utf8 -> str[i]);
                }
            }
            ct = qq_msgcontent_new(QQ_MSG_CONTENT_STRING_T, tmpstring -> str); 
            qq_recvmsg_add_content(msg, ct);
            g_debug("Msg Content: string %s(%s, %d)", utf8 -> str
                            , __FILE__, __LINE__);
        }else if(jnt == JSON_NODE_ARRAY){
            JsonArray *ja, *styleAry;
            JsonObject *fontObj;
            const char *cntType;
            
            ja = json_node_get_array(jn);
            cntType = json_array_get_string_element(ja, 0);
            if(cntType && g_strcmp0(cntType, "font") == 0
                    && (fontObj = json_array_get_object_element(ja, 1))){
                const gchar *name, *color;
                gint size, sa, sb, sc;
                
                //font name
                name = json_object_get_string_member(fontObj, "name");
                if(name == NULL){
                    g_warning("No font name found!(%s, %d)", __FILE__, __LINE__);
                    name = "Arial";
                }
                //g_string_truncate(utf8, 0);
                //ucs4toutf8(utf8, name);
                //name = utf8 -> str;
                //font color
                color = json_object_get_string_member(fontObj, "color");
                if(color == NULL){
                    g_warning("No font color found!(%s, %d)", __FILE__, __LINE__);
                    color = "000000";
                }
                //font size
                size = json_object_get_int_member(fontObj, "size");
                if(size < 0){
                    g_warning("No font size found!(%s, %d)", __FILE__, __LINE__);
                    size = 12;
                }
                //font style
                styleAry = json_object_get_array_member(fontObj, "style");
                if(styleAry == NULL){
                    g_warning("No font style found!(%s, %d)", __FILE__, __LINE__);
                    sa = 0;
                    sb = 0;
                    sc = 0;
                }else{
                    sa = json_array_get_int_element(styleAry, 0);
                    sb = json_array_get_int_element(styleAry, 1);
                    sc = json_array_get_int_element(styleAry, 2);
                }
                ct = qq_msgcontent_new(QQ_MSG_CONTENT_FONT_T, name, size
                                                    , color, sa, sb, sc);
                qq_recvmsg_add_content(msg, ct);
                g_debug("Msg Content: font %s %d %s %d%d%d(%s, %d)", name
                        , size, color, sa, sb, sc, __FILE__, __LINE__);
            }else if(cntType && g_strcmp0(cntType, "face") == 0){
                gint facenum = json_array_get_int_element(ja, 1);
                ct = qq_msgcontent_new(QQ_MSG_CONTENT_FACE_T, facenum);
                qq_recvmsg_add_content(msg, ct);
                g_debug("Msg Content: face %d(%s, %d)", facenum
                                        , __FILE__, __LINE__);
            }
        } else {
            g_warning("Unknown content!(%s, %d)", __FILE__, __LINE__);
        }

    g_string_free(utf8, TRUE);
    g_string_free(tmpstring, TRUE);
}

static void _content_array_foreach(JsonArray *array,
                                         guint index_,
                                         JsonNode *element_node,
                                         gpointer user_data)
{
    QQRecvMsg* msg = (QQRecvMsg*)user_data;
    parase_content(msg, element_node);
}

QQRecvMsg* gwq_create_buddy_message(JsonObject *json)
{
    QQRecvMsg *msg = qq_recvmsg_new(MSG_BUDDY_T);
    gint64 tmpInt64;
    JsonArray *ctentAry;
    
#define SET_INT64(x)    tmpInt64 = json_object_get_int_member(json, x); qq_recvmsg_set(msg, x, &tmpInt64);
    SET_INT64("msg_id");
    SET_INT64("from_uin");
    SET_INT64("to_uin");
    SET_INT64("msg_id2");
    SET_INT64("reply_ip");
    SET_INT64("time");
//    SET_VALUE("raw_content");
#undef SET_VALUE
    msg -> type = json_object_get_int_member(json, "msg_type");
    ctentAry = json_object_get_array_member(json, "content");
    json_array_foreach_element(ctentAry, _content_array_foreach, (gpointer)msg);
    return msg;
}

QQRecvMsg* gwq_create_group_message(JsonObject *json)
{

    QQRecvMsg *msg = qq_recvmsg_new(MSG_GROUP_T);
    gint64 tmpInt64;
    JsonArray *ctentAry;
    
#define SET_INT64(x)    tmpInt64 = json_object_get_int_member(json, x); qq_recvmsg_set(msg, x, &tmpInt64);
    SET_INT64("msg_id");
    SET_INT64("from_uin");
    SET_INT64("to_uin");
    SET_INT64("msg_id2");
    SET_INT64("reply_ip");
    SET_INT64("time");
    SET_INT64("group_code");
    SET_INT64("send_uin");
//    SET_VALUE("raw_content");
#undef SET_INT64


    //GString *utf8 = g_string_new("");
    //ucs4toutf8(utf8, msg -> raw_content -> str);
    qq_recvmsg_set(msg, "raw_content", msg -> raw_content -> str);
    //g_string_free(utf8, TRUE);

    msg -> type = json_object_get_int_member(json, "msg_type");
    
    msg -> info_seq = json_object_get_int_member(json, "info_seq");
    ctentAry = json_object_get_array_member(json, "content");
    json_array_foreach_element(ctentAry, _content_array_foreach, (gpointer)msg);
    return msg;
}

QQRecvMsg* gwq_create_status_change_message(JsonObject *json)
{

    QQRecvMsg *msg = qq_recvmsg_new(MSG_STATUS_CHANGED_T);
    gint64 tmpInt64;
    
#define SET_INT64(x)    tmpInt64 = json_object_get_int_member(json, x); qq_recvmsg_set(msg, x, &tmpInt64);
    SET_INT64("uin");
    SET_INT64("client_type");
#undef SET_INT64
    g_string_assign(msg->status, json_object_get_string_member(json, "status"));
    GWQ_DBG("Status change: %s (%s, %d)\n", msg -> status -> str
                        , __FILE__, __LINE__);
    return msg;
}

QQRecvMsg* gwq_create_kick_message(JsonObject *json)
{

    QQRecvMsg *msg = qq_recvmsg_new(MSG_KICK_T);
    const gchar *rn = json_object_get_string_member(json, "reason");
    QQMsgContent *cent = qq_msgcontent_new(QQ_MSG_CONTENT_STRING_T
                            , rn);
    qq_recvmsg_add_content(msg, cent);
    return msg;
}

