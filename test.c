#include "g_webqq.h"

static GMainLoop *MainLoop;
static void _LoginCallback(GWQSession* wqs, void* ctx)
{
	GWQ_DBG("==>__LoginCallback()\n");
	if (wqs->verifyCode) {
		GWQ_DBG("Got verifyCode:%s\n", wqs->verifyCode->str);
	}
	g_main_loop_quit(MainLoop);
}


int main(int argc, char** argv)
{
    static GWQSession wqs;
	
	g_type_init();
    MainLoop = g_main_loop_new(NULL, TRUE);
	GWQ_DBG("\n");
    if (GWQSessionInit(&wqs, "331088042", "zcfzcf215021")) {
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
