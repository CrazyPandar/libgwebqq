#ifndef _GWQ_RECVMSG_H_
#define _GWQ_RECVMSG_H_
#include <glib.h>
#include <json-glib/json-glib.h>
#include "gwq_sendmsg.h"
typedef struct _QQRecvMsg QQRecvMsg;
struct _QQRecvMsg{
    QQMsgType msg_type;
    
    gint64 msg_id, msg_id2;
    gint64 from_uin, to_uin;
    gint64 reply_ip;

    gint64 group_code;        // only group message
    gint64 send_uin;          // only group message
    gint64 seq;                   // only group message
    gint64 time;
    gint64 info_seq;              // only group message
    gint64 type;

    gint64 uin, client_type;       //only buddy status change
    GString *status;

    GPtrArray *contents;
    GString *raw_content;       // the raw content.
};
QQRecvMsg* qq_recvmsg_new(QQMsgType type);
void qq_recvmsg_set(QQRecvMsg *msg, const gchar *name,  void *value);
void qq_recvmsg_add_content(QQRecvMsg *msg, QQMsgContent *content);
void qq_recvmsg_free(QQRecvMsg *);

QQRecvMsg* gwq_create_buddy_message(JsonObject *json);
QQRecvMsg* gwq_create_group_message(JsonObject *json);
QQRecvMsg* gwq_create_status_change_message(JsonObject *json);
QQRecvMsg* gwq_create_kick_message(JsonObject *json);
#endif
