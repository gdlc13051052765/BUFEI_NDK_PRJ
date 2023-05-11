#include <stdio.h> 
#include <stdlib.h> 
#include <errno.h> 
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <sys/socket.h> 
#include <sys/wait.h> 
#include <unistd.h> 
#include <arpa/inet.h> 
#include "../debug.h"
#include "udpSocket.h"
#include "../json/cJSON.h"
#include "../cAppTask/cAppTask.h"


//udp命令列表 利用enum实现switch case 字符串功能
enum choice {init, getWeigh, endCmd};
const char *choices[] = {"init", "getWeigh","endCmd"};

#define RCV_PORT 6666//接收端口
#define SEN_PORT 8888//发送端口
#define MAXDATASIZE 2048  
#define SERVER_IP "127.0.0.1" 
//#define SERVER_IP "192.168.31.182" 
int javaSocketFd, numbytes;  
struct sockaddr_in server_addr; 
struct sockaddr_in cliaddr;
char ipbuf[64];
int len = sizeof(cliaddr);
char udpRcvBuf[1024]={};
char udpSendBuf[1024]={};//udp发送数据缓存

//安卓 udp 命令解析
static void udpsocket_data_processing(char *data);

//创建UDP——CLIENT跟JAVA UDP——server通讯；利用本地回环地址的形式
/*==================================================================================
* 函 数 名： make_udpsocket_connect
* 参    数：
* 功能描述:  工厂检测数据分析
* 返 回 值：创建成功返回0；
* 备    注： None
* 作    者： lc
* 创建时间： 2021/08/31
==================================================================================*/
int make_udpsocket_connect(void)
{
    debug("\n======================java client initialization======================\n"); 
    if ((javaSocketFd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) 
    { 
        perror("socket 创建失败");
        return FAIL;
    }
    debug("socket connect ok\n");

    server_addr.sin_family = AF_INET; 
    server_addr.sin_port = htons(RCV_PORT); 
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP); 
    //server_addr.sin_addr.s_addr = INADDR_ANY;

    int res = bind(javaSocketFd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if(res < 0) 
    {
        perror("绑定失败");
        close(javaSocketFd);
        return FAIL;
    }  
    return SUCCESS ;
}

/*==================================================================================
* 函 数 名： udpsocket_receive_data
* 参    数：
* 功能描述:  udp socket 数据接收
* 返 回 值： 创建成功返回0
* 备    注： None
* 作    者： lc
* 创建时间： 2023/03/30
==================================================================================*/
void udpsocket_receive_data(void)
{
    memset(udpRcvBuf, 0, sizeof(udpRcvBuf));
    int rlen = recvfrom(javaSocketFd, udpRcvBuf, sizeof(udpRcvBuf), 0, (struct sockaddr*)&cliaddr, &len);
    if(rlen>0)
    {
        //LogWrite("INFO",buf);
        usleep(1000);
        udpsocket_data_processing(udpRcvBuf);
       // udpsocket_send_data(udpSendBuf);
    }
}

/*==================================================================================
* 函 数 名： udpsocket_set_weigh_uuid_data
* 参    数：
* 功能描述:  udp socket 设置当前重量用户卡信息
* 返 回 值： 创建成功返回0
* 备    注： None
* 作    者： lc
* 创建时间： 2023/03/30
==================================================================================*/
void udpsocket_set_weigh_uuid_data(char *data)
{
    memset(udpSendBuf,0,strlen(udpSendBuf));
    memcpy(udpSendBuf,data,strlen(data));
}

/*==================================================================================
* 函 数 名： udpsocket_send_data
* 参    数：
* 功能描述:  udp socket 发送数据
* 返 回 值： 创建成功返回0
* 备    注： None
* 作    者： lc
* 创建时间： 2023/03/30
==================================================================================*/
int udpsocket_send_data(char *data)
{
    printf("client info, ip: %s, port: %d\n",
        inet_ntop(AF_INET, &cliaddr.sin_addr.s_addr, ipbuf, sizeof(ipbuf)),
        ntohs(cliaddr.sin_port));

    //发送到指定端口,不指定的话就发送到接收数据端口
    cliaddr.sin_port = htons(SEN_PORT);
    sendto(javaSocketFd, data, strlen(data), 0, (struct sockaddr*)&cliaddr, sizeof(cliaddr));
    return 0;
}

/*==================================================================================
* 函 数 名： udpsocket_data_processing
* 参    数：
* 功能描述:  udp socket 接收数据处理
* 返 回 值： 创建成功返回0
* 备    注： None
* 作    者： lc
* 创建时间： 2023/03/30
==================================================================================*/
static void udpsocket_data_processing(char *json_string)
{
    int i =0;
    char *buf =NULL;
    
    //JSON字符串到cJSON格式
    cJSON* cjson = cJSON_Parse(json_string); 
    //判断cJSON_Parse函数返回值确定是否打包成功
    if(cjson == NULL){
        printf("json pack into cjson error...");
    }
    else{//打包成功调用cJSON_Print打印输出
        cJSON_Print(cjson);
    }
    //获取字段值
    //cJSON_GetObjectItemCaseSensitive返回的是一个cJSON结构体所以我们可以通过函数返回结构体的方式选择返回类型！
    char *cmdStr = cJSON_GetObjectItemCaseSensitive(cjson,"cmd")->valuestring;
    //打印输出
    printf("cmd  = %s\n", cmdStr);
    //delete cjson
    cJSON_Delete(cjson);
    //命令解析
    for(i=init; i<=endCmd; i++)
    {
        if(strcmp(cmdStr, choices[i]) == 0)
        {
            break;
        }
    }
    switch(i)
    {
        case init:
           // udpsocket_send_data(udpSendBuf);
        break;

        case getWeigh: //获取当前重量用户卡信息
            buf = get_weigh_uuid_from_gd32(); 
            memset(udpSendBuf, 0, sizeof(udpSendBuf));
            memcpy(udpSendBuf, buf, strlen(buf));
            printf("udp send data = %s\n", udpSendBuf);
            udpsocket_send_data(udpSendBuf);
        break;

        default:
        break;
    }
}
