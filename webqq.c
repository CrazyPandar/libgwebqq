#include "string.h"
#include <glib.h>
#include <libsoup/soup.h>
#include "qqhosts.h"

#define GWQS_ERR_OUT(__label, ...) g_printf(__VA_ARGS__);goto __label;
#define WQ_QQ_NUM_MAX_LEN   20
#define WQ_QQ_PASSWORD_MAX_LEN  50

typedef void(*GWQSessionCallback)(GWQSession* wqs, void* context);

typedef struct {
    SoupSession *sps;
    SoupCookieJar *scj;
    enum {
        GWQS_ST_OFFLINE,
        GWQS_ST_LOGIN,
        GWQS_ST_IDLE,
    }st;
    gchar num[WQ_QQ_NUM_MAX_LEN];
    gchar passwd[ WQ_QQ_PASSWORD_MAX_LEN];
    void* context;
    GWQSessionCallback loginCallback;
}GWQSession;


static void _check_resp(SoupSession *session, SoupMessage *msg,  gpointer user_data);


int GWQSessionInit(GWQSession* wqs, const gchar* qqNum, const gchar* passwd)
{
    strncpy(wqs->num, qqNum, sizeof(wqs->num));
    strncpy(wqs->passwd, passwd, sizeof(wqs->passwd));
    if (!(wqs->sps = soup_session_async_new_with_options(NULL))) {
        GWQS_ERR_OUT(ERR_OUT, "\n");
    }
    if (!(wqs->scj = soup_cookie_jar_new())) {
        GWQS_ERR_OUT(ERR_UNREF_SPS, "\n")
    }
    wqs->st = GWQS_ST_OFFLINE;
    return 0;
ERR_UNREF_SCJ:
    g_object_unref(wqs->scj);
ERR_UNREF_SPS:
    g_object_unref(wqs->sps);
ERR_OUT:
    return -1;
}

int GWQSessionExit(GWQSession* wqs)
{
    g_object_unref(wqs->scj);
    g_object_unref(wqs->sps);
    return 0;
}

int GWQSessionLogin(GWQSession* wqs, GWQSessionCallback callback, void* callbackCtx)
{
    SoupMessage *msg;
    GString *str;
    
    str = g_string_new("");
    g_string_printf(str, 
            "http://"LOGINHOST""VCCHECKPATH"?uin=%s&appid="APPID"&r=%.16f",
            wqs->num, g_random_double());
    msg = soup_message_new("GET", str->str);
    soup_session_queue_message (wqs->sps, msg, _check_resp, wqs);
    g_string_free(str, TRUE);
    wqs->context = callbackCtx;
    wqs->loginCallback = callback;
    return 0;
}

static void _check_resp(SoupSession *ss, SoupMessage *msg,  gpointer user_data)
{
    GWQSession *wqs;
    const guint8* data;
    gsize size;
    SoupBuffer *sBuf;
    
    wqs = (GWQSession*)user_data;
    sBuf = soup_message_body_flatten(msg->response_body);
    g_printf("retcode=%d, reason:%s\n", msg->status_code, msg->reason_phrase);
    if (sBuf) {
        soup_buffer_get_data(sBuf, &data, &size);
        g_printf("responsed:%s, size=%d\n", data, size);
        soup_buffer_free(sBuf);
        soup_session_cancel_message(ss, msg, SOUP_STATUS_CANCELLED);
        wqs->loginCallback(wqs, wqs->context);
    }
    
}

int main(int argc, char** argv)
{
    static GWQSession wqs;
    g_printf("0\n");
    gtk_init(&argc, &argv);
    g_printf("1\n");
    GWQSessionInit(&wqs, "331088042", "zcfzcf215021");
    GWQSessionLogin(&wqs);
    g_printf("2\n");
    
    g_printf("3\n");
    gtk_main();
    GWQSessionExit(&wqs);
    return 0;
}

