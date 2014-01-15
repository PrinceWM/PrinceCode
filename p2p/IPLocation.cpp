//#ifdef WIN32
#include "stdafx.h"
//#endif

#include "IPLocation.h"
#include "http_util.h"
#include <stdio.h>
#include "cJSON.h"





#ifdef WIN32
#include "EncodingConverter.h"
#endif

#include <cassert>
#include <vector>
#include <algorithm>
#include <string.h>

#include "message.h"

 

#ifndef WIN32
#include <stdio.h>  
#include <sys/types.h>  
#include <sys/socket.h>  
#include <netinet/in.h>  
#include <arpa/inet.h> 
#include <netdb.h>			//gethostbyname
#include <pthread.h>
#include <errno.h>

#endif


#define  LOG messager






void printUTF8String(char* utf8str)
{
	if(!utf8str)
	{
		return ;
	}
	#ifdef WIN32
		//std::string strUtf8  = utf8str;
		//std::string strAscii ;
		//EncodingConverter::Utf8StrToAnsiStr(strUtf8,strAscii);
		LOG(msgINFO, "%s\n",utf8str);
	#else
		LOG(msgINFO,"%s\n",utf8str);
	#endif
}


int http_request(const char* url,std::vector<char>& content_buff, SOCKET* sock)
{
	SOCKET s ;
	int ret = -1;
	s = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
	if(s < 0 )
	{
		LOG(msgINFO,"创建套接字失败\n");
		return -1;
	}

	*sock = s;
	LOG(msgINFO,"http_request sock:%d\n",s);

	

	http_client* httpClient = new_http_client();
	
	do 
	{
		if(httpClient->parse_url(httpClient,url)!= 0 )
		{
			LOG(msgERROR,"parse_url failed\n");
			ret =  -1;
			break;
		}

		struct hostent *server = gethostbyname(httpClient->host);
		if(!server)
		{
			LOG(msgERROR,"gethostbyname failed,host:%s\n",httpClient->host);
			ret =  -1;
			break;
		}

		LOG(msgINFO,"gethostbyname succes\n");
		sockaddr_in addr;
		memset(&addr,0,sizeof(sockaddr_in));
		addr.sin_family=AF_INET;
		addr.sin_port = htons(httpClient->port);
	#ifdef WIN32
		memcpy((char*)&addr.sin_addr.S_un.S_addr,(char*)server->h_addr,server->h_length);
	#else
		bcopy((char *)server->h_addr,
			(char *)&addr.sin_addr.s_addr,
			server->h_length);
	#endif

		LOG(msgINFO,"addr:%s\n",inet_ntoa(addr.sin_addr));

		if(connect(s, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR)
		{
#ifdef WIN32
			LOG(msgINFO,"IPLocation  connect server,failed\n");
#else
			LOG(msgINFO,"IPLocation  connect server,failed,errno:%d\n",errno);
#endif
			ret =  -1;
			break;
		}
		


		char* request_string = httpClient->request_string(httpClient);

		LOG(msgDEBUG,"\nrequest:%s\n  request_stirng_len:%d",request_string,strlen(request_string));

		int len = 0 ;
		if((len = send(s,request_string,strlen(request_string),0)) == SOCKET_ERROR )
		{
			LOG(msgINFO,"IPLocation  send request to server,failed\n");
			ret =  -1;
			break;
		}
		LOG(msgINFO,"send finish,len:%d\n",len);

		char buff[100] = {0};


		while((len = recv(s,buff,sizeof(buff),0))  > 0  )
		{
			
			httpClient->process_data(httpClient,buff,len);
			if(httpClient->finished(httpClient))
			{
				LOG(msgINFO,"httpClient finished \n");
				break;
			}
		}

		LOG(msgDEBUG,"len = %d\n",len);

		LOG(msgDEBUG,"content:\n",httpClient->content_data);
		LOG(msgDEBUG, httpClient->content_data);
		LOG(msgDEBUG,"\n");

		//assert(httpClient->content_len == strlen(httpClient->content_data));

		content_buff.resize(httpClient->content_len);
		std::copy(httpClient->content_data,
			httpClient->content_data + httpClient->content_len,
			content_buff.begin());
		ret =  0;
		break;

	} while (0);


	*sock = -1 ;
	delete_http_client(httpClient);
	return ret;
}


void copydata(char* dstbuff,const char* srcbuff)
{
	strcpy(dstbuff,srcbuff);
}



#if 0 //ndef WIN32
#include <iconv.h>      // iconv_open iconv iconv_close

// UTF-8 转换成 ANSI_X3.110-1983
int UTF8_2_ANSI(char* in, int inLen, char* out, int outLen)
{
    iconv_t cd = iconv_open( "GB2312", "UTF-8" );
    // check cd
    if(cd == (iconv_t)-1 )
        return -1;

    char *pin = in;
    char *pout = out;
    int  inLen_ = inLen + 1;
    int  outLen_ = outLen;

    iconv( cd, &pin, (size_t*)&inLen_, &pout, (size_t*)&outLen_ );
    // check iconv


    iconv_close(cd);

    return 0;
}
#endif

void processCityData(char* dstbuff, char* cityUtf8)
{
	std::string strUtf8  = cityUtf8;
	std::string strAnsi;


	char city_utf8[] = {0xe5,0xb8,0x82,0x0};//"市"
	std::string old_value = city_utf8;


	std::string::size_type   pos(0);   
	if(   (pos=strUtf8.find(old_value))!=std::string::npos   )   
		strUtf8.replace(pos,old_value.length(),"");  


#ifdef  WIN32 
	EncodingConverter::Utf8StrToAnsiStr(strUtf8,strAnsi);
#else
	strAnsi = strUtf8;
#endif

	if(strAnsi.length() >= CITY_LEN-1)
	{
		memcpy(dstbuff,strAnsi.c_str(),CITY_LEN - 1 );
	}
	else
	{
		copydata(dstbuff,strAnsi.c_str());
	}
}



void processCarrierData(char* dstbuff, char* carrirUtf8)
{
	std::string strUtf8  = carrirUtf8;
	std::string strAnsi;


#ifdef  WIN32
	EncodingConverter::Utf8StrToAnsiStr(strUtf8,strAnsi);
#else
	strAnsi = strUtf8;
#endif



	if(strAnsi.length() >= CARRIR_LEN-1)
	{
		memcpy(dstbuff,strAnsi.c_str(),CARRIR_LEN - 1 );
	}
	else
	{
		copydata(dstbuff,strAnsi.c_str());
	}

}



char* processProviceData(char* proviceUtf8)
{
#if 0//def WIN32
	static char* provice[] = {
		"河北",	
		"山西",
		"辽宁",
		"吉林",
		"黑龙江",
		"江苏",
		"浙江",
		"安徽",
		"福建",
		"江西",
		"山东",
		"河南",
		"湖北",
		"湖南",
		"广东",
		"海南",
		"四川",
		"贵州",
		"云南",
		"陕西",
		"甘肃",
		"青海",

		"广西",
		"内蒙古",
		"西藏",
		"宁夏",
		"新疆"
	};



	std::string strUtf8  = proviceUtf8;
	std::string strAnsi;


#ifdef  WIN32
	EncodingConverter::Utf8StrToAnsiStr(strUtf8,strAnsi);
#else
    //char out[256] = {0};
    //UTF8_2_ANSI((char*)strUtf8.c_str(),strUtf8.length(),out,256);    
    //strAnsi = out;
    strAnsi = strUtf8;
#endif





	static char buff[256] = {0};
	strcpy(buff,strAnsi.c_str());

	for(int i = 0 ; i < sizeof(provice) / sizeof(provice[0]) ; i ++ )
	{
		if(strAnsi.find(provice[i]) != std::string::npos)
		{
			return provice[i];
		}
	}
	return buff;

#else
	static  char provice[][32] = {
		{0xe6,0xb2,0xb3,0xe5,0x8c,0x97,0x0},							  //河北 
		{0xe5,0xb1,0xb1,0xe8,0xa5,0xbf,0x0},                              //山西 
		{0xe8,0xbe,0xbd,0xe5,0xae,0x81,0x0},                              //辽宁 
		{0xe5,0x90,0x89,0xe6,0x9e,0x97,0x0},                              //吉林 
		{0xe9,0xbb,0x91,0xe9,0xbe,0x99,0xe6,0xb1,0x9f,0x0},               //黑龙江
		{0xe6,0xb1,0x9f,0xe8,0x8b,0x8f,0x0},                              //江苏 
		{0xe6,0xb5,0x99,0xe6,0xb1,0x9f,0x0},                              //浙江 
		{0xe5,0xae,0x89,0xe5,0xbe,0xbd,0x0},                              //安徽 
		{0xe7,0xa6,0x8f,0xe5,0xbb,0xba,0x0},                              //福建 
		{0xe6,0xb1,0x9f,0xe8,0xa5,0xbf,0x0},                              //江西 
		{0xe5,0xb1,0xb1,0xe4,0xb8,0x9c,0x0},                              //山东 
		{0xe6,0xb2,0xb3,0xe5,0x8d,0x97,0x0},                              //河南 
		{0xe6,0xb9,0x96,0xe5,0x8c,0x97,0x0},                              //湖北 
		{0xe6,0xb9,0x96,0xe5,0x8d,0x97,0x0},                              //湖南 
		{0xe5,0xb9,0xbf,0xe4,0xb8,0x9c,0x0},                              //广东 
		{0xe6,0xb5,0xb7,0xe5,0x8d,0x97,0x0},                              //海南 
		{0xe5,0x9b,0x9b,0xe5,0xb7,0x9d,0x0},                              //四川 
		{0xe8,0xb4,0xb5,0xe5,0xb7,0x9e,0x0},                              //贵州 
		{0xe4,0xba,0x91,0xe5,0x8d,0x97,0x0},                              //云南 
		{0xe9,0x99,0x95,0xe8,0xa5,0xbf,0x0},                              //陕西 
		{0xe7,0x94,0x98,0xe8,0x82,0x83,0x0},                              //甘肃 
		{0xe9,0x9d,0x92,0xe6,0xb5,0xb7,0x0},                              //青海 
		{0xe5,0xb9,0xbf,0xe8,0xa5,0xbf,0x0},                              //广西 
		{0xe5,0x86,0x85,0xe8,0x92,0x99,0xe5,0x8f,0xa4,0x0},               //内蒙古
		{0xe8,0xa5,0xbf,0xe8,0x97,0x8f,0x0},                              //西藏 
		{0xe5,0xae,0x81,0xe5,0xa4,0x8f,0x0},                              //宁夏 
		{0xe6,0x96,0xb0,0xe7,0x96,0x86,0x0},                              //新疆 


	};

	std::string strUtf8  = proviceUtf8;
	

	for(int i = 0 ; i < sizeof(provice) / sizeof(provice[0]) ; i ++ )
	{
		if(strUtf8.find(provice[i]) != std::string::npos)
		{

			return provice[i];

		}
	}

	return  proviceUtf8;

#endif

}

//#define  COPY_PROVICE_DATA(dstbuff,proviceUtf8)	\

void copy_provice_data(char* dstbuff,char* proviceUtf8)
{
	char* ansidata = processProviceData(proviceUtf8) ; 
	if(ansidata)
	{	
#ifdef WIN32
		std::string origin = proviceUtf8;
		std::string dst ;
		EncodingConverter::Utf8StrToAnsiStr(origin,dst);
		copydata(dstbuff,dst.c_str());

#else
		copydata(dstbuff,ansidata);
#endif
	}
	else 
	{ 
		LOG(msgERROR,"convert provice failed ,line:%d",__LINE__); 
	}

}


int sina_parsedata(const std::vector<char>& contentdata,IpQueryReplay* replay)
{

	int ret = 0;
	if(contentdata.size() <= 0 )
	{
		LOG(msgERROR,"sina_parsedata ,data is invalid\n");
		return -1;
	}

	cJSON* j_root = cJSON_Parse(&*contentdata.begin());

	if(!j_root)
	{
		LOG(msgERROR,"sina_parsedata j_root failed\n");
		return -1;
	}


	cJSON* j_country = cJSON_GetObjectItem(j_root,"country");
	if(j_country && j_country->valuestring)
	{
		LOG(msgDEBUG,"sina_parsedata country:%s" ,j_country->valuestring);
	}


	do{
	
		cJSON* j_provice = cJSON_GetObjectItem(j_root,"province");
		if(j_provice && j_provice->valuestring
			&& strlen(j_provice->valuestring) > 0   )
		{
			LOG(msgDEBUG,"sina_parsedata provice:%s" ,j_provice->valuestring);
			copy_provice_data(replay->provice,j_provice->valuestring);

		}
		else
		{
			LOG(msgERROR,"sina_parsedata  ,get province failed");
			ret = -1;
		}

		cJSON* j_city = cJSON_GetObjectItem(j_root,"city");
		if(j_city && j_city->valuestring
			&& strlen(j_city->valuestring) > 0 )
		{
			LOG(msgDEBUG,"sina_parsedata city:%s", j_city->valuestring);
			processCityData(replay->city,j_city->valuestring);
		}
		else
		{

			LOG(msgERROR,"sina_parsedata  ,get city failed");
			if(ret == -1)	//省市都没查到
			{
				break;
			}
		}


		cJSON* j_carrier = cJSON_GetObjectItem(j_root,"isp");
		if(j_carrier && j_carrier->valuestring
			&& strlen(j_carrier->valuestring) > 0 )
		{
			LOG(msgDEBUG,"sina_parsedata isp:%s", j_carrier->valuestring);
			processCarrierData(replay->carrir,j_carrier->valuestring);
		}
		else
		{
			LOG(msgERROR,"sina_parsedata  ,get carrier failed");
			ret = -1;
			break;
		}

	}while(0);
		
	cJSON_Delete(j_root);

	return ret;
}

int sinaLocation(char* ip,IpQueryCxt* ctx)
{

	std::string url = "http://int.dpool.sina.com.cn/iplookup/iplookup.php?format=json&ip=";
	//std::string url = "http://m3.file.xiami.com/h/2WzPHIYaumlmgKSTS4w3SiUc5uHcHzOOxI1pkLIlsR0mIYsBZZz9kpqTRg9X2SeVoVAZF7ud3FRPGOhdLNCjpvWVQgPa";
	
	if(ip)
		url.append(ip);


	std::vector<char> content_buff;
	if(http_request(url.c_str(),content_buff,&(ctx->sock)) < 0 )
	{
		LOG(msgERROR,"sinaLocation\n");
		return -1;
	}

	return sina_parsedata(content_buff,ctx->replay);
}






int taobao_parsedata(const std::vector<char>& contentdata,IpQueryReplay* replay)
{


	int  ret = 0 ;
	if(contentdata.size() <= 0 )
	{
		LOG(msgERROR,"taobao_parsedata ,data is invalid\n");
		return -1;
	}


	std::vector<char>content_buff = contentdata;
	std::vector<char>::iterator  iter =  std::find(content_buff.begin(),content_buff.end(),'{');
	if(iter != content_buff.end())
	{
		content_buff.erase(content_buff.begin(),iter);
	}

	cJSON* j_root = cJSON_Parse(&*content_buff.begin());

	if(!j_root)
	{
		LOG(msgINFO,"cJSON_Parse j_root failed\n");
		return -1;
	}

	do
	{

		cJSON* j_data = cJSON_GetObjectItem(j_root,"data");
		if(!j_data)
		{
			LOG(msgERROR,"taobao_parsedata ,get j_data failed\n");
			ret = -1;
			break;
		}
		

		cJSON* j_provice = cJSON_GetObjectItem(j_data,"region");
		if(j_provice && j_provice->valuestring
			&& strlen(j_provice->valuestring) > 0 )
		{
			LOG(msgDEBUG,"taobao_parsedata isp:%s", j_provice->valuestring);
			copy_provice_data(replay->provice,j_provice->valuestring);
		}
		else
		{
			LOG(msgERROR,"taobao_parsedata ,get region/provice failed\n");
			ret = -1;
		}

		cJSON* j_city = cJSON_GetObjectItem(j_data,"city");
		if(j_city && j_city->valuestring
			&& strlen(j_city->valuestring) > 0  )
		{
			LOG(msgDEBUG,"taobao_parsedata city:%s", j_city->valuestring);
			processCityData(replay->city,j_city->valuestring);
		}
		else
		{
			LOG(msgERROR,"taobao_parsedata ,get city  failed\n");		//省，城市都没取到
			if(ret == -1 )
			{
				break;
			}
		}

		cJSON* j_carrier = cJSON_GetObjectItem(j_data,"isp");
		if(j_carrier && j_carrier->valuestring
			&& strlen(j_carrier->valuestring) > 0 )
		{
			LOG(msgDEBUG,"taobao_parsedata isp:%s", j_carrier->valuestring);
			processCarrierData(replay->carrir,j_carrier->valuestring);
		}
		else
		{
			LOG(msgERROR,"taobao_parsedata ,get carrie  failed\n");		//省，城市都没取到
			ret = -1;			
			break;
			
		}
	}while(0);

	cJSON_Delete(j_root);

	return ret ;
}

int taobaoLocation(char* ip,IpQueryCxt* ctx)
{

	std::string url = "http://ip.taobao.com/service/getIpInfo.php?ip=";

	assert(ip);

	if(ip)
		url.append(ip);


	std::vector<char> content_buff;
	if(http_request(url.c_str(),content_buff,&(ctx->sock)) < 0 )
	{
		LOG(msgERROR,"taobaoLocation\n");
		return -1;
	}

	return taobao_parsedata(content_buff,ctx->replay);
}






//http://pv.sohu.com/cityjson?ie=utf-8

int sohu_parsedata(const std::vector<char>& contentdata,IpQueryCxt* ctx)
{

	if(contentdata.size() <= 0 )
	{
		LOG(msgINFO,"sohu_parsedata ,data is invalid\n");
		return -1;
	}


	std::vector<char>content_buff = contentdata;
	std::vector<char>::iterator  iter =  std::find(content_buff.begin(),content_buff.end(),'{');
	if(iter != content_buff.end())
	{
		content_buff.erase(content_buff.begin(),iter);
	}


	cJSON* j_root = cJSON_Parse(&*content_buff.begin());

	if(!j_root)
	{
		LOG(msgERROR,"sohu_parsedata  j_root failed\n");
		return -1;
	}


	cJSON* j_cip = cJSON_GetObjectItem(j_root,"cip");
	if(j_cip && j_cip->valuestring)
	{
		LOG(msgDEBUG,"sohu_parsedata cip:%s", j_cip->valuestring);
		if(ctx->ip)
		{
			free(ctx->ip);
		}
		ctx->ip = (char*)malloc(strlen(j_cip->valuestring) + 1);
		memset(ctx->ip,0,strlen(j_cip->valuestring) + 1);
		copydata(ctx->ip,j_cip->valuestring);
	}
	else
	{
		LOG(msgERROR,"sohu_parsedata ,get cip failed\n");
		return -1;
	}

	cJSON_Delete(j_root);

	return 0;
}

int sohuGetInternetIp(IpQueryCxt* ctx)
{
	std::vector<char> content_buff;
	if(http_request("http://pv.sohu.com/cityjson?ie=utf-8",content_buff,&(ctx->sock)) < 0 )
	{
		LOG(msgINFO,"sinaLocation\n");
		return -1;
	}
	return sohu_parsedata(content_buff,ctx);
}


#ifdef WIN32
static DWORD WINAPI 
#else
void*
#endif
query_thread(void* lpParam)
{
	int ret = -1;
	IpQueryCxt* ctx = (IpQueryCxt*)lpParam;
	if(ctx && !ctx->quit)
	{
		if(!ctx->replay)
		{
			ctx->replay = (IpQueryReplay*)malloc(sizeof(IpQueryReplay));
			memset(ctx->replay,0,sizeof(IpQueryReplay));		
		}

        if(!ctx->quit)
		    ret = sinaLocation(ctx->ip,ctx);

		if(!ctx->quit && 0 != ret )
		{		

			if(!ctx->ip)	//如果没有指定IP,就先使用sohuGetInternetIp 得到本地IP。
			{
				ret = sohuGetInternetIp(ctx);
				if(ret == 0 )
					ret = taobaoLocation(ctx->ip,ctx);
			}
			else
			{
				ret = taobaoLocation(ctx->ip,ctx);
			}
		}


		if(!ctx->quit && ctx->cb)
			ctx->cb(ctx->replay,ret);

		ctx->result = ret ;
		ctx->threadid =  0;
			
	}
		
	return 0;
}

IpQueryCxt* IPLocation(const char* ip,IpQueryNotifyCB cb)
{	
	IpQueryCxt* ctx = (IpQueryCxt*)malloc(sizeof(IpQueryCxt));
	if(ctx)
	{
		memset(ctx,0,sizeof(IpQueryCxt));
		if(ip && strlen(ip) > 0 )
			ctx->ip = strdup(ip);

		ctx->sock = -1;

		ctx->cb = cb;
#ifdef WIN32
		ctx->threadid = CreateThread(NULL, 0, query_thread, ctx, 0, NULL);
#else
		pthread_create(&(ctx->threadid), NULL, query_thread, ctx);
#endif
		ctx->result = -1 ;
	}

	return ctx ;
}

void destroyQeuryCxt(IpQueryCxt* ctx)
{
	
	if(ctx)
	{
		ctx->quit = 1;
		LOG(msgINFO,"destoryQueryCtx ctx->sock :%d\n",ctx->sock );
		if(ctx->threadid)
		{
#ifdef WIN32
			if(ctx->sock  != -1  )
			{
				closesocket(ctx->sock);
				ctx->sock = -1 ;
			}
			DWORD dwRet = WaitForSingleObject(ctx->threadid, INFINITE);
			if(dwRet == WAIT_OBJECT_0)
			{
				LOG(msgINFO,"Thread exit success!");
			}
			else
			{
				DWORD dwRet = 0;
				GetExitCodeThread(ctx->threadid, &dwRet);
				TerminateThread(ctx->threadid, dwRet);
				LOG(msgINFO,"Thread timeout!");
			}
			CloseHandle(ctx->threadid);
#else
			LOG(msgINFO,"Thread exit !\n");
            if(ctx->sock != -1 )
            {
                close(ctx->sock);
                ctx->sock = -1;
            }
            //pthread_cancel(ctx->threadid);
			void *thread_result;
			pthread_join(ctx->threadid,&thread_result);
			LOG(msgINFO,"Thread exit end!\n");
#endif


			ctx->threadid = 0;
		}

		if(ctx->ip)
		{
			free(ctx->ip);
			ctx->ip = 0 ;
		}

		if(ctx->replay)
		{
			free(ctx->replay);
			ctx->replay = 0 ;
		}	
	}

}
 


int synchronGetLocation(const char* ip,IpQueryReplay& replay)
{
	IpQueryCxt* ctx  =  IPLocation(ip ,0);

#ifdef WIN32
	DWORD dwRet = WaitForSingleObject(ctx->threadid, INFINITE);
	if(dwRet == WAIT_OBJECT_0)
	{
		LOG(msgINFO,"Thread exit success!");
	}
#else
	void *thread_result;
	pthread_join(ctx->threadid,&thread_result);
#endif

	int ret = ctx->result;

	
	replay = *(ctx->replay);

	destroyQeuryCxt(ctx);


	return ret;

}

