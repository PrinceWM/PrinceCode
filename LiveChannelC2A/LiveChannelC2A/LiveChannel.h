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

//频道资源片描述，精确到秒
struct timeSlice
{
	time_t startTime;
	time_t endTime;//精确到秒
};

//频道资源目录监测线程加载参数：频道资源目录及对应的频道类指针
struct fileNotifyThreadInfo
{
	const char* filePath;
	class CLiveChannel *channel;
};

//void getStartTimeFromFileName(const char* fileName, time_t *t);
//void getEndTimeFromFileName(const char* fileName, time_t *t);


/*
    每个LiveChannel保存频道的信息，维护频道的数据读取、写入等功能
    频道信息集中在频道管理器中维护，获取、释放频道都通过管理器实现
*/
class CLiveChannel
{
private:

private:
private:



public:
    //收到直播流之后，根据流的相关信息，对需要数据的节点发送流，保存流数据
public:
    ~CLiveChannel(void);//此时只有一个线程可能访问到该节点

	CLiveChannel(UINT m_ID, std::string m_Name, std::string m_Path, std::string m_ExceptionPath, int m_Exception, BOOL m_isVOD, int m_channelStyle);

	UINT channelID;
	std::string channelName;	
	std::list<timeSlice>::iterator liveIter;

	//频道资源片链表，以时间先后排序
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



	//系统启动时读取频道资源目录下的资源文件，初始化资源片列表
	UINT initLiveChannelSlices();

	//加载频道目录监测线程
	UINT checkFileUpdate();

	//查询某事件片是否被频道资源片列表包含
	UINT isSlicesInList( timeSlice * );

	//插入一个时间片，并对资源列表进行合并
        BOOL insertTimeSlice(time_t t1, time_t t2);

	//根据文件名插入时间片
        BOOL insertTimeSliceFromFile(std::string fileName);

	//根据文件名删除时间片
        BOOL removeTimeSliceFromFile(std::string fileName);

	//合并时间片
	void mergeTimeSlice();

	//检查文件名称是否符合资源文件格式要求
        BOOL checkFileName(const char *fileName);

	//打印资源片信息到日志文件
	void logTimeSlice();

	//搜索时间片是否在资源片中
	time_t searchTimeSlice(time_t s, time_t e);

	//根据UTC时间确定包含该时间的资源片文件的序号1-9999
	int getTheFileIndex(time_t *t);

	UINT32 getTheMaxFileIndex();

	//得到资源片的最近时间
	time_t getTheLastTime();

	UINT getPCRPidFromFile(const char *fileName);

	DWORD lastFormatErrorLogTime;

	void logTimeSlice4Search();

};
