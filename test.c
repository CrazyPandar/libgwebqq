#include "g_webqq.h"

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

static void _LoginCallback(GWQSession* wqs, void* ctx)
{
	GWQ_DBG("==>__LoginCallback()\n");
	if (wqs->st != GWQS_ST_IDLE) {
		GWQ_MSG("Login failed\n");
		g_main_loop_quit(MainLoop);
	} else {
	    GWQ_MSG("Login success, sleep 2 seconds.\n");
	    sleep(2);
	    GWQSessionLogOut(wqs, logoutCallback, ctx);
	}
}

int main(int argc, char** argv)
{
    static GWQSession wqs;
	
	if (argc != 3) {
	    GWQ_ERR_OUT(ERR_OUT, "Usage: test <qq number> <password>\n");
	}
	g_type_init();
    MainLoop = g_main_loop_new(NULL, TRUE);
	GWQ_DBG("\n");
    if (GWQSessionInit(&wqs, argv[1], argv[2])) {
    	GWQ_ERR_OUT(ERR_OUT, "\n");
    }
    GWQSessionLogin(&wqs, _LoginCallback, NULL);
	
    g_main_loop_run(MainLoop);
    GWQSessionExit(&wqs);
    return 0;
ERR_EXIT_SESSION:
    GWQSessionExit(&wqs);
ERR_OUT:
    return -1;
}
