
#include "g_webqq.h"
#include "gwq_user.h"

static GMainLoop *MainLoop;

static void logoutCallback(GWQSession* wqs, void* ctx)
{
	if (wqs->st != GWQS_ST_OFFLINE) {
		GWQ_MSG("Logout failed\n");
		g_main_loop_quit(MainLoop);
	} else {
	    GWQ_MSG("Logout success\n");
        g_main_loop_quit(MainLoop);
	}
}

void _DisplayMsg(gpointer data, gpointer user_data)
{
    QQMsgContent *qmc;
    
    qmc = (QQMsgContent*)data;
    
    if (qmc->type == QQ_MSG_CONTENT_STRING_T) {
        GWQ_MSG("%s\n", qmc->value.str->str);
    }
}

void messageRecieved(GWQSession* wqs, QQRecvMsg* msg)
{
    GWQ_DBG("==>messageRecieved()\n");
    switch (msg->msg_type) {
        case MSG_STATUS_CHANGED_T:
            GWQ_MSG("uin=%"G_GINT64_FORMAT", status:%s\n", msg->uin, msg->status->str);
            break;
        case MSG_BUDDY_T:
            GWQ_MSG("[%"G_GINT64_FORMAT"]\n", msg->uin);
            g_ptr_array_foreach(msg->contents, _DisplayMsg, wqs);
        default:
            GWQ_MSG("Unknown message type received\n");
            break;
    }
}

static void messageSentHndl(GWQSession* wqs, QQSendMsg* msg, gint32 retCode)
{
    if (retCode) {
        GWQ_ERR("Message sent failed\n");
    } else {
        GWQ_MSG("Message sent OK\n");
    }
    qq_sendmsg_free(msg);
}

static void _UpdateUsersInfoCallback(GWQSession* wqs, void* ctx)
{
    QQSendMsg *qsm;
    QQMsgContent *qmc;
    
    GWQ_DBG("Waiting for message\n");
    GWQSessionDoPoll(wqs, messageRecieved);
    
    GWQ_DBG("Start sending test message\n");
    qsm = qq_sendmsg_new(MSG_BUDDY_T, 2395008827LL);  /* qsm should be freed with qq_sendmsg_free!! */
    //qmc = qq_msgcontent_new(QQ_MSG_CONTENT_STRING_T, "test from libgwebqq\n");
    //qq_sendmsg_add_content(qsm, qmc);

    qmc = qq_msgcontent_new(QQ_MSG_CONTENT_STRING_T, "来自<libgwebqq>的测试\n");
    qq_sendmsg_add_content(qsm, qmc);
    qmc = qq_msgcontent_new(QQ_MSG_CONTENT_FACE_T, 110);
    qq_sendmsg_add_content(qsm, qmc);
    qmc = qq_msgcontent_new(QQ_MSG_CONTENT_FONT_T, "宋体", 12, "000000", 0,0,0);
    qq_sendmsg_add_content(qsm, qmc);

    if (GWQSessionSendBuddyMsg(wqs, 2395008827LL, qsm, messageSentHndl)) {
        GWQ_ERR("Sent failed, BUSY sending message, please try later\n");
    }
}

static void _LoginCallback(GWQSession* wqs, void* ctx)
{
	GWQ_DBG("==>__LoginCallback()\n");
	if (wqs->st != GWQS_ST_IDLE) {
		GWQ_MSG("Login failed\n");
		g_main_loop_quit(MainLoop);
	} else {
	    GWQ_MSG("Login successfully\n");
        GWQSessionUpdateUsersInfo(wqs, _UpdateUsersInfoCallback);
	}
}

static GWQSession Wqs;
static void _sig_term_hndl(int sig)
{
     GWQSessionLogOut(&Wqs, logoutCallback);
}

int main(int argc, char** argv)
{

	
	if (argc != 3) {
	    GWQ_ERR_OUT(ERR_OUT, "Usage: test <qq number> <password>\n");
	}
    
    signal(SIGINT, _sig_term_hndl);
	
    g_type_init();
    MainLoop = g_main_loop_new(NULL, TRUE);
	GWQ_DBG("\n");
    if (GWQSessionInit(&Wqs, argv[1], argv[2], &Wqs)) {
    	GWQ_ERR_OUT(ERR_OUT, "\n");
    }
    GWQSessionLogin(&Wqs, _LoginCallback, GWQ_CHAT_ST_HIDDEN);
	
    g_main_loop_run(MainLoop);
    GWQSessionExit(&Wqs);
    return 0;
ERR_EXIT_SESSION:
    GWQSessionExit(&Wqs);
ERR_OUT:
    return -1;
}
