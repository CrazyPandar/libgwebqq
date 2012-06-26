
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

void messageRecieved(GWQSession* wqs, QQRecvMsg* msg)
{
    GWQ_DBG("==>messageRecieved()\n");
    switch (msg->msg_type) {
        case MSG_STATUS_CHANGED_T:
            GWQ_MSG("uin=%"G_GINT64_FORMAT", status:%s\n", msg->uin, msg->status->str);
            break;
        default:
            GWQ_MSG("Unknown message type received\n");
            break;
    }
}

static void _UpdateUsersInfoCallback(GWQSession* wqs, void* ctx)
{
    GWQ_DBG("Waiting for message\n");
    GWQSessionDoPoll(wqs, messageRecieved);
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
