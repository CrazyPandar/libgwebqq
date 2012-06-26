#ifndef _GWQ_SENDMSG_H_
#define _GWQ_SENDMSG_H_
#include <string.h>
#include <glib.h>
#include <libsoup/soup.h>
#include <json-glib/json-glib.h>
typedef enum   _QQMsgType           QQMsgType;
typedef struct _QQSendMsg           QQSendMsg;
typedef struct _QQRecvMsg           QQRecvMsg;
typedef struct _QQMsgContent        QQMsgContent;
typedef enum   _QQMsgContentType    QQMsgContentType;
typedef struct _QQMsgFont           QQMsgFont;
//
// The font of the messages
//
struct _QQMsgFont{
    GString *name;
    gint size;
    GString *color;
    struct{
        gint a, b, c; // bold , italic , underline
    }style;
};
QQMsgFont* qq_msgfont_new(const gchar *name, gint size, const gchar *color
                            , gint sa, gint sb, gint sc);
void qq_msgfont_free(QQMsgFont *font);
//
// If two font is the same
//
gboolean qq_msgfont_equal(QQMsgFont *a, QQMsgFont *b);

//
// Type of the message content
// type :
//      face.    eg: ["face", 21]
//      string.  eg: "some string"
//      font.    eg: ["font", {"name":"Arial", "size":10
//                          ,"style":[1, 0, 0], "color":"000000"}]
//
enum  _QQMsgContentType
{
    QQ_MSG_CONTENT_FACE_T = 256,        // face
    QQ_MSG_CONTENT_STRING_T,            // string
    QQ_MSG_CONTENT_FONT_T,              // font
    QQ_MSG_CONTENT_UNKNOWN_T            // unknown
};
//
// The message content.
//
struct _QQMsgContent{
    gint type;              //the type of the content.
    union{
        gint face;
        GString *str;
        QQMsgFont *font;
    }value;
};
//
// Create a new instance of QQMsgContent.
//
// Example:
//      qq_msgcontent_new(1, 20);               // face
//      qq_msgcontent_new(2, "somg string");    // string
//      qq_msgcontent_new(3,                    // string
//                         "name",      //name
//                         10,          //size
//                         "000000",    //color
//                         1,0,0);      //style
//
QQMsgContent *qq_msgcontent_new(gint type, ...);
void qq_msgcontent_free(QQMsgContent *cnt);
//
// Convert to string
//
GString* qq_msgcontent_tostring(QQMsgContent *cnt);

//
// Message type
//
enum _QQMsgType{
    MSG_BUDDY_T = 128,      /* buddy message */
    MSG_GROUP_T,            // group message
    MSG_STATUS_CHANGED_T,   // buddy status changed
    MSG_KICK_T,             // kick message. In other place logined
    MSG_UNKNOWN_T
};
//
// The send message.
//
struct _QQSendMsg{
    QQMsgType type;         // 0 this is buddy message, or 1 is group message.
    GString *to_uin;        // If buddy msg, this is "to"
                            // If group msg, this is "group_uin"
    GString *face;          // Only used when this is buddy message.

    GPtrArray *contents;    // Message contents. An array of QQMsgContent.
};
QQSendMsg* qq_sendmsg_new(QQMsgType type, const gchar *to_uin);
void qq_sendmsg_free(QQSendMsg *msg);
void qq_sendmsg_add_content(QQSendMsg *msg, QQMsgContent *content);
//
// Convert contents to string
//
GString * qq_sendmsg_contents_tostring(QQSendMsg *msg);
#endif
