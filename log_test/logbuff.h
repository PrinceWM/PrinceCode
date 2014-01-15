#pragma once
#include <windows.h>
#include <WinBase.h>
#define MAX_LOG_SIZE 64
typedef struct logger_entry 
{  
	unsigned short       len;    /* length of the payload */  
	unsigned short       pad;  /* no matter what, we get 2 bytes of padding */  
	SYSTEMTIME time;

	//int       sec;    /* seconds since Epoch */  
	//int       nsec;   /* nanoseconds */  
	//char     msg[0]; /* the entry's payload */  
}LOGGER;  


class logbuff
{
public:
	logbuff(int size);
	~logbuff();
	int log_read();
	int log_write(char* msg);
	int log_checkfreesize();
	int log_checkusesize();
	int logger_offset(int size)
	{
		return ((size) & (buffsize - 1)) ; 
	};

	int logger_getpayloadlen(void)
	{
		unsigned short len = 0;
		if(buffsize - r_offset == sizeof(unsigned short) -1)
		{//len is 2 byte ,one byte is buff last,one byte is first
			memcpy((char*)(&len),buff+r_offset,1);
			memcpy((char*)(&len)+1,buff,1);
		}
		else
		{//two byte is together
			memcpy((char*)(&len),buff+r_offset,2);
		}
		return len;
	};

private:
	char* buff;
	unsigned int buffsize;
	unsigned int r_offset;
	unsigned int w_offset;
};