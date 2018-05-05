/*
 * 对特定目录的所有文件进行分析，并将符合格式的文件按5分钟汇总，上报到kafuka的特定队列
 * $healthy_report_log_dir
 * $kafuka_url
 *
 * 用mmap记录每个文件的处理进度，以及汇总的结果的推送状态，推送成功后，才会去分析下一个周期（5分钟）汇总
 * $mmap_id
 * $summary_cycle
 *
 * 对分析完毕的文件，如果配置的周期内（1个）没有新的数据进来，就要进行删除
 * $eliminate_cycle_count
 *
 * 约束：每个特定目录只有一个进程去分析，如果性能不够，可考虑多个目录，多套部署。
 * healthy_report
 * 	+cnf
 * 		healthy_report.index.conf.xml
 * 	+dat
 * 		file_processing.shm
 *
 * 	+log
 *
 * 	+bin
 *
 * 命令行：可指定使用默认目录获取配置，还是从命令行运行
 * 	--log-dir=/data/healthy_report/log/
 * 	--kafuka-url=kafuka://127.0.0.1:9590/healthy_report/0
 * 	--mmap-id=.processing.shm.hra
 * 	--summary-cyle=300
 * 	--eliminate-cycle-count=2
 * 	--run-by-cmdline （swith ： on | off）
 * 	--max-thread=1
 * 	--cmd=(summary|delete|)
 *
 *	尺寸：
 *		char szFilename(128), unsigned long ulOffset, time_t ttLastModify, time_t ttLastReaded
 *		caller|callee|callernode|calleenode|retcode|count|average
 * */

#include "CCommandLineInfo.h"
#include "CUnixFileLock.h"

#include <sys/stat.h>
#include "CFileProcessingStatus.h"
#include "CSummaryRecord.h"
#include <fiostream.h>

void InitCommandLine()
{
	MyUtility::g_objCCommandLineInfo.AddEntry("log-dir","--log-dir=","/data/healthy_report/log/",false,true,"服务健康上报日志存放目录，将分析此目录的文件并上报消息队列");
	MyUtility::g_objCCommandLineInfo.AddEntry("ext-name","--ext-name=","*log",false,true,"服务健康上报日志的扩展名,如*log");
	MyUtility::g_objCCommandLineInfo.AddEntry("max_file_count","--max_file_count=","100",false,true,"为");
	MyUtility::g_objCCommandLineInfo.AddEntry("kafuka-url","--kafuka-url=","kafuka://127.0.0.1:9092/healthy_report/0",false,true,"kafuka消息队列的地址、topic名与分区号");
	MyUtility::g_objCCommandLineInfo.AddEntry("mmap-id","--mmap-id=",".hra.processing",false,true,"mmap文件名前缀，记录文件处理状态*file.1与统计结果*summary.1，默认1000条，不够时会自动拓展,.1,.2");
	MyUtility::g_objCCommandLineInfo.AddEntry("summary_cycle","--summary_cycle=","300",false,true,"统计周期，按这个时间间隔汇总，单位秒，默认5分钟");
	MyUtility::g_objCCommandLineInfo.AddEntry("eliminate-cycle","--eliminate-cycle=","600",false,true,"过期时间，处理完毕的文件如果持续时间未更新，则删除");

	MyUtility::g_objCCommandLineInfo.AddEntry("logtest-filename","--logtest-filename=","abcd1234",false,true,"日志文件名称");
	MyUtility::g_objCCommandLineInfo.AddEntry("logtest-begin-time","--eliminate-cycle=","1525527005",false,true,"日期记录开始时间");
	MyUtility::g_objCCommandLineInfo.AddEntry("logtest-end-time","--eliminate-cycle=","1525528005",false,true,"日期记录结束时间");
	MyUtility::g_objCCommandLineInfo.AddEntry("logtest-count","--logtest-count=","10000",false,true,"日志总条数");
	MyUtility::g_objCCommandLineInfo.AddEntry("logtest-caller","--logtest-caller=","app",false,true,"调用方名称");
	MyUtility::g_objCCommandLineInfo.AddEntry("logtest-callee","--logtest-callee=","webservice",false,true,"被调用方名称");
	MyUtility::g_objCCommandLineInfo.AddEntry("logtest-callee-node","--logtest-callee-node=","192.168.1.1：1989",false,true,"被调用方所在节点");
	MyUtility::g_objCCommandLineInfo.AddEntry("logtest-callee-method","--logtest-callee-method=","getPrice",false,true,"被调用方接口名");
	MyUtility::g_objCCommandLineInfo.AddEntry("logtest-retcode","--logtest-retcode=","0",false,true,"返回码");

	MyUtility::g_objCCommandLineInfo.AddEntry("run-by-cmdline","run-by-cmdline","off",true,true,"从命令行运行");

	MyUtility::g_objCCommandLineInfo.AddEntry("cmd","--cmd=","work",false,false,
		"work 开始分析、汇总、上报的工作  \r\n"
		"\twatch 查看工作进度：分析中的文件列表，输出的结果  "
		);
}

void WatchProcessing()
{
	/**
	 * 显示指定目录，正在对齐分析的进程信息、被其处理的文件进度、汇总的数据记录，上报的状态等等
	 * 如果希望看到动态的，可使用watch来输出：watch -d -n 1 "hra --cmd=watch --log-dir=/data/healthy_report/log/"
	 */
	CFileProcessingStatus objCFileProcessingStatus;

	objCFileProcessingStatus.Init(MyUtility::g_objCCommandLineInfo.GetArgVal("log-dir"),
			MyUtility::CBaseEncode::StringToInt(MyUtility::g_objCCommandLineInfo.GetArgVal("eliminate-cycle")),
			MyUtility::g_objCCommandLineInfo.GetArgVal("mmap-id")+".file",
			MyUtility::CBaseEncode::StringToInt(MyUtility::g_objCCommandLineInfo.GetArgVal("max_file_count")));

	objCFileProcessingStatus.DumpInfo();


	CSummaryRecord objCSummaryRecord;

	objCSummaryRecord.Init(MyUtility::g_objCCommandLineInfo.GetArgVal("log-dir"),
			MyUtility::CBaseEncode::StringToInt(MyUtility::g_objCCommandLineInfo.GetArgVal("summary_cycle")),
			MyUtility::g_objCCommandLineInfo.GetArgVal("mmap-id")+".record",
			MyUtility::CBaseEncode::StringToInt(MyUtility::g_objCCommandLineInfo.GetArgVal("max_file_count"))*100);

	objCSummaryRecord.DumpInfo();

}

// 随机打印一个文件，随机n条记录
void LogTest()
{
	// 目录：MyUtility::g_objCCommandLineInfo.GetArgVal("log-dir")

	// 后缀：MyUtility::g_objCCommandLineInfo.GetArgVal("ext-name")

	// 后缀：MyUtility::g_objCCommandLineInfo.GetArgVal("logtest-filename")

	// 文件名 路径
	std::string filepath = 	MyUtility::g_objCCommandLineInfo.GetArgVal("log-dir")+
			MyUtility::g_objCCommandLineInfo.GetArgVal("logtest-filename")+
			MyUtility::g_objCCommandLineInfo.GetArgVal("ext-name");


	// 时间区间  	MyUtility::g_objCCommandLineInfo.GetArgVal("logtest-begin-time")
	//			MyUtility::g_objCCommandLineInfo.GetArgVal("logtest-end-time")
	//			MyUtility::g_objCCommandLineInfo.GetArgVal("logtest-count")


	// 调用方与被调方返回码等 MyUtility::g_objCCommandLineInfo.GetArgVal("logtest-caller")
	// MyUtility::g_objCCommandLineInfo.GetArgVal("logtest-callee")
	// MyUtility::g_objCCommandLineInfo.GetArgVal("logtest-retcode")

	ofstream out(filepath);

	if (out.is_open())
	{
		std::cout << "Error on openning file :" << filepath << std::endl;
		return ;
	}

	int iCount = MyUtility::CBaseEncode::StringToInt(MyUtility::g_objCCommandLineInfo.GetArgVal("logtest-count"));

	unsigned int tmBegin = MyUtility::CBaseEncode::StringToInt(MyUtility::g_objCCommandLineInfo.GetArgVal("logtest-begin-time"));
	unsigned int tmEnd = MyUtility::CBaseEncode::StringToInt(MyUtility::g_objCCommandLineInfo.GetArgVal("logtest-end-time"));

	unsigned int tmTmp;

	for(int i = 0 ; i < iCount; i++ )
	{	// 1|1525443767|caller_2|callee_1|callee_node|method|-3|90
		tmTmp = (i*(tmEnd-tmBegin)/iCount ) + tmBegin;


		out << 1
			<< tmTmp
			<< MyUtility::g_objCCommandLineInfo.GetArgVal("logtest-caller")
			<< MyUtility::g_objCCommandLineInfo.GetArgVal("logtest-callee")
			<< MyUtility::g_objCCommandLineInfo.GetArgVal("logtest-callee-node")
			<< MyUtility::g_objCCommandLineInfo.GetArgVal("logtest-callee-method")
			<< MyUtility::g_objCCommandLineInfo.GetArgVal("logtest-retcode")
			<< random() % 10000
			<< "\n";
	}

	out.close();

}


void SummaryAndReport()
{
	/**
	 * 确保一个目录只有一个进程在运行，通过在目录里建立一个/$mmap-id/single_instance.lck来实现
	 */
	CUnixFileLock objCUnixFileLock( MyUtility::g_objCCommandLineInfo.GetArgVal("log-dir") );

	if ( objCUnixFileLock.GetLock() )
	{
		std::cout << "工作进行中，可通过命令行查看\r\n"
				<< " watch -d -n 1 \"" << MyUtility::g_objCCommandLineInfo.GetExePath()
				<< " --cmd=watch --log-dir=" << MyUtility::g_objCCommandLineInfo.GetArgVal("log-dir")
				<< "\" " << std::endl;

	}
	else
	{
		std::cout << "已经有进程在工作，进程号是：" << objCUnixFileLock.GetWorkingProcessID()<< std::endl;
		return;
	}
	/**
	 * 遍历目录里面的所有文件，目录会被忽略，更新文件的状态（最后修改时间、大小），并为每个文件分配一个线程去处理; 并循环
	 */
	CFileProcessingStatus objCFileProcessingStatus;

	objCFileProcessingStatus.Init(MyUtility::g_objCCommandLineInfo.GetArgVal("log-dir"),
			MyUtility::CBaseEncode::StringToInt(MyUtility::g_objCCommandLineInfo.GetArgVal("eliminate-cycle")),
			MyUtility::g_objCCommandLineInfo.GetArgVal("mmap-id")+".file",
			MyUtility::CBaseEncode::StringToInt(MyUtility::g_objCCommandLineInfo.GetArgVal("max_file_count")));

	// (const std::string & dir,int intval=300,const std::string & hra_file_name = ".hra.processing.recode",int max_count=1000)
	CSummaryRecord objCSummaryRecord;

	objCSummaryRecord.Init(MyUtility::g_objCCommandLineInfo.GetArgVal("log-dir"),
			MyUtility::CBaseEncode::StringToInt(MyUtility::g_objCCommandLineInfo.GetArgVal("summary_cycle")),
			MyUtility::g_objCCommandLineInfo.GetArgVal("mmap-id")+".record",
			MyUtility::CBaseEncode::StringToInt(MyUtility::g_objCCommandLineInfo.GetArgVal("max_file_count"))*100);

	std::cout << "开始列表文件并分析 " << time(NULL) << std::endl;

	while(true)
	{
		// 加载待处理的文件列表
		objCFileProcessingStatus.GetDirectoryFileStatus();

		// 遍历文件列表，并统计

		STFileProcessingStatus *pCurFile = NULL;

		do
		{
			pCurFile = objCFileProcessingStatus.GetFileProcess();

			if(pCurFile == NULL)
				break;

			// 打开文件并分析
			objCSummaryRecord.Parse(pCurFile);



		}
		while(NULL != pCurFile );

		sleep(1); // 间隔1秒，继续扫描目录里面的文件变化
	}


}

int main(int argc, const char **argv) {

	InitCommandLine();

	if(argc == 1)
	{
		std::cout <<"使用方法 : \r\n\r\n"<< MyUtility::g_objCCommandLineInfo.GetPrompt() << "\r\n"<<std::endl;
		return 0;
	}

	MyUtility::g_objCCommandLineInfo.Parse( argc, argv);

	if(MyUtility::g_objCCommandLineInfo.GetArgVal("cmd") == "work")
	{
		SummaryAndReport();
	}
	else if(MyUtility::g_objCCommandLineInfo.GetArgVal("cmd") == "watch")
	{
		WatchProcessing();
	}
	else if(MyUtility::g_objCCommandLineInfo.GetArgVal("cmd") == "logtest")
	{
		LogTest();
	}
	else
	{
		std::cout << "不支持这个命令字 : " << MyUtility::g_objCCommandLineInfo.GetArgVal("cmd") << std::endl;

		std::cout << MyUtility::g_objCCommandLineInfo.GetPrompt();

	}

	return 0;
}
