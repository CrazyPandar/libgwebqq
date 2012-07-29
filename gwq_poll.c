#include <glib/gprintf.h>
#include <json-glib/json-glib.h>
#include "g_webqq.h"

static void _process_poll_resp(SoupSession *ss, SoupMessage *msg,  gpointer user_data);
void _PollResultArrayForeach(JsonArray *array,
                                     guint index_,
                                     JsonNode *element_node,
                                     gpointer user_data);
int GWQSessionDoPoll(GWQSession* wqs)
{
    gchar *tmpCStr, *escaped;
    
    if (wqs->st != GWQS_ST_IDLE) {
        GWQ_ERR_OUT(ERR_OUT, "\n");
    }
    
    tmpCStr = g_strdup_printf("http://d.web2.qq.com/channel/poll2?clientid=%s&psessionid=%s&t=%ld", 
        wqs->clientId->str, wqs->psessionid->str, GetNowMillisecond()
    );
    wqs->pollMsg = soup_message_new("POST", tmpCStr);
    g_free(tmpCStr);
    
    tmpCStr = g_strdup_printf("{\"clientid\":\"%s\","
            "\"psessionid\":\"%s\","
            "\"key\":0,"
            "\"ids\":[]}"
            "&clientid=%s"
            "&psessionid=%s",
            wqs->clientId->str, 
            wqs->psessionid->str,
            wqs->clientId->str,
            wqs->psessionid->str);
    escaped = g_uri_escape_string(tmpCStr, NULL, FALSE);
    g_free(tmpCStr);
    tmpCStr = g_strdup_printf("r=%s", escaped);
    g_free(escaped);
    soup_message_set_request (wqs->pollMsg, "application/x-www-form-urlencoded",
              SOUP_MEMORY_COPY, tmpCStr, strlen(tmpCStr));
    soup_message_headers_append (wqs->pollMsg->request_headers, "Referer", "http://d.web2.qq.com/proxy.html?v=20110331002&callback=1&id=4"); /* this is must */
    g_free(tmpCStr);
	soup_session_queue_message(wqs->sps, wqs->pollMsg, _process_poll_resp, wqs);
    return 0;
ERR_OUT:
    return -1;
}

static void _process_poll_resp(SoupSession *ss, SoupMessage *msg,  gpointer user_data)
{
    GWQSession *wqs;
    const guint8* data;
    gsize size;
    SoupBuffer *sBuf;
    gchar *tmpCStr;
    JsonParser *jParser;
    JsonNode *jn;
    JsonObject *jo;
    gint32 tmpInt;
    SoupURI *su;
    JsonArray *resJa;
    
    wqs = (GWQSession*)user_data;
    
    GWQ_DBG("poll responsed, retcode=%d, reason:%s\n", msg->status_code, msg->reason_phrase);
    if (wqs->st != GWQS_ST_IDLE) {
        goto ERR_OUT;
    }
    
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
        GWQ_ERR("poll retcode=%d\n", tmpInt);
    } else if (!(jn = json_object_get_member(jo, "result"))
            || !(resJa = json_node_get_array(jn))) {
        GWQ_ERR("poll no result found\n");
    } 
    if (resJa) {
        json_array_foreach_element(resJa, _PollResultArrayForeach, wqs);
    }
    
    g_object_unref(jParser);
    soup_buffer_free(sBuf);
    tmpCStr = g_strdup_printf("clientid=%s&psessionid=%s&t=%ld", 
        wqs->clientId->str, wqs->psessionid->str, GetNowMillisecond()
    );
    su = soup_message_get_uri(msg);
    soup_uri_set_query(su, tmpCStr);

    g_free(tmpCStr);
    GWQ_ERR("Fix me : why soup_session_requeue_message() fails here!!!!!!!!!!!!!!!!!!!!!\n");
    //soup_session_requeue_message(wqs->sps, msg);
    GWQSessionDoPoll(wqs);
    return;
ERR_FREE_J_PARSER:
    g_object_unref(jParser);
ERR_FREE_SBUF:
    soup_buffer_free(sBuf);
ERR_OUT:
    soup_session_cancel_message(ss, msg, SOUP_STATUS_CANCELLED);
}

void _PollResultArrayForeach(JsonArray *array,
                                     guint index_,
                                     JsonNode *element_node,
                                     gpointer user_data)
{
    JsonObject *jo;
    const gchar *tmpCCStr;
    GWQSession *wqs;
    QQRecvMsg *rMsg = NULL;
    
    wqs = (GWQSession*)user_data;
    if (!(jo = json_node_get_object(element_node))) {
        GWQ_ERR_OUT(ERR_OUT, "\n");
    }
    if (!(tmpCCStr = json_object_get_string_member(jo, "poll_type"))) {
        GWQ_ERR("poll no pool_type found");
    }
    if (tmpCCStr) {
        if(g_strcmp0(tmpCCStr, "message") == 0
                && (jo=json_object_get_object(jo, "value"))){
            //buddy message
            rMsg = gwq_create_buddy_message(jo);
        }else if(g_strcmp0(tmpCCStr, "buddies_status_change") == 0
                && (jo=json_object_get_object(jo, "value"))){
            //buddy status change
            rMsg = gwq_create_status_change_message(jo);
        }else if(g_strcmp0(tmpCCStr, "group_message") == 0
                && (jo=json_object_get_object(jo, "value"))){
            //group message
            rMsg = gwq_create_group_message(jo);
        }else if(g_strcmp0(tmpCCStr, "kick_message") == 0
                && (jo=json_object_get_object(jo, "value"))){
            //kick message
            rMsg = gwq_create_kick_message(jo);
        }
        if (rMsg) {
            if (wqs->messageRecieved) {
                wqs->messageRecieved(wqs, rMsg);
            }
            qq_recvmsg_free(rMsg);
        }
    }
    return;
ERR_OUT:
    return;
}
