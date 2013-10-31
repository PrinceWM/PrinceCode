#pragma once
#include "MyMutex.h"
#include "timestamp.h"
#include "sourcecontrol.h"
//#include "MyThread.h"
#define AMOUNT 500
#define MAX_NUM 15
class CMyThread;
typedef struct _SESSIONINFO
{
	int randid;//connection fd
	Timestamp transfertime;//this connection start transfer data time
	Timestamp currenttime;//this connection current transfer data time
	Timestamp applytime;//when session apply,then check this session use or not 
	int length;//this connection recv byte 
	bool udpsessionuse;//use for udp transfer
	struct sockaddr_in clientadd;
	int clientaddlen /*= sizeof( iperf_sockaddr )*/;
	int checkid;



	int tmprecvnumpersec[MAX_NUM];//for test
	int tmpseccount;//for test
	double tmptime ;//for test
	int lastlen ;//for test

}SESSIONINFO;

class CMySession
{
public:
	CMySession(void);
	~CMySession(void);
	int setsession(int sessionid,int randid);
	int updatesession( int mAmount,CMyThread* taskThread);	
	int clearsession(int index );
	SESSIONINFO sessioninfostore[MAXSESSION];
//private:	
	//CMyMutex m_mutex;
};
