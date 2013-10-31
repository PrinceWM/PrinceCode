#include "server.h"

#define SESSIONMAXNUM 3     // how many pending connections queue will hold  

server::server( thread_Settings *inSettings ) 
{

	// initialize buffer
	serversock = INVALID_SOCKET;
	listenedsock = INVALID_SOCKET;
	serverbufflen = inSettings->mBufLen;
    mAmount = inSettings->mAmount;
	//printf("inSettings->mBufLen = %d \n",inSettings->mBufLen);
	serverbuff = new char[ serverbufflen ];
	serversockwin = inSettings->mTCPWin;
	
	memset(&localadd,0,sizeof(localadd));
	localadd.sin_family = AF_INET;
	localadd.sin_port = htons(inSettings->mPort);
	localadd.sin_addr.s_addr = INADDR_ANY;
	localaddlen = sizeof(localadd);

	

	isudp = inSettings->isudp;
	sessionnum = 0;
	sessioninfostore = new SESSIONINFO[SESSIONMAXNUM];
	memset(sessioninfostore,0,SESSIONMAXNUM*sizeof(SESSIONINFO));
	//printf("port %d\n",inSettings->mPort);

} 



server::~server() 
{
	int rc = -1;
	if(listenedsock != INVALID_SOCKET)
	{
		rc = close( listenedsock );		
		listenedsock = INVALID_SOCKET;
	}
	if ( serversock != INVALID_SOCKET )
	{
		rc = close( serversock );		
		serversock = INVALID_SOCKET;
	}
	
	if ( serverbuff != NULL ) 
	{                        
		delete [] serverbuff;                            
		serverbuff = NULL;                              
		serverbufflen = 0;
	}
	if ( sessioninfostore != NULL ) 
	{                        
		delete [] sessioninfostore;                            
		sessioninfostore = NULL;                              
		sessionnum = 0;
	}
} // end ~Listener 


void server::creat( ) 
{
	int rc;

	// create an internet  socket
	int type = ((isudp)?SOCK_DGRAM:SOCK_STREAM);
	int domain = AF_INET;
	if(isudp)
	{			
		serversock = socket( domain, type, IPPROTO_UDP );
		//printf(" sock type: %d serversock=%d\n",type ,serversock);
	}
	else
	{	
		serversock = socket( domain, type, 0 );
	}
	
	if(serversock == INVALID_SOCKET)
	{
		printf("server sock fail \n");
	}
	//set socket send buff size	
	//setsockopt( serversock, SOL_SOCKET, SO_SNDBUF,(char*) &serversockwin, sizeof( serversockwin ));
	do
	{
		int theTCPWin;
		//printf("serversockwin=%d\n",serversockwin);
		rc = setsockopt( serversock, SOL_SOCKET, SO_RCVBUF,(char*) &serversockwin, sizeof( serversockwin ));
		//printf("server send buff:%d rc = %d error=%d\n",theTCPWin,rc,WSAGetLastError());
		int len = sizeof(int);
		rc = getsockopt( serversock, SOL_SOCKET, SO_SNDBUF,(char*) &theTCPWin, &len );
		//printf("server send buff:%d rc = %d error=%d\n",theTCPWin,rc,WSAGetLastError());
		rc = getsockopt( serversock, SOL_SOCKET, SO_RCVBUF,(char*) &theTCPWin, &len );
		printf("server recv buff:%d rc=%d error=%d\n",theTCPWin,rc,WSAGetLastError());
	}while (0);
	//getsockopt(serversock, SOL_SOCKET, SO_SNDBUF,(char*) &serversockwin, sizeof( serversockwin ));
	// reuse the address, so we can run if a former server was killed off
	int boolean = 1;
	int len = sizeof(boolean);
	setsockopt( serversock, SOL_SOCKET, SO_REUSEADDR, (char*) &boolean, len );

	// bind socket to server address

	rc = bind( serversock, (sockaddr*) &localadd, localaddlen );
	if(rc == SOCKET_ERROR)
	{
		printf("server socket bind error \n");
	}
	if(!isudp)
	{
		listen(serversock, 5);
	}
} 



int server::checkselectfd(fd_set* fdset,int maxsock)
{
	struct timeval tv;
	int i = 0;
	int ret = 0;

	// initialize file descriptor set  
	FD_ZERO(fdset);  
	FD_SET(serversock, fdset);  
	// timeout setting  
	tv.tv_sec = 0;  
	tv.tv_usec = 20*1000;//20 ms timeout  

	// add active connection to fd set  
	for (i = 0; i < sessionnum; i++) 
	{  
		if (sessioninfostore[i].fd != 0) 
		{  
			FD_SET(sessioninfostore[i].fd, fdset);  
		}  
	}  

	ret = select(maxsock + 1, fdset, NULL, NULL, &tv);
	return ret;
}

int server::clearsessioninfo(int index,fd_set* fdset)
{//error
	if(sessioninfostore == NULL || index >= SESSIONMAXNUM || index < 0)
	{
		return -1;
	}

	close(sessioninfostore[index].fd);  
	FD_CLR(sessioninfostore[index].fd, fdset);
	if(index == sessionnum-1)
	{//final index session
		memset((SESSIONINFO*)(&(sessioninfostore[index])),0,sizeof(SESSIONINFO));									 
	}
	else
	{									
		//sessioninfostore[i].fd = 0;
		/*当session完成使用后，把末尾的session放到要close fd的session位置，然后把末尾的session清零，下次
		申请的session就会在末尾被添加*/
		memcpy((SESSIONINFO*)(&(sessioninfostore[index])),(SESSIONINFO*)(&(sessioninfostore[sessionnum-1])),sizeof(SESSIONINFO));
		memset((SESSIONINFO*)(&(sessioninfostore[sessionnum-1])),0,sizeof(SESSIONINFO));									 
	}     
	sessionnum--; 
	return 0;
}

//int count = 0;
int server::checkconnectfd(fd_set* fdset)
{
	int i = 0;
	int ret = 0;
	unsigned long nBytes = 0;
	int datagramID = 0;
	long msfeed = 0;
	float speed = 0.0;
	struct timeval packetTime;
	datagram* serverdgmbuff = (datagram*)serverbuff;
	


	for (i = 0; i < sessionnum; i++) 
	{  
		if (FD_ISSET(sessioninfostore[i].fd, fdset)) 
		{  
#ifdef WIN32_BANDTEST
			//ret = ioctlsocket(sessioninfostore[i].fd, FIONREAD, (u_long*)(&nBytes));
#else
			//ret = ioctl (sessioninfostore[i].fd, FIONREAD, &nBytes);	
#endif
			
			ret = 0;
RETRY:
			ret += read(sessioninfostore[i].fd, (char*)(&serverbuff[ret]), (serverbufflen-ret));  
			if (ret <= 0) 
			{ 
				// client close need sort session info store 
				
#ifdef WIN32_BANDTEST
				printf("server read error %d\n",WSAGetLastError());
#else
				printf("server read error %d\n",errno);
#endif

				clearsessioninfo(i,fdset);
			}
			else if(ret< serverbufflen)
			{
				goto RETRY;
			}
			else 
			{        // receive data  
				if (ret < serverbufflen)  
				{
					//printf("read some error \n");
				}
				else
				{
					datagramID = ntohl(((datagram*) serverbuff)->id ); 
					//printf("datagramID = %d \n",datagramID);
					if ( datagramID >= 0 ) 
					{
						// read the datagram ID and sentTime out of the buffer 
						//packetID = datagramID; 
						//sendTime.tv_sec = ntohl( ((datagram*) serverbuff)->send_sec  );
						//sendTime.tv_usec = ntohl( ((datagram*) serverbuff)->send_usec ); 

						if(sessioninfostore[i].length == 0)
						{
							sessioninfostore[i].transfertime.setnow();					
						}
						sessioninfostore[i].length += ret;
						//if(sessioninfostore[i].length>0)
						//{
						//	printf("sessioninfostore[%d].length = %d\n",i,sessioninfostore[i].length);
						//}
						//printf("start time %ld %ld\n",transfertime.getSecs(),transfertime.getUsecs());
					} 
					else 
					{
						//printf("datagramID = %d \n",datagramID);
						gettimeofday( &(packetTime), NULL );
						// read the datagram ID and sentTime out of the buffer 
						//packetID = -datagramID; 
						printf("-sessioninfostore[%d].length = %d\n",i,sessioninfostore[i].length);
						if(sessioninfostore[i].length >0)
						{
							msfeed = sessioninfostore[i].transfertime.delta_usec();
							speed = (((sessioninfostore[i].length*8)/(1024*1024))/((float)msfeed/(1000*1000)));//Mbit/sec
							printf("speed = %f sessioninfostore[i].length=%d msfeed=%ld\n",speed,sessioninfostore[i].length,msfeed);
							sessioninfostore[i].length = 0;

							serverdgmbuff->id      = htonl( datagramID ); 
							serverdgmbuff->send_sec  = htonl( packetTime.tv_sec ); 
							serverdgmbuff->send_sec = htonl( packetTime.tv_usec ); 				
							serverdgmbuff->speed = htonl((int)(speed*1000));
						
							printf("end time %ld %ld\n",sessioninfostore[i].transfertime.getSecs(),sessioninfostore[i].transfertime.getUsecs());
							ret = write(sessioninfostore[i].fd, serverbuff,sizeof(datagram)/*serverbufflen*/);
							if(ret < 0)
							{
#ifdef WIN32_BANDTEST
								printf("server write error %d\n",WSAGetLastError());
#else
								printf("server write error %d\n",errno);
#endif
							}
							else
							{
								clearsessioninfo(i,fdset);
							}
						}
						//printf("send to final packet ret =%d fd=%d\n",ret,sessioninfostore[i].fd);
					}
				}
				//printf("client[%d] send:%s/n", i, buf);  
			}//recv data else  
		}  //fd check
	} //for loop all fd 
	return 0;
}


int server::checkaccept(fd_set* fdset,int* maxsock)
{
	int new_fd = -1;
	struct sockaddr_in client_addr; // connector's address information  
	socklen_t sin_size = sizeof(client_addr); 
	// check whether a new connection comes  
	if (FD_ISSET(serversock, fdset)) 
	{  
		new_fd = accept(serversock, (struct sockaddr *)&client_addr, &sin_size);  
		if (new_fd <= 0) 
		{  
#ifdef WIN32_BANDTEST

			printf("server accept error %d\n",WSAGetLastError());
#else
			printf("server accept error %d\n",errno);
#endif
			//continue;  
			return -1;
		}  

		// add to fd queue  
		if (sessionnum < SESSIONMAXNUM) 
		{  
			sessioninfostore[sessionnum++].fd = new_fd;  
			printf("new connection client[%d] %s:%d \n", sessionnum,  
				inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));  
			if (new_fd > *maxsock)  
			{
				*maxsock = new_fd;  
			}
			send(new_fd, "connected", strlen("connected")+1, 0);			
		}  
		else 
		{  
			printf("max connections arrive, exit \n");  
			send(new_fd, "bye", strlen("bye")+1, 0);  
			close(new_fd);  
			//break;
			return -2;
		}  
	}
	return 0;
}

int server::getudptransferuid()
{
	int i = 0;
	if(sessionnum >= SESSIONMAXNUM)
	{
		return -1;
	}
	for(i = 0;i<SESSIONMAXNUM;i++)
	{
		if(sessioninfostore[i].udpsessionuse == false)
		{
			sessioninfostore[i].udpsessionuse = true;
			return i;
		}
	}
	return -1;
}

int server::udpclearsession(int index )
{
	if(index >= SESSIONMAXNUM)
	{
		printf("clear index is too big \n");
		return -1;
	}
	sessioninfostore[index].udpsessionuse = false;
	sessioninfostore[index].length = 0;
	sessioninfostore[index].transfertime.set(0,0);
	sessioninfostore[index].currenttime.set(0,0);
	sessioninfostore[index].applytime.set(0,0);
	memset(&sessioninfostore[index].clientadd,0,sizeof(sessioninfostore[index].clientadd));
	sessioninfostore[index].clientaddlen = 0;
	sessionnum--;
	return 0;
}

#define BORDER_TIME 10

int tmprecvnumpersec[MAX_NUM]={0};
int tmpseccount = 0;

int server::udpupdatesession( )
{
	int itemp;
	long time = 0;
	static long tmptime = (((mAmount/100+1))*(1e6));
	static int lastlen = 0;
	long msfeed = 0;
	double speed = 0.0;
	datagram* serverdgmbuff = (datagram*)serverbuff;
	int ret = 0;
	struct timeval currentTime;
	gettimeofday(&currentTime,NULL);
	for(itemp = 0;itemp<SESSIONMAXNUM;itemp++)
	{
		if(sessioninfostore[itemp].udpsessionuse ==  true)
		{
			sessioninfostore[itemp].currenttime.set(currentTime.tv_sec,currentTime.tv_usec);
			if(sessioninfostore[itemp].length > 0)
			{//check start transfer session time ,data				
				
				time = sessioninfostore[itemp].currenttime.subUsec(sessioninfostore[itemp].transfertime);				
				if(time >= (long)tmptime)
				{
					tmptime += 1e6;//1 sec 				
					tmprecvnumpersec[tmpseccount] = (int)((sessioninfostore[itemp].length-lastlen)/serverbufflen);
					lastlen = sessioninfostore[itemp].length;
					printf("time %d tmptime %d pack recv %d\n",time,tmptime,tmprecvnumpersec[tmpseccount]/*(sessioninfostore[itemp].length-lastlen)/serverbufflen*/);
					tmpseccount++;
				}

				if(time >= ((mAmount/100)+BORDER_TIME)*(1e6))//ns
				{
					if(tmpseccount<10)
					{
						printf("******************\n");
						printf("********%d**********\n",tmpseccount);
						printf("******************\n");
					}
					printf("send speed id\n");
					msfeed = sessioninfostore[itemp].transfertime.delta_usec();
					printf("end time %d: %d %d \n",itemp,sessioninfostore[itemp].transfertime.getSecs(),sessioninfostore[itemp].transfertime.getUsecs());
					//speed = (((double)(sessioninfostore[itemp].length*8)/(1024*1024))/((double)msfeed/(1000*1000)));//Mbit/sec
					speed = ((double)(sessioninfostore[itemp].length*8)/(1024*1024))/(mAmount/100);
					printf("speed = %f sessioninfostore[i].length=%d msfeed=%ld recvpack=%d\n",speed,sessioninfostore[itemp].length,msfeed,sessioninfostore[itemp].length/serverbufflen);

					serverdgmbuff->id      = htonl( /*datagramID*/-2 ); 
					serverdgmbuff->send_sec  = htonl( currentTime.tv_sec ); 
					serverdgmbuff->send_sec = htonl( currentTime.tv_usec ); 				
					serverdgmbuff->speed = htonl((int)(speed*1000));


					serverdgmbuff->seccount = htonl( tmpseccount );
					int i;
					for(i = 0;i<tmpseccount;i++)
					{
						serverdgmbuff->recvnumpersec[i] = htonl(tmprecvnumpersec[i]);
					}
					memset(tmprecvnumpersec,0,sizeof(tmprecvnumpersec));
					tmpseccount = 0;
					ret = sendto( serversock, serverbuff,sizeof(datagram)/*serverbufflen*/, 0,(struct sockaddr*) &sessioninfostore[itemp].clientadd, sessioninfostore[itemp].clientaddlen);
					if(ret > 0)
					{
						udpclearsession(itemp);
						tmptime = (((mAmount/100+1))*(1e6));
						lastlen = 0;
						return 1;
					}
					else
					{
	#ifdef WIN32_BANDTEST
						printf("final send to error %d\n",WSAGetLastError());
	#else
						printf("final send to error %d\n",errno);
	#endif
						return -1;
					}
				}
			}
			else if(sessioninfostore[itemp].length == 0)
			{//check not start transfer session time ,if time out close this session	
				time = sessioninfostore[itemp].currenttime.subUsec(sessioninfostore[itemp].applytime);				
				if(time >= ((mAmount/100))*(1e6))
				{//this session not work in amount time len so clear it
					printf("clear not use session\n");
					udpclearsession(itemp);
				}				
			}
		}
	}
	return 0;
}
//int count = 0;
int server::udprecvdata( )
{
	fd_set fdsr ;
	struct timeval tv;
//	struct timeval packetTime;
	int ret = -1;
	int datagramID = -1;
	int transferid = -1;
	datagram* serverdgmbuff = (datagram*)serverbuff;
	long msfeed = 0;
	double speed = 0.0;
	iperf_sockaddr clientadd;
	int clientaddlen /*= sizeof( iperf_sockaddr )*/;
	int tempid = -1;
	int checkid = 0;
	//char serverbuff11[512];
	long time = 0;

	udpupdatesession();
	FD_ZERO(&fdsr);  
	FD_SET(serversock, &fdsr);
	
	// timeout setting  
	tv.tv_sec = 0;  
	tv.tv_usec = 20*1000;//20 ms timeout  

	ret = select(serversock + 1, &fdsr, NULL, NULL, &tv);
	if (ret < 0) 
	{  
		printf("select error \n");  
		//break;
		return -1;
	} 
	else if (ret == 0) 
	{  
		//printf("udp select timeout \n");  
		//continue;  
		return -2;
	}
	if (FD_ISSET(serversock, &fdsr))
	{	
		clientaddlen = sizeof(clientadd);
		ret = (int)recvfrom( serversock, serverbuff, serverbufflen, 0,(struct sockaddr*) &clientadd, (socklen_t *)&clientaddlen );		
		
		if(memcmp(serverbuff,"request",strlen("request")+1)==0)
		{
			tempid = getudptransferuid();
			if(tempid < 0)
			{
				memcpy(serverbuff,"bye",strlen("bye")+1);
				printf("send bye \n");
				ret = sendto( serversock, serverbuff,serverbufflen, 0,(struct sockaddr*) &clientadd, clientaddlen);
			}
			else
			{
				int *tmp = (int *)(&(serverbuff[strlen("request")+1]));
				int tmpcheckid = ntohl( *tmp );
				tmpcheckid++;
				((int *)(serverbuff))[0] = ntohl( tempid );
				((int *)(serverbuff))[1] = ntohl( tmpcheckid );
				sessioninfostore[tempid].checkid = tmpcheckid;
				//set apply this session time
				sessioninfostore[tempid].applytime.setnow();
				sessionnum++;
				printf("send id \n");
				ret = sendto( serversock, serverbuff,serverbufflen, 0,(struct sockaddr*) &clientadd, clientaddlen);
			}
		}
		else
		{
			datagramID = ntohl(((datagram*) serverbuff)->id ); 
			transferid = ntohl(((datagram*) serverbuff)->udpid ); 
			checkid = ntohl(((datagram*) serverbuff)->checkid ); 
			if(sessioninfostore[transferid].udpsessionuse == false)
			{//check this session allow
				return -1;
			}
			if(sessioninfostore[transferid].checkid != checkid)
			{//check this session allow
				printf("check id check error\n");
				return -1;
			}

			//printf("datagramID = %d \n",datagramID);
			if ( datagramID >= 0 ) 
			{
				if(sessioninfostore[transferid].length == 0)
				{
					sessioninfostore[transferid].transfertime.setnow();
					memcpy(&sessioninfostore[transferid].clientadd,&clientadd,sizeof(clientadd));
					sessioninfostore[transferid].clientaddlen = sizeof(clientadd);
					printf("start time %d :%d %d \n",transferid,sessioninfostore[transferid].transfertime.getSecs(),sessioninfostore[transferid].transfertime.getUsecs());
				}
				sessioninfostore[transferid].length += ret;
				//printf("time =%d abidance =%d\n",time,((mAmount/100)+4)*(1e6));
			}
		}
	}
	return 0;
}

void server::recvdata( ) 
{
	fd_set fdsr;  
	int maxsock;  
	int ret = 0;
	maxsock = serversock;

	while ( sInterupted == 0) 
	{			
		if(isudp)
		{
			ret = udprecvdata();
			if( ret < 0 )
			{
				continue;
			}
		}
		else
		{
			ret = checkselectfd(&fdsr,maxsock);
			if (ret < 0) 
			{  
				printf("select error\n");  
				break;  
			} 
			else if (ret == 0) 
			{  
				//printf("tcp select timeout \n");  
				continue;  
			}
			// check every fd in the set 
			ret = checkconnectfd(&fdsr);	 

			// check whether a new connection comes
			ret = checkaccept(&fdsr,&maxsock);	
			if (ret < 0) 
			{  
				printf("server accept error");  
				continue;  
			}  
		}
	}
}


