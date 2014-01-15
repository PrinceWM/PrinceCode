#pragma once

#include "config.h"

#if defined(MOHO_X86)
#include <string>
#include <list>
#endif

#define SLICE_PERMISSIBLE_ERROR 5//second

#define SUPER_TS_DELAY_NUMBER 120

#define SUPPLIER_START_NUMBER 120

#define FTDS_START_NUMBER 3

#define SUPER_NORMAL_DELAY_NUMBER 2

typedef enum channelStyleEnum
{
	channelSuperTs,
	channelSuper,
	channelNormal,
	channelConsumerOnly
}channelStyleEnum;

//Ƶ����ԴƬ��������ȷ����
struct timeSlice
{
	time_t startTime;
	time_t endTime;//��ȷ����
};

//Ƶ����ԴĿ¼����̼߳��ز�����Ƶ����ԴĿ¼����Ӧ��Ƶ����ָ��
struct fileNotifyThreadInfo
{
	const char* filePath;
	class CLiveChannel *channel;
};

//void getStartTimeFromFileName(const char* fileName, time_t *t);
//void getEndTimeFromFileName(const char* fileName, time_t *t);


/*
    ÿ��LiveChannel����Ƶ������Ϣ��ά��Ƶ�������ݶ�ȡ��д��ȹ���
    Ƶ����Ϣ������Ƶ����������ά������ȡ���ͷ�Ƶ����ͨ��������ʵ��
*/
class CLiveChannel
{
private:

private:
private:



public:
    //�յ�ֱ����֮�󣬸������������Ϣ������Ҫ���ݵĽڵ㷢����������������
public:
    ~CLiveChannel(void);//��ʱֻ��һ���߳̿��ܷ��ʵ��ýڵ�

	CLiveChannel(UINT m_ID, std::string m_Name, std::string m_Path, std::string m_ExceptionPath, int m_Exception, BOOL m_isVOD, int m_channelStyle);

	UINT channelID;
	std::string channelName;	
	std::list<timeSlice>::iterator liveIter;

	//Ƶ����ԴƬ������ʱ���Ⱥ�����
	std::list<timeSlice> liveChannelSlices;

	std::string channelFilePath;
	std::string exceptionFilePath;
	int isException;
	UINT channelBitRate;
	UINT pcrPID;
	fileNotifyThreadInfo fileNotifyinfo;
        BOOL checked;
        BOOL isDeleted;
	BOOL isVOD;
	int channelStyle;//
	BOOL needToRestartSupplier;
	BOOL needToRestartConsumer;



	//ϵͳ����ʱ��ȡƵ����ԴĿ¼�µ���Դ�ļ�����ʼ����ԴƬ�б�
	UINT initLiveChannelSlices();

	//����Ƶ��Ŀ¼����߳�
	UINT checkFileUpdate();

	//��ѯĳ�¼�Ƭ�Ƿ�Ƶ����ԴƬ�б����
	UINT isSlicesInList( timeSlice * );

	//����һ��ʱ��Ƭ��������Դ�б���кϲ�
        BOOL insertTimeSlice(time_t t1, time_t t2);

	//�����ļ�������ʱ��Ƭ
        BOOL insertTimeSliceFromFile(std::string fileName);

	//�����ļ���ɾ��ʱ��Ƭ
        BOOL removeTimeSliceFromFile(std::string fileName);

	//�ϲ�ʱ��Ƭ
	void mergeTimeSlice();

	//����ļ������Ƿ������Դ�ļ���ʽҪ��
        BOOL checkFileName(const char *fileName);

	//��ӡ��ԴƬ��Ϣ����־�ļ�
	void logTimeSlice();

	//����ʱ��Ƭ�Ƿ�����ԴƬ��
	time_t searchTimeSlice(time_t s, time_t e);

	//����UTCʱ��ȷ��������ʱ�����ԴƬ�ļ������1-9999
	int getTheFileIndex(time_t *t);

	UINT32 getTheMaxFileIndex();

	//�õ���ԴƬ�����ʱ��
	time_t getTheLastTime();

	UINT getPCRPidFromFile(const char *fileName);

	DWORD lastFormatErrorLogTime;

	void logTimeSlice4Search();

};
