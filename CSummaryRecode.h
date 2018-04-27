/*
 * CSummaryRecode.h
 *
 *  Created on: 2018年4月25日
 *      Author: Administrator
 */

#ifndef CSUMMARYRECODE_H_
#define CSUMMARYRECODE_H_

#include <time.h>
#include <sys/types.h>
#include <unistd.h>

#include <string>
#include <map>

#include "CLineSpaceMgr.h"
#include "CBaseEncode.h"


/*
 * 积累的将要上报的数据
 * */
typedef struct ST_SummaryRecode
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

	inline std::string GetRecordID()
	{
		std::string res = "";

		res = res + szCaller
				+ szCallerNodeIp
				+ szCallee
				+ szCalleeNodeIp
				+ szCalleeNodePort
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
		}
		default:
			return -2;// 不支持的版本
		}

		for(int i=0;i<stringVect.size();i++)
		{

		}

		return 0;
	}


	ST_SummaryRecode()
	{

	}
	ST_SummaryRecode(const char * str,int period)
	{
		Parse(str,period);



	}



}STSummaryRecode;

class CSummaryRecode
{
private:
	std::map<std::string,STSummaryRecode *> m_mapSummaryRecode;

	CLineSpaceMgr objCLineSpaceMgr;

	std::string m_sDir,m_sMapFileName;

	int m_iIntval;

	size_t m_iRecordMaxCount;



public:
	SummaryRecode(){};
	virtual ~SummaryRecode(){};

	int Init(const std::string & dir,int intval=300,const std::string & hra_file_name = ".hra.processing.recode",int max_count=1000)
		{
			m_iIntval = intval;

			m_sDir = dir;

			m_sMapFileName = dir + hra_file_name;

			m_iRecordMaxCount = max_count;

			size_t szStruct = sizeof(STSummaryRecode);

			std::cout << "初始化共享内存 ： " << m_sMapFileName << std::endl;

			if( 0 != objCLineSpaceMgr.Init(szStruct,m_iRecordMaxCount,m_sMapFileName.c_str(),true) )
			{
				std::cout << objCLineSpaceMgr.GetTraceLog() << std::endl;
				return -1;
			}

			// 遍历 ,建立统计周期内调用关系与返回码到记录数据的信息
			for(int i=0;i<objCLineSpaceMgr.GetSize();i++)
			{
				STSummaryRecode *pSTSummaryRecode = (STSummaryRecode*) objCLineSpaceMgr.AsVoid(CPointer(1,i));

				// 系统错误
				if(pSTSummaryRecode == NULL)
					return -1;

				pSTSummaryRecode->uiRecordMemID = i;
	/**/
				std::cout
									<<std::left<< std::setw(20)<< pSTSummaryRecode->uiRecordMemID
									<<std::left<< std::setw(20)<< pSTSummaryRecode->cVer
									<<std::left<< std::setw(20)<< pSTSummaryRecode->tmPeriod
									<<std::left<< std::setw(50) << pSTSummaryRecode->szCaller
									<<std::left<< std::setw(20)<< pSTSummaryRecode->szCallerNodeIp
									<<std::left<< std::setw(20)<< pSTSummaryRecode->szCallee
									<<std::left<< std::setw(20)<< pSTSummaryRecode->szCalleeNodeIp
									<<std::left<< std::setw(20)<< pSTSummaryRecode->szCalleeNodePort
									<<std::left<< std::setw(20)<< pSTSummaryRecode->iRetcode
									<<std::left<< std::setw(20)<< pSTSummaryRecode->uiCount
									<<std::left<< std::setw(20)<< pSTSummaryRecode->uiAvgTime
									<<std::left<< std::setw(20)<< pSTSummaryRecode->uiMaxTime
									<<std::left<< std::setw(20)<< pSTSummaryRecode->uiMinTime
									<<std::left<< std::endl;

				// 空间释放了
				if(pSTSummaryRecode->szCaller[0] == 0)
					continue;

				m_mapSummaryRecode[pSTSummaryRecode->GetRecordID()] = pSTSummaryRecode;
			}

			return 0;

		}

		void DumpIno()
		{
			std::map<std::string,STFileProcessingStatus *>::iterator iter;

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
};



#endif /* CSUMMARYRECODE_H_ */
