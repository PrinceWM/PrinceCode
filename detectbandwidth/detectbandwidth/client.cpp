#include "client.h"

client::client( thread_Settings *inSettings ) 
{
	// initialize buffer
	printf("float =%d",sizeof(float));
	if(inSettings->mHost == NULL || inSettings->mPort == 0)
	{
		printf("client para error\n");
		return;
	}
	
	memset(&sockadd,0, sizeof(sockadd));
	sockadd.sin_family = AF_INET;
	sockadd.sin_port = htons(inSettings->mPort);


	int rc = inet_pton( AF_INET, inSettings->mHost,(unsigned char*)&(sockadd.sin_addr) );

	printf("add %s : %d\n",inSettings->mHost,inSettings->mPort);
	udprate = inSettings->mUDPRate;	
	sockaddlen = sizeof(sockadd);
	clientbufflen = inSettings->mBufLen;
	clientbuff = new char[ clientbufflen ];
	clientWin = inSettings->mTCPWin;
	mAmount = inSettings->mAmount;
	isudp = 1;
	clientsock = INVALID_SOCKET;
	speed = 0;
	memset( clientbuff,23,clientbufflen);

	// connect
	//Connect( );

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
	if(clientWin> 0)
	{
		setsock_windowsize(clientsock,clientWin,false);
	}

	// connect socket
	rc = connect( clientsock, (sockaddr*) &sockadd,sockaddlen);
	if( rc != 0 )
	{
		printf("client connect error \n");
		close(clientsock);
		return -1;
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

void client::run( void ) 
{
	datagram* clientdgmbuff = (datagram*) clientbuff; 
	unsigned long currLen = 0; 
	unsigned long packetLen = 0;
	int delay_target = 0; 
	int delay = 0; 
	int adjust = 0; 
	int packetID = 0;
	//int countsendtime = 0;
	struct timeval packetTime;
	int ret = -1;
	//set transfer end time
	mEndTime.setnow();
	mEndTime.add( mAmount / 100.0 );

	if ( isudp ) 
	{
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

		if ( isudp ) 
		{
			// store datagram ID into buffer 
			clientdgmbuff->id      = htonl( packetID++ ); 
			clientdgmbuff->send_sec  = htonl( packetTime.tv_sec ); 
			clientdgmbuff->send_usec = htonl( packetTime.tv_usec );

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
		//gettimeofday( &packetTime, NULL );
		//printf("before %d delay=%d \n",packetTime.tv_sec,packetTime.tv_usec,delay);

		currLen = write( clientsock, clientbuff, clientbufflen );
		
		if(currLen<=0)
		{
#ifdef WIN32_BANDTEST

			printf("read error %d\n",WSAGetLastError());
#else
			printf("read error %d\n",errno);
#endif

		}
		//gettimeofday( &packetTime, NULL );
		//printf("end %d delay=%d \n",packetTime.tv_sec,packetTime.tv_usec,delay);

		//countsendtime++;
		//printf("write data to server %d ret =%d delay=%d,countsendtime=%d id=%d sec=%d usec=%d\n",
		//	clientbufflen ,currLen,delay,countsendtime,packetID,packetTime.tv_sec,packetTime.tv_usec);
#ifndef WIN32_BANDTEST                                         
		if ( currLen < 0 && errno != ENOBUFS )
#else                                                 
		if ( currLen < 0 )       
#endif                                                
		{
			printf("write error\n");
			break; 
		}

		// report packets 
	//	packetLen = currLen;
	//	gettimeofday( &packetTime, NULL );
	//	printf("before %d delay=%d \n",packetTime.tv_sec,delay);
		if ( delay > 1000 ) 
		{
#ifdef WIN32_BANDTEST
			Sleep((int)(delay/1000));	//ms delay 	
#else
                    usleep(delay); //us delay
#endif
		}
//		gettimeofday( &packetTime, NULL );
//		printf("end %d delay=%d \n",packetTime.tv_sec,delay);


	} while ( ! (sInterupted  || (mEndTime.before( packetTime )))); 

	// stop timing
	gettimeofday( &packetTime, NULL );

	if ( isudp ) 
	{
		// send a final terminating datagram 
		// Don't count in the mTotalLen. The server counts this one, 
		// but didn't count our first datagram, so we're even now. 
		// The negative datagram ID signifies termination to the server. 

		// store datagram ID into buffer 
		clientdgmbuff->id      = htonl( -packetID  ); 
		clientdgmbuff->send_sec  = htonl( packetTime.tv_sec ); 
		clientdgmbuff->send_sec = htonl( packetTime.tv_usec ); 

		ret = write_UDP_FIN( ); 
		if(ret == 0)
		{
			storespeed(speed);
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
/* ------------------------------------------------------------------- 
* Send a datagram on the socket. The datagram's contents should signify 
* a FIN to the application. Keep re-transmitting until an 
* acknowledgement datagram is received. 
* ------------------------------------------------------------------- */ 

int client::write_UDP_FIN( ) 
{
	int rc; 
	fd_set readSet; 
	struct timeval timeout; 
	//iperf_sockaddr tmpadd;
	int tmpaddlen = sizeof( iperf_sockaddr );
	datagram* speeddatagram = NULL;
	int count = 0; 
        // write data
    write( clientsock, clientbuff, clientbufflen );
//	printf("write final data\n");
	while ( count < 10 ) 
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
				rc = read( clientsock, clientbuff, clientbufflen ); 
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
				if(speeddatagram->id < 0)
				{
					speed = ntohl(speeddatagram->speed);
					printf("client recv speed =%f \n",((float)speed)/1000);
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
