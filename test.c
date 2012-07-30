
#include "g_webqq.h"
#include "gwq_user.h"

static GMainLoop *MainLoop;
static gint64 QQNumForTestMsg;

static void _LogoutCallback(GWQSession* wqs, void* ctx)
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

void _MessageRecieved(GWQSession* wqs, QQRecvMsg* msg)
{
    GWQ_DBG("==>_MessageRecieved()\n");
    switch (msg->msg_type) {
        case MSG_STATUS_CHANGED_T:
            GWQ_MSG("uin=%"G_GINT64_FORMAT", status:%s\n", msg->uin, msg->status->str);
            break;
        case MSG_BUDDY_T:
            GWQ_MSG("[%"G_GINT64_FORMAT"]\n", msg->from_uin);
            g_ptr_array_foreach(msg->contents, _DisplayMsg, wqs);
        default:
            GWQ_MSG("Unknown message type received\n");
            break;
    }
}

static void _MessageSent(GWQSession* wqs, QQSendMsg* msg, gint32 retCode)
{
    if (retCode) {
        GWQ_ERR("Message sent failed\n");
    } else {
        GWQ_MSG("Message sent OK\n");
    }
    qq_sendmsg_free(msg);
}

void _usersForeach(GWQSession* wqs, GWQUserInfo* user)
{
    GWQ_MSG("qqNum:%"G_GINT64_FORMAT",\t nick:%s,\t markname:%s\n", 
            user->qqNum, user->nick ,user->markname);
}

static void _UpdateUsersInfoCallback(GWQSession* wqs, void* ctx)
{
    QQSendMsg *qsm;
    QQMsgContent *qmc;
    
    GWQ_MSG("=== Buddies List ===\n");
    GWQSessionUsersForeach(wqs, _usersForeach);
    GWQ_MSG("====================\n");
    
    GWQ_DBG("Waiting for message\n");
    GWQSessionDoPoll(wqs);
    
    GWQ_DBG("Start sending test message\n");
    qsm = qq_sendmsg_new(MSG_BUDDY_T, QQNumForTestMsg);  /* qsm should be freed with qq_sendmsg_free!! */

    qmc = qq_msgcontent_new(QQ_MSG_CONTENT_STRING_T, "来自<libgwebqq>的测试\n");
    qq_sendmsg_add_content(qsm, qmc);
    //qmc = qq_msgcontent_new(QQ_MSG_CONTENT_FACE_T, 110);
    //qq_sendmsg_add_content(qsm, qmc);
    qmc = qq_msgcontent_new(QQ_MSG_CONTENT_FONT_T, "宋体", 12, "000000", 0,0,0);
    qq_sendmsg_add_content(qsm, qmc);
    /*
    if (GWQSessionSendBuddyMsg(wqs, QQNumForTestMsg, -1LL, qsm)) {
        GWQ_ERR("Sent failed, BUSY sending message, please try later\n");
    }
    * */
}

static void _LoginCallback(GWQSession* wqs, void* ctx)
{
	GWQ_DBG("==>__LoginCallback()\n");
	if (wqs->st != GWQS_ST_IDLE) {
		GWQ_MSG("Login failed\n");
		g_main_loop_quit(MainLoop);
	} else {
	    GWQ_MSG("Login successfully\n");
        GWQ_MSG("Fetching buddies' information, please wait......\n");
        GWQSessionUpdateUsersInfo(wqs, _UpdateUsersInfoCallback);
	}
}

static GWQSession Wqs;
static void _sig_term_hndl(int sig)
{
     GWQSessionLogOut(&Wqs);
}

int main(int argc, char** argv)
{

	
	if (argc != 4) {
	    GWQ_ERR_OUT(ERR_OUT, "Usage: test <qq number> <password> <qqNUm for sending test message>\n");
	}
    
    signal(SIGINT, _sig_term_hndl);
	
    g_type_init();
    MainLoop = g_main_loop_new(NULL, TRUE);
	GWQ_DBG("\n");
    if (GWQSessionInit(&Wqs, argv[1], argv[2], &Wqs)) {
    	GWQ_ERR_OUT(ERR_OUT, "\n");
    }
    GWQSessionSetCallBack(&Wqs, 
            _LoginCallback,
            _LogoutCallback,
            _MessageRecieved,
            _MessageSent,
            NULL,
            NULL,
            NULL,
            NULL);
            
    QQNumForTestMsg = g_ascii_strtoll(argv[3], NULL, 10);
    GWQSessionLogin(&Wqs, GWQ_CHAT_ST_HIDDEN);
	
    g_main_loop_run(MainLoop);
    GWQSessionExit(&Wqs);
    return 0;
ERR_EXIT_SESSION:
    GWQSessionExit(&Wqs);
ERR_OUT:
    return -1;
}
