//消息和调试信息的输出
#ifndef _MESSAGE_H
#define _MESSAGE_H

#include "config.h"
#include <stdarg.h>
#include <time.h>
#include <string>

#include <stdio.h>
#include "mylock.h"

#if defined(MOHO_X86)
#include <sys/types.h>
#include <sys/stat.h>
#elif defined(MOHO_WIN32)
#include <ole2.h>
#include <locale.h>
#endif

#if defined(MOHO_ANDROID)
#include <android/log.h>
#define LOG_TAG "MyP2p"
#define LOGD(fmt, args...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, fmt, ##args)
#endif

extern std::string g_localPath;

#define MAX_LOG_STRING_LEN  4096
#ifdef MOHO_F20
#define MAX_LOG_FILE_LEN 524288
#else
#define MAX_LOG_FILE_LEN 1048576
#endif
//消息类型
typedef enum MSGTYPE
{
	msgDEBUG,//调试信息
	msgINFO,//一般性消息
	msgWARN,//警告
	msgERROR,//错误，应用错误，可以通过别的手段来弥补
	msgFATAL//无法恢复的错误，内存，网络等等
}MSGTYPE;

//日志级别
typedef enum LOGLEVEL
{
	logDEBUG,
	logINFO,
	logWARN,
	logERROR,
	logFATAL
}LOGLEVEL;

//消息类型名称
extern CHAR* msgTypeName[];

class CNetLog
{
private:
	//日志文件的指针
	FILE* pFile;

	std::string AppPath;
	std::string nodeType;

        int m_saved;
	LOGLEVEL logLevel;
	int MaxLogSize;

	//保护log文件读写的临界区
	CMyLock m_lock;

	char buffer[MAX_LOG_STRING_LEN];
public:
	CNetLog(char * nodeType)
	{
                m_saved = 1;
		m_lock.init();

                AppPath= g_localPath;
                AppPath.append("logs");
		//检查基本文件夹是否存在，如果不存在，创建
#if defined(MOHO_X86)
                if(-1 == mkdir(AppPath.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) && myGetLastError()!= 17)
		{
                     AppPath=".";
		}
#elif defined(MOHO_WIN32)
		if (CreateDirectory(AppPath.c_str(),NULL)==0 && myGetLastError()!=ERROR_ALREADY_EXISTS)
			AppPath=".";//日志文件夹不存在，并且创建失败
#endif

		logLevel=logDEBUG;//logDEBUG;logERROR
		MaxLogSize=MAX_LOG_FILE_LEN;

		this->nodeType=nodeType;
		openLogFile();
	}

	CNetLog(char * nodeType,LOGLEVEL level)
	{
                m_saved = 1;
		m_lock.init();

                AppPath= g_localPath;
                AppPath.append("logs");
		//检查基本文件夹是否存在，如果不存在，创建
#if defined(MOHO_X86)
                if(-1 == mkdir(AppPath.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) && myGetLastError()!= 17)
		{
                   AppPath=".";
		}
#elif defined(MOHO_WIN32)
		if (CreateDirectory(AppPath.c_str(),NULL)==0 && myGetLastError()!=ERROR_ALREADY_EXISTS)
			AppPath=".";//日志文件夹不存在，并且创建失败
#endif
		logLevel=level;//logDEBUG;
		MaxLogSize=MAX_LOG_FILE_LEN;

		this->nodeType=nodeType;
		openLogFile();
	}

	~CNetLog()
	{
		if (pFile!=NULL)
			fclose(pFile);

		m_lock.destroy();
	}

        void setlevel(int level)
        {
            if(level >= 0 && level < logLevel)
            {
                logLevel = (LOGLEVEL)level;
            }
        }
        void setSaved(int isSaved)
        {
            m_saved = isSaved;
        }
	void log(MSGTYPE msgType,const char* strFormat, ...)
	{
		//消息级别小于日志级别，不应该输出
		if ( msgType<logLevel)
		{
			return;
		}
		m_lock.lock();

		va_list pArgs;
		va_start(pArgs, strFormat);
		vsnprintf(buffer, MAX_LOG_STRING_LEN - 1, strFormat, pArgs);
		va_end(pArgs);
		writeLog(msgType, buffer);
		m_lock.unlock();
		return;
	}

	void message(MSGTYPE msgType,const char* strFormat, ...)
	{

		//消息级别小于日志级别，不应该输出
		if ( msgType<logLevel)
		{
			return;
		}
		m_lock.lock();
		va_list pArgs;
		va_start(pArgs, strFormat);
		vsnprintf(buffer, MAX_LOG_STRING_LEN - 1, strFormat, pArgs);
		va_end(pArgs);
		writeLog(msgType, buffer);
		m_lock.unlock();
		return;
	}

private:
	void writeLog(MSGTYPE msgType,char * pszLog)
	{
		if(pFile==NULL)
		{
			openLogFile();
		}
		else if (getFileLength(pFile) > MaxLogSize)
		{
			fclose(pFile);
			openLogFile();
		}
                time_t rawtime;
                struct tm * timeinfo;

                time ( &rawtime );
                timeinfo = localtime ( &rawtime );

		if (pFile==NULL)//再次检查文件是否存在，如果还不存在，直接退出
		{
#ifdef MOHO_ANDROID
                    LOGD("[%d-%02d-%02d %02d:%02d:%02d] %s %s\n",
                         timeinfo->tm_year+1900,timeinfo->tm_mon+1,timeinfo->tm_mday,
                         timeinfo->tm_hour,timeinfo->tm_min,timeinfo->tm_sec,
                         msgTypeName[msgType],
                         pszLog);

#else

                    printf("[%d-%02d-%02d %02d:%02d:%02d] %s %s\n",
                           timeinfo->tm_year+1900,timeinfo->tm_mon+1,timeinfo->tm_mday,
                           timeinfo->tm_hour,timeinfo->tm_min,timeinfo->tm_sec,
                           msgTypeName[msgType],
                           pszLog);
#endif
		}
                else
                {
                    fprintf(pFile,("[%d-%02d-%02d %02d:%02d:%02d] %s %s\n"),timeinfo->tm_year+1900,timeinfo->tm_mon+1,timeinfo->tm_mday,
                            timeinfo->tm_hour,timeinfo->tm_min,timeinfo->tm_sec,
                            msgTypeName[msgType],pszLog);
                    fflush(pFile);
                }

	}

	long getFileLength(FILE* fp)
	{
		long len=0;
		if(fp!=NULL)
		{
			fseek(fp,0L,SEEK_END);
			len=ftell(fp);
		}

		return len;
	}

	int openLogFile()
	{
		std::string filename=AppPath;
#if defined(MOHO_X86)
		filename+="/";
#elif defined(MOHO_WIN32)
		filename+="\\";
#endif
		filename+=nodeType;
		filename+=".log";

                if(m_saved)
                {
                    //新文件名，格式： 路径\nodeType-YYYYMMDD-HHMMSS.log
                    time_t rawtime;
                    struct tm * timeinfo;
                    time ( &rawtime );
                    timeinfo = localtime ( &rawtime );
                    char timeChar[32];//2008 1030 1610 00
                    sprintf(timeChar,("-%04d%02d%02d-%02d%02d%02d.log"),timeinfo->tm_year+1900,timeinfo->tm_mon+1,timeinfo->tm_mday,
                            timeinfo->tm_hour,timeinfo->tm_min,timeinfo->tm_sec);
                    std::string newFilename=AppPath;
#if defined(MOHO_X86)
                    newFilename+="/";
#elif defined(MOHO_WIN32)
                    newFilename+="\\";
#endif
                    newFilename+=nodeType;
                    newFilename+=timeChar;

                    rename(filename.c_str(),newFilename.c_str());
                }
		this->pFile=fopen(filename.c_str(),"w");
		if(this->pFile==NULL)
                    return -1;
		return 0;
            }
};

extern CNetLog * normalLog;
extern CNetLog * liveLog;

#define logger normalLog->log
#define messager normalLog->message

#endif //_M_MESSAGES
