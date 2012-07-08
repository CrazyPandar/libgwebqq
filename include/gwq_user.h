#ifndef _GWQ_USER_H_
#define _GWQ_USER_H_
#include "g_webqq.h"
typedef struct gwq_user_info GWQUserInfo;
struct gwq_user_info {
	gint64 uin;
	gint64 qqNum;
	GString* nick;
	GString* markname;
	gint32 face;
	gint32 category;
	gint32 flag;
	gint32 online;
};

void GWQUserInfoFree(GWQUserInfo* wui);
int GWQSessionUpdateUsersInfo(GWQSession* wqs, GWQSessionCallback callback);
int GWQSessionUpdateUserInfo(GWQSession* wqs, GWQSessionCallback callback, gpointer callbackCtx);
GWQUserInfo* GWQSessionGetUserInfo(GWQSession* wqs, gint64 qqNum, gint64 qqUin);
#endif
