/*
 * 对特定目录的所有文件进行分析，并将符合格式的文件按5分钟汇总，上报到kafuka的特定队列
 * $healthy_report_log_dir
 * $kafuka_url
 *
 * 用mmap记录每个文件的处理进度，以及汇总的结果的推送状态，推送成功后，才会去分析下一个周期（5分钟）汇总
 * $mmap_id
 * $file_max_count
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
 * */


int main(int argc, char **argv) {

	// 1 . read

	return 0;
}
