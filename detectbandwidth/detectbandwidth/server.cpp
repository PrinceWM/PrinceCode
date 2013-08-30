#include "server.h"



//#define MYPORT 1234    // the port users will be connecting to  

#define SESSIONMAXNUM 5     // how many pending connections queue will hold  

//#define BUF_SIZE 200  

//int fd_A[BACKLOG];    // accepted connection fd  
//int conn_amount;    // current connection amount  


server::server( thread_Settings *inSettings ) 
{

	// initialize buffer
	serversock = INVALID_SOCKET;
	listenedsock = INVALID_SOCKET;
	serverbufflen = inSettings->mBufLen;
	serverbuff = new char[ serverbufflen ];
	serversockwin = inSettings->mTCPWin;
	
	memset(&localadd,0,sizeof(localadd));
	localadd.sin_family = AF_INET;
	localadd.sin_port = htons(inSettings->mPort);
	localaddlen = sizeof(localadd);
	isudp = inSettings->isudp;
	sessionnum = 0;
	sessioninfostore = new SESSIONINFO[SESSIONMAXNUM];
	memset(sessioninfostore,0,SESSIONMAXNUM*sizeof(SESSIONINFO));
	printf("port %d\n",inSettings->mPort);

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

	// create an internet TCP socket
	int type = ((isudp)?SOCK_DGRAM:SOCK_STREAM);
	int domain = AF_INET;

	serversock = socket( domain, type, 0 );
	if(serversock == INVALID_SOCKET)
	{
		printf("server sock fail \n");
	}
	//set socket send buff size	
	setsockopt( serversock, SOL_SOCKET, SO_SNDBUF,(char*) &serversockwin, sizeof( serversockwin ));
	
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
	//	struct sockaddr_in client;
	//	memset(&client,0, sizeof(client));
	//	//bzero(&client, sizeof(client));  
	//	int len = sizeof(client);  
	//	//接受连接请求  
	//	listenedsock = accept(serversock, (struct sockaddr*)(&client),/* (size_t*)*/(&len));  
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
{
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
			ret = ioctl (sessioninfostore[i].fd, FIONREAD, &nBytes);	
#endif
			
			//if(ret < serverbufflen)
			//{
			//	continue;
			//}
			ret = 0;
RETRY:
			//ret = ioctlsocket(sessioninfostore[i].fd, FIONREAD, (u_long*)(&nBytes));
			//ret += (int)read( listenedsock, (char*)(&serverbuff[rc]), (serverbufflen-rc));
			ret += read(sessioninfostore[i].fd, (char*)(&serverbuff[ret]), (serverbufflen-ret));  
			//if(count<20)
			//{count++;
			//	printf("ret = %d serverbufflen=%d nBytes=%d\n",ret,serverbufflen,nBytes);
			//}
			if (ret <= 0) 
			{        // client close need sort session info store 
				printf("client[%d] close/n", i);  
				//close(sessioninfostore[i].fd);  
				//FD_CLR(sessioninfostore[i].fd, &fdset);  
				//sessioninfostore[i].fd = 0;  
				//sessionnum--;       
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
					datagramID = ntohl( ((datagram*) serverbuff)->id ); 
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
						if(sessioninfostore[i].length>0)
						{
							printf("sessioninfostore[%d].length = %d\n",i,sessioninfostore[i].length);
						}
						//printf("start time %ld %ld\n",transfertime.getSecs(),transfertime.getUsecs());
						//packetLen = ret;
						//gettimeofday( &(packetTime), NULL );
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
						//sessioninfostore[i].length = 0;

						//sendTime.tv_sec = ntohl( ((datagram*) serverbuff)->send_sec  );
						//sendTime.tv_usec = ntohl( ((datagram*)serverbuff)->send_usec ); 

						//packetLen = rc;
						//need check
						//if(isudp)
						//{			
							//ret = sendto( serversock, serverbuff,serverbufflen, 0,(struct sockaddr*) &clientadd, clientaddlen);
						//}
						//else
						//{
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
						//}
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
		}  
		else 
		{  
			printf("max connections arrive, exit/n");  
			send(new_fd, "bye", 4, 0);  
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

int server::udprecvdata( )
{
	fd_set fdsr ;
	struct timeval tv;
	struct timeval packetTime;
	int ret = -1;
	int datagramID = -1;
	int transferid = -1;
	datagram* serverdgmbuff = (datagram*)serverbuff;
	long msfeed = 0;
	float speed = 0.0;
	//struct sockaddr_in client_addr; // connector's address information  
	//socklen_t sin_size;  
	//sin_size = sizeof(client_addr);
	iperf_sockaddr clientadd;
	int clientaddlen = sizeof( iperf_sockaddr );
	int tempid = -1;



	FD_ZERO(&fdsr);  
	FD_SET(serversock, &fdsr);
	
	// timeout setting  
	tv.tv_sec = 0;  
	tv.tv_usec = 20*1000;//20 ms timeout  

	ret = select(serversock + 1, &fdsr, NULL, NULL, &tv);
	if (ret < 0) 
	{  
		perror("select");  
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
		ret = (int)recvfrom( serversock, serverbuff, serverbufflen, 0,(struct sockaddr*) &clientadd, (socklen_t *)&clientaddlen );		
		if(memcmp(serverbuff,"request",strlen("request")+1)==0)
		{
			tempid = getudptransferuid();
			if(tempid < 0)
			{
				memcpy(serverbuff,"bye",strlen("bye")+1);
				ret = sendto( serversock, serverbuff,serverbufflen, 0,(struct sockaddr*) &clientadd, clientaddlen);
			}
			else
			{
				((int *)(serverbuff))[0] = ntohl( tempid );
				sessionnum++;
				ret = sendto( serversock, serverbuff,serverbufflen, 0,(struct sockaddr*) &clientadd, clientaddlen);
			}
		}
		else
		{
			datagramID = ntohl( ((datagram*) serverbuff)->id ); 
			transferid = ntohl( ((datagram*) serverbuff)->udpid ); 
			printf("datagramID = %d \n",datagramID);
			if ( datagramID >= 0 ) 
			{
				// read the datagram ID and sentTime out of the buffer 
				//packetID = datagramID; 
				//sendTime.tv_sec = ntohl( ((datagram*) serverbuff)->send_sec  );
				//sendTime.tv_usec = ntohl( ((datagram*) serverbuff)->send_usec ); 

				if(sessioninfostore[transferid].length == 0)
				{
					sessioninfostore[transferid].transfertime.setnow();					
				}
				sessioninfostore[transferid].length += ret;
				//printf("start time %ld %ld\n",transfertime.getSecs(),transfertime.getUsecs());
				//packetLen = ret;
				//gettimeofday( &(packetTime), NULL );
			} 
			else 
			{
				printf("datagramID = %d \n",datagramID);
				gettimeofday( &(packetTime), NULL );
				// read the datagram ID and sentTime out of the buffer 
				//packetID = -datagramID; 
				if(sessioninfostore[transferid].length >0)
				{
					msfeed = sessioninfostore[transferid].transfertime.delta_usec();
					speed = (((sessioninfostore[transferid].length*8)/(1024*1024))/((float)msfeed/(1000*1000)));//Mbit/sec
					printf("speed = %f sessioninfostore[i].length=%d msfeed=%ld\n",speed,sessioninfostore[transferid].length,msfeed);
					sessioninfostore[transferid].length = 0;

					serverdgmbuff->id      = htonl( datagramID ); 
					serverdgmbuff->send_sec  = htonl( packetTime.tv_sec ); 
					serverdgmbuff->send_sec = htonl( packetTime.tv_usec ); 				
					serverdgmbuff->speed = htonl((int)(speed*1000));
					ret = sendto( serversock, serverbuff,serverbufflen, 0,(struct sockaddr*) &clientadd, clientaddlen);
					if(ret > 0)
					{
						sessioninfostore[transferid].udpsessionuse = false;
						sessioninfostore[transferid].length = 0;
						sessioninfostore[transferid].transfertime.set(0,0);
						sessionnum--;
					}
				}
				printf("end time %ld %ld\n",sessioninfostore[transferid].transfertime.getSecs(),sessioninfostore[transferid].transfertime.getUsecs());
				//sessioninfostore[i].length = 0;

				//sendTime.tv_sec = ntohl( ((datagram*) serverbuff)->send_sec  );
				//sendTime.tv_usec = ntohl( ((datagram*)serverbuff)->send_usec ); 

				//packetLen = rc;
				//need check
				//if(isudp)
				//{			
				
				//}
				//else
				//{
				//	ret = write(listenedsock, serverbuff,serverbufflen);
				//}
				printf("send to final packet ret =%d \n",ret);
			}

		}
	}
	return 0;

}

void server::recvdata( ) 
{

	//Timestamp transfertime;
	//long msfeed = 0;
	//int rc = 0;
	//unsigned int packetLen = 0;
	//int datagramID;
	//int packetID = 0;
	//int recvlen = 0;
	//struct timeval sendTime;
	//struct timeval packetTime;
	//iperf_sockaddr clientadd;
	//int clientaddlen = sizeof( iperf_sockaddr );

	//datagram* serverdgmbuff = (datagram*)serverbuff;
	//float speed = 0.0;
	//int itemp = 0;



	fd_set fdsr;  
	int maxsock;  
	//struct timeval tv;  
	//int datagramID;
	//int transferid = -1;;
	//struct sockaddr_in client_addr; // connector's address information  
	//socklen_t sin_size;  
	//int i = 0;
	int ret = 0;
	//int new_fd = -1;
	//int nBytes = 0;
	//conn_amount = 0;  
	//sin_size = sizeof(client_addr);  
	maxsock = serversock;
	
	while ( sInterupted == 0) 
	{		
		if(isudp)
		{
			ret = udprecvdata();
			if( ret <0 )
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
//			rc = 0;
//RETRY:
//			//printf("read before \n");
//			rc += (int)read( listenedsock, (char*)(&serverbuff[rc]), (serverbufflen-rc));
//			//printf("read end \n");
//			if(rc == 0)
//			{//disconnect
//				printf("server break");
//				break;
//			}
//			else if(rc < serverbufflen)
//			{
//				printf("retry rc=%d \n",rc);
//				goto RETRY;
//			}
//		}
//		if ( rc == SOCKET_ERROR ) 
//		{
//			printf("recvfrom error \n");
//			return;
//		}
//		//printf("received data %d id =%d sec=%d usec=%d\n"
//		//	,rc,ntohl(serverdgmbuff->id),ntohl(serverdgmbuff->send_sec),ntohl(serverdgmbuff->send_usec));
//
//		// Handle connection for UDP sockets.
//		
//		datagramID = ntohl( ((datagram*) serverbuff)->id ); 
//		printf("datagramID = %d \n",datagramID);
//		//if(datagramID >10)
//		//{
//		//	break;
//		//}
//		if ( datagramID >= 0 ) 
//		{
//			// read the datagram ID and sentTime out of the buffer 
//			packetID = datagramID; 
//			sendTime.tv_sec = ntohl( ((datagram*) serverbuff)->send_sec  );
//			sendTime.tv_usec = ntohl( ((datagram*) serverbuff)->send_usec ); 
//			
//			if(recvlen == 0)
//			{
//				transfertime.setnow();					
//			}
//			recvlen += rc;
//			//printf("start time %ld %ld\n",transfertime.getSecs(),transfertime.getUsecs());
//			packetLen = rc;
//			gettimeofday( &(packetTime), NULL );
//		} 
//		else 
//		{
//			printf("datagramID = %d \n",datagramID);
//			gettimeofday( &(packetTime), NULL );
//			// read the datagram ID and sentTime out of the buffer 
//			packetID = -datagramID; 
//			if(recvlen >0)
//			{
//				msfeed = transfertime.delta_usec();
//				speed = (((recvlen*8)/(1024*1024))/((float)msfeed/(1000*1000)));//Mbit/sec
//				printf("speed = %f recvlen=%d msfeed=%ld\n",speed,recvlen,msfeed);
//				recvlen = 0;
//
//				serverdgmbuff->id      = htonl( datagramID ); 
//				serverdgmbuff->send_sec  = htonl( packetTime.tv_sec ); 
//				serverdgmbuff->send_sec = htonl( packetTime.tv_usec ); 				
//				serverdgmbuff->speed = htonl((int)(speed*1000));
//			}
//			printf("end time %ld %ld\n",transfertime.getSecs(),transfertime.getUsecs());
//			recvlen = 0;
//
//			//sendTime.tv_sec = ntohl( ((datagram*) serverbuff)->send_sec  );
//			//sendTime.tv_usec = ntohl( ((datagram*)serverbuff)->send_usec ); 
//
//			//packetLen = rc;
//			//need check
//			if(isudp)
//			{			
//				rc = sendto( serversock, serverbuff,serverbufflen, 0,(struct sockaddr*) &clientadd, clientaddlen);
//			}
//			else
//			{
//				rc = write(listenedsock, serverbuff,serverbufflen);
//			}
//			printf("send to final packet rc =%d \n",rc);
//		}
//	}
}


//void server::listenrequest::listenrequest(int sock)
//{
//	if(sock == INVALID_SOCKET)
//	{
//		return;
//	}
//	listensock = sock;
//}
//
//void server::listenrequest::~listenrequest()
//{
//	listensock = INVALID_SOCKET;
//}
//
//void server::listenrequest::listenling()
//{
//	int ret = -1;
//	if(!isudp)
//	{
//		listen(serversock, 5);
//		struct sockaddr_in client;
//		memset(&client,0, sizeof(client));
//		//bzero(&client, sizeof(client));  
//		int len = sizeof(client);  
//		//接受连接请求  
//		listenedsock = accept(serversock, (struct sockaddr*)(&client),/* (size_t*)*/(&len));  
//		if(listenedsock != INVALID_SOCKET)
//		{
//			creatthread();
//		}
//	}
//	else
//	{
//		ret = recvfrom();
//		if(ret>0)
//		{
//			//do some thing 
//			creatthread();
//		}
//	}
//}