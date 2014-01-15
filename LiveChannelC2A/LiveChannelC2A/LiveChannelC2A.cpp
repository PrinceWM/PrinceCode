// LiveChannelC2A.cpp : 定义应用程序的入口点。
#include "stdafx.h"
#include "LiveChannelC2A.h"
#include "message.h"
#include "netmain.h"
#include "LiveChannel.h"
#include "xmlParser.h"
#include "myfile.h"
#include "tracker.h"

#define MAX_LOADSTRING 100

extern INT64 _GetTickCount64();

std::string g_localPath;
CNetLog* normalLog = NULL;
CNetLog * liveLog = NULL;
// 全局变量:

CLiveChannel *tstChannel;										//测试模块使用的全局变量，正式版本中不使用
std::list<CLiveChannel *> globalLiveChannelList;				//频道资源列表
std::list<neighborNode> globalNeighborNodeList;					//邻居列表
std::list<tsSupplierTask> globalTsSupplierTaskList;				//提供者任务列表
std::list<tsConsumerTask> globalTsConsumerTaskList;				//消费者任务列表
std::list<tsRelayTask> globalTsRelayTaskList;					//中转任务列表
std::list<tsQueryTask> globalTsQueryTaskList;					//中转查询列表
std::deque<trackerNode> globalTrackerDeque;
myConfig globalConfig;											//节点配置


CMyLock lockLiveChannelList;							//频道资源列表访问锁
CMyLock lockNeighborNodeList;						//邻居访问锁，在1225版本后不再使用
CMyLock lockTsSupplierTaskList;						//提供者任务列表访问锁
CMyLock lockRelayTaskList;								//中转任务列表访问锁
CMyLock lockQueryTaskList;								//中转查询任务列表访问锁

extern void sendChannelUpdate(tsSupplierTask *task, int channelEvent);
extern SOCKET sockPublicationSrv;

void deleteOldFile(tsConsumerTask *pInfo)//删除接收端目录下所有ts类型的文件，在程序重新启动时调用
{
    CMyFile myfile;
    char filespec[256];

#if defined(MOHO_X86)
    mysprintf(filespec, 256, "(.*).ts");
#elif defined(MOHO_WIN32)
    mysprintf(filespec, 256, "*.ts");
#endif

    if(1 == myfile.findFirst((char*)pInfo->channelPath.c_str(),filespec))
    {
        char filename[256];
        do
        {
            char* name = myfile.getName();
            mysprintf(filename, 256, "%s%s", pInfo->channelPath.c_str(), name);
            remove(filename);
        }while(myfile.findNext());
    }
    myfile.close();
}

void exitSafely()
{
				std::list<tsSupplierTask>::iterator sIter;
				for(sIter = globalTsSupplierTaskList.begin(); sIter != globalTsSupplierTaskList.end(); sIter ++)
				{
					//if(sIter->channelID == ppRelease->channelID)
					{
						sendChannelUpdate(&(*sIter), evtQuit);
						sendChannelUpdate(&(*sIter), evtQuit);
						sendChannelUpdate(&(*sIter), evtQuit);

						sIter->EXITsupplier = TRUE;
						while(sIter->thread.isCreat() != 0)
						{
							mySleep(100);
						}
					   // globalTsSupplierTaskList.erase(sIter);
						messager(msgINFO, "ERASE channel %d from supplier task list,%d", sIter->channelID,sIter->thread.isCreat());
						break;
					}
				}

				std::list<tsConsumerTask>::iterator cIter;
				for(cIter = globalTsConsumerTaskList.begin(); cIter != globalTsConsumerTaskList.end(); cIter ++)
				{
					//if(cIter->channelID == ppRelease->channelID)
					{
						cIter->EXITconsumer = TRUE;
						cIter->EXITftds = TRUE;
						while(cIter->ftdsThread.isCreat() != 0 ||
							  cIter->consumerThread.isCreat() != 0)
						{
							SOCKADDR_IN localIP;
							int len;
							int tolen = sizeof(SOCKADDR_IN);
							char msg[1500];
							localIP.sin_family = AF_INET;
			#if defined(MOHO_X86)
							localIP.sin_addr.s_addr = globalConfig.ipAddress;
			#elif defined(MOHO_WIN32)
							localIP.sin_addr.S_un.S_addr = globalConfig.ipAddress;
			#endif

							localIP.sin_port = htons(cIter->consumerPort);
							strcpy(msg, "SEND TO END THE CONSUMER TASK");
							len = sendto(sockPublicationSrv, msg, strlen(msg), 0, (SOCKADDR *)(&localIP), tolen);
							mySleep(100);
						}
			#if defined(MOHO_X86)
						deleteOldFile((tsConsumerTask*)&(*cIter));
			#endif
						//globalTsConsumerTaskList.erase(cIter);
						messager(msgINFO, "ERASE channel %d from consumer task list", cIter->channelID);
						break;
					}
				}

				lockLiveChannelList.destroy();
				lockTsSupplierTaskList.destroy();
				lockRelayTaskList.destroy();
				lockQueryTaskList.destroy();
}

void checkGlobalList(BOOL checkValue)
{
    std::list<neighborNode>::iterator nIter;
    lockNeighborNodeList.lock();
    for(nIter = globalNeighborNodeList.begin(); nIter != globalNeighborNodeList.end(); nIter ++)
    {
        nIter->checked = checkValue;
    }
    lockNeighborNodeList.unlock();

    std::list<CLiveChannel *>::iterator lIter;
    lockLiveChannelList.lock();
    for(lIter = globalLiveChannelList.begin(); lIter != globalLiveChannelList.end(); lIter ++)
    {
        (*lIter)->checked = checkValue;
    }
    lockLiveChannelList.unlock();

    std::list<tsConsumerTask>::iterator cIter;
    std::list<ftds>::iterator fIter;
    for(cIter = globalTsConsumerTaskList.begin(); cIter != globalTsConsumerTaskList.end(); cIter ++)
    {
        cIter->checked = checkValue;
        for(fIter = cIter->ftdsList.begin(); fIter != cIter->ftdsList.end(); fIter ++)
        {
            fIter->checked = checkValue;
        }
    }
}

int deleteUnCheckedNeighbor()
{
    return 0;
}

int deleteUnCheckedTsFile()
{
    std::list<CLiveChannel *>::iterator lIter;
    BOOL itemDeleted = false;

    lockLiveChannelList.lock();
    do
    {
        itemDeleted = false;
        for(lIter = globalLiveChannelList.begin(); lIter != globalLiveChannelList.end(); lIter ++)
        {
            if((*lIter)->checked == false && (*lIter)->isDeleted == false)
            {
                logger(msgINFO, "channel %d unchecked", (*lIter)->channelID);
                std::list<tsSupplierTask>::iterator sIter;
                for(sIter = globalTsSupplierTaskList.begin(); sIter != globalTsSupplierTaskList.end(); sIter ++)
                {
                    if(sIter->channel == (*lIter))
                    {
                        logger(msgINFO, "STOP supplier where channelID = %d", sIter->channelID);
                        sIter->EXITsupplier = true;
                        mySleep(1000);
                        break;
                    }
                }
                logger(msgINFO, "ERASE channel %d", (*lIter)->channelID);
                delete (*lIter);
                globalLiveChannelList.erase(lIter);
                //(*lIter)->isDeleted = true;
                //(*lIter)->checked = true;
                itemDeleted = true;
                break;
            }
        }
    }while(itemDeleted);
    lockLiveChannelList.unlock();
    return 0;
}

int deleteUnCheckedConsumer()
{
    std::list<tsConsumerTask>::iterator cIter;
    std::list<ftds>::iterator fIter;
    BOOL itemDeleted = false;
    do
    {
        itemDeleted = false;
        for(cIter = globalTsConsumerTaskList.begin(); cIter != globalTsConsumerTaskList.end(); cIter ++)
        {
            //cIter->checked = checkValue;
            if(cIter->checked == false)
            {

                //for(fIter = cIter->ftdsList.begin(); fIter != cIter->ftdsList.end(); fIter ++)
                //{
                //	fIter->checked = checkValue;
                //}
                logger(msgINFO, "consumer channel %d unchecked", cIter->channelID);

                char msg[1500];
                logger(msgINFO, "STOP ftds where channelID = %d", cIter->channelID);
                cIter->EXITftds = true;
                while(cIter->ftdsThread.isCreat())
                {
                    mySleep(100);
                }

                int len;
                SOCKET localSendSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
                logger(msgINFO, "STOP consumerTask channel %d", cIter->channelID);
                cIter->EXITconsumer = true;
                while(cIter->consumerThread.isCreat())
                {
                    SOCKADDR_IN localIP;
                    localIP.sin_family = AF_INET;
#if defined(MOHO_X86)
                    localIP.sin_addr.s_addr = globalConfig.ipAddress;
#elif defined(MOHO_WIN32)
                    localIP.sin_addr.S_un.S_addr = globalConfig.ipAddress;
#endif
                    localIP.sin_port = htons(cIter->consumerPort);
                    int tolen = sizeof(localIP);
                    strcpy(msg, "SEND TO END THE CONSUMER TASK");
                    len = sendto(localSendSocket, msg, strlen(msg), 0, (SOCKADDR *)(&localIP), tolen);
                    mySleep(100);
                }
#if defined(MOHO_X86)
                close(localSendSocket);
#elif defined(MOHO_WIN32)
                closesocket(localSendSocket);
#endif


                logger(msgINFO, "ERASE consumer channelID = %d", cIter->channelID);
                //delete cIter->channel;
                globalTsConsumerTaskList.erase(cIter);
                itemDeleted = true;
                break;
            }
        }
    }while(itemDeleted);
    return 0;
}

int deleteUnCheckedFTDS()
{
    std::list<tsConsumerTask>::iterator cIter;
    std::list<ftds>::iterator fIter;
    for(cIter = globalTsConsumerTaskList.begin(); cIter != globalTsConsumerTaskList.end(); cIter ++)
    {
        //cIter->checked = checkValue;
        BOOL itemDeleted = false;
        do
        {
            itemDeleted = false;
            for(fIter = cIter->ftdsList.begin(); fIter != cIter->ftdsList.end(); fIter ++)
            {
                //fIter->checked = checkValue;

                if(fIter->checked == false)
                {
                    logger(msgINFO, "channel %d FTDS unchecked", cIter->channelID);
                    logger(msgINFO, "ERASE ftds channel %d", cIter->channelID);
                    cIter->ftdsList.erase(fIter);
                    itemDeleted = true;
                    break;
                }
            }
        }while(itemDeleted);

        if(cIter->ftdsList.size() == 0)
        {
            logger(msgWARN, "channel %d has no ftds", cIter->channelID);
        }

    }
    return 0;
}

/*
int tsReloadConfig()//重新加载配置文件，当配置管理线程监测到配置目录下的文件变化后，会调用此模块
{
    //读取XML文件，获取对应的配置信息
	XMLResults results;
    XMLNode xMainNode = XMLNode::parseFile("./config/config.xml","config",&results);

	if(xMainNode.isEmpty()|| results.error!=eXMLErrorNone)
	{
		logger(msgERROR,"loadConfig：获取根节点config出错");
        return -1;
	}
	
	checkGlobalList(false);

	XMLNode tsNode = xMainNode.getChildNode("tsTransNode");
	if (!tsNode.isEmpty())
	{
		//globalConfig.ipAddress = inet_addr(tsNode.getAttribute("ipAddress"));
		//globalConfig.supplyPort = atoi(tsNode.getAttribute("supplyPort"));
		globalConfig.zoneNumber = atoi(tsNode.getAttribute("zoneNumber"));
		globalConfig.Capability = atoi(tsNode.getAttribute("Capability"));
		globalConfig.maxSupplyBuffer = atoi(tsNode.getAttribute("maxSupplyBuffer"));
		//globalConfig.UID = rand();
		//globalConfig.seqNumber = 0;
		//globalConfig.currLoad = 0;
    }

	//globalNeighborNodeList.clear();
    tsNode = xMainNode.getChildNode("tsNeighbor");
    if (!tsNode.isEmpty())
    {
		int num = tsNode.nChildNode();
		std::list<neighborNode>::iterator nIter;
		neighborNode nn;
		for(int i = 0; i<num; i++)
		{
			XMLNode neighborNode = tsNode.getChildNode("neighborNode", i);
			nn.ipAddress = ntohl(inet_addr(neighborNode.getAttribute("ipAddress")));
			nn.Status = nnsDead;
			//nn.Status = nnsAlive;
			nn.Capability = 0;
			nn.currLoad = 0;
			nn.supplyPort = globalConfig.publicationPort;
			nn.UID = 0;
			nn.zoneNumber = 0;
			nn.checked = true;
			//globalNeighborNodeList.push_back(nn);

			EnterCriticalSection(&lockNeighborNodeList);
			for(nIter = globalNeighborNodeList.begin(); nIter != globalNeighborNodeList.end(); nIter ++)
			{
				if(nn.ipAddress == nIter->ipAddress)
				{
					nIter->checked = true;
					break;
				}
			}

			if(nIter == globalNeighborNodeList.end())
			{

				globalNeighborNodeList.push_back(nn);
				logger(msgINFO, "add new neighbor: %s", neighborNode.getAttribute("ipAddress"));
			}
			LeaveCriticalSection(&lockNeighborNodeList);
			
		}
	}

	deleteUnCheckedNeighbor();

	//globalLiveChannelList.clear();
	tsNode = xMainNode.getChildNode("tsFile");
	if(!tsNode.isEmpty())
	{
		int channelID;
		char channelName[256];
		char channelPath[256];
		char exceptionPath[256];
		int isException;
		int num = tsNode.nChildNode();
		std::list<CLiveChannel *>::iterator lIter;
		for(int i = 0; i<num; i++)
		{
			XMLNode channelNode = tsNode.getChildNode("channel", i);
			channelID = atoi(channelNode.getAttribute("channelID"));
			strcpy(channelName, channelNode.getAttribute("channelName"));
			strcpy(channelPath, channelNode.getAttribute("channelPath"));
			strcpy(exceptionPath, channelNode.getAttribute("exceptionPath"));
			isException = atoi(channelNode.getAttribute("isException"));
			BOOL isVOD;
			const char* isVODString = channelNode.getAttribute("isVOD");
			if(isVODString != NULL)
			{
				logger(msgINFO, "isVOD existed, isVOD = %s", isVODString);
				isVOD = atoi(isVODString);
			}
			else
			{
				logger(msgINFO, "isVOD not existed");
				isVOD = false;
			}
			EnterCriticalSection(&lockLiveChannelList);
			for(lIter = globalLiveChannelList.begin(); lIter != globalLiveChannelList.end(); lIter ++)
			{
				if((*lIter)->channelID == channelID)
				{
					(*lIter)->isDeleted = false;
					(*lIter)->checked = true;
					(*lIter)->channelName = channelName;
					(*lIter)->channelFilePath = channelPath;
					(*lIter)->exceptionFilePath = exceptionPath;
					(*lIter)->isException = isException;
					(*lIter)->isVOD = isVOD;
					(*lIter)->channelBitRate = atoi(channelNode.getAttribute("bitRate"));
					sscanf(channelNode.getAttribute("pcrPID"), "%x", &((*lIter)->pcrPID));
					logger(msgINFO, "channel %d changed: Name = %s, Path = %s, exPath = %s, isEx = %d, BitRate = %d, pcrPID = 0x%x, isVOD = %d",
						(*lIter)->channelID,
						(*lIter)->channelName.c_str(),
						(*lIter)->channelFilePath.c_str(),
						(*lIter)->exceptionFilePath.c_str(),
						(*lIter)->isException,
						(*lIter)->channelBitRate,
						(*lIter)->pcrPID,
						(*lIter)->isVOD);
					break;
				}
			}
			if(lIter == globalLiveChannelList.end())
			{
				CLiveChannel *newChannel = new CLiveChannel(channelID, channelName, channelPath, exceptionPath, isException, isVOD);
				newChannel->checked = true;
				newChannel->channelBitRate = atoi(channelNode.getAttribute("bitRate"));
				sscanf(channelNode.getAttribute("pcrPID"), "%x", &newChannel->pcrPID);
				globalLiveChannelList.push_back(newChannel);
			}
			LeaveCriticalSection(&lockLiveChannelList);
		}
	}


	//globalTsConsumerTaskList.clear();
	tsNode = xMainNode.getChildNode("tsConsumerTask");
	if(!tsNode.isEmpty())
	{
		tsConsumerTask tsCT;
		int num = tsNode.nChildNode();
		std::list<tsConsumerTask>::iterator cIter;
		for(int i = 0; i<num; i++)//should new CLiveChannel
		{
			XMLNode consumerNode = tsNode.getChildNode("channel", i);
			tsCT.channelID = (UINT)(atoi(consumerNode.getAttribute("channelID")));
			char channelName[256];
			char channelPath[256];
			strcpy(channelName, consumerNode.getAttribute("channelName"));
			strcpy(channelPath, consumerNode.getAttribute("channelPath"));
			tsCT.consumerPort = atoi(consumerNode.getAttribute("consumerPort"));
			tsCT.linkNumberReadFromConfigFile = atoi(consumerNode.getAttribute("linkNumber"));
			//tsCT.ipAddressFTDS = (inet_addr(consumerNode.getAttribute("ipAddressFTDS")));
			//tsCT.portFTDS = htons(atoi(consumerNode.getAttribute("portFTDS")));
			tsCT.channelName = channelName;
			tsCT.channelPath = channelPath;
			tsCT.bitRate = 0;
			tsCT.lastPDUTime = 0;
			tsCT.pcrPID = 0;
			tsCT.relay = false;
			tsCT.startTime = 0;
			tsCT.supplierUID = 0;
			tsCT.lastSupplierUID = 0;
			tsCT.Status = ctsNotStarted;
			tsCT.relayRealSupplierUID = 0;
			tsCT.transDataEvent = NULL;
			tsCT.threadHandle = NULL;
			tsCT.tickCount = GetTickCount();
			tsCT.recvBytes = 0;
			tsCT.lastResentStartSeqNum = 0;
			tsCT.lastResentStopSeqNum = 0;
			tsCT.resendTime = 0;
			tsCT.recvBuf = new char[1500];
			tsCT.arrivedBufList.clear();
			tsCT.consumerThread = NULL;
			tsCT.consumerThreadID = 0;
			tsCT.ftdsThread = NULL;
			tsCT.ftdsThreadID = 0;
			tsCT.fileIndex = 0;
			tsCT.savedFile.clear();
			tsCT.readyForFtds = false;
			tsCT.consumerTimes = 0;
			tsCT.lastPDUTimeTick = 0;
			tsCT.linkNumber = 1;
			tsCT.linkTwoTimeTick = 0;

			tsCT.ftdsList.clear();
			tsCT.waitForReplySeq.clear();

			tsCT.EXITftds = false;
			tsCT.EXITconsumer = false;

			//deleteOldFile(&tsCT);

			//InitializeCriticalSection(&tsCT.lockConsumerThread);

			//EnterCriticalSection(&lockTsConsumerTaskList);
			//logger(msgINFO, "consumer list size = %d", globalTsConsumerTaskList.size());
			for(cIter = globalTsConsumerTaskList.begin(); cIter != globalTsConsumerTaskList.end(); cIter ++)
			{
				//logger(msgINFO, "%d - %d", cIter->channelID, tsCT.channelID);
				if(cIter->channelID == tsCT.channelID)
				{
					cIter->checked = true;
					cIter->channel->checked = true;
					cIter->channelName = tsCT.channelName;
					cIter->channelPath = tsCT.channelPath;
					cIter->consumerPort = tsCT.consumerPort;
					cIter->linkNumberReadFromConfigFile = tsCT.linkNumberReadFromConfigFile;
					logger(msgINFO, "channel %d EXISTED", tsCT.channelID);
					break;
				}
			}
			if(cIter == globalTsConsumerTaskList.end())
			{
				CLiveChannel *newChannel = new CLiveChannel(tsCT.channelID, channelName, channelPath, "", 0, 0);
				newChannel->channelBitRate = 0;
				newChannel->pcrPID = 0;
				newChannel->checked = true;

				EnterCriticalSection(&lockLiveChannelList);
				globalLiveChannelList.push_back(newChannel);
				LeaveCriticalSection(&lockLiveChannelList);

				tsCT.channel = newChannel;
				tsCT.checked = true;
				globalTsConsumerTaskList.push_back(tsCT);
				//cIter = globalTsConsumerTaskList.back();
				//addNewConsumer(&(globalTsConsumerTaskList.back()));
				logger(msgINFO, "add new consumer, channelID = %d, channelName = %s, channelPath = %s", tsCT.channelID, channelName, channelPath);
			}
			//LeaveCriticalSection(&lockTsConsumerTaskList);
		}
	}

	deleteUnCheckedConsumer();
	deleteUnCheckedTsFile();

	tsNode = xMainNode.getChildNode("tsFTDS");
	if(!tsNode.isEmpty())
	{
		ftds ftdsItem;
		UINT channelID;
		int num = tsNode.nChildNode();
		for(int i = 0; i<num; i++)
		{
			XMLNode ftdsNode = tsNode.getChildNode("ftds", i);
			channelID = (UINT)(atoi(ftdsNode.getAttribute("channelID")));
			ftdsItem.ipAddressFTDS = (inet_addr(ftdsNode.getAttribute("ipAddressFTDS")));
			ftdsItem.portFTDS = htons(atoi(ftdsNode.getAttribute("portFTDS")));
			ftdsItem.checked = true;
			std::list<tsConsumerTask>::iterator iterCT;
			std::list<ftds>::iterator fIter;
			for(iterCT = globalTsConsumerTaskList.begin(); iterCT != globalTsConsumerTaskList.end(); iterCT ++)
			{
				if(iterCT->channelID == channelID)
				{
					for(fIter = iterCT->ftdsList.begin(); fIter != iterCT->ftdsList.end(); fIter ++)
					{
						if(fIter->ipAddressFTDS == ftdsItem.ipAddressFTDS && fIter->portFTDS == ftdsItem.portFTDS)
						{
							fIter->checked = true;
							break;
						}
					}
					if(fIter == iterCT->ftdsList.end())
					{
						iterCT->ftdsList.push_back(ftdsItem);
						logger(msgINFO, "add new ftds: channelID = %d, ip = %s, port = %s", channelID, ftdsNode.getAttribute("ipAddressFTDS"), ftdsNode.getAttribute("portFTDS"));
					}
					break;
				}
			}
			if(iterCT == globalTsConsumerTaskList.end())
			{				
				logger(msgERROR, "FTDS (%d, %s, %s) has no channel", channelID, ftdsNode.getAttribute("ipAddressFTDS"), ftdsNode.getAttribute("portFTDS"));
			}
		}
	}

	deleteUnCheckedFTDS();

	checkGlobalList(false);

	return 0;
}*/


int initSuperTsSupplierTask()
{
    std::list<CLiveChannel *>::iterator Iter;
	int i = 0;
    for(Iter = globalLiveChannelList.begin(); Iter != globalLiveChannelList.end(); Iter ++)
    {
		if((*Iter)->channelStyle == channelSuperTs)
		{
			startOneSupplier(*Iter, 1, globalConfig.UID, 0);
			i++;
		}
    }
	return i;
}

int tsLoadConfig()//系统启动时加载配置文件，初始化全局列表
{
    //读取XML文件，获取对应的配置信息

    memset(globalConfig.linkOneMonitorFilePath, 0, sizeof(globalConfig.linkOneMonitorFilePath));
    memset(globalConfig.linkTwoMonitorFilePath, 0, sizeof(globalConfig.linkTwoMonitorFilePath));

    XMLResults results;
    std::string name = g_localPath;
    name.append("config.xml");
    XMLNode xMainNode = XMLNode::parseFile(name.c_str(),"config",&results);

    if(xMainNode.isEmpty()|| results.error!=eXMLErrorNone)
    {
        logger(msgERROR,"2loadConfig：获取根节点config出错");
        return -1;
    }
    XMLNode levelNode = xMainNode.getChildNode("logLevel");
    globalConfig.logLevel = -1;
    globalConfig.logSaved = 0;
    if (!levelNode.isEmpty())
    {
        globalConfig.logLevel = atoi(levelNode.getAttribute("level"));
        globalConfig.logSaved = atoi(levelNode.getAttribute("isSaved"));
    }
    XMLNode tsNode = xMainNode.getChildNode("tsTransNode");
    if (!tsNode.isEmpty())
    {
		globalConfig.nodeType = (char)atoi(tsNode.getAttribute("nodeType"));
        globalConfig.ipAddress = inet_addr(tsNode.getAttribute("ipAddress"));
		

        globalConfig.publicationPort = atoi(tsNode.getAttribute("publicationPort"));
		messager(msgERROR, "globalConfig.publicationPort = %d",globalConfig.publicationPort);
        globalConfig.supplyPort = atoi(tsNode.getAttribute("supplyPort"));
        globalConfig.zoneNumber = atoi(tsNode.getAttribute("zoneNumber"));
        globalConfig.Capability = atoi(tsNode.getAttribute("Capability"));
        globalConfig.timeZone = (UCHAR)(atoi(tsNode.getAttribute("timeZone")));
        globalConfig.maxSupplyBuffer = atoi(tsNode.getAttribute("maxSupplyBuffer"));
		globalConfig.carrier = tsNode.getAttribute("carrier");
		globalConfig.province = tsNode.getAttribute("province");
		globalConfig.city = tsNode.getAttribute("city");

        srand((UINT)_GetTickCount64());
        int r1 = rand();
        mySleep(100);
        srand((UINT)(_GetTickCount64()));
        globalConfig.UID = rand()*r1;

        globalConfig.seqNumber = 0;
        globalConfig.currLoad = 0;
    }

	globalTrackerDeque.clear();
    tsNode = xMainNode.getChildNode("tsTracker");
    if (!tsNode.isEmpty())
    {
        int num = tsNode.nChildNode();
        trackerNode tn;
        for(int i = 0; i<num; i++)
        {
            XMLNode xmlTrackerNode = tsNode.getChildNode("tracker", i);
            tn.ipAddress = ntohl(inet_addr(xmlTrackerNode.getAttribute("ipAddress")));
			tn.servicePort = (USHORT)atoi(xmlTrackerNode.getAttribute("trackerPort"));
            globalTrackerDeque.push_back(tn);
        }
    }

	globalNeighborNodeList.clear();
    tsNode = xMainNode.getChildNode("tsNeighbor");
    if (!tsNode.isEmpty())
    {
        int num = tsNode.nChildNode();
        neighborNode nn;
        for(int i = 0; i<num; i++)
        {
            XMLNode neighborNode = tsNode.getChildNode("neighborNode", i);
            nn.ipAddress = ntohl(inet_addr(neighborNode.getAttribute("ipAddress")));
            nn.Status = nnsUnknown;
            //nn.Status = nnsAlive;
            nn.Capability = 0;
            nn.currLoad = 0;
            nn.supplyPort = globalConfig.publicationPort;
            nn.UID = 0;
            nn.zoneNumber = 0;
            globalNeighborNodeList.push_back(nn);
        }
    }

    globalLiveChannelList.clear();
    tsNode = xMainNode.getChildNode("tsFile");
    if(!tsNode.isEmpty())
    {
        int channelID;
        char channelName[256];
        char channelPath[256];
        char exceptionPath[256];
        int isException;
        int num = tsNode.nChildNode();
        for(int i = 0; i<num; i++)
        {
            XMLNode channelNode = tsNode.getChildNode("channel", i);
            channelID = atoi(channelNode.getAttribute("channelID"));
            strcpy(channelName, channelNode.getAttribute("channelName"));
            strcpy(channelPath, channelNode.getAttribute("channelPath"));
            strcpy(exceptionPath, channelNode.getAttribute("exceptionPath"));
            isException = atoi(channelNode.getAttribute("isException"));
            BOOL isVOD;
            const char* isVODString = channelNode.getAttribute("isVOD");
            if(isVODString != NULL)
            {
                logger(msgINFO, "isVOD existed, isVOD = %s", isVODString);
                isVOD = atoi(isVODString);
            }
            else
            {
                logger(msgINFO, "isVOD not existed");
                isVOD = 0;
            }
            CLiveChannel *newChannel = new CLiveChannel(channelID, channelName, channelPath, exceptionPath, isException, isVOD, channelSuperTs);
            newChannel->channelBitRate = atoi(channelNode.getAttribute("bitRate"));
            sscanf(channelNode.getAttribute("pcrPID"), "%x", &newChannel->pcrPID);
            globalLiveChannelList.push_back(newChannel);
        }
    }

    globalTsConsumerTaskList.clear();
    tsNode = xMainNode.getChildNode("tsConsumerTask");
    if(!tsNode.isEmpty())
    {
        tsConsumerTask tsCT;
        int num = tsNode.nChildNode();
        for(int i = 0; i<num; i++)//should new CLiveChannel
        {
            XMLNode consumerNode = tsNode.getChildNode("channel", i);
            tsCT.channelID = atoi(consumerNode.getAttribute("channelID"));
            char channelName[256];
            char channelPath[256];
            strcpy(channelName, consumerNode.getAttribute("channelName"));
            strcpy(channelPath, consumerNode.getAttribute("channelPath"));
            tsCT.consumerPort = atoi(consumerNode.getAttribute("consumerPort"));
            tsCT.linkNumberReadFromConfigFile = atoi(consumerNode.getAttribute("linkNumber"));
            tsCT.linkNumberLast = tsCT.linkNumberReadFromConfigFile;
            //tsCT.ipAddressFTDS = (inet_addr(consumerNode.getAttribute("ipAddressFTDS")));
            //tsCT.portFTDS = htons(atoi(consumerNode.getAttribute("portFTDS")));
            tsCT.channelName = channelName;
            tsCT.channelPath = channelPath;
            tsCT.bitRate = 0;
            tsCT.lastPDUTime = 0;
            tsCT.pcrPID = 0;
            tsCT.startTime = 0;
            tsCT.supplierUID = 0;
            tsCT.lastSupplierUID = 0;
            tsCT.Status = ctsTrackerQuery;
            tsCT.relayRealSupplierUID = 0;
            //			tsCT.transDataEvent = NULL;
            //			tsCT.threadHandle = NULL;
            //  tsCT.tickCount = GetTickCount();
            tsCT.recvBytes = 0;
            tsCT.lastResentStartSeqNum = 0;
            tsCT.lastResentStopSeqNum = 0;
            tsCT.resendTime = 0;
            tsCT.recvBuf = new char[1500];
            tsCT.arrivedBufList.clear();
            //   tsCT.consumerThread = NULL;
            //  tsCT.consumerThreadID = 0;
            //tsCT.ftdsThread = NULL;
            //tsCT.ftdsThreadID = 0;
            tsCT.fileIndex = 0;
            tsCT.savedFile.clear();
            tsCT.readyForFtds = false;
            tsCT.consumerTimes = 0;
            tsCT.lastPDUTimeTick = 0;
            tsCT.linkNumber = 1;
            tsCT.linkTwoTimeTick = 0;
            tsCT.EXITftds = false;
            tsCT.EXITconsumer = false;
            //tsCT.checked = false;
            tsCT.ftdsList.clear();
            tsCT.waitForReplySeq.clear();
            tsCT.recvdFilesNumber = 0;//zhanghh
			tsCT.supplierDeque.clear();
			tsCT.timesInCtsNotStartedStatus = 0;
			tsCT.nextPduSeq = 0;
            deleteOldFile(&tsCT);

            //InitializeCriticalSection(&tsCT.lockConsumerThread);

            CLiveChannel *newChannel = new CLiveChannel(tsCT.channelID, channelName, channelPath, "", 0, 0, channelSuper);
            newChannel->channelBitRate = 0;
            newChannel->pcrPID = 0;
            globalLiveChannelList.push_back(newChannel);
            tsCT.channel = newChannel;

            globalTsConsumerTaskList.push_back(tsCT);
			globalTsConsumerTaskList.back().lockConsumerArrivedDeque.init();
            logger(msgINFO, "load channel %d consumer, size = %d", globalTsConsumerTaskList.back().channelID, globalTsConsumerTaskList.size());
        }
    }

    tsNode = xMainNode.getChildNode("tsFTDS");
    if(!tsNode.isEmpty())
    {
        ftds ftdsItem;
        UINT channelID;
        int num = tsNode.nChildNode();
        for(int i = 0; i<num; i++)
        {
            XMLNode ftdsNode = tsNode.getChildNode("ftds", i);
            channelID = atoi(ftdsNode.getAttribute("channelID"));
            ftdsItem.ipAddressFTDS = (inet_addr(ftdsNode.getAttribute("ipAddressFTDS")));
            ftdsItem.portFTDS = htons(atoi(ftdsNode.getAttribute("portFTDS")));
#if defined(MOHO_X86)
                        ftdsItem.recvAddr.sin_addr.s_addr = (ftdsItem.ipAddressFTDS);
#elif defined(MOHO_WIN32)
			ftdsItem.recvAddr.sin_addr.S_un.S_addr = (ftdsItem.ipAddressFTDS);
#endif
			ftdsItem.recvAddr.sin_port = ftdsItem.portFTDS;
            std::list<tsConsumerTask>::iterator iterCT;
            for(iterCT = globalTsConsumerTaskList.begin(); iterCT != globalTsConsumerTaskList.end(); iterCT ++)
            {
                if(iterCT->channelID == channelID)
                {
                    iterCT->ftdsList.push_back(ftdsItem);
                    break;
                }
            }
            if(iterCT == globalTsConsumerTaskList.end())
            {

                logger(msgERROR, "FTDS (%d, %s, %s) has no channel", channelID, ftdsNode.getAttribute("ipAddressFTDS"), ftdsNode.getAttribute("portFTDS"));
            }
        }
    }

    tsNode = xMainNode.getChildNode("tsMonitor");
    if(!tsNode.isEmpty())
    {
        int num = tsNode.nChildNode();
        int linkNumber;
        for(int i = 0; i<num; i++)
        {
            XMLNode linkNode = tsNode.getChildNode("networkLink", i);
            linkNumber = atoi(linkNode.getAttribute("linkNumber"));
            if(linkNumber == 1)
            {
                strcpy(globalConfig.linkOneMonitorFilePath, linkNode.getAttribute("monitorFilePath"));
                logger(msgINFO, "link 1 monitor path: %s", globalConfig.linkOneMonitorFilePath);
            }
            else if(linkNumber == 2)
            {
                strcpy(globalConfig.linkTwoMonitorFilePath, linkNode.getAttribute("monitorFilePath"));
                logger(msgINFO, "link 2 monitor path: %s", globalConfig.linkTwoMonitorFilePath);
            }
            else
            {
                logger(msgERROR, "monitor UNSUPPORTED link number %d", linkNumber);
            }
        }
    }

    return 0;
}

/*
static DWORD WINAPI configFileNotifyThreadProc(LPVOID lpParameter)//配置文件改动监测线程，监测到改动后，调用tsReloadConfig
{
	configFileNotifyThreadInfo *info = (configFileNotifyThreadInfo *)lpParameter;
	  char buffer_[1024*32];
	  DWORD receives_bytes_ = 0;
	  memset(buffer_, 0, 1024*4);

//	FILE_NOTIFY_INFORMATION notifyInfo;
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
		logger(msgERROR, "CreateFile ERROR, path = %s", info->filePath);
		return -1;
	}
   while (TRUE) 
   { 
			  memset(buffer_, 0, 1024*4);
			 if(	ReadDirectoryChangesW(
					readDirHandle,
					buffer_,
					1024*4,
					FALSE,
					FILE_NOTIFY_CHANGE_LAST_WRITE,
					&bytesReturned,
					NULL,
					NULL))
			 {
				FILE_NOTIFY_INFORMATION *lp = (FILE_NOTIFY_INFORMATION *)buffer_;
				 do
				 {
					 memset(changedFileName, 0, 256);
					 if(lp->Action == FILE_ACTION_MODIFIED)
					 {
 						 int   nLen   =   lp->FileNameLength;   
						 WideCharToMultiByte(CP_ACP,   0,   lp->FileName,   nLen,   changedFileName,   2*nLen,   NULL,   NULL);  
						 if(strcmp(changedFileName, "config.xml") == 0)
						 {
							logger(msgINFO, "file %s changed", changedFileName);
							tsReloadConfig();
							break;
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
				logger(msgERROR, "ReadDirectoryChangesW ERROR");
			 }
   }
	return 0;
}
*/


#if defined(MOHO_ANDROID)
#include <jni.h>
#include <android/log.h>
#define LOG_TAG "MyP2p"
#define LOGD(fmt, args...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, fmt, ##args)

#ifdef __cplusplus
extern "C"
{
#endif
  //   jstring JNICALL Java_cn_com_xinli_android_p2p_MohoHttpServer_mohohttpserver(JNIEnv *env, jobject a,jstring b)
    jstring JNICALL Java_cn_com_xinli_android_MohoP2PServer_mohohttpserver(JNIEnv *env, jobject a,jstring b)
    {
        char* szPath = (char*)(env)->GetStringUTFChars(b, 0 );
        if(szPath)
        {
            g_localPath = strdup(szPath);
        }
        else
        {
            g_localPath = "./";
        }

        normalLog = new CNetLog("normal",logERROR);
        liveLog = new CNetLog("liveLog",logERROR);

        tsLoadConfig();

        normalLog->setlevel(globalConfig.logLevel);
        normalLog->setSaved(globalConfig.logSaved);
        liveLog->setlevel(globalConfig.logLevel);
        liveLog->setSaved(globalConfig.logSaved);
        initNetwork();
        initTimer();

        //zhanghh
        while(1)
        {
            sleep(3600*1000);
        }
    }
#ifdef __cplusplus
}
#endif
#elif defined(MOHO_X86)

void sigsegvHandler(int sig)
{
    messager(msgFATAL,"exit, signal=%d",sig);
    exit(0);
}


int main(int argc, char *argv[])
{
    signal(SIGSEGV, sigsegvHandler);
    signal(SIGTERM, sigsegvHandler);
    signal(SIGKILL, sigsegvHandler);

    if(argc >= 2)
    {
        g_localPath = argv[1];
    }
    else
    {
        g_localPath = "./";
    }

    normalLog = new CNetLog("normal",logERROR);
    liveLog = new CNetLog("liveLog",logERROR);

    tsLoadConfig();

    normalLog->setlevel(globalConfig.logLevel);
    normalLog->setSaved(globalConfig.logSaved);
    liveLog->setlevel(globalConfig.logLevel);
    liveLog->setSaved(globalConfig.logSaved);

    initNetwork();
    initTimer();

    //zhanghh
    while(1)
    {
        sleep(3600*1000);
    }
}
#elif defined(MOHO_WIN32)
HWND pMainWnd;
HINSTANCE hInst;								// 当前实例
TCHAR szTitle[MAX_LOADSTRING];					// 标题栏文本
TCHAR szWindowClass[MAX_LOADSTRING];			// 主窗口类名

// 此代码模块中包含的函数的前向声明:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY _tWinMain(HINSTANCE hInstance,
                       HINSTANCE hPrevInstance,
                       LPTSTR    lpCmdLine,
                       int       nCmdShow)
{
    g_localPath = "./";

    normalLog = new CNetLog("normal",logERROR);
    liveLog = new CNetLog("liveLog",logERROR);

    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: 在此放置代码。
    MSG msg;
    HACCEL hAccelTable;

    // 初始化全局字符串
    LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadString(hInstance, IDC_LIVECHANNELC2A, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // 执行应用程序初始化:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_LIVECHANNELC2A));


	lockLiveChannelList.init();
	lockNeighborNodeList.init();
	lockTsSupplierTaskList.init();
	lockRelayTaskList.init();
	lockQueryTaskList.init();

    //InitializeCriticalSection(&lockLiveChannelList);
    //InitializeCriticalSection(&lockNeighborNodeList);
    //InitializeCriticalSection(&lockTsSupplierTaskList);
    ////	InitializeCriticalSection(&lockTsConsumerTaskList);
    //InitializeCriticalSection(&lockRelayTaskList);
    //InitializeCriticalSection(&lockQueryTaskList);


    tsLoadConfig();

    normalLog->setlevel(globalConfig.logLevel);
    normalLog->setSaved(globalConfig.logSaved);
    liveLog->setlevel(globalConfig.logLevel);
    liveLog->setSaved(globalConfig.logSaved);
    //std::list<CLiveChannel *>::iterator Iter;
    //for(Iter = globalLiveChannelList.begin(); Iter != globalLiveChannelList.end(); Iter ++)
    //{
    //	(*Iter)->checkFileUpdate();
    //}
	messager(msgINFO, "2111");
    initNetwork();

    initTimer();

	initSuperTsSupplierTask();

    //ftdsTest();

    //if(!SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS))
    //{
    //	logger(msgERROR, "SetPriorityClass Failed");
    //}


    //DWORD dwProcessId = GetCurrentProcessId();
    //HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwProcessId );
    //BOOL SetSuccess = SetPriorityClass( hProcess, REALTIME_PRIORITY_CLASS);



    /*
        GetCurrentDirectory(256, configInfo.filePath);
        strcat(configInfo.filePath, "\\config");
        DWORD dwThreadID = 0;
        HANDLE hThread = CreateThread(
                                                NULL,                    //安全属性使用缺省。
                                                0,                         //线程的堆栈大小。
                                                configFileNotifyThreadProc,                 //线程运行函数地址。
                                                &configInfo,               //传给线程函数的参数。
                                                0,                         //创建标志。
                                                &dwThreadID);       //成功创建后的线程标识码。
        logger(msgINFO, "CreateThread, config path = %s", configInfo.filePath);
*/
    // 主消息循环:
    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}



//
//  函数: MyRegisterClass()
//
//  目的: 注册窗口类。
//
//  注释:
//
//    仅当希望
//    此代码与添加到 Windows 95 中的“RegisterClassEx”
//    函数之前的 Win32 系统兼容时，才需要此函数及其用法。调用此函数十分重要，
//    这样应用程序就可以获得关联的
//    “格式正确的”小图标。
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style			= CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc	= WndProc;
    wcex.cbClsExtra		= 0;
    wcex.cbWndExtra		= 0;
    wcex.hInstance		= hInstance;
    wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_LIVECHANNELC2A));
    wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_LIVECHANNELC2A);
    wcex.lpszClassName	= szWindowClass;
    wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassEx(&wcex);
}

//
//   函数: InitInstance(HINSTANCE, int)
//
//   目的: 保存实例句柄并创建主窗口
//
//   注释:
//
//        在此函数中，我们在全局变量中保存实例句柄并
//        创建和显示主程序窗口。
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    HWND hWnd;

    hInst = hInstance; // 将实例句柄存储在全局变量中

    hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
                        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

    if (!hWnd)
    {
        return FALSE;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    pMainWnd = hWnd;


    return TRUE;
}

//
//  函数: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  目的: 处理主窗口的消息。
//
//  WM_COMMAND	- 处理应用程序菜单
//  WM_PAINT	- 绘制主窗口
//  WM_DESTROY	- 发送退出消息并返回
//
//



BOOL first = true;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    int wmId, wmEvent;
    PAINTSTRUCT ps;
    HDC hdc;


    switch (message)
    {
    case WM_COMMAND:
        wmId    = LOWORD(wParam);
        wmEvent = HIWORD(wParam);
        // 分析菜单选择:
        switch (wmId)
        {
        case IDM_ABOUT:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
            //tsLoadConfig();

            //if(first)
            //{
            //	tstChannel = new CLiveChannel(1, "CCTV1", "F:\\网络工具\\tsFile\\Ts2FileAnsi\\");
            //	first = false;
            //	tstChannel->checkFileUpdate();
            //}
            //tstChannel.checkFileUpdate();


            //if(netState==nsUNINIT)//网络未初始化
            //{
            //	int ret=initNetwork();
            //	if(ret==-1)
            //	{
            //		messager(msgFATAL,_T("网络初始化失败！"));
            //		return -1;
            //	}
            //}
            //if(loadConfig()!=0)
            //{
            //	messager(msgFATAL,_T("加载配置文件失败"));
            //}
            //else
            //{
            //	messager(msgINFO, _T("加载配置文件成功"));
            //}
            break;
        case IDM_EXIT:
			{
				exitSafely();
				/*DeleteCriticalSection(&lockLiveChannelList);
				DeleteCriticalSection(&lockTsSupplierTaskList);
				DeleteCriticalSection(&lockRelayTaskList);
				DeleteCriticalSection(&lockQueryTaskList);*/
				DestroyWindow(hWnd);
				break;
			}
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        break;
        case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);
        // TODO: 在此添加任意绘图代码...
        EndPaint(hWnd, &ps);
        break;
        case WM_DESTROY:
			{

				exitSafely();
				PostQuitMessage(0);
				break;
			}
        default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// “关于”框的消息处理程序。
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
#endif
