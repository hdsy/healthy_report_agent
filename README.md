# healthy_report_agent 健康上报代理
base on certain format log file , summary and report to an kafuka topic
1.   数据上报类日志

数据上报类日志严格遵从制定的格式，便于分析汇总。如下是以调用者身份上报被调用服务使用状态的日志格式。每一项之间用|分割，供参考。
<table border=1>
<tr>
<td>编号</td>
<td>内容</td>
<td>例子</td>
</tr>
<tr>
<td>1
<td>版本
<td>1
</tr>
<tr>
<td>2
<td>日期时间
<td>1508227752
</tr>
<tr>
<td>3
<td>调用方ID
<td>CGI
</tr>
<tr>
<td>4
<td>调用方所在节点 ID
<td>WX1
</tr>
<tr>
<td>5
<td>被调方ID
<td>ORDERSVR
</tr>
<tr>
<td>6
<td>被调方节点ID
<td>LG1
</tr>
<tr>
<td>7
<td>服务与方法ID
<td>Create
</tr>
<tr>
<td>8
<td>返回码
<td>0
</tr>
<tr>
<td>9
<td>耗时
<td>10ms
</tr>
 </table> 

2.   数据上报类日志格式[本地日志文件]

每一个客户在调用相关服务的事后都记录如下格式的日志
<table border=1>
<tr>
<td>编号
<td>内容
<td>例子</tr>
<tr>
<td>1
<td>版本
<td>1</tr>
<tr>
<td>2
<td>日期时间
<td>1508227752</tr>
<tr>
<td>3
<td>调用方ID
<td>CGI</tr>
<tr>
<td>5
<td>被调方ID
<td>ORDERSVR</tr>
<tr>
<td>6
<td>被调方节点ID
<td>LG1</tr>
<tr>
<td>7
<td>服务与方法ID
<td>Create</tr>
<tr>
<td>8
<td>返回码
<td>0</tr>
<tr>
<td>9
<td>耗时
<td>10ms
</tr>
 </table> 

代码片段如下示：

CHealthReport objCHealthReport(sCallerID) ;

......

objCHealthReport.prepareCall(sCalleeID,sCallerNodeInfo);

iRetCode = CallService( .......);

objCHealthReport.report(sMethod,iRetCode);

......



相关日志上报文件路径 /data/healthy_report/ ，文件名按照时间段命名【可配置】，如2018年4月8日凌晨到中午12点的日志文件名：20180408000000_20180408120000.hrl 。其文件内容如下：

...

1|1508227752|CGI|ORDERSVR|LG1|Create|0|5

1|1508227752|CGI|ORDERSVR|LG1|Create|0|5

1|1508227752|CGI|ORDERSVR|LG1|Create|0|10

1|1508227752|CGI|ORDERSVR|LG1|Create|0|20

1|1508227752|CGI|ORDERSVR|LG1|Create|0|10

...



3.   数据上报类日志格式[kafuka队列格式]

本地agent会对上述日志进行统计，按照5分钟汇总【可配置】，增加服务器节点信息，上报到kafuka消息队列topic为healty_report_of_service_call. 其格式如下：

编号
内容
例子
1
版本
1
2
日期时间
1508227752 5分钟间隔的起始位，如1523116800
3
调用方ID
CGI
4
调用方所在节点 ID
WX1
5
被调方ID
ORDERSVR
6
被调方节点ID
LG1
7
服务与方法ID
Create
8
返回码
0
9	总调用次数	5
10
平均耗时
10ms
11	最大耗时	20ms
12	最小耗时	5ms

例如
1|1523116800|CGI|WX1|ORDERSVR|LG1|Create|0|5|10|20|5

上述字符串就是上报到kafuka的消息内容，可通过msg->payload()获取。

C++开发可参考如下示例，编写消费程序对此数据进行汇总
https://github.com/edenhill/librdkafka/blob/master/examples/rdkafka_consume_batch.cpp
