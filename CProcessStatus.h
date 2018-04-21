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
#include "CBaseEncode.h"


/*
 * 文件处理的进展
 * */
typedef struct ST_FileProcessingStatus
{
	unsigned int uiRecordMemID;
	char szFileName[128];

	time_t tmLastModified;
	off_t ilSize;

	time_t tmLastProcessing;
	off_t ilOffset;

	bool IsFinished(int intval)
	{
		if (ilSize == ilOffset)
		{
			if(tmLastProcessing > tmLastModified)
			{
				if(intval <= (tmLastProcessing-tmLastModified))
					return true;
			}
		}
		else // 数据出现问题，修整下
		{
			ilOffset = 0;
			tmLastProcessing = 0;
		}

		return false;
	}

}STFileProcessingStatus;

class CFileProcessingStatus
{
private:
	std::map<std::string,STFileProcessingStatus *> m_mapFileProcessingData;

	CLineSpaceMgr objCLineSpaceMgr;

	std::string m_sDir,m_sMapFileName;

	int m_iIntval;

	size_t m_iFileMaxCount;
public:

	int Init(const std::string & dir,int intval=3000,const std::string & hra_file_name = ".hra.processing.file",int max_file_count=100)
	{
		m_iIntval = intval;

		m_sDir = dir;

		m_sMapFileName = dir + hra_file_name;

		m_iFileMaxCount = max_file_count;

		size_t szStruct = sizeof(STFileProcessingStatus);  

		//std::cout << "初始化共享内存 ： " << m_sMapFileName << std::endl;

		if( 0 != objCLineSpaceMgr.Init(szStruct,m_iFileMaxCount,m_sMapFileName.c_str(),true) )
		{
			std::cout << objCLineSpaceMgr.GetTraceLog() << std::endl;
			return -1;
		}

		// 遍历 ,建立文件名到文件相关处理状态的数据
		for(int i=0;i<objCLineSpaceMgr.GetSize();i++)
		{
			STFileProcessingStatus *pSTFileProcessingStatus = (STFileProcessingStatus*) objCLineSpaceMgr.AsVoid(CPointer(1,i));

			// 系统错误
			if(pSTFileProcessingStatus == NULL)
				return -1;

			pSTFileProcessingStatus->uiRecordMemID = i;

			// 空间释放了
			if(pSTFileProcessingStatus->szFileName[0] == 0)
				continue;

			m_mapFileProcessingData[pSTFileProcessingStatus->szFileName] = pSTFileProcessingStatus;
		}

		return 0;

	}

	void DumpIno()
	{
		std::map<std::string,STFileProcessingStatus *>::iterator iter;

		std::cout << "正在处理的文件列表 : \r\n =========================================\r\n " ;
		std::cout
				<<std::left << std::setw(50)<< "文件名"
				<<std::left << std::setw(20)<< "尺寸"
				<<std::left << std::setw(20)<< "修改时间"
				<<std::left << std::setw(20)<< "偏移量"
				<<std::left << std::setw(20)<< "处理时间"
				<<std::left << std::setw(20)<< "内存ID"
				<<std::left << std::endl;


		for(iter=m_mapFileProcessingData.begin();iter != m_mapFileProcessingData.end();iter++)
		{
			std::cout
					<<std::left<< std::setw(50)<< "[" << (iter->second)->szFileName << "]"
					<<std::left<< std::setw(20)<< (iter->second)->ilSize
					<<std::left<< std::setw(20)<< (iter->second)->tmLastModified
					<<std::left<< std::setw(20)<< (iter->second)->ilOffset
					<<std::left<< std::setw(20)<< (iter->second)->tmLastProcessing
					<<std::left<< std::setw(20)<< (iter->second)->uiRecordMemID
					<<std::left<< std::endl;
		}

	}

	void GetDirectoryFileStatus()
	{
		char szCmd[1024],szRes[1024];
		memset(szCmd,0,sizeof(szCmd));
		memset(szRes,0,sizeof(szRes));

		sprintf(szCmd,"ls -1 -t %s/%s 2>/dev/null",MyUtility::g_objCCommandLineInfo.GetArgVal("log-dir").c_str(),
				MyUtility::g_objCCommandLineInfo.GetArgVal("ext-name").c_str());

		FILE *dl;	//list all *trans.so in fdir
		dl = popen(szCmd, "r");

		if(!dl)
		{
			std::cout << "查看日志上报文件列表失败： " << szCmd << std::endl;
		}
		else
		{
			while(fgets(szRes, sizeof(szRes), dl))
			{
				char *ws = strpbrk(szRes, " \t\n");
				if(ws) *ws = '\0';



				// 启动线程进行文件分析，并上报
				STFileProcessingStatus *pSTFileProcessingStatus = GetFileProcess(szRes);

				if(NULL != pSTFileProcessingStatus )
				{
					struct stat s_buff;

					int status = stat(szRes,&s_buff); //获取文件对应属

					if (status == 0)
					{
						pSTFileProcessingStatus->tmLastModified = s_buff.st_mtime;
						pSTFileProcessingStatus->ilSize = s_buff.st_size;

						if (pSTFileProcessingStatus->ilSize < pSTFileProcessingStatus->ilOffset)
							pSTFileProcessingStatus->ilOffset = 0;

						if (pSTFileProcessingStatus->ilSize == pSTFileProcessingStatus->ilOffset)
						{
							// 1
								if(pSTFileProcessingStatus->tmLastProcessing >= (pSTFileProcessingStatus->tmLastModified + m_iIntval ))
									RemoveFile(pSTFileProcessingStatus);
								else
									pSTFileProcessingStatus->tmLastProcessing = time(NULL);
							// 2


						}
						std::cout << "记录文件处理进度 ： " << szRes <<std::endl;
					}
					else
					{

						std::cout << "获取文件信息失败 ： " << szRes <<std::endl;
					}

				}
				else
				{
					std::cout << "没有足够的空间记录文件的进程 ： " << szRes <<std::endl;
				}

			}
			pclose(dl);
		}

	}

	void RemoveFile(STFileProcessingStatus * file)
	{
		m_mapFileProcessingData.erase(file->szFileName);

		remove(file->szFileName);

		CPointer pt(1,file->uiRecordMemID);

		objCLineSpaceMgr.Free(pt);

		memset(file,0 ,sizeof(STFileProcessingStatus));
	}

	STFileProcessingStatus * GetFileProcess(const char * filename)
	{
		STFileProcessingStatus * data = NULL;

		std::cout << "文件名["  << filename << "]" << std::endl;


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

				data->uiRecordMemID = pt.m_uiOffset;

				strncpy(data->szFileName,filename,sizeof(data->szFileName));
			}
		}

		return data;
	}

	CFileProcessingStatus(const std::string & dir,int intval=3000,const std::string & hra_file_name = ".hra.processing.file",int max_file_count=100)
	{
		m_mapFileProcessingData.clear();
		Init(dir,intval,hra_file_name,max_file_count);
	}
	~CFileProcessingStatus()
	{

	}
	CFileProcessingStatus()
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
	char szCalleeNodeIp[30];
	char szCalleeNodePort[6];

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
