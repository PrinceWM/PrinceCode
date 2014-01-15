#include "stdafx.h"
#include "config.h"
#include "natupnp.h"
#include <iostream>
#include <string>
#include <vector>
#include <ctime>
#include "message.h"
#include "def.h"
using namespace std;

#ifdef _UNICODE
#undef _UNICODE
#endif

#ifdef UNICODE
#undef UNICODE
#endif

#include "xmlParser.h"

bool parseUrl(const char* url,string& host,unsigned int* port,string& path);

#define MAX_BUFF_SIZE 102400

#define HTTPMU_HOST_ADDRESS "239.255.255.250"
#define HTTPMU_HOST_PORT ( htons(1900))
#define SEARCH_REQUEST "M-SEARCH * HTTP/1.1\r\n"                  \
	"HOST: 239.255.255.250:1900\r\n"           \
	"MAN: \"ssdp:discover\" \r\n"              \
	"MX: 5 \r\n"                               \
	"ST: urn:schemas-upnp-org:device:WANConnectionDevice:1\r\n"              \
	"\r\n"
//"ST :upnp:rootdevice\r\n"  "ST: urn:schemas-upnp-org:device:WANConnectionDevice:1\r\n"                  \

#define GET_FILE_REQUEST "GET %s HTTP/1.1\r\nHost: %s:%d\r\n\r\n"
//参数1为要下载的文件的相对路径 2和3为主机地址和端口

#define HTTP_OK "200 OK"

//查找controlURL需要查找的device和service
#define DEVICE_TYPE_1  "urn:schemas-upnp-org:device:InternetGatewayDevice:1"
#define DEVICE_TYPE_2  "urn:schemas-upnp-org:device:WANDevice:1"
#define DEVICE_TYPE_3  "urn:schemas-upnp-org:device:WANConnectionDevice:1"
#define SERVICE_TYPE_1 "urn:schemas-upnp-org:service:WANPPPConnection:1"
#define SERVICE_TYPE_2 "urn:schemas-upnp-org:service:WANIPConnection:1"

/*************************
*   SOAP Action 的http头格式
*       参数1 path of controlURL  
*       参数2 host of controlURL  
*       参数3 port of controlURL   
*       参数4 length of content   
*       参数5 type of service    eg. (urn:schemas-upnp-org:service:WANPPPConnection :1) 
*       参数6 action name        eg. (AddPortMapping)
***************************/
#define HTTP_ACTION_HEADER  "POST %s HTTP/1.1\r\n"                      \
	"HOST: %s:%u\r\n"                           \
	"Content-Length: %d\r\n"                    \
	"CONTENT-TYPE: text/xml; charset=\"utf-8\"\r\n"\
	"SOAPACTION: \"%s#%s\"\r\n"                        \
	"\r\n"
//最后一行的\r\n让我调试了半天

/****************************
*  SOAP Action 的http报文体(符合xml规范)
*   参数1 action name
*   参数2 serviceType
*   参数3 action的参数 (下面有宏定义) 
*   参数4 action name
*****************************/
#define SOAP_ACTION         "<?xml version=\"1.0\" ?>\r\n"                       \
	"<s:Envelope xmlns:s="                               \
	/*此行最后一个空格*/        "\"http://schemas.xmlsoap.org/soap/envelope/\" "     \
	"s:encodingStyle="                                   \
	"\"http://schemas.xmlsoap.org/soap/encoding/\">\r\n" \
	"<s:Body> \r\n"                                      \
	"<u:%s xmlns:u=\"%s\"> \r\n"                         \
	"%s"                                                 \
	"</u:%s> \r\n "                                      \
	"</s:Body>\r\n"                                      \
	"</s:Envelope>\r\n"                                  \
	"\r\n"

#define PORT_MAPPING_DESCRIPTION "p2p_upnp_mapping" 


/****************************
*  addPortMapping 的参数列表(符合xml规范)
*   参数1 externPort:unsigned int
*   参数2 Portocol  :TCP or UDP
*   参数3 internalPort:unsigned int 
*   参数4 InternalClient : 本机的ip地址
*****************************/

#define ADD_PORT_MAPPING_ARGS "<NewRemoteHost></NewRemoteHost>\r\n"              \
	"<NewExternalPort>%u</NewExternalPort>\r\n"        \
	"<NewProtocol>%s</NewProtocol>\r\n"                \
	"<NewInternalPort>%u</NewInternalPort>\r\n"        \
	"<NewInternalClient>%s</NewInternalClient>\r\n"    \
	"<NewEnabled>1</NewEnabled>\r\n"                   \
	"<NewPortMappingDescription>"                      \
	PORT_MAPPING_DESCRIPTION                    \
	"</NewPortMappingDescription>\r\n"                 \
	"<NewLeaseDuration>0</NewLeaseDuration>\r\n"       


/****************************
*  delPortMapping 的参数列表(符合xml规范)
*   参数1 externPort:unsigned int
*   参数2 Portocol  :TCP or UDP
*****************************/

#define DEL_PORT_MAPPING_ARGS "<NewRemoteHost></NewRemoteHost>\r\n"              \
	"<NewExternalPort>%u</NewExternalPort>\r\n"        \
	"<NewProtocol>%s</NewProtocol>\r\n"                

/****************************
*   GetExternalIPAddress SOAP_ACTION 的参数列表(符合xml规范)
*   参数1 serviceType
*****************************/
#define GET_EXTERNAL_IP_ADDR_ACTION  "<?xml version=\"1.0\" ?>\r\n"                       \
	"<s:Envelope xmlns:s="                               \
	/*此行最后一个空格*/				 "\"http://schemas.xmlsoap.org/soap/envelope/\" "     \
	"s:encodingStyle="                                   \
	"\"http://schemas.xmlsoap.org/soap/encoding/\">\r\n" \
	"<s:Body> \r\n"                                      \
	"<u:GetExternalIPAddress xmlns:u=\"%s\"> \r\n"                         \
	""                                                 \
	"</u:GetExternalIPAddress> \r\n "                                      \
	"</s:Body>\r\n"                                      \
	"</s:Envelope>\r\n"                                  \
	"\r\n"


#define GET_GENERIC_PORT_MAPPING_ARGS  "<NewPortMappingIndex>%d</NewPortMappingIndex>\r\n"


unsigned int gen_rand()
{
	unsigned int nI = 0; 
	unsigned int nJ = 65535;//随机数的最大值 
	unsigned int nK = 1024; 
	srand(time(NULL)); //取系统时间为随机种子 
	nI = rand()%nJ+nK; //取随机数 
	if(nI>1024&&nI<65535)
		return nI;
	else return 0;
} 
//为了在比较时方便 将协议转换为小写格式
bool myToLower(string protocol,char *proto)
{
	if(strcmp(protocol.c_str(),"UDP")==0||strcmp(protocol.c_str(),"udp")==0)
	{
		strcpy(proto,"udp");
		return true;
	}
	else if(strcmp(protocol.c_str(),"TCP")==0||strcmp(protocol.c_str(),"tcp")==0)
	{
		strcpy(proto,"tcp");
		return true;
	}
	else 
	{
		cerr<<"not a valid protocol"<<endl;

		return false;

	}
}



bool NatUPnP::isInUse(unsigned int port,string protocol)
{
	char proto[10];
	char mProto[10];
	myToLower(protocol,proto);
	//cout<<"test\ntest\ntest\n "<<protocal<<"\t"<<proto<<endl;

	for(unsigned int i=0;i<mapping_infos.size();i++)
	{
		if(mapping_infos[i].externalPort.size()==0)
			continue;
		myToLower(mapping_infos[i].protocol,mProto);
		//cout<<"test\ntest\ntest\n "<<protocal<<"\t"<<proto<<endl;
		if((atoi(mapping_infos[i].externalPort.c_str())==port)&&(strcmp(proto,mProto)==0))
			return true;
	}
	return false;
}

bool NatUPnP::hasBeenMapped(unsigned int port,string protocol)
{
	char proto[10];
	char mProto[10];
	myToLower(protocol,proto);
	for(unsigned int i=0;i<mapping_infos.size();i++)
	{
		if(mapping_infos[i].internalPort.size()==0)
			continue;
		myToLower(mapping_infos[i].protocol,mProto);
		//cout<<"test\ntest\ntest\n "<<protocal<<"\t"<<proto<<endl;
		if((atoi(mapping_infos[i].internalPort.c_str())==port)&&(strcmp(proto,mProto)==0))
		{
			if(strcmp(mapping_infos[i].description.c_str(),PORT_MAPPING_DESCRIPTION)==0&&strcmp(mapping_infos[i].internalClient.c_str(),local_ip.c_str())==0)
				return true;
		}
	}
	return false;
}

bool NatUPnP::init(int timeout,int timeinterval)
{
	time_out=timeout;
	interval=timeinterval;
	status=NAT_INIT;

// 	WSAData wsaData;
// 	if(WSAStartup(MAKEWORD(2,1),&wsaData)!=0)
// 	{
// 		//cerr<<"failed to find winsock2.1 or better:"<<endl;
// 		status=NAT_ERROR;
// 		last_error="init() ERROR:Failed to find Winsock 2.1 or Better!";
// 		return false;
// 	}
	return true;
}

bool NatUPnP::get_localIP()
{
#if 0 //change
	struct sockaddr_in myAddr;
	int len;
	cout<<"get_localIP was called !\n";
	if(getsockname(tcp_socket_fd,(struct sockaddr *)&myAddr,&len)==0)
	{
		status=NAT_ERROR;
		last_error="get_localIP() ERROR:Faild to get localIP!";
		return false;
	}
	local_ip=inet_ntoa(myAddr.sin_addr);
	cout<<"local_ip is "<<local_ip<<"\n";

	return true;

	hostent *he;
	char hostname[50];
	int ret=gethostname(hostname,50);
	if(ret!=0)
	{
		cerr<<"gethostname error"<<endl;
		return false;
	}
	he=gethostbyname(hostname);

	if(!he)
	{
		cerr<<"gethostbyname error"<<endl;
		return false;
	}
	struct in_addr addr;
	memcpy(&addr,he->h_addr_list[0],sizeof(struct in_addr));
	local_ip=inet_ntoa(addr);
	//cout<<local_ip<<endl;
	return true;
#endif 
	local_ip=localIP;

	return true;
}

bool NatUPnP::tcp_connect(const char * address,unsigned int port)
{
	if((tcp_socket_fd=socket(AF_INET,SOCK_STREAM,0))==SOCKET_ERROR)
	{
		status=NAT_ERROR;
		last_error="tcp_connect() ERROR:Failed create SOCKET";
		return false;
	}

	sockaddr_in servAddr;
	servAddr.sin_addr.s_addr=inet_addr(address);
	servAddr.sin_family=AF_INET;
	servAddr.sin_port=htons(port);//

	int ret;

	for(int i=0;i<time_out;i++)
	{
		if(i>1)
                        mySleep(interval);

		ret=connect(tcp_socket_fd,(struct sockaddr *)&servAddr,sizeof(struct sockaddr));

		if(ret==0)
		{
			status =NAT_TCP_CONNECTED;
			return true;
		}	

	}
	status=NAT_ERROR;
	char temp[100];
        mysprintf(temp,100,"tcp_connect() ERROR:Failed to connect to server->%s:%d",address,port);
	last_error=temp;
	return false;
}

bool NatUPnP::discovery()
{
	struct sockaddr_in sAddr;
	sAddr.sin_addr.s_addr=inet_addr(HTTPMU_HOST_ADDRESS);
	//cout<<HTTPMU_HOST_ADDRESS<<endl;
	sAddr.sin_family=AF_INET;
	sAddr.sin_port=(HTTPMU_HOST_PORT);

	udp_socket_fd=socket(AF_INET,SOCK_DGRAM,0);
	if(udp_socket_fd==INVALID_SOCKET)
	{
		status=NAT_ERROR;
		last_error="discovery() ERROR:failed to create udpSocket!";
		return false;
	}

	//cout<<"[Discovery()]:create udp_socket successfull!\n";

	string send_buff=SEARCH_REQUEST;
	string recv_buff;
	bool bOptVal=true;
	int bOptLen=sizeof(bool);

	int ret;
	ret=setsockopt(udp_socket_fd,SOL_SOCKET,SO_BROADCAST,(char *)&bOptVal,bOptLen);
	if(ret == SOCKET_ERROR)
	{
		status=NAT_ERROR;
		last_error="discovery() ERROR: failed to setsockopt to broadcast!";
		return false;
	}
	//cout << "[Discovery()]: set opt to broadcast successfull!\n";

	ret=sendto(udp_socket_fd,send_buff.c_str(),send_buff.size(),0,(struct sockaddr *)&sAddr,sizeof(struct sockaddr));


	struct timeval tv;
	tv.tv_sec=0;
	tv.tv_usec=1000*1000; // 1000 ms 

	fd_set fdSet; 
	unsigned int fdSetSize;


	char buff[MAX_BUFF_SIZE+1];
	int i=0;
	size_t index;
	while(i++<time_out)
	{
		FD_ZERO(&fdSet);
		fdSetSize=0;
		FD_SET(udp_socket_fd,&fdSet); 
		fdSetSize = udp_socket_fd+1;

		int  err = select(fdSetSize, &fdSet, NULL, NULL, &tv);
		if(err<0)
		{
			status=NAT_ERROR;
			last_error="discovery() ERROR: failed to select()";
			return false;
		}
		else if(err==0)
		{
                        mySleep(interval);
			//cout<<"here "<<i<<endl;
			ret=sendto(udp_socket_fd,send_buff.c_str(),send_buff.size(),0,(struct sockaddr *)&sAddr,sizeof(struct sockaddr));
			continue;
		}
		else
		{
			if (( udp_socket_fd!=INVALID_SOCKET )&&( FD_ISSET(udp_socket_fd,&fdSet) ))
			{
				ret=recvfrom(udp_socket_fd,buff,MAX_BUFF_SIZE,0,NULL,NULL);
				if(ret == -1)
					continue;
				recv_buff=buff;

				index=recv_buff.find(HTTP_OK);
				if(index==string::npos)
					continue;
				string::size_type begin=recv_buff.find("http://");
				if(begin==string::npos)
					continue;
				string::size_type end=recv_buff.find("\r",begin);
				if(end==string::npos)
					continue;
				describe_url=describe_url.assign(recv_buff,begin,end-begin);
				cout<<"[Discovery()] : Find a UPnP  device:  "<<describe_url.c_str()<<"\n";
#ifdef MOHO_WIN32
        closesocket(udp_socket_fd);
#elif defined(MOHO_X86)
        close(udp_socket_fd);
#endif
				status=NAT_FOUND;
				return true;
			}
		}
	}
	status=NAT_ERROR;
#ifdef MOHO_WIN32
        closesocket(udp_socket_fd);
#elif defined(MOHO_X86)
        close(udp_socket_fd);
#endif
	last_error="cannot find a upnp device!";
	return false;
}

bool NatUPnP::get_description()
{
	string host,path;
	unsigned int port;
	parseUrl(describe_url.c_str(),host,&port,path);
#if 0
	cout<<"describe_url:"<<describe_url<<"\n";
	cout<<"\thost: "<<host.c_str()<<" port: "<<port<<" path: "<<path.c_str()<<"\n";
#endif
	//开启tcpSocket
	if(!tcp_connect(host.c_str(),port))
		return false;
	char reqbuf[2048];
        mysprintf(reqbuf, 2048, GET_FILE_REQUEST,path.c_str(),host.c_str(),port);
	string req=reqbuf;
#if 0
	cout<<"request msg : \n"<<req.c_str()<<"\n";
#endif

	send(tcp_socket_fd,req.c_str(),req.length(),0);
	string respon;
	char buff[MAX_BUFF_SIZE+1];
	/********************************************************************
	*
	*	阻塞等待服务器回复
	*
	********************************************************************/

	int ret;
	memset(buff,0,MAX_BUFF_SIZE+1);
	while((ret=recv(tcp_socket_fd,buff,MAX_BUFF_SIZE,0))>0)
	{
		respon+=buff;
		memset(buff,0,MAX_BUFF_SIZE+1);
	}
	description_info_xml=respon;
	messager(msgINFO, "uPnP device description: %s", respon.c_str());
	status=NAT_GETDESCRIPTION;
	//关闭tcpSocket
#ifdef MOHO_WIN32
        closesocket(tcp_socket_fd);
#elif defined(MOHO_X86)
        close(tcp_socket_fd);
#endif
	return true;

#if 0
	struct timeval tv;
	tv.tv_sec=0;
	tv.tv_usec=1000*1000; // 1000 ms 

	fd_set fdSet; 
	unsigned int fdSetSize;
	int i=0;
	while(i++<time_out)
	{
		//memset(buff,0,MAX_BUFF_SIZE+1];
		FD_ZERO(&fdSet);
		fdSetSize=0;
		FD_SET(tcp_socket_fd,&fdSet);
		fdSetSize=tcp_socket_fd+1;

		int err=select(fdSetSize,&fdSet,NULL,NULL,&tv);
		if(err<0)
		{
			status=NAT_ERROR;
			last_error="error is select!\n";
#ifdef MOHO_WIN32
        closesocket(tcp_socket_fd);
#elif defined(MOHO_X86)
        close(tcp_socket_fd);
#endif
			return false;
		}
		else if(err==0)
		{
			continue;
		}
		else
		{
			if(tcp_socket_fd!=INVALID_SOCKET&&FD_ISSET(tcp_socket_fd,&fdSet))
			{
				int ret;
				memset(buff,0,MAX_BUFF_SIZE+1);
				if(recv(tcp_socket_fd,buff,MAX_BUFF_SIZE,0)==SOCKET_ERROR)
				{
					status=NAT_ERROR;
					last_error="recv error!\n";
#ifdef MOHO_WIN32
        closesocket(tcp_socket_fd);
#elif defined(MOHO_X86)
        close(tcp_socket_fd);
#endif
					return false;
				}
				resp+=buff;
				description_info_xml=buff;
				status=NAT_GETDESCRIPTION;
				//关闭tcpSocket
#ifdef MOHO_WIN32
        closesocket(tcp_socket_fd);
#elif defined(MOHO_X86)
        close(tcp_socket_fd);
#endif
				return true;
			}
		}
	}

	status=NAT_ERROR;
	last_error="cannot get describtion_xml!\n";
	return false;
#endif
}


bool NatUPnP::parse_description()
{
	XMLNode rNode=XMLNode::parseString(description_info_xml.c_str(),"root");
	if(rNode.isEmpty())
	{
		status=NAT_ERROR;
		last_error="parse_desciption() ERROR: xml is not valid !";
		return false;
	}
	XMLNode urlBaseNode=rNode.getChildNode("URLBase",0);
	if(urlBaseNode.getText())
	{	
		base_url=urlBaseNode.getText();
		//cout<<"TEST:getURLBase :"<<base_url<<endl;
	}
	else
	{
		size_t index=describe_url.find("/",8);
		if(index==string::npos)
		{
			status=NAT_ERROR;
			last_error="parse_desciption() ERROR:cannot get baseurl from describe_url!";
			return false;
		}
		base_url=base_url.assign(describe_url,0,index + 1);
		//cout<<"TEST:parse describe_url to find URLBase:"<<base_url<<endl;
	}

#if 0
	cout<<"baseURL is :\n\t"<<base_url<<"\n";
#endif

	XMLNode device_node,deviceType_node,deviceList_node;


	int num=rNode.nChildNode("device");
	for(int i=0;i<num;i++)
	{
		device_node=rNode.getChildNode("device",i);
		if(device_node.isEmpty())
			continue;
		deviceType_node=device_node.getChildNode("deviceType",0);
		if(strcmp(deviceType_node.getText(),DEVICE_TYPE_1)==0)
			break;
	}
	if(device_node.isEmpty()||strcmp(deviceType_node.getText(),DEVICE_TYPE_1)!=0)
	{
		status=NAT_ERROR;
		char temp[100];
                mysprintf(temp,100,"parse_description() ERROR:\n\tcannot find %s\n!",DEVICE_TYPE_1);
		last_error=temp;
		return false;
	}
	deviceList_node=device_node.getChildNode("deviceList",0);
	if(deviceList_node.isEmpty())
	{
		status=NAT_ERROR;
		char temp[100];
                mysprintf(temp,100,"parse_description() ERROR:\n\tcannot find deviceList for device: %s\n!",DEVICE_TYPE_1);
		last_error=temp;
		return false;
	}


	num=deviceList_node.nChildNode("device");
	for(int i=0;i<num;i++)
	{
		device_node=deviceList_node.getChildNode("device",i);
		if(device_node.isEmpty())
			continue;
		deviceType_node=device_node.getChildNode("deviceType",0);
		if(strcmp(deviceType_node.getText(),DEVICE_TYPE_2)==0)
			break;
	}
	if(device_node.isEmpty()||strcmp(deviceType_node.getText(),DEVICE_TYPE_2)!=0)
	{
		status=NAT_ERROR;
		char temp[200];
                mysprintf(temp,200,"parse_description() ERROR:\n\tcannot find %s\n!",DEVICE_TYPE_2);
		last_error=temp;
		return false;
	}
	deviceList_node=device_node.getChildNode("deviceList",0);
	if(deviceList_node.isEmpty())
	{
		status=NAT_ERROR;
		char temp[200];
                mysprintf(temp,200,"parse_description() ERROR:\n\tcannot find deviceList for device: %s\n!",DEVICE_TYPE_2);
		last_error=temp;
		return false;
	}

	num=deviceList_node.nChildNode("device");

	for(int i=0;i<num;i++)
	{
		device_node=deviceList_node.getChildNode("device",i);
		if(device_node.isEmpty())
			continue;
		deviceType_node=device_node.getChildNode("deviceType",0);
		if(strcmp(deviceType_node.getText(),DEVICE_TYPE_3)==0)
			break;
	}
	if(device_node.isEmpty()||strcmp(deviceType_node.getText(),DEVICE_TYPE_3)!=0)
	{
		status=NAT_ERROR;
		char temp[200];
                mysprintf(temp,200,"parse_description() ERROR:\n\tcannot find %s\n!",DEVICE_TYPE_3);
		last_error=temp;
		return false;
	}

	XMLNode service_node,serviceType_node,serviceList_node;

	serviceList_node=device_node.getChildNode("serviceList",0);
	if(serviceList_node.isEmpty())
	{
		status=NAT_ERROR;
		char temp[200];
                mysprintf(temp,200,"parse_description() ERROR:\n\tcannot find serviceList for device: %s\n!",DEVICE_TYPE_3);
		last_error=temp;
		return false;
	}

	bool isFound=false;
	num=serviceList_node.nChildNode("service");
	for(int i=0;i<num;i++)
	{
		service_node=serviceList_node.getChildNode("service",i);
		if(service_node.isEmpty())
			continue;
		serviceType_node=service_node.getChildNode("serviceType",0);
		if(strcmp(serviceType_node.getText(),SERVICE_TYPE_1)==0||strcmp(serviceType_node.getText(),SERVICE_TYPE_2)==0)
		{
			isFound=true;
			break;
		}
	}
	if(service_node.isEmpty())
	{
		status=NAT_ERROR;
		char temp[200];
                mysprintf(temp,200,"parse_description() ERROR:\n\tcannot find the twoservice:%s\n\t\t\t%s",SERVICE_TYPE_1,SERVICE_TYPE_2);
		last_error=temp;
		return false;
	}
	if(isFound)
	{
		XMLNode controlURL_node=service_node.getChildNode("controlURL",0);
		XMLNode SCPDURL_node=service_node.getChildNode("SCPDURL",0);
		if(controlURL_node.isEmpty())
		{
			status=NAT_ERROR;
			last_error="parse_description() ERROR:\ncontrolURL is empty!";
			return false;
		}
		if(SCPDURL_node.isEmpty())
		{
			status=NAT_ERROR;
			last_error="parse_description() ERROR:\nSCPDURL_node is empty!";
			return false;
		}
		service_type=serviceType_node.getText();
		control_url=controlURL_node.getText();
		service_describe_url=SCPDURL_node.getText();
		if(control_url.find("http://")==string::npos||control_url.find("HTTP://")==string::npos)
		{
			control_url=base_url+control_url.substr(1, control_url.size()-1);
			//cout<<"control_url is "<<control_url<<endl;
		}
		if(service_describe_url.find("http://")==string::npos||service_describe_url.find("HTTP://")==string::npos)
		{
			service_describe_url=base_url+service_describe_url.substr(1, service_describe_url.size() - 1);
			//cout<<"service_describe_url"<<service_describe_url<<endl;
		}

		status=NAT_GETCONTROL;
		return true;
	}
	else
	{
		status=NAT_ERROR;
		char temp[200];
                mysprintf(temp,200,"parse_description() ERROR:\n\tcannot find the two service:%s\n\t\t\t%s",SERVICE_TYPE_1,SERVICE_TYPE_2);
		last_error=temp;
		return false;
	}
}

bool NatUPnP::add_port_mapping(unsigned int port,const char * protocal,unsigned int * externPort)
{
	string host,path;
	unsigned int c_port;
	if(!parseUrl(control_url.c_str(),host,&c_port,path))
	{
		status=NAT_ERROR;
		last_error="add_port_mapping ERROR:cannot get invalid controlURL!";
		return false;
	}
#if 0
	cout<<"parse control url to"<<host.c_str()<<":"<<c_port<<"\n";
#endif	


	char temp[MAX_BUFF_SIZE+1];


	if(!get_port_mapping())
	{
		status=NAT_ERROR;
		last_error="add_port_mapping() ERROR: get mappings error";
		return false;
	}
	if(hasBeenMapped(port,protocal))
	{
		status=NAT_ERROR;
		last_error="add_port_mapping() ERROR: internal port has been mapped!\n";
		return false;
	}
	if(!tcp_connect(host.c_str(),c_port))
	{
		return false;
	}
	if(!get_localIP())
	{
		return false;
	}

	//生成一个合法的随机端口
	unsigned int ran_port;
	while(true)
	{
		ran_port=gen_rand();
		if(ran_port<1024||ran_port>65535)
			continue;
		if(!isInUse(ran_port,protocal))
			break;
	}
	//cout<<"ran_port is "<<ran_port<<endl;
	//cout<<"local_ip is :"<<local_ip.c_str()<<endl;

	memset(temp,0,MAX_BUFF_SIZE+1);
        mysprintf(temp,MAX_BUFF_SIZE,ADD_PORT_MAPPING_ARGS,ran_port,protocal,port,local_ip.c_str());
	//参数顺序 1外部端口 2协议类型 3内部端口 4本机ip地址
	string action_args=temp;
	string soap_action;
	string http_header;

#if 0
	cout<<"action_args is \n"<<action_args.c_str()<<endl;
#endif

	memset(temp,0,MAX_BUFF_SIZE+1);
        mysprintf(temp,MAX_BUFF_SIZE,SOAP_ACTION,"AddPortMapping",service_type.c_str(),action_args.c_str(),"AddPortMapping");
	//参数 1 action name参数2 serviceType 参数3action的参数  参数4 action name
	soap_action=temp;
#if 0
	cout<<"soap_action is \n"<<soap_action.c_str()<<endl;
#endif

	memset(temp,0,MAX_BUFF_SIZE+1);
        mysprintf(temp,MAX_BUFF_SIZE,HTTP_ACTION_HEADER,path.c_str(),host.c_str(),c_port,soap_action.length(),service_type.c_str(),"AddPortMapping");
	http_header=temp;

#if 0
	cout<<"http_header is \n"<<http_header.c_str()<<endl;
#endif

	string add_port_msg=http_header+soap_action;
#if 0
	cout<<"add_port_msg is \n"<<add_port_msg.c_str()<<endl;
#endif
	messager(msgINFO, "send add_port: %s", add_port_msg.c_str());
	if(send(tcp_socket_fd,add_port_msg.c_str(),add_port_msg.size(),0)==SOCKET_ERROR)
	{
                cerr<<"发送add_port_msg 出错 errno :\n"<<myGetLastError()<<"\n";
#ifdef MOHO_WIN32
        closesocket(tcp_socket_fd);
#elif defined(MOHO_X86)
        close(tcp_socket_fd);
#endif
		return false;
	}

	char buff[MAX_BUFF_SIZE+1];
	string resp;
	memset(buff,0,MAX_BUFF_SIZE+1);
        mySleep(1000);
	while(recv(tcp_socket_fd,buff,MAX_BUFF_SIZE,0) >0 )//阻塞模式？？？？
	{
		resp+=buff;
		memset(buff,0,MAX_BUFF_SIZE+1);
	}

	messager(msgINFO, "add_map_reply: %s", buff);
	if(resp.find(HTTP_OK)==string::npos)
	{
		status=NAT_ERROR;
		last_error="add port mapping error";
#ifdef MOHO_WIN32
        closesocket(tcp_socket_fd);
#elif defined(MOHO_X86)
        close(tcp_socket_fd);
#endif
		return false;
	}
	*externPort=ran_port;
#if 0
	cout<<"extern port is "<<ran_port<<"\n";
#endif
#ifdef MOHO_WIN32
        closesocket(tcp_socket_fd);
#elif defined(MOHO_X86)
        close(tcp_socket_fd);
#endif

	pri_mapping_info mInfo;
	mInfo.exPort=*externPort;
	mInfo.inPort=port;
	mInfo.protocal=protocal;
	mInfo.isDelete=false;
	pri_mapping_infos.push_back(mInfo);

	status=NAT_ADD;
	return true;		
}

bool NatUPnP::del_port_mapping(unsigned int port, const char *protocal)
{
	string path,host;
	unsigned int c_port;
	if(!parseUrl(control_url.c_str(),host,&c_port,path))
	{
		return false;
	}
#if 0
	cout<<"parse control url to"<<host.c_str()<<":"<<c_port<<"\n";
#endif	
	if(!tcp_connect(host.c_str(),c_port))
	{
		return false;
	}
	string action_args;
	string soap_action;
	string http_header;
	char temp[MAX_BUFF_SIZE+1];
        mysprintf(temp,MAX_BUFF_SIZE,DEL_PORT_MAPPING_ARGS,port,protocal);
	action_args=temp;

	memset(temp,0,MAX_BUFF_SIZE+1);
        mysprintf(temp,MAX_BUFF_SIZE,SOAP_ACTION,"DeletePortMapping",service_type.c_str(),action_args.c_str(),"DeletePortMapping");
	soap_action=temp;

#if 0
	cout<<"soap_action is \n"<<soap_action.c_str()<<endl;
#endif

	memset(temp,0,MAX_BUFF_SIZE+1);
        mysprintf(temp,MAX_BUFF_SIZE,HTTP_ACTION_HEADER,path.c_str(),host.c_str(),c_port,soap_action.size(),service_type.c_str(),"DeletePortMapping");
	http_header=temp;

#if 0
	cout<<"http_header is \n"<<http_header.c_str()<<endl;
#endif

	string del_port_msg=http_header+soap_action;

#if 0
	cout<<"del_port_msg is \n"<<del_port_msg.c_str()<<endl;
#endif

	if(send(tcp_socket_fd,del_port_msg.c_str(),del_port_msg.size(),0)==SOCKET_ERROR)
	{
                cerr<<"发送del_port_msg 出错 errno :\n"<<myGetLastError()<<"\n";
		return false;
	}
	char buff[MAX_BUFF_SIZE+1];
	string resp; 
	memset(buff,0,MAX_BUFF_SIZE+1);
        mySleep(100);
	while(recv(tcp_socket_fd,buff,MAX_BUFF_SIZE,0)>0)
	{
		resp+=buff;
		memset(buff,0,MAX_BUFF_SIZE+1);
	}
	if(resp.find(HTTP_OK)==string::npos)
	{
		status=NAT_ERROR;
		last_error="del port mapping error";
#ifdef MOHO_WIN32
        closesocket(tcp_socket_fd);
#elif defined(MOHO_X86)
        close(tcp_socket_fd);
#endif
		return false;
	}
	status=NAT_DEL;
#ifdef MOHO_WIN32
        closesocket(tcp_socket_fd);
#elif defined(MOHO_X86)
        close(tcp_socket_fd);
#endif
	/***********************************************************************
	*
	*        在自己添加的映射表中 标记删除由自己添加的映射
	*
	**********************************************************************/
	for(unsigned int i=0;i<pri_mapping_infos.size();i++)
	{
		if(pri_mapping_infos[i].exPort==port)
		{
			pri_mapping_infos[i].isDelete=true;
			break;
		}
	}
	return true;
}

bool NatUPnP::get_external_ip()
{
	string host,path;
	unsigned int c_port;
	if(!parseUrl(control_url.c_str(),host,&c_port,path))
	{
		status=NAT_ERROR;
		last_error="get_external_ip() ERROR:cannot get invalid controlURL!";
		return false;
	}
	if(!tcp_connect(host.c_str(),c_port))
	{
		return false;
	}
	string soap_action;
	string http_header;
	char temp[MAX_BUFF_SIZE+1];
        mysprintf(temp,MAX_BUFF_SIZE,GET_EXTERNAL_IP_ADDR_ACTION,service_type.c_str());
	soap_action=temp;
#if 0
	cout<<"soap_action is"<<soap_action<<"\n";
#endif 

	memset(temp,0,MAX_BUFF_SIZE+1);
        mysprintf(temp,MAX_BUFF_SIZE,HTTP_ACTION_HEADER,path.c_str(),host.c_str(),c_port,soap_action.length(),service_type.c_str(),"GetExternalIPAddress");
	http_header=temp;
#if 0
	cout<<"http_header is"<<http_header<<"\n";
#endif 

	string get_external_ip_msg=http_header+soap_action;
#if 0
	cout<<"get_external_ip_msg is"<<get_external_ip_msg<<"\n";
#endif 

	int send_len=send(tcp_socket_fd,get_external_ip_msg.c_str(),get_external_ip_msg.length(),0);
	if(send_len==SOCKET_ERROR)
	{
		//cerr<<"发送get_external_ip_msg错误\n";
		status=NAT_ERROR;
		last_error="发送get_external_ip_msg错误\n";
#ifdef MOHO_WIN32
        closesocket(tcp_socket_fd);
#elif defined(MOHO_X86)
        close(tcp_socket_fd);
#endif
		return false;
	}

        mySleep(300);
	char buff[MAX_BUFF_SIZE+1];
	memset(buff,0,MAX_BUFF_SIZE+1);
	string resp;
	while(recv(tcp_socket_fd,buff,MAX_BUFF_SIZE,0)>0)
	{
		resp=resp+buff;
		memset(buff,0,MAX_BUFF_SIZE+1);
	}
#if 0
	cout<<"respons msg is \n";
	cout<<resp<<"\n";
#endif
	if(resp.find(HTTP_OK)==string::npos)
	{
		status=NAT_ERROR;
		last_error="get_external_ip 未成功!\n";
#ifdef MOHO_WIN32
        closesocket(tcp_socket_fd);
#elif defined(MOHO_X86)
        close(tcp_socket_fd);
#endif
		return false;
	}

	status=NAT_FOUND_EXTERNALIP;
#ifdef MOHO_WIN32
        closesocket(tcp_socket_fd);
#elif defined(MOHO_X86)
        close(tcp_socket_fd);
#endif
	/*********************************************************************
	*      分析传回的文档 提取出externalIp 存入变量external_ip中。
	*******************************************************************/
	XMLNode node;
	XMLNode cnode;
	node=XMLNode::parseString(resp.c_str(),"SOAP-ENV:Envelope");
	if(node.isEmpty())
	{
		node=XMLNode::parseString(resp.c_str(),"s:Envelope");
		if(node.isEmpty())
		{
			status=NAT_ERROR;
			last_error="Fail to get externIP from response data.(NO \"SOAP-ENV:Envelope|s:Envelope\" node.)\n";
			return false;
		}
	}

	cnode=node.getChildNode("SOAP-ENV:Body");
	if(cnode.isEmpty())
	{
		cnode=node.getChildNode("s:Body");
		if(cnode.isEmpty())
		{
			status=NAT_ERROR;
			last_error="Fail to get externIP from response data.(NO \"SOAP-ENV:Body|s:Body\" node.)\n";
			return false;
		}
	}

	node=cnode;
	node=node.getChildNode("u:GetExternalIPAddressResponse");
	if(node.isEmpty())
	{
		status=NAT_ERROR;
		last_error="Fail to get externalIp from response data.(NO \"u:GetGernericPortMappingEntryResponse\" node.)\n";
		return false;
	}

	cnode=node.getChildNode("NewExternalIPAddress");
	if(cnode.isEmpty())
	{
		status=NAT_ERROR;
		last_error="Fail to get externalIP from response data.(NO \"u:NewExternalIPAddress\" node.)\n";
		return false;
	}
	external_ip=cnode.getText();
#if 0
	cout<<"parse respon msg to get external_ip : "<<external_ip;
#endif
	/*******************************************************************/
	return true;
}


bool NatUPnP::del_port_mapping_all()
{
	string host,path;
	unsigned int c_port;
	if(!parseUrl(control_url.c_str(),host,&c_port,path))
	{
		status=NAT_ERROR;
		last_error="add_port_mapping ERROR:cannot get invalid controlURL!";
		return false;
	}
#if 0
	cout<<"parse control url to"<<host.c_str()<<":"<<c_port<<"\n";
#endif	

	if(!get_port_mapping())
	{
		status=NAT_ERROR;
		last_error="add_port_mapping() ERROR: get mappings error";
		return false;
	}
	if(!get_localIP())
	{
		return false;
	}
	for(unsigned int i=0;i<mapping_infos.size();i++)
	{
		if(strcmp(mapping_infos[i].description.c_str(),PORT_MAPPING_DESCRIPTION)==0)
		{
			if(strcmp(mapping_infos[i].internalClient.c_str(),local_ip.c_str())==0)
				if(!del_port_mapping(atoi(mapping_infos[i].externalPort.c_str()),mapping_infos[i].protocol.c_str()))
				{
					return false;
				}
				cout<<"del port mapping "<<mapping_infos[i].protocol<<":"<<mapping_infos[i].internalPort<<"-->"<<mapping_infos[i].externalPort<<"\n";
		}
	}
	return true;
}





bool NatUPnP::get_port_mapping()
{
	mapping_infos.clear();
	string host,path;
	unsigned int c_port;
	if(!parseUrl(control_url.c_str(),host,&c_port,path))
	{
		status=NAT_ERROR;
		last_error="get_port_mapping ERROR:cannot get invalid controlURL!";
		return false;
	}
#if 0
	cout<<"parse control url to"<<host.c_str()<<":"<<c_port<<"\n";
#endif	

	if(!get_localIP())
	{
		return false;
	}
	int index=0;
	while(true)
	{
		if(!tcp_connect(host.c_str(),c_port))
		{
			return false;
		}
		char temp[MAX_BUFF_SIZE+1];
                mysprintf(temp,MAX_BUFF_SIZE,GET_GENERIC_PORT_MAPPING_ARGS,index);

		string action_args=temp;
		string soap_action;
		string http_header;

#if 0
		cout<<"action_args is \n"<<action_args.c_str()<<endl;
#endif

		memset(temp,0,MAX_BUFF_SIZE+1);

                mysprintf(temp,MAX_BUFF_SIZE,SOAP_ACTION,"GetGenericPortMappingEntry",service_type.c_str(),action_args.c_str(),"GetGenericPortMappingEntry");
		//参数 1 action name参数2 serviceType 参数3 action的参数  参数4 action name
		soap_action=temp;
#if 0
		cout<<"soap_action is \n"<<soap_action.c_str()<<endl;
#endif

		memset(temp,0,MAX_BUFF_SIZE+1);
                mysprintf(temp,MAX_BUFF_SIZE,HTTP_ACTION_HEADER,path.c_str(),host.c_str(),c_port,soap_action.length(),service_type.c_str(),"GetGenericPortMappingEntry");
		http_header=temp;

#if 0
		cout<<"http_header is \n"<<http_header.c_str()<<endl;
#endif

		string get_port_mapping_msg=http_header+soap_action;
#if 0
		cout<<"[index]: "<<index<<"\nGET_PORT_MAPPING_MSG is \n"<<get_port_mapping_msg.c_str()<<endl;
#endif

		messager(msgINFO, "get map entry: %s", get_port_mapping_msg.c_str());
		int send_len=send(tcp_socket_fd,get_port_mapping_msg.c_str(),get_port_mapping_msg.size(),0);
		if(send_len==SOCKET_ERROR)
		{
                        cerr<<"发送 get_port_mapping_msg 出错 errno :\n"<<myGetLastError()<<"\n";
			return false;
		}

		//cout<<"get_port_mapping_msg.size() is :"<<get_port_mapping_msg.size()<<"\n";
		//cout<<"send msg length is : " <<send_len<<"\n";

                mySleep(100);

		char buff[MAX_BUFF_SIZE+1];
		string resp;

		memset(buff,0,MAX_BUFF_SIZE+1);
		int recv_len;
		while((recv_len=recv(tcp_socket_fd,buff,MAX_BUFF_SIZE,0))>0)
		{
			//cout<<recv_len<<endl;

			//cout<<buff<<endl;
			resp+=buff;
			memset(buff,0,MAX_BUFF_SIZE+1);
		}
		messager(msgINFO, "get entry reply: %s", resp.c_str());
		mapping_info_str=resp;
		if(mapping_info_str.find(HTTP_OK)==string::npos)
		{
			status=NAT_ERROR;
			char temp[200];
                        mysprintf(temp,200,"totally %d mapping entries!",index);
			last_error=temp;
			return true;
		}
		if(!parse_mapping_info())
			return false;

		index++;                               //get next mapping entry
		mapping_info_str="";
		status=NAT_GET;
#ifdef MOHO_WIN32
        closesocket(tcp_socket_fd);
#elif defined(MOHO_X86)
        close(tcp_socket_fd);
#endif

	}

	status=NAT_GET;
#if 0
	print_help();
#endif
	return true;

}

bool NatUPnP::parse_mapping_info()
{

#if 0
	cout<<"mapping info cstr \n"<<mapping_info_str.c_str()<<endl;
#endif

	XMLNode node;
	XMLNode cnode;
	node=XMLNode::parseString(mapping_info_str.c_str(),"SOAP-ENV:Envelope");
	if(node.isEmpty())
	{
		node=XMLNode::parseString(mapping_info_str.c_str(),"s:Envelope");
		if(node.isEmpty())
		{
			status=NAT_ERROR;
			last_error="Fail to get mapping info from response data.(NO \"SOAP-ENV:Envelope|s:Envelope\" node.)\n";
			return false;
		}
	}

	cnode=node.getChildNode("SOAP-ENV:Body");
	if(cnode.isEmpty())
	{
		cnode=node.getChildNode("s:Body");
		if(cnode.isEmpty())
		{
			status=NAT_ERROR;
			last_error="Fail to get mapping info from response data.(NO \"SOAP-ENV:Body|s:Body\" node.)\n";
			return false;
		}
	}
	node=cnode;

	node=node.getChildNode("u:GetGenericPortMappingEntryResponse");
	if(node.isEmpty())
	{
		status=NAT_ERROR;
		last_error="Fail to get mapping info from response data.(NO \"u:GetGernericPortMappingEntryResponse\" node.)\n";
		return false;
	}
	struct mapping_info instance;

	cnode=node.getChildNode("NewExternalPort");
	if(cnode.isEmpty())
	{
		status=NAT_ERROR;
		last_error="Fail to get mapping info from response data.(NO \"u:NewExternalPort\" node.)\n";
		return false;
	}
	instance.externalPort=(cnode.getText()?cnode.getText():"");

	cnode=node.getChildNode("NewInternalPort");
	if(cnode.isEmpty())
	{
		status=NAT_ERROR;
		last_error="Fail to get mapping info from response data.(NO \"u:NewInternalPort\" node.)\n";
		return false;
	}
	instance.internalPort=cnode.getText()?cnode.getText():"";

	cnode=node.getChildNode("NewProtocol");
	if(cnode.isEmpty())
	{
		status=NAT_ERROR;
		last_error="Fail to get mapping info from response data.(NO \"u:NewProtocol\" node.)\n";
		return false;
	}
	instance.protocol=cnode.getText()?cnode.getText():"";

	cnode=node.getChildNode("NewPortMappingDescription");
	if(cnode.isEmpty())
	{
		status=NAT_ERROR;
		last_error="Fail to get mapping info from response data.(NO \"u:NewPortMappingDescription\" node.)\n";
		return false;
	}
	instance.description=cnode.getText()?cnode.getText():"";

	cnode=node.getChildNode("NewInternalClient");
	if(cnode.isEmpty())
	{
		status=NAT_ERROR;
		last_error="Fail to get mapping info from response data.(NO \"u:NewInternalClient\" node.)\n";
		return false;
	}
	instance.internalClient=cnode.getText()?cnode.getText():"";

	mapping_infos.push_back(instance);

	return true;
}


void NatUPnP::print_help()
{
	int num=mapping_infos.size();
	cout<<"总共 "<<num<<" mapping entries :\n";
	for(int i=0;i<num;i++)
	{
		printf("第[%d]个\n",i+1);
		printf("  协议描述  外部端口  内部端口         映射描述            内部地址    \n");
		printf("%8s",mapping_infos[i].protocol.c_str());
		printf("%10s",mapping_infos[i].externalPort.c_str());
		printf("%10s",mapping_infos[i].internalPort.c_str());
		printf("%25s",mapping_infos[i].description.c_str());
		printf("%20s",mapping_infos[i].internalClient.c_str());
		printf("\n");
	}
	return ;

}

void NatUPnP::print_help_me()
{
	int num=pri_mapping_infos.size();
	cout<<"总共 "<<num<<" ME--mapping entries :\n";
	for(int i=0;i<num;i++)
	{
		printf("第[%d]个\n",i+1);
		printf("  协议描述  外部端口  内部端口 isDeleted\n");
		printf("%8s",pri_mapping_infos[i].protocal.c_str());
		printf("%10d",pri_mapping_infos[i].exPort);
		printf("%10d",pri_mapping_infos[i].inPort);
		printf("%10s",pri_mapping_infos[i].isDelete?"true":"false");

		printf("\n");
	}
	return ;
}

bool parseUrl(const char* url,string& host,unsigned int* port,string& path)
{
	string str_url=url;

	string::size_type pos1,pos2,pos3;
	pos1=str_url.find("://");
	if(pos1==string::npos)
	{
		return false;
	}
	pos1=pos1+3;


	pos2=str_url.find(":",pos1);
	if(pos2==string::npos)
	{
		*port=80;
		pos3=str_url.find("/",pos1);
		if(pos3==string::npos)
		{
			return false;
		}

		host=str_url.substr(pos1,pos3-pos1);

	}
	else
	{

		host=str_url.substr(pos1,pos2-pos1);
		pos3=str_url.find("/",pos1);
		if(pos3==string::npos)
		{
			return false;
		}

		string str_port=str_url.substr(pos2+1,pos3-pos2-1);
		*port=(unsigned int)atoi(str_port.c_str());
	}

	if(pos3+1>=str_url.size())
	{
		path="/";
	}
	else
	{

		path=str_url.substr(pos3,str_url.size());
	}	

	return true;
}
