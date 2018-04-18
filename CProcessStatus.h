/*
 * CProcessStatus.h
 *
 *  Created on: 2018年4月17日
 *      Author: Administrator
 */

#ifndef CPROCESSSTATUS_H_
#define CPROCESSSTATUS_H_

#include <time.h>
#include <sys/types.h>
#include <unistd.h>

#include <string>
#include <map>

#include "CLineSpaceMgr.h"


/*
 * 文件处理的进展
 * */
typedef struct ST_FileProcessingStatus
{
	char szFileName[128];

	time_t tmLastModified;
	off_t ilSize;

	time_t tmLastProcessing;
	off_t ilOffset;

}STFileProcessingStatus;

class CFileProcessingStatus
{
private:
	std::map<std::string,STFileProcessingStatus *> m_mapFileProcessingData;

	CLineSpaceMgr objCLineSpaceMgr;
public:

	int Init(const std::string & dir,const std::string & hra_file_name = ".hra.processing.file",int max_file_count=100)
	{
		int iRet = objCLineSpaceMgr.Init(sizeof(STFileProcessingStatus),max_file_count,hra_file_name,true);

		if(iRet != OK)
		{
			return iRet;
		}

		// 遍历 ,建立文件名到文件相关处理状态的数据
		for(int i=0;i<objCLineSpaceMgr.GetSize();i++)
		{
			STFileProcessingStatus *pSTFileProcessingStatus = (STFileProcessingStatus*) objCLineSpaceMgr.AsVoid(CPointer(1,i));

			// 系统错误
			if(pSTFileProcessingStatus == NULL)
				return -1;

			// 到末尾了
			if(pSTFileProcessingStatus->szFileName[0] == 0)
				break;

			m_mapFileProcessingData[pStLastDeviceData->szFileName] = pSTFileProcessingStatus;
		}

		return 0;

	}

	STFileProcessingStatus * GetFileProcess(const char * filename)
	{
		STFileProcessingStatus * data = NULL;


		if(m_mapFileProcessingData.find(filename) != m_mapFileProcessingData.end())
		{
			data = m_mapFileProcessingData[filename];
		}
		else
		{
			CPointer pt = objCLineSpaceMgr.Alloc();

			data = (STFileProcessingStatus*) objCLineSpaceMgr.AsVoid(pt);

			if(data != NULL)
			{

				memset(data,0,sizeof(STFileProcessingStatus));

				strncpy(data->szFileName,filename,sizeof(data->szFileName));
			}
		}

		return data;
	}

	CFileProcessingStatus(const std::string & dir,const std::string & hra_file_name = ".hra.processing.file",int max_file_count=100)
	{
		m_mapFileProcessingData.clear();
		Init(dir,hra_file_name,max_file_count);
	}
	~CFileProcessingStatus()
	{

	}
};

/*
 * 积累的将要上报的数据
 * */
typedef struct ST_SummaryRecode
{
	time_t tmPeriod; // 时间范围：按设定的统计时间，如果是5分钟，则取5分钟分段的开始时间

	char cVer;
	char szCaller[64];
	char szCallerNodeIp[30];

	char szCallee[64];
	char szCallerNodeIp[30];
	char szCallerNodePort[6];

	int iRetcode;

	unsigned int uiCount;

	unsigned int uiMaxTime;
	unsigned int uiMinTime;
	unsigned int uiAvgTime;

	char cStatus;  // 0 统计中，1 统计完毕 ，2 上报完毕


}STSummaryRecode;

class CProcessStatus {
public:
	CProcessStatus();
	virtual ~CProcessStatus();
};

#endif /* CPROCESSSTATUS_H_ */
