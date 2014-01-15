#include "stdafx.h"
#include "LiveChannel.h"
#include <list>
#include <bitset>
#include "config.h"
#include "message.h"
#include "netmain.h"
#include "myfile.h"


/*
static DWORD WINAPI fileNotifyThreadProc(LPVOID lpParameter)
{
	fileNotifyThreadInfo *info = (fileNotifyThreadInfo *)lpParameter;
//   DWORD dwWaitStatus; 
//   HANDLE dwChangeHandles[1]; 
//   //TCHAR lpDrive[4];
//   //TCHAR lpDir[256];
//   //TCHAR lpFile[_MAX_FNAME];
//   //TCHAR lpExt[_MAX_EXT];
//
//	//strcpy(lpFile, info->filePath);
//
//   //_tsplitpath_s(lpDir, lpDrive, 4, NULL, 0, lpFile, _MAX_FNAME, lpExt, _MAX_EXT);
//
//   //lpDrive[2] = (TCHAR)'\\';
//   //lpDrive[3] = (TCHAR)'\0';
// 
//// Watch the directory for file creation and deletion. 
// 
//   dwChangeHandles[0] = FindFirstChangeNotification( 
//      info->filePath,                         // directory to watch 
//      FALSE,                         // do not watch subtree 
//      FILE_NOTIFY_CHANGE_FILE_NAME); // watch file name changes 
// 
//   if (dwChangeHandles[0] == INVALID_HANDLE_VALUE) 
//      ExitProcess(GetLastError()); 
// 
// Change notification is set. Now wait on both notification 
// handles and refresh accordingly. 

  char buffer_[1024*32];
  DWORD receives_bytes_ = 0;
  memset(buffer_, 0, 1024*4);

	DWORD bytesReturned;

	char changedFileName[256];
	HANDLE readDirHandle = ::CreateFile(info->filePath
														, FILE_LIST_DIRECTORY
                                                        , FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE
                                                        , NULL
                                                        , OPEN_EXISTING
                                                        , FILE_FLAG_BACKUP_SEMANTICS|FILE_FLAG_OVERLAPPED
                                                        , NULL
                                                        );


	if(readDirHandle == INVALID_HANDLE_VALUE)
	{
		liveLog->log(msgERROR, "CreateFile ERROR, path = %s", info->filePath);
		return -1;
	}
   while (TRUE) 
   { 
   // Wait for notification.

      //dwWaitStatus = WaitForMultipleObjects(1, dwChangeHandles, 
      //   FALSE, INFINITE); 

      //switch (dwWaitStatus) 
      //{ 
      //   case WAIT_OBJECT_0: 
			  memset(buffer_, 0, 1024*4);
			 if(	ReadDirectoryChangesW(
					readDirHandle,
					buffer_,
					1024*4,
					FALSE,
					FILE_NOTIFY_CHANGE_FILE_NAME,
					&bytesReturned,
					NULL,
					NULL))
			 {
				FILE_NOTIFY_INFORMATION *lp = (FILE_NOTIFY_INFORMATION *)buffer_;
				 do
				 {
					 memset(changedFileName, 0, 256);
					 if(lp->Action == FILE_ACTION_ADDED)
					 {
						 int   nLen   =   lp->FileNameLength;   
						 WideCharToMultiByte(CP_ACP,   0,   lp->FileName,   nLen,   changedFileName,   2*nLen,   NULL,   NULL);  
						 liveLog->log(msgDEBUG, "%s added", changedFileName);
						 if(info->channel->insertTimeSliceFromFile(changedFileName))
						 {
							 info->channel->logTimeSlice();
							 info->channel->mergeTimeSlice();
							 info->channel->logTimeSlice();
						 }
					 }
					 else if(lp->Action == FILE_ACTION_REMOVED)
					 {
						 int   nLen   =   lp->FileNameLength;   
						 WideCharToMultiByte(CP_ACP,   0,   lp->FileName,   nLen,   changedFileName,   2*nLen,   NULL,   NULL);  
						 liveLog->log(msgDEBUG, "%s removed", changedFileName);
						 
						 if(info->channel->removeTimeSliceFromFile(changedFileName))
						 {
							info->channel->logTimeSlice();
						 }
					 }
					 else if(lp->Action == FILE_ACTION_RENAMED_NEW_NAME)
					 {
						 int   nLen   =   lp->FileNameLength;   
						 WideCharToMultiByte(CP_ACP,   0,   lp->FileName,   nLen,   changedFileName,   2*nLen,   NULL,   NULL);  
						 liveLog->log(msgDEBUG, "%s renamed", changedFileName);
						 if(info->channel->insertTimeSliceFromFile(changedFileName))
						 {
							 info->channel->logTimeSlice();
							 info->channel->mergeTimeSlice();
							 info->channel->logTimeSlice();
						 }
					 }
					 if(lp ->NextEntryOffset == 0)
					 {
						 break;
					 }

					 lp = (FILE_NOTIFY_INFORMATION *)(buffer_ + lp ->NextEntryOffset);

				 }while( true );
			 }
			 else
			 {
				liveLog->log(msgERROR, "ReadDirectoryChangesW ERROR");
			 }

      //   // A file was created or deleted in the directory.
      //   // Refresh this directory and restart the notification.

      //      //RefreshDirectory(lpDir); 
      //      if ( FindNextChangeNotification( 
      //              dwChangeHandles[0]) == FALSE ) 
      //          ExitProcess(GetLastError()); 
      //      break; 

      //  default: 
      //      ExitProcess(GetLastError()); 
      //}
   }
	return 0;
}*/

//void getStartTimeFromFileName(const char* fileName, time_t *t)
//{
//	001_CCTV1_0001_20110704_093806_060033.ts
//	time_t rawtime;
//	*t = 0;
//	string tmp(fileName);
//	struct tm *timeInfo;
//	char buf[256];
//
//	int l = strlen(fileName);
//
//	time ( &rawtime );
//	timeInfo = localtime ( &rawtime );
//
//	tmp.copy(buf, 2, l - 9 - 3);
//	buf[2] = '\0';
//	timeInfo->tm_sec = atoi(buf);
//
//	tmp.copy(buf, 2, l - 9 - 3 - 2);
//	buf[2] = '\0';
//	timeInfo->tm_min = atoi(buf);
//
//	tmp.copy(buf, 2, l - 9 - 3 - 2 - 2);
//	buf[2] = '\0';
//	timeInfo->tm_hour = atoi(buf);
//
//	tmp.copy(buf, 2, l - 9 - 3 - 2 - 2 - 3);
//	buf[2] = '\0';
//	timeInfo->tm_mday = atoi(buf);
//
//	tmp.copy(buf, 2, l - 9 - 3 - 2 - 2 - 3 - 2);
//	buf[2] = '\0';
//	timeInfo->tm_mon = atoi(buf) - 1;
//
//	tmp.copy(buf, 4, l - 9 - 3 - 2 - 2 - 3 - 2 - 4);
//	buf[4] = '\0';
//	timeInfo->tm_year = atoi(buf) - 1900;
//
//	*t += mktime(timeInfo);
//
//}



void getStartTimeFromFileName(const char* fileName, time_t *t)
{
    //001_ch0_20111220_122510_005600_899_A3B597CE.ts

    time_t rawtime;
    *t = 0;
    struct tm *timeInfo;
    char buf[256];

    int l = strlen(fileName);

    int i = 0;
    while(1)
    {
        if(fileName[i] != '_')
        {
            i ++;
        }
        else
        {
            break;
        }
        if((UINT)i >= strlen(fileName))
        {
            liveLog->log(msgERROR, "FILE FORMAT ERROR: %s", fileName);
            return;
        }
    }

    i ++;

    while(1)
    {
        if(fileName[i] != '_')
        {
            i ++;
        }
        else
        {
            break;
        }
        if((UINT)i >= strlen(fileName))
        {
            liveLog->log(msgERROR, "FILE FORMAT ERROR: %s", fileName);
            return;
        }
    }

    i ++;

    time ( &rawtime );
    timeInfo = localtime ( &rawtime );
    std::string tmp(fileName + i);

    //20111220_122510_005600_899_A3B597CE.ts
    tmp.copy(buf, 2, 13);
    buf[2] = '\0';
    timeInfo->tm_sec = atoi(buf);

    tmp.copy(buf, 2, 13 - 2);
    buf[2] = '\0';
    timeInfo->tm_min = atoi(buf);

    tmp.copy(buf, 2, 13 - 2 - 2);
    buf[2] = '\0';
    timeInfo->tm_hour = atoi(buf);

    tmp.copy(buf, 2, 13 - 2 - 2 - 3);
    buf[2] = '\0';
    timeInfo->tm_mday = atoi(buf);

    tmp.copy(buf, 2, 13 - 2 - 2 - 3 - 2);
    buf[2] = '\0';
    timeInfo->tm_mon = atoi(buf) - 1;

    tmp.copy(buf, 4, 13 - 2 - 2 - 3 - 2 - 4);
    buf[4] = '\0';
    timeInfo->tm_year = atoi(buf) - 1900;

    *t += mktime(timeInfo);

}


//void getEndTimeFromFileName(const char* fileName, time_t *t)
//{
//	//001_CCTV1_0001_20110704_093806_060033.ts
//	time_t rawtime;
//	*t = 0;
//	string tmp(fileName);
//	struct tm *timeInfo;
//	char buf[256];
//
//	int l = strlen(fileName);
//
//	time ( &rawtime );
//	timeInfo = localtime ( &rawtime );
//
//
//	tmp.copy(buf, 6, l - 9);
//	buf[6] = '\0';
//	*t += (atoi(buf)/1000);
//
//	tmp.copy(buf, 2, l - 9 - 3);
//	buf[2] = '\0';
//	timeInfo->tm_sec = atoi(buf);
//
//	tmp.copy(buf, 2, l - 9 - 3 - 2);
//	buf[2] = '\0';
//	timeInfo->tm_min = atoi(buf);
//
//	tmp.copy(buf, 2, l - 9 - 3 - 2 - 2);
//	buf[2] = '\0';
//	timeInfo->tm_hour = atoi(buf);
//
//	tmp.copy(buf, 2, l - 9 - 3 - 2 - 2 - 3);
//	buf[2] = '\0';
//	timeInfo->tm_mday = atoi(buf);
//
//	tmp.copy(buf, 2, l - 9 - 3 - 2 - 2 - 3 - 2);
//	buf[2] = '\0';
//	timeInfo->tm_mon = atoi(buf) - 1;
//
//	tmp.copy(buf, 4, l - 9 - 3 - 2 - 2 - 3 - 2 - 4);
//	buf[4] = '\0';
//	timeInfo->tm_year = atoi(buf) - 1900;
//
//
//	(*t) += mktime(timeInfo);
//
//}

void getEndTimeFromFileName(const char* fileName, time_t *t)
{
    //001_ch0_20111220_122510_005600_899_A3B597CE.ts
    time_t rawtime;
    *t = 0;
    struct tm *timeInfo;
    char buf[256];

    int l = strlen(fileName);

    UINT i = 0;
    while(1)
    {
        if(fileName[i] != '_')
        {
            i ++;
        }
        else
        {
            break;
        }
        if(i >= strlen(fileName))
        {
            liveLog->log(msgERROR, "FILE FORMAT ERROR: %s", fileName);
            return;
        }
    }

    i ++;

    while(1)
    {
        if(fileName[i] != '_')
        {
            i ++;
        }
        else
        {
            break;
        }
        if(i >= strlen(fileName))
        {
            liveLog->log(msgERROR, "FILE FORMAT ERROR: %s", fileName);
            return;
        }
    }

    i ++;

    time ( &rawtime );
    timeInfo = localtime ( &rawtime );
    std::string tmp(fileName + i);

    //20111220_122510_005600_899_A3B597CE.ts


    tmp.copy(buf, 6, 16);
    buf[6] = '\0';
    *t += (atoi(buf)/1000);

    tmp.copy(buf, 2, 16 - 3);
    buf[2] = '\0';
    timeInfo->tm_sec = atoi(buf);

    tmp.copy(buf, 2, 16 - 3 - 2);
    buf[2] = '\0';
    timeInfo->tm_min = atoi(buf);

    tmp.copy(buf, 2, 16 - 3 - 2 - 2);
    buf[2] = '\0';
    timeInfo->tm_hour = atoi(buf);

    tmp.copy(buf, 2, 16 - 3 - 2 - 2 - 3);
    buf[2] = '\0';
    timeInfo->tm_mday = atoi(buf);

    tmp.copy(buf, 2, 16 - 3 - 2 - 2 - 3 - 2);
    buf[2] = '\0';
    timeInfo->tm_mon = atoi(buf) - 1;

    tmp.copy(buf, 4, 16 - 3 - 2 - 2 - 3 - 2 - 4);
    buf[4] = '\0';
    timeInfo->tm_year = atoi(buf) - 1900;


    (*t) += mktime(timeInfo);

}

CLiveChannel::CLiveChannel(UINT m_ID, std::string m_Name, std::string m_Path, std::string m_ExceptionPath, int m_Exception, BOOL m_isVOD, int m_channelStyle)
{
    channelID = m_ID;
    channelName = m_Name;
    channelFilePath = m_Path;
    exceptionFilePath = m_ExceptionPath;
    //checked = false;
    isException = m_Exception;
    isDeleted = false;
    isVOD = m_isVOD;
	channelStyle = m_channelStyle;
	needToRestartConsumer = FALSE;
	needToRestartSupplier = FALSE;

    //liveChannelSlices.clear();
    //if(initLiveChannelSlices() > 0)
    //{
    //	;
    //}
    //else
    //{
    //	liveLog->log(msgINFO,"initLiveChannelSlices no file found: channelID = %d,channelName = %s, channelPath = %s",channelID, channelName, channelFilePath);
    //}

    //logTimeSlice();
    //lastFormatErrorLogTime = 0;
}

CLiveChannel::~CLiveChannel(void)//此时只有一个线程可能访问到该节点
{
}

UINT CLiveChannel::getPCRPidFromFile(const char *fileName)
{
    //038_38_20120424_110635_005005_13126_B425FB32_256.ts

    //liveLog->log(msgINFO, "fileName: %s", fileName);
    UINT ret = 0;
    int i = 0;
    int j = 0;
    char tmpPcrPID[256];
    memset(tmpPcrPID, 0, 256);
    int len = strlen(fileName);
    while(i < len)
    {
        if(fileName[i] != '_')
        {
            i ++;
        }
        else
        {
            j ++;
            i ++;
            if( j == 7)
            {
                break;
            }
        }
    }

    //liveLog->log(msgINFO, "fileName: %s", fileName + i);

    j = 0;
    while(i < len)
    {
        if(fileName[i] != '_' && fileName[i] != '.')
        {
            tmpPcrPID[j] = fileName[i];
            j ++;
            i ++;
        }
        else
        {
            break;
        }
    }
    tmpPcrPID[j] = '\0';

    //liveLog->log(msgINFO, "pcrPID = %s", tmpPcrPID);
    ret = atoi(tmpPcrPID);
    if(ret > 0)
    {
        pcrPID = ret;
    }
    return ret;
}

//int CLiveChannel::getTheFileIndex(time_t* t)
//{
//	
//	int ret = -1;
//    _finddata_t file; 
//    long lf; 
//	char filespec[256];
//	//001_CCTV1_0001_20110704_093806_060033.ts
//	snprintf(filespec, 256, "%s%3d_%s_????_*.ts", channelFilePath.c_str(), channelID, channelName.c_str());
//	for(int i = 0; i<strlen(filespec); i++)
//	{
//		if(filespec[i] == ' ')
//			filespec[i] = '0';
//	}
//
//	if((lf = _findfirst(filespec,&file))==-1l)
//	{
//		liveLog->log(msgERROR, "getTheFileIndex ERROR, no related files found");
//	}
//    else 
//    { 
//		time_t t1, t2;
//        do{ 
//            //cout<<file.name;
//			string fileName(file.name);
//			if(checkFileName(fileName.c_str()))
//			{
//				getStartTimeFromFileName(fileName.c_str(), &t1);
//				getEndTimeFromFileName(fileName.c_str(), &t2);
//				if(*t >= t1 && *t <= t2)
//				{
//					char index[10];
//					strncpy(index, file.name + 3 + 2 + strlen(channelName.c_str()), 4);
//					*t = t1;
//					ret = atoi(index);
//					liveLog->message(msgDEBUG, "%s: %s: %d", file.name, index, ret);
//					break;
//				}
//			}
//			else
//			{
//				liveLog->log(msgINFO, "%s is not %s's file", fileName.c_str(), channelName.c_str());
//			}
//        } while( _findnext( lf, &file ) == 0 );
//	}
//	return ret;
//}


int CLiveChannel::getTheFileIndex(time_t* t)
{

    int ret = -1;

    CMyFile myfile;
    char filespec[256];
	struct tm * timeinfo = localtime(t);
	//001_ch0_20111220_122510_005600_899_A3B597CE.ts
#if defined(MOHO_X86)
	mysprintf(filespec, 256, "%3d_%s_%4d%2d%2d_%2d%2d%1d(.?)_(.*).ts", 
		//channelFilePath.c_str(), 
		channelID, 
		channelName.c_str(),
		timeinfo->tm_year+1900,
		timeinfo->tm_mon+1,
		timeinfo->tm_mday,
		timeinfo->tm_hour,
		timeinfo->tm_min,
		timeinfo->tm_sec / 10);
#elif defined(MOHO_WIN32)
	mysprintf(filespec, 256, "%3d_%s_%4d%2d%2d_%2d%2d%1d?_*.ts", 
		//channelFilePath.c_str(), 
		channelID, 
		channelName.c_str(),
		timeinfo->tm_year+1900,
		timeinfo->tm_mon+1,
		timeinfo->tm_mday,
		timeinfo->tm_hour,
		timeinfo->tm_min,
		timeinfo->tm_sec / 10);
#endif
	
	for(UINT i = 0; i<strlen(filespec); i++)
	{
		if(filespec[i] == ' ')
			filespec[i] = '0';
	}

    //点播数据断点续传
    int succeededID = -1;
    if(isVOD)
    {
        //001_ch0_20111220_122510_005600_899_A3B597CE.ts
        std::string succeededIDFile = channelFilePath + "succeededID.txt";
        FILE *fp = fopen(succeededIDFile.c_str(), "rb");
        if(fp != NULL)
        {
            fscanf(fp, "%d", &succeededID);
            //liveLog->log(msgINFO, "VOD channel %s last succeededID = %d", channelName.c_str(), succeededID);
            fclose(fp);
        }
        else
        {
            messager(msgERROR, "open %s ERROR", succeededIDFile.c_str());
        }

#if defined(MOHO_X86)
        mysprintf(filespec, 256, "%3d_%s_(.?)(.?)(.?)(.?)(.?)(.?)(.?)(.?)_(.?)(.?)(.?)(.?)(.?)(.?)_(.?)(.?)(.?)(.?)(.?)(.?)_%d_(.*).ts", channelID, channelName.c_str(), succeededID + 1);
#elif defined(MOHO_WIN32)
        mysprintf(filespec, 256, "%3d_%s_????????_??????_??????_%d_*.ts", channelID, channelName.c_str(), succeededID + 1);
#endif
        for(UINT i = 0; i<strlen(filespec); i++)
        {
            if(filespec[i] == ' ')
                filespec[i] = '0';
        }

        if(0 == myfile.findFirst((char*)channelFilePath.c_str(),filespec))
        {
            //liveLog->log(msgERROR, "getTheFileIndex ERROR, no related files found");
        }
        else
        {
            do{
                char* name = myfile.getName();

                if(checkFileName(name))
                {
                    char index[20];
                    UINT j = 3 + strlen(channelName.c_str()) + 8 + 6 + 6 + 5;
                    UINT jIndex = 0;
                    while(name[j] != '_' && j < strlen(name))
                    {
                        index[jIndex] = name[j];
                        jIndex ++;
                        j ++;
                    }
                    index[jIndex] = '\0';
                    int fileIndex = atoi(index);
                    if(fileIndex == succeededID + 1)
                    {
                        time_t s, t;
                        time(&s);
                        getStartTimeFromFileName(name, &t);
                        if(s - t > (TS_CONSUMER_START_TIME_OFFSET * 60))
                        {
                            liveLog->log(msgINFO, "VOD channel %s find the CONTINUED ID %d, s - t = %d S", channelName.c_str(), fileIndex, s-t);
                            ret = fileIndex;
                        }
                        else
                        {
                            //liveLog->log(msgINFO, "VOD channel %s find the CONTINUED ID %d, but HAS NO ENOUGH FILES NOW, s - t = %d S", channelName.c_str(), fileIndex, s - t);
                        }
                        break;
                    }
                    else
                    {
                        //liveLog->log(msgINFO, "channel %s's id = %d ISNOT the CONTINUED ID %d", channelName.c_str(), fileIndex, succeededID);
                    }
                }
                else
                {
                    liveLog->log(msgINFO, "%s is not %s's file", name, channelName.c_str());
                }
            } while(myfile.findNext());
        }

        myfile.close();
    }
    else
    {
        //liveLog->log(msgINFO, "%s is not VOD, isVOD = %d", channelName.c_str(), isVOD);
    }
    if(ret > 0)
    {
        return ret;
    }
    else
    {
        //liveLog->log(msgINFO, "%s has NO CONTINUED ID %d + 1 for resend", channelName.c_str(), succeededID);
    }

    //点播数据断点续传
//#if defined(MOHO_X86)
//    mysprintf(filespec, 256, "%3d_%s_(.?)(.?)(.?)(.?)(.?)(.?)(.?)(.?)_(.?)(.?)(.?)(.?)(.?)(.?)_(.*).ts", channelID, channelName.c_str());
//#elif defined(MOHO_WIN32)
//    //001_ch0_20111220_122510_005600_899_A3B597CE.ts
//    mysprintf(filespec, 256, "%3d_%s_????????_*.ts", channelID, channelName.c_str());
//#endif
    for(UINT i = 0; i<strlen(filespec); i++)
    {
        if(filespec[i] == ' ')
            filespec[i] = '0';
    }

    if(0 == myfile.findFirst((char*)channelFilePath.c_str(),filespec))
    {
        //liveLog->log(msgERROR, "getTheFileIndex ERROR, no related files found");
    }
    else 
    { 
        time_t t1, t2;
        do{ 
            char *name = myfile.getName();

            if(checkFileName(name))
            {
                getStartTimeFromFileName(name, &t1);
                getEndTimeFromFileName(name, &t2);
                if(*t >= t1 && *t <= t2)
                {
                    char index[20];
                    //strncpy(index, file.name + 3 + 2 + strlen(channelName.c_str()), 4);

                    UINT j = 3 + strlen(channelName.c_str()) + 8 + 6 + 6 + 5;
                    UINT jIndex = 0;
                    while(name[j] != '_' && j < strlen(name))
                    {
                        index[jIndex] = name[j];
                        jIndex ++;
                        j ++;
                    }
                    index[jIndex] = '\0';

                    *t = t1;
                    ret = atoi(index);
                    liveLog->message(msgDEBUG, "%s: %s: %d", name, index, ret);
                    break;
                }
            }
            else
            {
                liveLog->log(msgINFO, "%s is not %s's file", name, channelName.c_str());
            }
        } while(myfile.findNext());
    }

    myfile.close();

    if(ret == -1)
    {
        liveLog->log(msgERROR, "%s NOT FOUND DATA", channelName.c_str());
    }
    return ret;
}


UINT32 CLiveChannel::getTheMaxFileIndex()
{

    UINT32 ret = 0;

    CMyFile myfile;
    char filespec[256];
	//001_ch0_20111220_122510_005600_899_A3B597CE.ts
#if defined(MOHO_X86)
	mysprintf(filespec, 256, "%3d_%s_(.?)(.?)(.?)(.?)(.?)(.?)(.?)(.?)_(.*).ts", 
		//channelFilePath.c_str(), 
		channelID, 
		channelName.c_str());
#elif defined(MOHO_WIN32)
	mysprintf(filespec, 256, "%3d_%s_????????_*.ts", 
		//channelFilePath.c_str(), 
		channelID, 
		channelName.c_str());
#endif
	
	for(UINT i = 0; i<strlen(filespec); i++)
	{
		if(filespec[i] == ' ')
			filespec[i] = '0';
	}

    if(0 == myfile.findFirst((char*)channelFilePath.c_str(),filespec))
    {
        //liveLog->log(msgERROR, "getTheFileIndex ERROR, no related files found");
    }
    else 
    { 
        do{ 
            char *name = myfile.getName();

            if(checkFileName(name))
            {
                char index[20];
                //strncpy(index, file.name + 3 + 2 + strlen(channelName.c_str()), 4);

                UINT j = 3 + strlen(channelName.c_str()) + 8 + 6 + 6 + 5;
                UINT jIndex = 0;
                while(name[j] != '_' && j < strlen(name))
                {
                    index[jIndex] = name[j];
                    jIndex ++;
                    j ++;
                }
                index[jIndex] = '\0';
				UINT32 tmp = atoi(index);
				if(tmp > ret)
				{
					ret = tmp;
				}
            }
            else
            {
                liveLog->log(msgINFO, "%s is not %s's file", name, channelName.c_str());
            }
        } while(myfile.findNext());
    }

    myfile.close();

    if(ret == 0)
    {
        liveLog->log(msgERROR, "%s NOT FOUND DATA", channelName.c_str());
    }
	else if(channelStyle == channelSuperTs && ret <= SUPER_TS_DELAY_NUMBER ||
		channelStyle > channelSuperTs && ret <= SUPER_NORMAL_DELAY_NUMBER)
	{
		liveLog->log(msgERROR, "%s HAS NOT ENOUGH DATA, ret = %d", channelName.c_str(), ret);
		ret = 0;
	}
	else
	{
		if(channelStyle == channelSuperTs)
		{
			ret -= SUPER_TS_DELAY_NUMBER;
		}
		else
		{
			ret -= SUPER_NORMAL_DELAY_NUMBER;
		}
		liveLog->log(msgINFO, "Max Index of %s is %d", channelName.c_str(), ret);
	}
    return ret;
}

//UINT CLiveChannel::initLiveChannelSlices()
//{
//	UINT ret = 0;
//    _finddata_t file; 
//    long lf; 
//	char filespec[256];
//	//001_CCTV1_0001_20110704_093806_060033.ts
//	snprintf(filespec, 256, "%s%3d_%s_????_*.ts", channelFilePath.c_str(), channelID, channelName.c_str());
//	for(int i = 0; i<strlen(filespec); i++)
//	{
//		if(filespec[i] == ' ')
//			filespec[i] = '0';
//	}
//
//	if((lf = _findfirst(filespec,&file))==-1l)
//	{
//		liveLog->log(msgERROR, "initLiveChannelSlices ERROR, no related files found");
//	}
//    else 
//    { 
//		time_t t1, t2;
//        do{ 
//            //cout<<file.name; 
//			string fileName(file.name);
//			if(checkFileName(fileName.c_str()))
//			{
//				getStartTimeFromFileName(fileName.c_str(), &t1);
//				getEndTimeFromFileName(fileName.c_str(), &t2);
//				if(!insertTimeSlice(t1, t2))
//				{
//					liveLog->log(msgERROR, "initLiveChannelSlices insert ERROR, %s is not an independent slice", fileName.c_str() );
//				}
//				else
//				{
//					ret ++;
//				}
//			}
//			else
//			{
//				liveLog->log(msgINFO, "%s is not %s's file", fileName.c_str(), channelName.c_str());
//			}
//        } while( _findnext( lf, &file ) == 0 );
//	}
//	return ret;
//}

/*
UINT CLiveChannel::initLiveChannelSlices()
{
	return 1;
	UINT ret = 0;
    _finddata_t file; 
    long lf; 
	char filespec[256];
	//001_ch0_20111220_122510_005600_899_A3B597CE.ts
        snprintf(filespec, 256, "%s%3d_%s_????????_*.ts", channelFilePath.c_str(), channelID, channelName.c_str());
	for(UINT i = 0; i<strlen(filespec); i++)
	{
		if(filespec[i] == ' ')
			filespec[i] = '0';
	}

	if((lf = _findfirst(filespec,&file))==-1l)
	{
		liveLog->log(msgERROR, "initLiveChannelSlices ERROR, no related files found");
	}
    else 
    { 
		time_t t1, t2;
        do{ 
            //cout<<file.name; 
			std::string fileName(file.name);
			if(checkFileName(fileName.c_str()))
			{
				getStartTimeFromFileName(fileName.c_str(), &t1);
				getEndTimeFromFileName(fileName.c_str(), &t2);
				if(!insertTimeSlice(t1, t2))
				{
					liveLog->log(msgERROR, "initLiveChannelSlices insert ERROR, %s is not an independent slice", fileName.c_str() );
				}
				else
				{
					ret ++;
				}
			}
			else
			{
				liveLog->log(msgINFO, "%s is not %s's file", fileName.c_str(), channelName.c_str());
			}
        } while( _findnext( lf, &file ) == 0 );
	}
	_findclose(lf);
	return ret;
}


bool CLiveChannel::insertTimeSlice(time_t t1, time_t t2)
{
	bool ret = false;
	timeSlice newSlice;// = new timeSlice;
	newSlice.startTime = t1;
	newSlice.endTime = t2;
	if(liveChannelSlices.empty())
	{
		liveChannelSlices.push_back(newSlice);
		return true;
	}
	for(liveIter = liveChannelSlices.begin(); liveIter != liveChannelSlices.end(); liveIter ++)
	{
		if(t1 == (*liveIter).endTime)
		{
			(*liveIter).endTime += (t2 - t1);
			ret = true;
			break;
		}
		if(t2 == (*liveIter).startTime)
		{
			(*liveIter).startTime -= (t2 - t1);
			ret = true;
			break;
		}
		if(t2 < (*liveIter).startTime)
		{
			liveChannelSlices.insert(liveIter, newSlice); 
			ret = true;
			break;
		}
	}
	if(liveIter == liveChannelSlices.end())
	{
		//liveIter --;
		if(t1 >= liveChannelSlices.back().endTime)
		{
			liveChannelSlices.insert(liveChannelSlices.end(), newSlice); 
			ret = true;
		}
		if(t1 >= liveChannelSlices.back().startTime)
		{
			liveChannelSlices.insert(liveChannelSlices.end(), newSlice);
			liveLog->log(msgWARN, "SLICE OVER LAPPED");
			ret = true;
		}
	}
	return ret;
}

bool CLiveChannel::removeTimeSliceFromFile(std::string fileName)
{
	bool ret = false;
	std::list<timeSlice>::iterator lastIter;
	if(checkFileName(fileName.c_str()))
	{
		time_t t1, t2;
		getStartTimeFromFileName(fileName.c_str(), &t1);
		getEndTimeFromFileName(fileName.c_str(), &t2);
		for(liveIter = liveChannelSlices.begin(); liveIter != liveChannelSlices.end(); liveIter ++)
		{
			if(t1 >= (*liveIter).startTime && t1 <= (*liveIter).endTime &&
			   t2 >= (*liveIter).startTime && t2 <= (*liveIter).endTime)
			{
				timeSlice s1, s2;
				s1.startTime = (*liveIter).startTime;
				s1.endTime = t1;

				s2.startTime = t2;
				s2.endTime = (*liveIter).endTime;

				if(s1.startTime < s1.endTime)
				{
					liveChannelSlices.insert(liveIter, s1);
				}
				
				if(s2.startTime < s2.endTime)
				{
					liveChannelSlices.insert(liveIter, s2);
				}

				liveChannelSlices.erase(liveIter);	
				
				ret = true;
				break;
			}			
		}
		//if(liveIter == liveChannelSlices.end())
		//{
		//	if(t1 >= (*liveIter).startTime && t1 <= (*liveIter).startTime &&
		//	   t2 >= (*liveIter).startTime && t2 <= (*liveIter).startTime)
		//	{
		//		timeSlice s1, s2;
		//		s1.startTime = (*liveIter).startTime;
		//		s1.endTime = t1;

		//		s2.startTime = t2;
		//		s2.endTime = (*liveIter).endTime;

		//		liveChannelSlices.erase(liveIter);

		//		if(s1.startTime < s1.endTime)
		//		{
		//			liveChannelSlices.insert(liveIter, s1); 
		//		}
		//		
		//		if(s2.startTime < s2.endTime)
		//		{
		//			liveChannelSlices.insert(liveIter, s1); 
		//		}
		//		
		//		ret = true;
		//	}			
		//}
	}
	else
	{
		liveLog->log(msgDEBUG, "%s is not %s's file", fileName.c_str(), channelName.c_str());
	}
	return ret;
}


bool CLiveChannel::insertTimeSliceFromFile(std::string fileName)
{
	bool ret = false;
	if(checkFileName(fileName.c_str()))
	{
		time_t t1, t2;
		getStartTimeFromFileName(fileName.c_str(), &t1);
		getEndTimeFromFileName(fileName.c_str(), &t2);
		if(!insertTimeSlice(t1, t2))
		{
			liveLog->log(msgERROR, "initLiveChannelSlices insert ERROR, %s is not a independent slice", fileName.c_str() );
		}
		else
		{
			ret = true;
		}
	}
	else
	{
		liveLog->log(msgDEBUG, "%s is not %s's file", fileName.c_str(), channelName.c_str());
	}
	return ret;
}

//bool CLiveChannel::checkFileName(const char *fileName)
//{
//	//001_ch0_20111220_122510_005600_899_A3B597CE.ts
//	bool ret = false;
//	int i = strlen(fileName);
//	if(i != 35 + strlen(channelName.c_str()))
//	{
//		ret = false;
//	}
//	else
//	{
//		i--;
//		if(fileName[i] == 's' || fileName[i] == 'S' &&
//		   fileName[i - 1] == 't' || fileName[i] == 'T' &&
//		   fileName[i - 2] == '.' &&
//		   fileName[i - 3] >= '0' && fileName[i-3] <= '9' &&
//		   fileName[i - 4] >= '0' && fileName[i-4] <= '9' &&
//		   fileName[i - 5] >= '0' && fileName[i-5] <= '9' &&
//		   fileName[i - 6] >= '0' && fileName[i-6] <= '9' &&
//		   fileName[i - 7] >= '0' && fileName[i-7] <= '9' &&
//		   fileName[i - 8] >= '0' && fileName[i-8] <= '9' &&
//		   fileName[i - 9] == '_' &&
//		   fileName[i - 10] >= '0' && fileName[i-10] <= '9' &&
//		   fileName[i - 11] >= '0' && fileName[i-11] <= '9' &&
//		   fileName[i - 12] >= '0' && fileName[i-12] <= '9' &&
//		   fileName[i - 13] >= '0' && fileName[i-13] <= '9' &&
//		   fileName[i - 14] >= '0' && fileName[i-14] <= '9' &&
//		   fileName[i - 15] >= '0' && fileName[i-15] <= '9' &&
//		   fileName[i - 16] == '_' &&
//		   fileName[i - 17] >= '0' && fileName[i-17] <= '9' &&
//		   fileName[i - 18] >= '0' && fileName[i-18] <= '9' &&
//		   fileName[i - 19] >= '0' && fileName[i-19] <= '9' &&
//		   fileName[i - 20] >= '0' && fileName[i-20] <= '9' &&
//		   fileName[i - 21] >= '0' && fileName[i-21] <= '9' &&
//		   fileName[i - 22] >= '0' && fileName[i-22] <= '9' &&
//		   fileName[i - 23] >= '0' && fileName[i-23] <= '9' &&
//		   fileName[i - 24] >= '0' && fileName[i-24] <= '9' &&
//		   fileName[i - 25] == '_' &&
//		   fileName[i - 26] >= '0' && fileName[i-26] <= '9' &&
//		   fileName[i - 27] >= '0' && fileName[i-27] <= '9' &&
//		   fileName[i - 28] >= '0' && fileName[i-28] <= '9' &&
//		   fileName[i - 29] >= '0' && fileName[i-29] <= '9' &&
//		   fileName[i - 30] == '_' &&
//		   fileName[0] >= '0' && fileName[0] <= '9' &&
//		   fileName[1] >= '0' && fileName[1] <= '9' &&
//		   fileName[2] >= '0' && fileName[2] <= '9')
//		{
//			char tmp[256];
//			strncpy(tmp, fileName, 3);
//			tmp[3] = '\0';
//			if(atoi(tmp) == channelID)
//			{
//				strncpy(tmp, fileName + 4, strlen(channelName.c_str()));
//				tmp[strlen(channelName.c_str())] = '\0';
//				if(strcmp(tmp, channelName.c_str()) == 0)
//				{
//					ret = true;
//				}
//			}
//		}
//	}
//	return ret;
//}
*/
BOOL CLiveChannel::checkFileName(const char *fileName)
{
    //001_ch0_20111220_122510_005600_899_A3B597CE.ts
	return true;
    BOOL ret = false;
    int i = strlen(fileName);
    if(i < 3 + 1 + 1 + 8 + 1 + 6 + 1 + 6 + 1 + 8 + 3)
    {
        ret = false;
    }
    else
    {
        i--;
        if(fileName[i] == 's' || fileName[i] == 'S' &&
           fileName[i - 1] == 't' || fileName[i] == 'T' &&
           fileName[i - 2] == '.' &&
           fileName[0] >= '0' && fileName[0] <= '9' &&
           fileName[1] >= '0' && fileName[1] <= '9' &&
           fileName[2] >= '0' && fileName[2] <= '9')
        {
            int j = 4;
            while(1)
            {
                if(fileName[j] != '_')
                {
                    j ++;
                }
                else
                {
                    break;
                }
                if(j >= i)
                {
                    liveLog->log(msgERROR, "FILE FORMAT ERROR: %s", fileName);
                    return false;
                }
            }

            j ++;

            if(fileName[j] >= '0' && fileName[j] <= '9' &&
               fileName[j + 1] >= '0' && fileName[j + 1] <= '9' &&
               fileName[j + 2] >= '0' && fileName[j + 2] <= '9' &&
               fileName[j + 3] >= '0' && fileName[j + 3] <= '9' &&
               fileName[j + 4] >= '0' && fileName[j + 4] <= '9' &&
               fileName[j + 5] >= '0' && fileName[j + 5] <= '9' &&
               fileName[j + 6] >= '0' && fileName[j + 6] <= '9' &&
               fileName[j + 7] >= '0' && fileName[j + 7] <= '9' &&
               fileName[j + 8] == '_' &&
               fileName[j + 9] >= '0' && fileName[j + 9] <= '9' &&
               fileName[j + 10] >= '0' && fileName[j + 10] <= '9' &&
               fileName[j + 11] >= '0' && fileName[j + 11] <= '9' &&
               fileName[j + 12] >= '0' && fileName[j + 12] <= '9' &&
               fileName[j + 13] >= '0' && fileName[j + 13] <= '9' &&
               fileName[j + 14] >= '0' && fileName[j + 14] <= '9' &&
               fileName[j + 15] == '_' &&
               fileName[j + 16] >= '0' && fileName[j + 16] <= '9' &&
               fileName[j + 17] >= '0' && fileName[j + 17] <= '9' &&
               fileName[j + 18] >= '0' && fileName[j + 18] <= '9' &&
               fileName[j + 19] >= '0' && fileName[j + 19] <= '9' &&
               fileName[j + 20] >= '0' && fileName[j + 20] <= '9' &&
               fileName[j + 21] >= '0' && fileName[j + 21] <= '9' &&
               fileName[j + 22] == '_')
            {

                char tmp[256];
                strncpy(tmp, fileName, 3);
                tmp[3] = '\0';
                if(atoi(tmp) == channelID)
                {
                    strncpy(tmp, fileName + 4, strlen(channelName.c_str()));
                    tmp[strlen(channelName.c_str())] = '\0';
                    if(strcmp(tmp, channelName.c_str()) == 0)
                    {
                        ret = true;
                    }
                }
            }
        }
    }
    if(ret == false)
    {
        UINT tickcount;
#if defined(MOHO_X86)
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        tickcount = ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
#elif defined(MOHO_WIN32)
        tickcount = GetTickCount();
#endif

        if(tickcount - lastFormatErrorLogTime > 60 * 1000 || tickcount < lastFormatErrorLogTime)
        {
            lastFormatErrorLogTime = tickcount;
            liveLog->log(msgERROR, "FILE FORMAT ERROR: %s", fileName);
        }
    }
    return ret;
}

/*
UINT CLiveChannel::checkFileUpdate()
{
	UINT ret = 0;

	fileNotifyinfo.filePath = channelFilePath.c_str();
	fileNotifyinfo.channel = this;
	//DWORD dwThreadID = 0;
	//HANDLE hThread = CreateThread(
	//					NULL,                    //安全属性使用缺省。
	//					0,                         //线程的堆栈大小。
	//					fileNotifyThreadProc,                 //线程运行函数地址。
	//					&fileNotifyinfo,               //传给线程函数的参数。
	//					0,                         //创建标志。
	//					&dwThreadID);       //成功创建后的线程标识码。
	return ret;
}


void CLiveChannel::mergeTimeSlice()
{
	if(liveChannelSlices.size() <= 1)
	{
		return;
	}

	bool merged = false;
	do{
		merged = false;
		for(liveIter = liveChannelSlices.begin(); liveIter != liveChannelSlices.end(); liveIter ++)
		{
			std::list<timeSlice>::iterator lastIter = liveIter;
			liveIter ++;
			if(liveIter != liveChannelSlices.end())
			{
				if((*liveIter).startTime == (*lastIter).endTime)
				{
					(*liveIter).startTime = (*lastIter).startTime;
					liveChannelSlices.erase(lastIter);
					merged = true;
					break;
				}
				else if(abs(liveIter->startTime - lastIter->endTime) < SLICE_PERMISSIBLE_ERROR)
				{
					(*liveIter).startTime = (*lastIter).startTime;
					liveChannelSlices.erase(lastIter);
					merged = true;
					break;
				}
			}
			liveIter --;
		}
	}while(merged);
}

void CLiveChannel::logTimeSlice()
{
	int i = 0;
	char t1[256], t2[256];
	for(liveIter = liveChannelSlices.begin(); liveIter != liveChannelSlices.end(); liveIter ++)
	{
		strcpy(t1, ctime(&(*liveIter).startTime));
		t1[strlen(t1)-1] = '\0';
		strcpy(t2, ctime(&(*liveIter).endTime));
		t2[strlen(t2)-1] = '\0';
		liveLog->log(msgDEBUG, "%s SLICE %d %s-%s", channelName.c_str(), i, t1, t2);
	}
	//if(liveChannelSlices.size() > 1)
	//{
	//	strcpy(t1, ctime(&(*liveIter).startTime));
	//	strcpy(t2, ctime(&(*liveIter).startTime));
	//	liveLog->log(msgINFO, "%s SLICE %d %s-%s", channelName.c_str(), i, t1, t2);
	//}
}


void CLiveChannel::logTimeSlice4Search()
{
	int i = 0;
	char t1[256], t2[256];
	for(liveIter = liveChannelSlices.begin(); liveIter != liveChannelSlices.end(); liveIter ++)
	{
		strcpy(t1, ctime(&(*liveIter).startTime));
		t1[strlen(t1)-1] = '\0';
		strcpy(t2, ctime(&(*liveIter).endTime));
		t2[strlen(t2)-1] = '\0';
		liveLog->log(msgINFO, "%s SLICE %d %s-%s", channelName.c_str(), i, t1, t2);
	}
	//if(liveChannelSlices.size() > 1)
	//{
	//	strcpy(t1, ctime(&(*liveIter).startTime));
	//	strcpy(t2, ctime(&(*liveIter).startTime));
	//	liveLog->log(msgINFO, "%s SLICE %d %s-%s", channelName.c_str(), i, t1, t2);
	//}
}
*/
time_t CLiveChannel::searchTimeSlice(time_t s, time_t e)
{
    char t1[256];
    char t2[256];

    strcpy(t1, ctime(&s));
    t1[strlen(t1)-1] = '\0';

    strcpy(t2, ctime(&e));
    t2[strlen(t2)-1] = '\0';

    liveLog->log(msgINFO, "SEARCH REQUEST: channel %d s = %s, e = %s", channelID, t1, t2);
    if(isDeleted)
    {
        liveLog->log(msgINFO, "channel %d is DELETED", channelID);
        return 0;
    }

    int index = getTheFileIndex( &s );
    if(index != -1)
    {
        liveLog->log(msgINFO, "channel %d FOUND, index = %d", channelID, index);
        return s;
    }
    else
    {
        liveLog->log(msgINFO, "channel %d NOT FOUND", channelID);
        return 0;
    }

}

/*
time_t CLiveChannel::getTheLastTime()
{
	time_t ret = 0;
	for(liveIter = liveChannelSlices.begin(); liveIter != liveChannelSlices.end(); liveIter ++)
	{
		if(liveIter->endTime > ret)
		{
			ret = liveIter->endTime;
		}
	}
	return ret;
}*/
