#include "stdafx.h"
#ifdef MOHO_WIN32
#include <winsock.h>
#include <winsock2.h>
#elif defined(MOHO_X86)
#include <stdio.h>
#include <fcntl.h>
#include <sys/socket.h>
#endif
#include <iostream>
#include <string>

#include "upnpnat.h"
#include "xmlParser.h"
#include "message.h"
#include "getgateway.h"

#define MAX_BUFF_SIZE 102400




static bool parseUrl(const char* url,std::string& host,unsigned short* port,std::string& path)
{
	std::string str_url=url;
	
	std::string::size_type pos1,pos2,pos3;
	pos1=str_url.find("://");
	if(pos1==std::string::npos)
	{
		return false;
	}
	pos1=pos1+3;

	pos2=str_url.find(":",pos1);
	if(pos2==std::string::npos)
	{
		*port=80;
		pos3=str_url.find("/",pos1);
		if(pos3==std::string::npos)
		{
			return false;
		}
		
		host=str_url.substr(pos1,pos3-pos1);
	}
	else
	{
		host=str_url.substr(pos1,pos2-pos1);
		pos3=str_url.find("/",pos1);
		if(pos3==std::string::npos)
		{
			return false;
		}
		
		std::string str_port=str_url.substr(pos2+1,pos3-pos2-1);
		*port=(unsigned short)atoi(str_port.c_str());
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


/******************************************************************
** Discovery Defines                                                 *
*******************************************************************/
#define HTTPMU_HOST_ADDRESS "239.255.255.250"
#define HTTPMU_HOST_PORT 1900
#if 0
#define SEARCH_REQUEST_STRING	"M-SEARCH * HTTP/1.1\r\n"            \
								"ST: ssdp:all\r\n"             \
								"MX: 3\r\n"                          \
								"Man:\"ssdp:discover\"\r\n"          \
								"HOST: 239.255.255.250:1900\r\n"     \
															"\r\n"
#else
#define SEARCH_REQUEST_STRING "M-SEARCH * HTTP/1.1\r\n"            \
	"ST: urn:schemas-upnp-org:service:WANIPConnection:1\r\n" \
	"MX: 10\r\n"                          \
	"Man: \"ssdp:discover\"\r\n"          \
	"HOST: 239.255.255.250:1900\r\n"\
	"\r\n"

#define SEARCH_REQUEST_STRING_2 "M-SEARCH * HTTP/1.1\r\n"            \
	"ST: urn:schemas-upnp-org:service:WANPPPConnection:1\r\n" \
	"MX: 10\r\n"                          \
	"Man: \"ssdp:discover\"\r\n"          \
	"HOST: 239.255.255.250:1900\r\n"\
	"\r\n"
	
#endif
#define HTTP_OK "200 OK"
#define DEFAULT_HTTP_PORT 80


/******************************************************************
** Device and Service  Defines                                                 *
*******************************************************************/

#define DEVICE_TYPE_1	"urn:schemas-upnp-org:device:InternetGatewayDevice:1"
#define DEVICE_TYPE_2	"urn:schemas-upnp-org:device:WANDevice:1"
#define DEVICE_TYPE_3	"urn:schemas-upnp-org:device:WANConnectionDevice:1"

#define SERVICE_WANIP	"urn:schemas-upnp-org:service:WANIPConnection:1"
#define SERVICE_WANPPP	"urn:schemas-upnp-org:service:WANPPPConnection:1" 


/******************************************************************
** Action Defines                                                 *
*******************************************************************/
#define HTTP_HEADER_ACTION "POST %s HTTP/1.1\r\n"                         \
                           "HOST: %s:%u\r\n"                                  \
                           "SOAPACTION: \"%s#%s\"\r\n"                           \
                           "CONTENT-TYPE: text/xml ; charset=\"utf-8\"\r\n"\
                           "Content-Length: %d \r\n\r\n"


#define SOAP_ACTION  "<?xml version=\"1.0\" encoding=\"utf-8\"?>\r\n"     \
                     "<s:Envelope xmlns:s="                               \
                     "\"http://schemas.xmlsoap.org/soap/envelope/\" "     \
                     "s:encodingStyle="                                   \
                     "\"http://schemas.xmlsoap.org/soap/encoding/\">\r\n" \
                     "<s:Body>\r\n"                                       \
                     "<u:%s xmlns:u=\"%s\">\r\n%s"         \
                     "</u:%s>\r\n"                                        \
                     "</s:Body>\r\n"                                      \
                     "</s:Envelope>\r\n"


#define PORT_MAPPING_LEASE_TIME "0"                                //one week

#define ADD_PORT_MAPPING_PARAMS "<NewRemoteHost></NewRemoteHost>\r\n"      \
                                "<NewExternalPort>%u</NewExternalPort>\r\n"\
                                "<NewProtocol>%s</NewProtocol>\r\n"        \
                                "<NewInternalPort>%u</NewInternalPort>\r\n"\
                                "<NewInternalClient>%s</NewInternalClient>\r\n"  \
                                "<NewEnabled>1</NewEnabled>\r\n"           \
                                "<NewPortMappingDescription>%s</NewPortMappingDescription>\r\n"  \
                                "<NewLeaseDuration>"                       \
                                PORT_MAPPING_LEASE_TIME                    \
                                "</NewLeaseDuration>\r\n"

#define DEL_PORT_MAPPING_ARGS "<NewRemoteHost></NewRemoteHost>\r\n"              \
	"<NewExternalPort>%u</NewExternalPort>\r\n"        \
	"<NewProtocol>%s</NewProtocol>\r\n"    

#define ACTION_ADD	 "AddPortMapping"
#define ACTION_DEL	 "DeletePortMapping"
//*********************************************************************************


bool UPNPNAT::init(int time,int inter)
{
	time_out=time;
	interval=inter;
	status=NAT_INIT;
#ifdef MOHO_WIN32
    WORD wVersionRequested;
    WSADATA wsaData;
    int err;
    wVersionRequested = MAKEWORD (2, 2);
    err = WSAStartup (wVersionRequested, &wsaData);
	if(err != 0)
		return false;
#endif
	return true;
}

bool UPNPNAT::tcp_connect(const char * _host,unsigned short int _port)
{
	int ret,i;
	tcp_socket_fd=socket(AF_INET,SOCK_STREAM,0);
	struct sockaddr_in r_address;

    r_address.sin_family = AF_INET;
	r_address.sin_port=htons(_port);
    r_address.sin_addr.s_addr=inet_addr(_host);

	for(i=1;i<=time_out;i++)
	{	
		if(i>1)
                        mySleep(1000);
		
		ret=connect(tcp_socket_fd,(const struct sockaddr *)&r_address,sizeof(struct sockaddr_in) );
		if(ret==0)
		{
			status=NAT_TCP_CONNECTED;
			return true;
		}
	}

	status=NAT_ERROR;
	char temp[100];
	sprintf(temp,"Fail to connect to %s:%i (using TCP)\n",_host,_port);
	last_error=temp;

	return false;	
}

bool UPNPNAT::discovery()
{
    udp_socket_fd=socket(AF_INET,SOCK_DGRAM,0);
	
    int ret;
	int i = 0 ;
    std::string send_buff=SEARCH_REQUEST_STRING;
	std::string send_buff_2=SEARCH_REQUEST_STRING_2;
    std::string recv_buff;
    char buff[MAX_BUFF_SIZE+1]; //buff should be enough big
   
	

    struct sockaddr_in r_address;
    r_address.sin_family=AF_INET;
    r_address.sin_port=htons(HTTPMU_HOST_PORT);
    r_address.sin_addr.s_addr=inet_addr(HTTPMU_HOST_ADDRESS);


    bool bOptVal = true;
    int bOptLen = sizeof(bool);


    ret=setsockopt(udp_socket_fd, SOL_SOCKET, SO_BROADCAST, (char*)&bOptVal, bOptLen); 
	


    ret=sendto(udp_socket_fd,send_buff.c_str(),send_buff.size(),0,(struct sockaddr*)&r_address,sizeof(struct sockaddr_in));
	ret=sendto(udp_socket_fd,send_buff_2.c_str(),send_buff_2.size(),0,(struct sockaddr*)&r_address,sizeof(struct sockaddr_in));



#if 0
	for(i=1;i<=time_out;i++)
    {

#ifdef MOHO_WIN32
        u_long val = 1;
        ioctlsocket (udp_socket_fd,FIONBIO,&val);//none block
#elif defined(MOHO_X86)
        int flags = fcntl(udp_socket_fd,F_GETFL,0);
        fcntl(udp_socket_fd,F_SETFL,flags|O_NONBLOCK);
#endif
		memset(buff, 0, sizeof(buff));
        ret=recvfrom(udp_socket_fd,buff,MAX_BUFF_SIZE,0,NULL,NULL);
		if(ret==SOCKET_ERROR){
                        mySleep(1000);
            continue;
		}

		recv_buff=buff;
		messager(msgINFO, "uPnP discovery recv: \r\n%s", recv_buff.c_str());
        ret=recv_buff.find(HTTP_OK);
        if(ret==std::string::npos)
			continue;                       //invalid response

        std::string::size_type begin=recv_buff.find("http://");
        if(begin==std::string::npos)
            continue;                       //invalid response
        std::string::size_type end=recv_buff.find("\r",begin);
        if(end==std::string::npos)
			continue;    //invalid response
		
		describe_url=describe_url.assign(recv_buff,begin,end-begin);

		if(!get_description()){
                        mySleep(1000);
			continue;
		}

		if(!parser_description()){
                        mySleep(1000);
			continue;
		}

#ifdef MOHO_WIN32
        closesocket(udp_socket_fd);
#elif defined(MOHO_X86)
        close(udp_socket_fd);
#endif
		status=NAT_FOUND;					//find a router
		return true ;
    }
#else

	int udp_socket_host_fd =socket(AF_INET,SOCK_DGRAM,0);

	ret=setsockopt(udp_socket_host_fd, SOL_SOCKET, SO_BROADCAST, (char*)&bOptVal, bOptLen); 


	struct in_addr gatewayaddr;
	int getwayResult;
#ifdef WIN32
	getwayResult = getdefaultgateway(&(gatewayaddr.S_un.S_addr));
#else
	getwayResult = getdefaultgateway(&(gatewayaddr.s_addr));
#endif

	if(0 == getwayResult)
		messager(msgINFO, "getdefaultgateway success, ip:%s" ,inet_ntoa(gatewayaddr));
	else
		messager(msgINFO, "getdefaultgateway   failed");


	if( 0 == getwayResult)
	{
		struct sockaddr_in r_address2;
		r_address2.sin_family=AF_INET;
		r_address2.sin_port=htons(HTTPMU_HOST_PORT);
		r_address2.sin_addr=gatewayaddr;
		ret=sendto(udp_socket_host_fd,send_buff.c_str(),send_buff.size(),0,(struct sockaddr*)&r_address2,sizeof(struct sockaddr_in));
		ret=sendto(udp_socket_host_fd,send_buff_2.c_str(),send_buff_2.size(),0,(struct sockaddr*)&r_address2,sizeof(struct sockaddr_in));
	}

	fd_set read_set;

	struct timeval timeout;
	 
	timeout.tv_sec = 10;
	timeout.tv_usec = 0 ;
	
#ifdef MOHO_WIN32
	u_long val = 1;
	ioctlsocket (udp_socket_fd,FIONBIO,&val);//none block
	val = 1;
	ioctlsocket (udp_socket_host_fd,FIONBIO,&val);//none block
#elif defined(MOHO_X86)
	int flags = fcntl(udp_socket_fd,F_GETFL,0);
	fcntl(udp_socket_fd,F_SETFL,flags|O_NONBLOCK);
	fcntl(udp_socket_host_fd,F_SETFL,flags|O_NONBLOCK);
#endif

	for (;i<time_out;i++)
	{
		memset(buff, 0, sizeof(buff));

		FD_ZERO(&read_set);
		FD_SET(udp_socket_fd,&read_set);
		FD_SET(udp_socket_host_fd,&read_set);

		int nb = select(udp_socket_fd>udp_socket_host_fd?udp_socket_fd:udp_socket_host_fd ,
			&read_set,NULL,NULL,&timeout);
		if (nb < 0)
		{
			messager(msgERROR,"select error");
		}

		
		if (nb>0)
		{
			messager( msgINFO,"pkt received\n");
			if (FD_ISSET(udp_socket_fd, &read_set))  // ¿É¶Á  
			{  
				
				ret=recvfrom(udp_socket_fd,buff,MAX_BUFF_SIZE,0,NULL,NULL);
				if(ret==SOCKET_ERROR){
					continue;
				}

				recv_buff=buff;
				

			}  

			if (FD_ISSET(udp_socket_host_fd, &read_set)) //  ¿É¶Á  
			{  
	
				ret=recvfrom(udp_socket_host_fd,buff,MAX_BUFF_SIZE,0,NULL,NULL);
				if(ret==SOCKET_ERROR){

					continue;
				}

				recv_buff=buff;
			}  

			messager(msgINFO, "uPnP discovery recv: \r\n%s", recv_buff.c_str());
			ret=recv_buff.find(HTTP_OK);
			if(ret==std::string::npos)
				continue;                       //invalid response

			std::string::size_type begin=recv_buff.find("http://");
			if(begin==std::string::npos)
				continue;                       //invalid response
			std::string::size_type end=recv_buff.find("\r",begin);
			if(end==std::string::npos)
				continue;    //invalid response

			describe_url=describe_url.assign(recv_buff,begin,end-begin);

			if(!get_description()){
				continue;
			}

			if(!parser_description()){
				continue;
			}

#ifdef MOHO_WIN32
			closesocket(udp_socket_fd);
			closesocket(udp_socket_host_fd);
#elif defined(MOHO_X86)
			close(udp_socket_fd);
			close(udp_socket_host_fd);
#endif
			status=NAT_FOUND;					//find a router
			return true ;

		}
	}

		
#endif

	status=NAT_ERROR;
	last_error="Fail to find an UPNP NAT.\n";

    return false;                               //no router finded 
}

bool UPNPNAT::get_description()
{
	std::string host,path;
	unsigned short int port;
	int ret=parseUrl(describe_url.c_str(),host,&port,path);
	if(!ret)
	{
		status=NAT_ERROR;
		last_error="Failed to parseURl: "+describe_url+"\n";
		return false;
	}
	
	//connect 
	ret=tcp_connect(host.c_str(),port);
	if(!ret){
		return false;
	}
	
	char request[200];
	sprintf (request,"GET %s HTTP/1.1\r\nHost: %s:%d\r\n\r\n",path.c_str(),host.c_str(),port); 
	std::string http_request=request;
	
	//send request
	ret=send(tcp_socket_fd,http_request.c_str(),http_request.size(),0);
	//get description xml file
	char buff[MAX_BUFF_SIZE+1]; 	
	memset(buff, 0, sizeof(buff));
	std::string response;	
	while ( ret=recv(tcp_socket_fd,buff,MAX_BUFF_SIZE,0) >0) 
	{
		response+=buff;
		memset(buff, 0, sizeof(buff));
	}

	description_info=response;

	return true;
}


bool UPNPNAT::parser_description()
{
	XMLNode node=XMLNode::parseString(description_info.c_str(),"root");
	if(node.isEmpty())
	{
		status=NAT_ERROR;
		last_error="The device descripe XML file is not a valid XML file. Cann't find root element.\n";
		return false;
	}
	
	XMLNode baseURL_node=node.getChildNode("URLBase",0);
	if(!baseURL_node.getText())
	{
		std::string::size_type index=describe_url.find("/",7);
		if(index==std::string::npos )
		{
			status=NAT_ERROR;
			last_error="Fail to get base_URL from XMLNode \"URLBase\" or describe_url.\n";
			return false;
		}
		base_url=base_url.assign(describe_url,0,index);
	}
	else
		base_url=baseURL_node.getText();
	
	int num,i;
	XMLNode device_node,deviceList_node,deviceType_node;
	num=node.nChildNode("device");
	for(i=0;i<num;i++)
	{
		device_node=node.getChildNode("device",i);
		if(device_node.isEmpty())
			break;
		deviceType_node=device_node.getChildNode("deviceType",0);
		if(strcmp(deviceType_node.getText(),DEVICE_TYPE_1)==0)
			break;
	}

	if(device_node.isEmpty())
	{
		status=NAT_ERROR;
		last_error="Fail to find device \"urn:schemas-upnp-org:device:InternetGatewayDevice:1 \"\n";
		return false;
	}	

	deviceList_node=device_node.getChildNode("deviceList",0);
	if(deviceList_node.isEmpty())
	{
		status=NAT_ERROR;
		last_error=" Fail to find deviceList of device \"urn:schemas-upnp-org:device:InternetGatewayDevice:1 \"\n";
		return false;
	}

	// get urn:schemas-upnp-org:device:WANDevice:1 and it's devicelist 
	num=deviceList_node.nChildNode("device");
	for(i=0;i<num;i++)
	{
		device_node=deviceList_node.getChildNode("device",i);
		if(device_node.isEmpty())
			break;
		deviceType_node=device_node.getChildNode("deviceType",0);
		if(strcmp(deviceType_node.getText(),DEVICE_TYPE_2)==0)
			break;
	}

	if(device_node.isEmpty())
	{
		status=NAT_ERROR;
		last_error="Fail to find device \"urn:schemas-upnp-org:device:WANDevice:1 \"\n";
		return false;
	}	

	deviceList_node=device_node.getChildNode("deviceList",0);
	if(deviceList_node.isEmpty())
	{
		status=NAT_ERROR;
		last_error=" Fail to find deviceList of device \"urn:schemas-upnp-org:device:WANDevice:1 \"\n";
		return false;
	}

	// get urn:schemas-upnp-org:device:WANConnectionDevice:1 and it's servicelist 
	num=deviceList_node.nChildNode("device");
	for(i=0;i<num;i++)
	{
		device_node=deviceList_node.getChildNode("device",i);
		if(device_node.isEmpty())
			break;
		deviceType_node=device_node.getChildNode("deviceType",0);
		if(strcmp(deviceType_node.getText(),DEVICE_TYPE_3)==0)
			break;
	}
	if(device_node.isEmpty())
	{
		status=NAT_ERROR;
		last_error="Fail to find device \"urn:schemas-upnp-org:device:WANConnectionDevice:1\"\n";
		return false;
	}	
	
	XMLNode serviceList_node,service_node,serviceType_node;
	serviceList_node=device_node.getChildNode("serviceList",0);
	if(serviceList_node.isEmpty())
	{
		status=NAT_ERROR;
		last_error=" Fail to find serviceList of device \"urn:schemas-upnp-org:device:WANDevice:1 \"\n";
		return false;
	}	

	num=serviceList_node.nChildNode("service");
	const char * serviceType;
	bool is_found=false;
	for(i=0;i<num;i++)
	{
		service_node=serviceList_node.getChildNode("service",i);
		if(service_node.isEmpty())
			break;
		serviceType_node=service_node.getChildNode("serviceType");
		if(serviceType_node.isEmpty())
			continue;
		serviceType=serviceType_node.getText();
		if(strcmp(serviceType,SERVICE_WANIP)==0||strcmp(serviceType,SERVICE_WANPPP)==0)
		{
			is_found=true;
			break;
		}
	}

	if(!is_found)
	{
		status=NAT_ERROR;
		last_error="can't find  \" SERVICE_WANIP \" or \" SERVICE_WANPPP \" service.\n";
		return false;
	}

	this->service_type=serviceType;
	
	XMLNode controlURL_node=service_node.getChildNode("controlURL");
	control_url=controlURL_node.getText();



	//make the complete control_url;
	if(control_url.find("http://")==std::string::npos&&control_url.find("HTTP://")==std::string::npos)
	{
		if(base_url[base_url.size() -1 ] == '/' && control_url.at(0) == '/')
		{

			base_url.erase(base_url.end() -1 );
		}
		control_url=base_url+control_url;
	}
	if(service_describe_url.find("http://")==std::string::npos&&service_describe_url.find("HTTP://")==std::string::npos)
		service_describe_url=base_url+service_describe_url;
	
#ifdef MOHO_WIN32
        closesocket(tcp_socket_fd);
#elif defined(MOHO_X86)
        close(tcp_socket_fd);
#endif
	status=NAT_GETCONTROL;
	return true;	
}


bool UPNPNAT::add_port_mapping(char * _description, char * _destination_ip, unsigned short int _port_ex, unsigned short int _port_in, char * _protocal)
{
	int ret;

	std::string host,path;
	unsigned short int port;
	ret=parseUrl(control_url.c_str(),host,&port,path);
	if(!ret)
	{
		status=NAT_ERROR;
		last_error="Fail to parseURl: "+describe_url+"\n";
		return false;
	}
	
	//connect 
	ret=tcp_connect(host.c_str(),port);
	if(!ret)
		return false;
	
	char buff[MAX_BUFF_SIZE+1];
	sprintf(buff,ADD_PORT_MAPPING_PARAMS,_port_ex,_protocal,_port_in,_destination_ip,_description);
	std::string action_params=buff;
	
	sprintf(buff,SOAP_ACTION,ACTION_ADD,service_type.c_str(),action_params.c_str(),ACTION_ADD);
	std::string soap_message=buff;
    
	sprintf(buff,HTTP_HEADER_ACTION,path.c_str(),host.c_str(),port,service_type.c_str(),ACTION_ADD,soap_message.size());
	std::string action_message=buff;
	
	std::string http_request=action_message+soap_message;
	
	//send request
	messager(msgINFO, "add_port_mapping request: \r\n%s", http_request.c_str());
	ret=send(tcp_socket_fd,http_request.c_str(),http_request.size(),0);

	//wait for response 			
	std::string response;
	memset(buff, 0, sizeof(buff));
	while (ret=recv(tcp_socket_fd,buff,MAX_BUFF_SIZE,0) >0)
	{
		response+=buff;
		memset(buff, 0, sizeof(buff));
	}

	if(response.find(HTTP_OK)==std::string::npos)
	{
		status=NAT_ERROR;
		char temp[100];
		sprintf(temp,"Fail to add port mapping (%s/%s)\n",_description,_protocal);
		last_error=temp;
		messager(msgERROR, "add port mapping failed, resp = %s", response.c_str());
		return false;
	}

#ifdef MOHO_WIN32
        closesocket(tcp_socket_fd);
#elif defined(MOHO_X86)
        close(tcp_socket_fd);
#endif
	status=NAT_ADD;
	return true;	
}

bool UPNPNAT::del_port_mapping(unsigned short port, const char *protocal)
{
	std::string path,host;
	unsigned short c_port;
	int ret;
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
	std::string action_args;
	std::string soap_action;
	std::string http_header;
	char temp[MAX_BUFF_SIZE+1];
	sprintf(temp,DEL_PORT_MAPPING_ARGS,port,protocal);
	action_args=temp;

	memset(temp,0,MAX_BUFF_SIZE+1);
	sprintf(temp,SOAP_ACTION,ACTION_DEL,service_type.c_str(),action_args.c_str(),ACTION_DEL);
	soap_action=temp;

#if 0
	cout<<"soap_action is \n"<<soap_action.c_str()<<endl;
#endif

	memset(temp,0,MAX_BUFF_SIZE+1);
	//sprintf(temp,HTTP_HEADER_ACTION,path.c_str(),host.c_str(),c_port,soap_action.size(),service_type.c_str(),"DeletePortMapping");
	sprintf(temp,HTTP_HEADER_ACTION,path.c_str(),host.c_str(),c_port,service_type.c_str(),ACTION_DEL,soap_action.size());
	http_header=temp;

#if 0
	cout<<"http_header is \n"<<http_header.c_str()<<endl;
#endif

	std::string del_port_msg=http_header+soap_action;

#if 0
	cout<<"del_port_msg is \n"<<del_port_msg.c_str()<<endl;
#endif

	messager(msgINFO,"del_port_mapping send buff :\n\"%s\"",del_port_msg.c_str());
	ret = send(tcp_socket_fd,del_port_msg.c_str(),del_port_msg.size(),0);

	char buff[MAX_BUFF_SIZE+1];
	std::string resp; 
	memset(buff,0,MAX_BUFF_SIZE+1);

	while(recv(tcp_socket_fd,buff,MAX_BUFF_SIZE,0)>0)
	{
		resp+=buff;
		memset(buff,0,MAX_BUFF_SIZE+1);
	}

	messager(msgINFO,"del_port_mapping recvbuff:\n\"%s\"",resp.c_str());
	if(resp.find(HTTP_OK)==std::string::npos)
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
	return true;
}


