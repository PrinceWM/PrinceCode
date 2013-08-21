#include "server.h"




server::server( thread_Settings *inSettings ) 
{

	// initialize buffer
	serversock = INVALID_SOCKET;
	serverbufflen = inSettings->mBufLen;
	serverbuff = new char[ serverbufflen ];
	serversockwin = inSettings->mTCPWin;
	
	memset(&localadd,0,sizeof(localadd));
	localadd.sin_family = AF_INET;
	localadd.sin_port = htons(inSettings->mPort);
	localaddlen = sizeof(localadd);
	printf("port %d\n",inSettings->mPort);

} 



server::~server() 
{
	if ( serversock != INVALID_SOCKET )
	{
		int rc = close( serversock );		
		serversock = INVALID_SOCKET;
	}
	if ( serverbuff != NULL ) 
	{                        
		delete [] serverbuff;                            
		serverbuff = NULL;                              
		serverbufflen = 0;
	}
} // end ~Listener 


void server::creat( ) 
{
	int rc;

	// create an internet TCP socket
	int type = SOCK_DGRAM;
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

} 

void server::UDPSingleServer( ) 
{

	Timestamp transfertime;
	long msfeed = 0;
	int rc = 0;
	unsigned int packetLen = 0;
	int datagramID;
	int packetID = 0;
	int recvlen = 0;
	struct timeval sendTime;
	struct timeval packetTime;
	iperf_sockaddr clientadd;
	int clientaddlen = sizeof( iperf_sockaddr );

	datagram* serverdgmbuff = (datagram*)serverbuff;
	
	int itemp = 0;

	while ( sInterupted == 0) 
	{			
                rc = (int)recvfrom( serversock, serverbuff, serverbufflen, 0,(struct sockaddr*) &clientadd, (socklen_t *)&clientaddlen );
		
		if ( rc == SOCKET_ERROR ) 
		{
			printf("recvfrom error \n");
			return;
		}
		//printf("received data %d id =%d sec=%d usec=%d\n"
		//	,rc,ntohl(serverdgmbuff->id),ntohl(serverdgmbuff->send_sec),ntohl(serverdgmbuff->send_usec));

		// Handle connection for UDP sockets.
		
		datagramID = ntohl( ((datagram*) serverbuff)->id ); 
		if ( datagramID >= 0 ) 
		{
			// read the datagram ID and sentTime out of the buffer 
			packetID = datagramID; 
			sendTime.tv_sec = ntohl( ((datagram*) serverbuff)->send_sec  );
			sendTime.tv_usec = ntohl( ((datagram*) serverbuff)->send_usec ); 
			
			if(recvlen == 0)
			{
				transfertime.setnow();					
			}
			recvlen += rc;
			printf("start time %ld %ld\n",transfertime.getSecs(),transfertime.getUsecs());
			packetLen = rc;
			gettimeofday( &(packetTime), NULL );
		} 
		else 
		{
			printf("datagramID = %d \n",datagramID);
			// read the datagram ID and sentTime out of the buffer 
			packetID = -datagramID; 
			if(recvlen >0)
			{
				msfeed = transfertime.delta_usec();
				printf("speed = %f recvlen=%d msfeed=%ld\n",(((recvlen*8)/(1024*1024))/((double)msfeed/(1000*1000))),recvlen,msfeed);
				recvlen = 0;
			}
			printf("end time %ld %ld\n",transfertime.getSecs(),transfertime.getUsecs());
			recvlen = 0;

			sendTime.tv_sec = ntohl( ((datagram*) serverbuff)->send_sec  );
			sendTime.tv_usec = ntohl( ((datagram*)serverbuff)->send_usec ); 

			packetLen = rc;
			gettimeofday( &(packetTime), NULL );
			//need check
			rc = sendto( serversock, serverbuff,serverbufflen, 0,(struct sockaddr*) &clientadd, clientaddlen);
			printf("send to final packet rc =%d \n",rc);
		}
	}
}
