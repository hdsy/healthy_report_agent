/*
 * CProcessStatus.h
 *
 *  Created on: 2018��4��17��
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
 * �ļ�����Ľ�չ
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
		else // ���ݳ������⣬������
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

	int m_iFileMaxCount;
public:

	int Init(const std::string & dir,int intval=3000,const std::string & hra_file_name = ".hra.processing.file",int max_file_count=100)
	{
		m_iIntval = intval;

		m_sDir = dir;

		m_sMapFileName = dir + hra_file_name;

		m_iFileMaxCount = max_file_count;


		int iRet = objCLineSpaceMgr.Init(sizeof(STFileProcessingStatus),m_iFileMaxCount,m_sMapFileName,true);

		if(iRet != OK)
		{
			return iRet;
		}

		// ���� ,�����ļ������ļ���ش���״̬������
		for(int i=0;i<objCLineSpaceMgr.GetSize();i++)
		{
			STFileProcessingStatus *pSTFileProcessingStatus = (STFileProcessingStatus*) objCLineSpaceMgr.AsVoid(CPointer(1,i));

			// ϵͳ����
			if(pSTFileProcessingStatus == NULL)
				return -1;

			pSTFileProcessingStatus->uiRecordMemID = i

			// �ռ��ͷ���
			if(pSTFileProcessingStatus->szFileName[0] == 0)
				continue;

			m_mapFileProcessingData[pStLastDeviceData->szFileName] = pSTFileProcessingStatus;
		}

		return 0;

	}

	void DumpIno()
	{
		std::map<std::string,STFileProcessingStatus *>::iterator iter;

		std::cout << "���ڴ�����ļ��б� : \r\n "
				"=========================================\r\n " ;
		std::cout << std::setw(50)<< "�ļ���"
					<< std::setw(12)<< "�ߴ�"
					<< std::setw(12)<< "�޸�ʱ��"
					<< std::setw(12)<< "ƫ����"
					<< std::setw(12)<< "����ʱ��"
					<< std::setw(12)<< "�ڴ�ID"
					<< std::endl;


		for(iter=m_mapFileProcessingData.begin();iter != m_mapFileProcessingData.end();iter++)
		{
			std::cout << std::setw(50)<< (iter->second)->szFileName
					<< std::setw(12)<< (iter->second)->ilSize
					<< std::setw(12)<< (iter->second)->tmLastModified
					<< std::setw(12)<< (iter->second)->ilOffset
					<< std::setw(12)<< (iter->second)->tmLastProcessing
					<< std::setw(12)<< (iter->second)->uiRecordMemID
					<< std::endl;
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
			std::cout << "�鿴��־�ϱ��ļ��б�ʧ�ܣ� " << szCmd << std::endl;
		}
		else
		{
			while(fgets(szRes, sizeof(szRes), dl))
			{
				char *ws = strpbrk(szRes, " \t\n");
				if(ws) *ws = '\0';

				std::cout << "��ʼ�ļ��ķ��� �� " << szRes <<std::endl;

				// �����߳̽����ļ����������ϱ�
				STFileProcessingStatus *pSTFileProcessingStatus = GetFileProcess(szRes);

				if(NULL != pSTFileProcessingStatus )
				{
					struct stat s_buff;

					int status = stat(szRes,&s_buff); //��ȡ�ļ���Ӧ��

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
									pSTFileProcessingStatus->tmLastProcessing = time();
							// 2


						}
					}

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
};

/*
 * ���۵Ľ�Ҫ�ϱ�������
 * */
typedef struct ST_SummaryRecode
{
	time_t tmPeriod; // ʱ�䷶Χ�����趨��ͳ��ʱ�䣬�����5���ӣ���ȡ5���ӷֶεĿ�ʼʱ��

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

	char cStatus;  // 0 ͳ���У�1 ͳ����� ��2 �ϱ����


}STSummaryRecode;

class CProcessStatus {
public:
	CProcessStatus();
	virtual ~CProcessStatus();
};

#endif /* CPROCESSSTATUS_H_ */
