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
                            		//��ʼ���������ó�ʱ����������� 
    bool discovery();       		
									//�����豸��describe_url �豸����URL 
    bool get_description();			
									//ȡ���豸����XML�ļ��洢��description_info_xml
    bool parse_description();		
									//�����豸����XML�ļ� ���control_url��service_describe_url
									//��service_type
    bool add_port_mapping(unsigned  int port,const char * protocal,unsigned int * externPort);	
									//���һ���˿�ӳ�� ����externPort������ӳ����ⲿ�˿�
									//ͬʱ����pri_mapping_infos��һ��������isDelete=false
									//��Ǵ�ӳ��Ŀǰû�б�ɾ��
    bool del_port_mapping(unsigned  int port,const char * protocal);
									//ɾ��һ���˿�ӳ�� ����Ϊ�ⲿ�˿ں�Э��
									//ͬʱ��pri_mapping_infos�еĶ���
									//����isDelete=true����Ѿ���ɾ��
	bool del_port_mapping_all();
									//ɾ�������ɴ˳��򴴽��Ķ˿�ӳ��
									//�ж�׼��
									//    1 �˿�ӳ������ 2 �ڲ��ͻ��˵�ַ
								    
	//bool del_port_mapping_all_v2();
									//ɾ�������ɴ˳��򴴽��Ķ˿�ӳ��
									//�ж�׼��
									//    ��pri_mapping_infos��isDeleteΪfalse�����ж���
									//

	bool get_external_ip();
									//����ⲿ��ַ������external_ip��

   	bool get_port_mapping();                                                
									//������еĶ˿�ӳ����Ϣ ����parse_mapping_info()
									//����mapping_info_str ,����mapping_info����
									//����mapping_infos Vector��
    const char * get_last_error()
	{ 
		return last_error.c_str();
	}           
									//������һ������
	void print_help();              
							        //���·������ӳ�����Ϣ
	void print_help_me();			
									//�������Ķ�����ӵ�ӳ����Ϣ
private:
	bool get_localIP();
									//�����ȳ�ʼ��tcp_socket �˺���ͨ��getsocketname()ʵ��
	bool tcp_connect(const char * addr,unsigned int port);
	bool parse_mapping_info();												
									//�����˿�ӳ����Ϣ,����vector�С�
	bool isInUse(unsigned int port,std::string protocol);
									//���ⲿӳ��˿��Ƿ��Ѿ���ʹ��
	bool hasBeenMapped(unsigned int port,std::string protocol);
									//�����ⲿ�˿��Ƿ�ӳ���
									//�жϱ�׼ 1 ��ͬ��Э�� 2 ͬһ���ͻ��� 3 �ڲ��˿�
									//�Ƿ��Ѿ���ӳ���

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
								//�����ļ���url
	std::string control_url;
								//����url
	std::string base_url;
								//URLBase
	std::string service_describe_url;
								
    std::string description_info_xml;
								//�����ļ� xml�ļ����ַ�����ʽ
	std::string mapping_info_str;
	struct mapping_info			//��·�����ϵĶ˿�ӳ��һһ��Ӧ
	{
		std::string internalClient;
		std::string externalPort;
		std::string internalPort;
		std::string protocol;
		std::string description;
	};

	std::vector<struct mapping_info> mapping_infos;
							   //�洢·�����ϵĶ˿�ӳ��
	
	struct pri_mapping_info    //�� ����Ķ��󴴽��Ķ˿�ӳ�� һһ��Ӧ
	{
		std::string protocal;
		unsigned int inPort;
		unsigned int exPort;
		bool isDelete;
	};

	std::vector<struct pri_mapping_info> pri_mapping_infos;
								//�洢�ɴ���Ķ��󴴽��Ķ˿�ӳ��
//friend int main(int argc,char *argv[]);
		
friend bool parseUrl(const char* url,std::string& host,unsigned int* port,std::string& path);
};
#endif
