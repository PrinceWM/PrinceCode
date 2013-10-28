#include "client.h"

client::client( thread_Settings *inSettings ) 
{
	int rc = 0;
	if(strlen(inSettings->mHost) == 0 || inSettings->mPort == 0)
	{
		printf("client para error\n");
		return;
	}
	
	memset(&sockadd,0, sizeof(sockadd));
	sockadd.sin_family = AF_INET;
	sockadd.sin_port = htons(inSettings->mPort);

#ifdef WIN32_BANDTEST

	sockadd.sin_addr.s_addr = inet_addr(inSettings->mHost);
	if(sockadd.sin_addr.s_addr == INADDR_NONE) 	// The address wasn't in numeric
		// form, resolve it
	{
		struct hostent *host = NULL;
		printf("Resolving host...");
		host = gethostbyname(inSettings->mHost);	// Get the IP address of the server
		// and store it in host
		if(host == NULL)
		{
			printf("Error\nUnknown host: %s\n", inSettings->mHost);
			return;
		}
		memcpy(&sockadd.sin_addr, host->h_addr_list[0],host->h_length);
		//printf("OK\n");
	}
#else

	rc = inet_pton( AF_INET, inSettings->mHost,(unsigned char*)&(sockadd.sin_addr) );
	if( rc <= 0 )
	{
#ifdef WIN32_BANDTEST

		printf("inet_pton error %d\n",WSAGetLastError());
#else
		printf("inet_pton error %d\n",errno);
#endif
		
	}

	//rc = inet_aton(inSettings->mHost,(unsigned char*)&(sockadd.sin_addr));
#endif


	printf("add %s : %d rate:%d\n",inSettings->mHost,inSettings->mPort,inSettings->mUDPRate);
	udprate = inSettings->mUDPRate;	
	sockaddlen = sizeof(sockadd);
	clientbufflen = inSettings->mBufLen;
	clientbuff = new char[ clientbufflen ];
	clientWin = inSettings->mTCPWin;
	mAmount = inSettings->mAmount;
	isudp = inSettings->isudp;
	clientsock = INVALID_SOCKET;
	speed = 0;
	clientcheckid = inSettings->checkid;
	memset( clientbuff,23,clientbufflen);

} // end Client

/* -------------------------------------------------------------------
* Delete memory (hostname strings).
* ------------------------------------------------------------------- */

client::~client() 
{
	if ( clientsock != INVALID_SOCKET ) 
	{
		close(clientsock);		
		clientsock = INVALID_SOCKET;
	}
	if ( clientbuff != NULL ) 
	{                        
		delete [] clientbuff;                            
		clientbuff = NULL;                              
		clientbufflen = 0;
	}
	printf("client release \n");
} // end ~Client


int client::Connect( ) 
{
	int rc;
	// create an internet socket
	int type = (( isudp )  ?  SOCK_DGRAM : SOCK_STREAM);

	int domain = AF_INET;

	clientsock = socket( domain, type, 0 );
	if(clientsock == INVALID_SOCKET)
	{
		printf("creat client socket fail \n");
		return -1;
	}
	int theTCPWin;
	int len = sizeof(int);;
	rc = getsockopt( clientsock, SOL_SOCKET, SO_SNDBUF,(char*) &theTCPWin, &len );
	//printf("client send buff:%d \n",theTCPWin);
	rc = getsockopt( clientsock, SOL_SOCKET, SO_RCVBUF,(char*) &theTCPWin, &len );
	//printf("client recv buff:%d \n",theTCPWin);



	if(clientWin> 0)
	{
		setsock_windowsize(clientsock,clientWin,false);
	}
	
	if(!isudp)
	{// connect socket
		rc = connect( clientsock, (sockaddr*) &sockadd,sockaddlen);
		if( rc != 0 )
		{
	#ifdef WIN32_BANDTEST

			printf("client connect error %d\n",WSAGetLastError());
	#else
			printf("client connect error %d\n",errno);
	#endif
			close(clientsock);
			return -1;
		}
	}

	return 0;
} // end Connect


const double kSecs_to_usecs = 1e6; 
const int    kBytes_to_Bits = 8; 
/* ------------------------------------------------------------------- 
* Send data using the connected UDP/TCP socket, 
* until a termination flag is reached. 
* Does not close the socket. 
* ------------------------------------------------------------------- */ 

int client::run( void ) 
{
	datagram* clientdgmbuff = (datagram*) clientbuff; 
	unsigned long currLen = 0; 
	unsigned long packetLen = 0;
	int delay_target = 0; 
	int delay = 0; 
	int adjust = 0; 
	int packetID = 0;
	int udptransferid = -1;
	struct timeval packetTime;
	fd_set fd_read;
	struct timeval tv;
	int ret = -1;
	int packcount = 0;
	int lastspeed = 0;
	int timoutcount = 0;
	int checkid = 0;
	//set transfer end time
	mEndTime.setnow();
	mEndTime.add( mAmount / 100.0 ); 

	ret = checkconnectack(&checkid);
	if(ret < 0)
	{
		printf("check ack error\n");
		return -1;
	}

	if ( isudp ) 
	{
		udptransferid = ret;
		// compute delay for bandwidth restriction, constrained to [0,1] seconds 
		//delay_target it means send clientbufflen byte need how long us
		//((kSecs_to_usecs * kBytes_to_Bits) / udprate) this means send one byte need us 
		delay_target = (int) ( (clientbufflen * ((kSecs_to_usecs * kBytes_to_Bits)) / udprate) ); 
		if ( delay_target < 0  || delay_target > (int) 1 * kSecs_to_usecs ) 
		{			
			delay_target = (int) (kSecs_to_usecs * 1); 
		}
		printf( "delay = %d clientbufflen=%d,udprate=%d \n" ,delay_target,clientbufflen,udprate ); 
	}

	lastPacketTime.setnow();


	do {
		gettimeofday( &(packetTime), NULL );
		clientdgmbuff->id      = htonl( packetID++ ); 
		
		//printf("packet id = %d \n",packetID);
		if ( isudp ) 
		{
			// store datagram ID into buffer 			
			clientdgmbuff->send_sec  = htonl( packetTime.tv_sec ); 
			clientdgmbuff->send_usec = htonl( packetTime.tv_usec );
			clientdgmbuff->udpid = htonl( udptransferid );
			clientdgmbuff->checkid = htonl( checkid );
			// delay between writes 
			// make an adjustment for how long the last loop iteration took 
			// TODO this doesn't work well in certain cases, like 2 parallel streams 
			adjust = delay_target + lastPacketTime.subUsec( packetTime ); 
			lastPacketTime.set( packetTime.tv_sec,packetTime.tv_usec ); 

			if ( adjust > 0  ||  delay > 0 ) 
			{
				delay += adjust; 
			}
		}

		// perform write 
		//printf("before %d delay=%d \n",packetTime.tv_sec,packetTime.tv_usec,delay);
		if(isudp)
		{
			currLen = sendto( clientsock, clientbuff,clientbufflen, 0,(struct sockaddr*) &sockadd, sockaddlen);
		}
		else
		{		
			currLen = write( clientsock, clientbuff, clientbufflen );
		}
		if(currLen<=0)
		{
#ifdef WIN32_BANDTEST

			printf("write error %d\n",WSAGetLastError());
#else
			printf("wrtie error %d\n",errno);
#endif

		}
		//printf("end %d delay=%d \n",packetTime.tv_sec,packetTime.tv_usec,delay);

#ifndef WIN32_BANDTEST                                         
		if ( currLen < 0 && errno != ENOBUFS )
#else                                                 
		if ( currLen < 0 )       
#endif                                                
		{
			printf("write error\n");
			break; 
		}
		packcount++;
		//printf("before %d delay=%d adjust=%d\n",packetTime.tv_sec,delay,adjust);
		if ( delay > 1000 && isudp ) 
		{
#ifdef WIN32_BANDTEST
			Sleep((int)(delay/1000));	//ms delay 	
#else
            usleep(delay); //us delay
#endif
		}
//		printf("end %d delay=%d \n",packetTime.tv_sec,delay);


	} while ( ! (sInterupted  || (mEndTime.before( packetTime )))); 

	printf("send finish packcount =%d wait recv \n",packcount);
#define MAXTIMOUT (100)
	tv.tv_sec = 0;
	tv.tv_usec = 200*1000;//200 ms
	printf("tv.tv_usec = %d\n",tv.tv_usec);
	//send packet finish,then wait recv speed
	while(1)
	{
		FD_ZERO(&fd_read);
		FD_SET(clientsock, &fd_read);

		ret = select(clientsock + 1, (fd_set*)&fd_read,0, (fd_set*)0, &tv);
		if ( ret == 0 ) 
		{
			if(timoutcount++ > MAXTIMOUT)
			{//did't receive speed
				return -1;
			}
			// select timed out 
			continue; 
		}
		else if( ret == SOCKET_ERROR)
		{
			printf("select error \n");
			return -1;
		}
		else if(ret > 0)
		{
			if(FD_ISSET(clientsock, &fd_read))
			{
				//printf("receive speed packet \n");
				if(isudp)
				{
					ret = (int)recvfrom( clientsock, clientbuff, clientbufflen, 0,(struct sockaddr*) &sockadd, (socklen_t *)&sockaddlen );		
				}
				else
				{				
					ret = read( clientsock, clientbuff, clientbufflen ); 
				}
				if( ret < 0)
				{
	#ifdef WIN32_BANDTEST
					printf("read error %d\n",WSAGetLastError());
	#else
					printf("read error %d\n",errno);
	#endif
					return -1;
				}		
				if(ntohl(clientdgmbuff->id) == -2)
				{
					speed = ntohl(clientdgmbuff->speed);
					if(speed > 0)
					{	
						storespeed(speed);
						//currLen = sendto( clientsock, clientbuff,clientbufflen, 0,(struct sockaddr*) &sockadd, sockaddlen);
						printf("client speed =%d kbit/sec packcount=%d\n",speed,packcount);
						int itmp = 0;
						int recvseccount = ntohl(clientdgmbuff->seccount);
						printf("recvseccount=%d \n",recvseccount);
						for(itmp=0;itmp<recvseccount;itmp++)
						{
							printf( "%d ",ntohl(clientdgmbuff->recvnumpersec[itmp]));
						}
						printf("\n");
						return speed;
					}	
				}
				else
				{
					printf("packet info error\n");
					return -1;
				}
			}
		}
	}
} // end Run

void client::initiateserver() 
{
	return;
}



int client::setsock_windowsize( int inSock, int inTCPWin, int inSend ) 
{
	int rc;
	int newTCPWin;
	if ( inTCPWin > 0 ) 
	{
		if ( !inSend ) 
		{
			/* receive buffer -- set
			* note: results are verified after connect() or listen(),
			* since some OS's don't show the corrected value until then. */
			newTCPWin = inTCPWin;
			rc = setsockopt( inSock, SOL_SOCKET, SO_RCVBUF,(char*) &newTCPWin, sizeof( newTCPWin ));
		} 
		else 
		{
			/* send buffer -- set
			* note: results are verified after connect() or listen(),
			* since some OS's don't show the corrected value until then. */
			newTCPWin = inTCPWin;
			rc = setsockopt( inSock, SOL_SOCKET, SO_SNDBUF,(char*) &newTCPWin, sizeof( newTCPWin ));
		}
		if ( rc < 0 ) 
		{
			return rc;
		}
	}
	return 0;
} /* end setsock_tcp_windowsize */

/* -------------------------------------------------------------------
* Setup a socket connected to a server.
* If inLocalhost is not null, bind to that address, specifying
* which outgoing interface to use.
* ------------------------------------------------------------------- */

void client::storespeed(int speed)
{
#ifndef WIN32_BANDTEST
	if(speed <= 0)
	{
		return;
	}
	FILE* filefd = NULL;
	char* filename = "/tmp/storespeed";
	//ever time open file ,will drop old data
	filefd = fopen(filename,"w");
	fwrite((char*)(&speed),sizeof(speed),1,filefd);
	fclose(filefd);
	return;
#endif
}
#define  MAX_CHECK_TIME 3
int client::checkconnectack(int* checkid)
{
	//char request[] = "request";
	fd_set readSet; 
	int id = 0;
	struct timeval timeout; 
	int rc = 0;
	int count = 0;
	int checktime = 0;
	iperf_sockaddr serveraddtmp;
	
	int serveraddtmplen = sizeof( iperf_sockaddr );
RECHECK:
	if (clientsock < 0)
	{
		return -1;
	}
	if(isudp)
	{	
		memcpy(clientbuff,"request",strlen("request")+1);
		int *tmp = (int *)(&(clientbuff[strlen("request")+1]));
		*tmp = htonl( clientcheckid );
		//write( clientsock, clientbuff, clientbufflen );
		rc = sendto( clientsock, clientbuff,clientbufflen, 0,(struct sockaddr*) &sockadd, sockaddlen);
		if(rc < 0)
		{
#ifdef WIN32_BANDTEST
			printf("send to error %d\n",WSAGetLastError());
#else
			printf("send to error %d\n",errno);
#endif

		}
		else
		{
			//printf("rc = %d \n",rc);
		}
	}

	while ( count < 10 ) 
	{
		count++;
		// wait until the socket is readable, or our timeout expires 
		FD_ZERO( &readSet ); 
		FD_SET( clientsock, &readSet ); 
		timeout.tv_sec  = 0; 
		timeout.tv_usec = (400*1000); // quarter second, 400 ms 

		rc = select( clientsock+1, &readSet, NULL, NULL, &timeout ); 
		if( rc == SOCKET_ERROR)
		{
			printf("select error \n");
			return -1;
		}

		if ( rc == 0 ) 
		{
			// select timed out 
			continue; 
		} 
		else 
		{
			if(FD_ISSET(clientsock,&readSet)>0)
			{				
				// socket ready to read 
				//printf("get read data \n");
				if(isudp)
				{
					rc = (int)recvfrom( clientsock, clientbuff, clientbufflen, 0,(struct sockaddr*) &serveraddtmp, (socklen_t *)&serveraddtmplen );		
				}
				else
				{				
					rc = read( clientsock, clientbuff, clientbufflen ); 
				}
				if( rc < 0)
				{
#ifdef WIN32_BANDTEST
					printf("read error %d\n",WSAGetLastError());
#else
					printf("read error %d\n",errno);
#endif
					return -1;
				}			
				
				if(memcmp(clientbuff,"bye",strlen("bye")+1)==0)
				{
					return -1;
				}
				else if(isudp)
				{//do udp ack
					//printf("get udp ack data \n");
					id = ntohl(((int*)(clientbuff))[0]);
					*checkid = ntohl(((int*)(clientbuff))[1]);
					printf("recv checkid %d\n",*checkid);
					return id;
				}
				else
				{//do tcp ack 					
					if(memcmp(clientbuff,"connected",strlen("connected")+1)==0)
					{
						return 0;
					}
					else
					{
						return -1;
					}
				}
			}
			else
			{
				continue;
			}
		} 
	}
	if(count>=10 && checktime<MAX_CHECK_TIME)
	{
		printf("**********************\n");
		printf("check ack retry %d \n",checktime);
		printf("********************** \n");
		checktime++;
		count = 0;
		goto RECHECK;
	}
	return -1;
}


/* ------------------------------------------------------------------- 
* Send a datagram on the socket. The datagram's contents should signify 
* a FIN to the application. Keep re-transmitting until an 
* acknowledgement datagram is received. 
* ------------------------------------------------------------------- */ 

int client::write_UDP_FIN( ) 
{
	int rc; 
	fd_set readSet; 
	int id = 0;
	struct timeval timeout; 
	iperf_sockaddr tmpadd;
	int tmpaddlen = sizeof( iperf_sockaddr );
	datagram* speeddatagram = NULL;
	int count = 0; 
        // write data
	if(isudp)
	{
		rc = sendto( clientsock, clientbuff,clientbufflen, 0,(struct sockaddr*) &sockadd, sockaddlen);
	}
	else
	{
		rc = write( clientsock, clientbuff, clientbufflen );
	}

	if( rc < 0)
	{
#ifdef WIN32_BANDTEST

		printf("send last %d\n",WSAGetLastError());
#else
		printf("send last %d\n",errno);
#endif
		return -1;
	}			

//	printf("write final data\n");
	while ( count < 100 ) 
	{
        count++;
		// wait until the socket is readable, or our timeout expires 
		FD_ZERO( &readSet ); 
		FD_SET( clientsock, &readSet ); 
		timeout.tv_sec  = 0; 
		timeout.tv_usec = 250000; // quarter second, 250 ms 

		rc = select( clientsock+1, &readSet, NULL, NULL, &timeout ); 
		if( rc == SOCKET_ERROR)
		{
			printf("select error \n");
			return -1;
		}

		if ( rc == 0 ) 
		{
			// select timed out 
			continue; 
		} 
		else 
		{
			if(FD_ISSET(clientsock,&readSet)>0)
			{				
				// socket ready to read 
				if (isudp)
				{
					rc = recvfrom( clientsock, clientbuff, clientbufflen, 0,(struct sockaddr*) &tmpadd, &tmpaddlen );
				}
				else
				{				
					rc = read( clientsock, clientbuff, sizeof(datagram)/*clientbufflen*/ ); 
				}
				//rc = recvfrom( clientsock, clientbuff, clientbufflen, 0,(struct sockaddr*) &tmpadd, &tmpaddlen );
				printf("client rc = %d\n",rc);
				if( rc < 0)
				{
#ifdef WIN32_BANDTEST

					printf("read error %d\n",WSAGetLastError());
#else
                    printf("read error %d\n",errno);
#endif
					return -1;
				}			

				speeddatagram = (datagram*) clientbuff;
				id = ntohl(speeddatagram->id);
				printf("final id =%d \n",id);
				if(id < 0)
				{
					speed = ntohl(speeddatagram->speed);
					//printf("client recv speed =%f \n",((float)speed)/1000);
					return 0;
				}
			}
			else
			{
				continue;
			}
		} 
	} 
	return -1;

	//fprintf( stderr, warn_no_ack, mSettings->mSock, count ); 
}// end write_UDP_FIN 
