#ifndef NATUPNP_H
#define NATUPNP_H
#include <string>
#include <vector>
#ifdef MOHO_WIN32
#include <winsock2.h>
#elif defined(MOHO_X86)
#include <stdio.h>
#include <fcntl.h>
#include <sys/socket.h>
#endif

#define DefaultTimeOut  10
#define DefaultInterval 200

class NatUPnP
{
public:

	int getTime_out()
	{
		return time_out;
	}
	int getInterval()
	{
		return interval;
	}

	std::string getExternalIp()
	{
		return external_ip;
	}
 
    bool init(int time_out=DefaultTimeOut,int interval=DefaultInterval); 
                            		//初始化，并设置超时和搜索间隔。 
    bool discovery();       		
									//发现设备，describe_url 设备描述URL 
    bool get_description();			
									//取得设备描述XML文件存储于description_info_xml
    bool parse_description();		
									//解析设备描述XML文件 获得control_url和service_describe_url
									//和service_type
    bool add_port_mapping(unsigned  int port,const char * protocal,unsigned int * externPort);	
									//添加一个端口映射 参数externPort传出来映射的外部端口
									//同时加入pri_mapping_infos中一个对象，用isDelete=false
									//标记此映射目前没有被删除
    bool del_port_mapping(unsigned  int port,const char * protocal);
									//删除一个端口映射 参数为外部端口和协议
									//同时将pri_mapping_infos中的对象
									//属性isDelete=true标记已经被删除
	bool del_port_mapping_all();
									//删除所有由此程序创建的端口映射
									//判断准则：
									//    1 端口映射描述 2 内部客户端地址
								    
	//bool del_port_mapping_all_v2();
									//删除所有由此程序创建的端口映射
									//判断准则：
									//    在pri_mapping_infos中isDelete为false的所有对象
									//

	bool get_external_ip();
									//获得外部地址，存入external_ip中

   	bool get_port_mapping();                                                
									//获得已有的端口映射信息 调用parse_mapping_info()
									//分析mapping_info_str ,构造mapping_info对象
									//存入mapping_infos Vector中
    const char * get_last_error()
	{ 
		return last_error.c_str();
	}           
									//获得最后一个错误
	void print_help();              
							        //输出路由器上映射的信息
	void print_help_me();			
									//输出此类的对象添加的映射信息
private:
	bool get_localIP();
									//必须先初始化tcp_socket 此函数通过getsocketname()实现
	bool tcp_connect(const char * addr,unsigned int port);
	bool parse_mapping_info();												
									//解析端口映射信息,存入vector中。
	bool isInUse(unsigned int port,std::string protocol);
									//看外部映射端口是否已经被使用
	bool hasBeenMapped(unsigned int port,std::string protocol);
									//返回外部端口是否被映射过
									//判断标准 1 相同的协议 2 同一个客户端 3 内部端口
									//是否已经被映射过

	SOCKET udp_socket_fd;
	SOCKET tcp_socket_fd;

	typedef enum 
	{
		NAT_INIT=0,
		NAT_FOUND,
		NAT_TCP_CONNECTED,
		NAT_GETDESCRIPTION,
		NAT_FOUND_EXTERNALIP,
		NAT_GETCONTROL,
		NAT_ADD,
		NAT_DEL,
		NAT_GET,
		NAT_ERROR
	} NAT_STAT;
	NAT_STAT status;
	std::string last_error;

    int time_out;
    int interval;

	std::string local_ip;

	std::string external_ip;

	std::string service_type;

    std::string describe_url;
								//描述文件的url
	std::string control_url;
								//控制url
	std::string base_url;
								//URLBase
	std::string service_describe_url;
								
    std::string description_info_xml;
								//描述文件 xml文件的字符串行式
	std::string mapping_info_str;
	struct mapping_info			//跟路由器上的端口映射一一对应
	{
		std::string internalClient;
		std::string externalPort;
		std::string internalPort;
		std::string protocol;
		std::string description;
	};

	std::vector<struct mapping_info> mapping_infos;
							   //存储路由器上的端口映射
	
	struct pri_mapping_info    //与 此类的对象创建的端口映射 一一对应
	{
		std::string protocal;
		unsigned int inPort;
		unsigned int exPort;
		bool isDelete;
	};

	std::vector<struct pri_mapping_info> pri_mapping_infos;
								//存储由此类的对象创建的端口映射
//friend int main(int argc,char *argv[]);
		
friend bool parseUrl(const char* url,std::string& host,unsigned int* port,std::string& path);
};
#endif
