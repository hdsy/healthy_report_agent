/*
 * CSummaryRecord.h
 *
 *  Created on: 2018年4月25日
 *      Author: Administrator
 */

#ifndef CSummaryRecord_H_
#define CSummaryRecord_H_

#include <time.h>
#include <sys/types.h>
#include <unistd.h>

#include <string>
#include <map>


#include <iostream>     // std::cout
#include <fstream>      // std::ifstream

#include "CLineSpaceMgr.h"
#include "CBaseEncode.h"

#include "CFileProcessingStatus.h"


/*
 * 积累的将要上报的数据
 * */
typedef struct ST_SummaryRecord
{
	unsigned int uiRecordMemID;
	time_t tmPeriod; // 时间范围：按设定的统计时间，如果是5分钟，则取5分钟分段的开始时间

	char cVer;
	char szCaller[64];
	char szCallerNodeIp[30];

	char szCallee[64];
	char szCalleeNodeIp[30];
	char szCalleeNodePort[6];

	char szMethodID[64];

	int iRetcode;

	unsigned int uiCount;

	unsigned int uiMaxTime;
	unsigned int uiMinTime;
	unsigned int uiAvgTime;

	char cStatus;  // 0 统计中，1 统计完毕 ，2 上报完毕

	inline std::string GetRecordID() const
	{
		std::string res = "";

		res = res + szCaller +"|"
				+ szCallerNodeIp +"|"
				+ szCallee +"|"
				+ szCalleeNodeIp +"|"
				+ szCalleeNodePort +"|"
				+ MyUtility::CBaseEncode::IntToString(iRetcode);


		return res;
	}



	int  Parse(const char * str,int period)
	{
		MyUtility::CStringVector stringVect;

		stringVect.Split(str,"|");

		if((stringVect.size() <= 2)&&stringVect.at(0).size() <= 0)
			return -1;//不支持的格式

		cVer = stringVect.at(0).at(0);

		switch(stringVect.at(0).at(0))
		{
		case '1':// 1|1508227752|CGI|ORDERSVR|LG1|Create|0|5
		{
			if(stringVect.size() != 8)
				return -3; // 格式与版本不符合

			tmPeriod = MyUtility::CBaseCode::StringToUInt( stringVect.at(1));

			tmPeriod -= tmPeriod % period;

			strncpy(szCaller,stringVect.at(2).c_str(),sizeof(szCaller)-1);
			//strncpy(szCallerNodeIp,stringVect.at(3).c_str(),sizeof(szCallerNodeIp)-1);
			strncpy(szCallee,stringVect.at(3).c_str(),sizeof(szCallee)-1);
			strncpy(szCalleeNodeIp,stringVect.at(4).c_str(),sizeof(szCalleeNodeIp)-1);
			strncpy(szMethodID,stringVect.at(5).c_str(),sizeof(szMethodID)-1);

			iRetcode = MyUtility::CBaseCode::StringToInt( stringVect.at(6));
			uiAvgTime = MyUtility::CBaseCode::StringToInt( stringVect.at(7));
			uiMinTime = uiMaxTime = uiAvgTime;
		}
		default:
			return -2;// 不支持的版本
		}

		return 0;
	}


	ST_SummaryRecord()
	{

	}
	ST_SummaryRecord(const char * str,int period)
	{
		Parse(str,period);
	}



}STSummaryRecord;

class CSummaryRecord
{
private:
	std::map<std::string,STSummaryRecord *> m_mapSummaryRecord;

	CLineSpaceMgr objCLineSpaceMgr;

	std::string m_sDir,m_sMapFileName;

	int m_iIntval;

	size_t m_iRecordMaxCount;



public:
	SummaryRecord(){};
	virtual ~SummaryRecord(){};

	int Init(const std::string & dir,int intval=300,const std::string & hra_file_name = ".hra.processing.recode",int max_count=1000)
		{
			m_iIntval = intval;

			m_sDir = dir;

			m_sMapFileName = dir + hra_file_name;

			m_iRecordMaxCount = max_count;

			size_t szStruct = sizeof(STSummaryRecord);

			std::cout << "初始化共享内存 ： " << m_sMapFileName << std::endl;

			if( 0 != objCLineSpaceMgr.Init(szStruct,m_iRecordMaxCount,m_sMapFileName.c_str(),true) )
			{
				std::cout << objCLineSpaceMgr.GetTraceLog() << std::endl;
				return -1;
			}

			// 遍历 ,建立统计周期内调用关系与返回码到记录数据的信息
			for(int i=0;i<objCLineSpaceMgr.GetSize();i++)
			{
				STSummaryRecord *pSTSummaryRecord = (STSummaryRecord*) objCLineSpaceMgr.AsVoid(CPointer(1,i));

				// 系统错误
				if(pSTSummaryRecord == NULL)
					return -1;

				pSTSummaryRecord->uiRecordMemID = i;
	/**/
				std::cout
						<<std::left<< std::setw(20)<< pSTSummaryRecord->uiRecordMemID
						<<std::left<< std::setw(20)<< pSTSummaryRecord->cVer
						<<std::left<< std::setw(20)<< pSTSummaryRecord->tmPeriod
						<<std::left<< std::setw(50) << pSTSummaryRecord->szCaller
						<<std::left<< std::setw(20)<< pSTSummaryRecord->szCallerNodeIp
						<<std::left<< std::setw(20)<< pSTSummaryRecord->szCallee
						<<std::left<< std::setw(20)<< pSTSummaryRecord->szCalleeNodeIp
						<<std::left<< std::setw(20)<< pSTSummaryRecord->szCalleeNodePort
						<<std::left<< std::setw(20)<< pSTSummaryRecord->iRetcode
						<<std::left<< std::setw(20)<< pSTSummaryRecord->uiCount
						<<std::left<< std::setw(20)<< pSTSummaryRecord->uiAvgTime
						<<std::left<< std::setw(20)<< pSTSummaryRecord->uiMaxTime
						<<std::left<< std::setw(20)<< pSTSummaryRecord->uiMinTime
						<<std::left<< std::endl;

				// 空间释放了
				if(pSTSummaryRecord->szCaller[0] == 0)
					continue;

				m_mapSummaryRecord[pSTSummaryRecord->GetRecordID()] = pSTSummaryRecord;
			}

			return 0;

		}

		void DumpInfo()
		{
			std::map<std::string,STSummaryRecord *>::iterator iter;

			std::cout << "正在汇总的统计记录: \r\n =========================================\r\n "
					<< "12345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890" ;
			std::cout
				<<std::left<< std::setw(20)<< "内存ID"
				<<std::left<< std::setw(20)<< "版本"
				<<std::left<< std::setw(20)<< "时间段"
				<<std::left<< std::setw(50) << "调用方ID"
				<<std::left<< std::setw(20)<< "调用方IP"
				<<std::left<< std::setw(20)<< "被调方ID"
				<<std::left<< std::setw(20)<< "被调方IP"
				<<std::left<< std::setw(20)<< "被调方PORT"
				<<std::left<< std::setw(20)<< "返回码"
				<<std::left<< std::setw(20)<< "总次数"
				<<std::left<< std::setw(20)<< "平均时长"
				<<std::left<< std::setw(20)<< "最大时长"
				<<std::left<< std::setw(20)<< "最小时长"
				<<std::left<< std::endl;


			for(iter=m_mapFileProcessingData.begin();iter != m_mapFileProcessingData.end();iter++)
			{
				std::cout
						<<std::left<< std::setw(20)<< (iter->second)->uiRecordMemID
						<<std::left<< std::setw(20)<< (iter->second)->cVer
						<<std::left<< std::setw(20)<< (iter->second)->tmPeriod
						<<std::left<< std::setw(50) << (iter->second)->szCaller
						<<std::left<< std::setw(20)<< (iter->second)->szCallerNodeIp
						<<std::left<< std::setw(20)<< (iter->second)->szCallee
						<<std::left<< std::setw(20)<< (iter->second)->szCalleeNodeIp
						<<std::left<< std::setw(20)<< (iter->second)->szCalleeNodePort
						<<std::left<< std::setw(20)<< (iter->second)->iRetcode
						<<std::left<< std::setw(20)<< (iter->second)->uiCount
						<<std::left<< std::setw(20)<< (iter->second)->uiAvgTime
						<<std::left<< std::setw(20)<< (iter->second)->uiMaxTime
						<<std::left<< std::setw(20)<< (iter->second)->uiMinTime
						<<std::left<< std::endl;
			}

		}

		STSummaryRecord * GetSummaryRecord(const STSummaryRecord * objSTSummaryRecord)
		{
			STSummaryRecord * data = NULL;

			if(m_mapSummaryRecord.find(objSTSummaryRecord->GetRecordID()) != m_mapSummaryRecord.end())
			{
				data = m_mapSummaryRecord[objSTSummaryRecord->GetRecordID()];

				data->uiAvgTime = data->uiCount/(data->uiCount+1) * data->uiAvgTime + objSTSummaryRecord->uiAvgTime /(data->uiCount+1)

				if(data->uiMinTime > objSTSummaryRecord->uiAvgTime)
					data->uiMinTime = objSTSummaryRecord->uiAvgTime;
				if(data->uiMaxTime < objSTSummaryRecord->uiAvgTime)
					data->uiMaxTime = objSTSummaryRecord->uiAvgTime;
				data->uiCount++;

			}
			else
			{
				CPointer pt = objCLineSpaceMgr.Alloc();

				data = (STSummaryRecord*) objCLineSpaceMgr.AsVoid(pt);


				if(data != NULL)
				{

					memcpy(data,objSTSummaryRecord,sizeof(STSummaryRecord));


					data->uiRecordMemID = pt.m_uiOffset;


					m_mapSummaryRecord[objSTSummaryRecord->GetRecordID()] = data;


				}
			}

			return data;
		}

		void Parse(STFileProcessingStatus * pSTFile)
		{
			std::ifstream ifs (pSTFile->szFileName, std::ifstream::in);

			ifs.seekg (pSTFile->ilOffset);

			string itemline = "";

			while(true)
			{
				getline(ifs, itemline);
				if(itemline == "" || itemline.length() <= 1)
				{
					break;
				}

				pSTFile->ilOffset = ifs.tellg();

				if(itemline.length() < 20 || itemline.length() > 800)
				{
					continue;
				}


				STSummaryRecord objSTSummaryRecord;

				if(0 == objSTSummaryRecord.Parse(itemline.c_str(),m_iIntval))
				{
					if(NULL != GetSummaryRecord(&objSTSummaryRecord))
					{
						std::cout << "获取统计记录失败：" << objSTSummaryRecord.GetRecordID() << std::endl;
					}
				}
				else
				{
					std::cout << "解析行失败：" << itemline << std::endl;
				}
			}

			ifs.close();
		}
};



#endif /* CSummaryRecord_H_ */
