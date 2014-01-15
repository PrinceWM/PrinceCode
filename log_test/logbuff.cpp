#include "logbuff.h"
#include <stdio.h>
#include <string.h>
logbuff::logbuff(int size)
{
	buff = new char[size];
	buffsize = size;
	w_offset = r_offset = 0;
}

logbuff::~logbuff()
{
	printf("~logbuff");
	delete buff;
	buffsize = 0;
}

int logbuff::log_checkfreesize()
{//offset is 0 ~ buffsize-1
	return logger_offset(buffsize - 1 -(w_offset-r_offset));
}

int logbuff::log_checkusesize()
{	
	return logger_offset(buffsize-1+(w_offset-r_offset));
}

int logbuff::log_read()
{
	int readsize = 0;
	SYSTEMTIME* time = NULL;
	char tmp[256];
	unsigned short len = 0;
	unsigned int bfinalsize = 0;
	int usesize = log_checkusesize();
	if(usesize+1 <= sizeof(LOGGER))
	{
		return -1;
	}
	bfinalsize = buffsize - r_offset;
	len = logger_getpayloadlen();
	//read all log byte LOGGER info|msg
	if((MAX_LOG_SIZE - sizeof(LOGGER)) < len)
	{
		return -1;
	}

	if(bfinalsize < sizeof(LOGGER)+len)
	{
		memcpy(tmp,buff+r_offset,bfinalsize);
		memcpy(tmp+bfinalsize,buff,(sizeof(LOGGER)+len)-bfinalsize);
	}
	else
	{
		memcpy(tmp,buff+r_offset,sizeof(LOGGER)+len);
	}
	r_offset = logger_offset(r_offset+sizeof(LOGGER)+len);
	time = (SYSTEMTIME*)(&(tmp[sizeof(int)]));
	printf("msg = %4d-%2d-%2d %02d:%02d:%02d",time->wYear,time->wMonth,time->wDay, time->wHour, time->wMinute, time->wSecond);
	printf("%s \n",(char*)(&(tmp[sizeof(LOGGER)])));
	return readsize;
}

int logbuff::log_write(char* msg)
{
	LOGGER lg;
	unsigned int len = 0;
	unsigned int bfreesize = log_checkfreesize();
	unsigned int writesize = 0;
	char tmpbuff[MAX_LOG_SIZE+sizeof(LOGGER)];
	lg.len = strlen(msg)+1;
	GetLocalTime(&lg.time);
	memcpy(tmpbuff,&lg,sizeof(lg));
	memcpy(tmpbuff+sizeof(lg),msg,lg.len);

	if(MAX_LOG_SIZE < lg.len+sizeof(lg))
	{
		return -1;
	}
	writesize = lg.len + sizeof(lg);
	if(bfreesize +1 < writesize)
	{
		//remain need free size
		int tmplen = writesize - (bfreesize+1);
		while(w_offset != r_offset)
		{
			len = logger_getpayloadlen();	
			//new read pos
			r_offset = logger_offset(r_offset + sizeof(LOGGER)+len );
			tmplen -=  sizeof(LOGGER)+len;
			if(tmplen <= 0)
			{				
				break;
			}
			
		}		
	}

	//write msg ,and check w_offset
	if(writesize > buffsize - w_offset)
	{
		memcpy(buff+w_offset,tmpbuff,buffsize - w_offset);
		memcpy(buff,&(tmpbuff[buffsize - w_offset]),(writesize-(buffsize - w_offset)));
	}
	else
	{
		memcpy(buff+w_offset,tmpbuff,writesize);
	}
	w_offset = logger_offset(w_offset+writesize);
	return writesize;

}






