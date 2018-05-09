/*
 * CProcessStatus.h
 *
 *  Created on: 2018年4月17日
 *      Author: Administrator
 */

#ifndef CFILEPROCESSINGSTATUS_H_
#define CFILEPROCESSINGSTATUS_H_

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
	//std::map<std::string,STFileProcessingStatus *> m_mapFileProcessingData;

	CLineSpaceMgr objCLineSpaceMgr;

	std::string m_sDir,m_sMapFileName;

	int m_iIntval;

	size_t m_iFileMaxCount;
public:
	std::map<std::string,STFileProcessingStatus *> m_mapFileProcessingData;

	int Init(const std::string & dir,int intval=3000,const std::string & hra_file_name = ".hra.processing.file",int max_file_count=100)
	{
		m_iIntval = intval;

		m_sDir = dir;

		m_sMapFileName = dir + hra_file_name;

		m_iFileMaxCount = max_file_count;

		size_t szStruct = sizeof(STFileProcessingStatus);  

		std::cout << "初始化共享内存 ： " << m_sMapFileName << std::endl;

		if( 0 != objCLineSpaceMgr.Init(szStruct,m_iFileMaxCount,m_sMapFileName.c_str(),true) )
		{
			std::cout << objCLineSpaceMgr.GetTraceLog() << std::endl;
			return -1;
		}

		// 遍历 ,建立文件名到文件相关处理状态的数据
		for(int i=0;i<= objCLineSpaceMgr.GetTotalSize();i++)
		{
			if(m_mapFileProcessingData.size() == objCLineSpaceMgr.GetSize()) break;

			STFileProcessingStatus *pSTFileProcessingStatus = (STFileProcessingStatus*) objCLineSpaceMgr.AsVoid(CPointer(1,i));

			// 系统错误
			if(pSTFileProcessingStatus == NULL)
			{
				std::cout  << "NULL while CPoint(1," << i << ")" << std::endl;
				return -1;
			}

			pSTFileProcessingStatus->uiRecordMemID = i;
/*
			std::cout
								<<std::left<< std::setw(50) << pSTFileProcessingStatus->szFileName
								<<std::left<< std::setw(20)<< pSTFileProcessingStatus->ilSize
								<<std::left<< std::setw(20)<< pSTFileProcessingStatus->tmLastModified
								<<std::left<< std::setw(20)<< pSTFileProcessingStatus->ilOffset
								<<std::left<< std::setw(20)<< pSTFileProcessingStatus->tmLastProcessing
								<<std::left<< std::setw(20)<< pSTFileProcessingStatus->uiRecordMemID
								<<std::left<< std::endl;
*/

			// 空间释放了
			if(pSTFileProcessingStatus->szFileName[0] == 0)
				continue;

			m_mapFileProcessingData[pSTFileProcessingStatus->szFileName] = pSTFileProcessingStatus;
		}

		return 0;

	}

	void DumpInfo()
	{
		std::map<std::string,STFileProcessingStatus *>::iterator iter;

		std::cout << "On processing file list : \r\n"
				<< "=========================================\r\n "
				<< std::endl;//<< "12345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890\r\n" ;
		std::cout
				<<std::left << std::setw(50)<< "File Path"
				<<std::left << std::setw(20)<< "Size"
				<<std::left << std::setw(20)<< "Last Modify Time"
				<<std::left << std::setw(20)<< "Handled Offset"
				<<std::left << std::setw(20)<< "Handled Time"
				<<std::left << std::setw(20)<< "Memory ID"
				<<std::left << std::endl;


		for(iter=m_mapFileProcessingData.begin();iter != m_mapFileProcessingData.end();iter++)
		{
			std::cout
					<<std::left<< std::setw(50) << (iter->second)->szFileName
					<<std::left<< std::setw(20)<< (iter->second)->ilSize
					<<std::left<< std::setw(20)<< (iter->second)->tmLastModified
					<<std::left<< std::setw(20)<< (iter->second)->ilOffset
					<<std::left<< std::setw(20)<< (iter->second)->tmLastProcessing
					<<std::left<< std::setw(20)<< (iter->second)->uiRecordMemID
					<<std::left<< std::endl;
		}

		std::cout << "-----------------------------------------\r\n ";

	}

	void GetDirectoryFileStatus()
	{
		char szCmd[1024],szRes[1024];
		memset(szCmd,0,sizeof(szCmd));
		memset(szRes,0,sizeof(szRes));

		std::set<std::string> tSetFileName;

		sprintf(szCmd,"ls -1 -t %s/*%s 2>/dev/null",MyUtility::g_objCCommandLineInfo.GetArgVal("log-dir").c_str(),
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
				//std::cout << "before:" << szRes << std::endl;

				char *ws = strpbrk(szRes, " \t\n");
				if(ws) *ws = '\0';

				//std::cout << "after:" << szRes << std::endl;


				struct stat s_buff;

				int status = stat(szRes,&s_buff); //获取文件对应属

				if (status == 0)
				{
					STFileProcessingStatus *pSTFileProcessingStatus = NULL;
					if(s_buff.st_size <= 6)
					{
						pSTFileProcessingStatus = GetFileProcessAlready(szRes);

						if(NULL != pSTFileProcessingStatus )
						{
							RemoveFile(pSTFileProcessingStatus);
						}
						continue;

					}


					// 启动线程进行文件分析，并上报
					pSTFileProcessingStatus = GetFileProcess(szRes);

					if(NULL != pSTFileProcessingStatus )
					{
							pSTFileProcessingStatus->tmLastModified = s_buff.st_mtime;
							pSTFileProcessingStatus->ilSize = s_buff.st_size;

							if (pSTFileProcessingStatus->ilSize < pSTFileProcessingStatus->ilOffset)
							{
								pSTFileProcessingStatus->ilOffset = 0;
							}
							if (pSTFileProcessingStatus->ilSize == pSTFileProcessingStatus->ilOffset)
							{
								// 1
									if(pSTFileProcessingStatus->tmLastProcessing >= (pSTFileProcessingStatus->tmLastModified + m_iIntval ))
										RemoveFile(pSTFileProcessingStatus);
									else
										pSTFileProcessingStatus->tmLastProcessing = time(NULL);
								// 2


							}
							//std::cout << "记录文件处理进度 ： [" << szRes <<"]" <<std::endl;
					}
					else
					{
						std::cout << "没有足够的空间记录文件的进程 ： [" << szRes << "]" <<std::endl;
					}

					tSetFileName.insert(szRes);
				}
				else
				{

					std::cout << "获取文件信息失败 ： [" << szRes << "] "<<std::endl;
				}

			}
			pclose(dl);
		}

		// 清除不存在的文件：被手工删除的文件
		std::map<std::string,STFileProcessingStatus *>::iterator iter;

		for(iter=m_mapFileProcessingData.begin();iter != m_mapFileProcessingData.end();iter++)
		{
			if(tSetFileName.end() == tSetFileName.find((iter->second)->szFileName))
				RemoveFile(iter->second);
		}

	}

	void RemoveFile(STFileProcessingStatus * file)
	{
		m_mapFileProcessingData.erase(file->szFileName);

		//remove(file->szFileName);


		truncate(file->szFileName,0);

		CPointer pt(1,file->uiRecordMemID);

		objCLineSpaceMgr.Free(pt);

		memset(file,0 ,sizeof(STFileProcessingStatus));
	}

	// 取文件，只到空
	STFileProcessingStatus* GetFileProcess()
	{

		std::map<std::string,STFileProcessingStatus *>::iterator iter;

		for(iter=m_mapFileProcessingData.begin();iter != m_mapFileProcessingData.end();iter++)
		{
			if ((iter->second)->ilOffset < (iter->second)->ilSize)
				return (iter->second);
		}

		return NULL;
	}

	STFileProcessingStatus * GetFileProcessAlready(const char * filename)
		{
			STFileProcessingStatus * data = NULL;




			if(m_mapFileProcessingData.find(filename) != m_mapFileProcessingData.end())
			{


				data = m_mapFileProcessingData[filename];

				//std::cout << "文件名["  << filename << "]  已经跟踪 : " << data->uiRecordMemID << std::endl;
			}

			return data;
		}

	STFileProcessingStatus * GetFileProcess(const char * filename)
	{
		STFileProcessingStatus * data = NULL;




		if(m_mapFileProcessingData.find(filename) != m_mapFileProcessingData.end())
		{


			data = m_mapFileProcessingData[filename];

			//std::cout << "文件名["  << filename << "]  已经跟踪 : " << data->uiRecordMemID << std::endl;
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

				m_mapFileProcessingData[filename] = data;


				std::cout << "文件名["  << filename << "]  新增跟踪 : " << data->uiRecordMemID << std::endl;
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
		std::cout << "构造文件进度对象 ：" << time(NULL) <<std::endl;
	}
};



#endif /* CFILEPROCESSINGSTATUS_H_ */
