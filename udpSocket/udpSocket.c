/*****************************************************************************************
 * 文件说明：
 * UDP SOCKET 创建，数据接收以及命令解析处理
 *****************************************************************************************/
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

#include "udpSocket.h"
#include "../json/cJSON.h"
#include "../cAppTask/cAppTask.h"
#include "../perDevice/perDevice.h"
#include "../devConfig.h"
#include "udpSocket/udpfifo.h"
#include "../weighApi/getWeighApi.h"


//udp命令列表 利用enum实现switch case 字符串功能
enum choice {getVersion, setLed, getWeigh, getCard, getCardWeigh, getDevUUID, ota, endCmd};
const char *choices[] = {"getVersion", "setLed", "getWeigh", "getCard", "getCardWeigh", "getDevUUID", "ota" "endCmd"};

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
char udpRcvBuf[1024]={0};
char udpSendBuf[1024]={0};//udp发送数据缓存

// //安卓 udp 命令解析
// static void udpsocket_data_processing(char *data);

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
    debug_print("\n======================java client initialization======================\n"); 
    if ((javaSocketFd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) 
    { 
        perror("socket 创建失败");
        return FAIL;
    }
    debug_print("socket connect ok\n");

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
    _udp_Msg pUdpmag;

    memset(udpRcvBuf, 0, sizeof(udpRcvBuf));
    int rlen = recvfrom(javaSocketFd, udpRcvBuf, sizeof(udpRcvBuf), 0, (struct sockaddr*)&cliaddr, &len);
    if(rlen>0)
    {
        //LogWrite("INFO",buf);
        #ifdef UDP_FIFO_MODE
            pUdpmag.byte_count = rlen;
            memset(pUdpmag.data,0,sizeof(pUdpmag.data));
            memcpy(pUdpmag.data, udpRcvBuf, rlen);
            debug_print("udpsocket_receive_data = %s\n",pUdpmag.data);
            //udp接收数据添加到缓存队列
            udp_fifo_push_data_msg(&pUdpmag);
        #else
            udpsocket_data_processing(udpRcvBuf);
        #endif
        usleep(1000);
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
* 函 数 名： udpsocket_ack_result_code
* 参    数：
* 功能描述:  udp socket 发送错误应答
* 返 回 值： 创建成功返回0
* 备    注： None
* 作    者： lc
* 创建时间： 2023/03/30
==================================================================================*/
static void udpsocket_ack_result_code(int status, char *string)
{
    //创建根节点JSON(最外面大括号的JSON对象)
    cJSON *json_root=cJSON_CreateObject();
    cJSON_AddBoolToObject(json_root, "success", status); 
    cJSON_AddStringToObject(json_root, "message", string);
    char *data = cJSON_Print(json_root);
    cJSON_Delete(json_root); 
    debug_print("udp send data = %s\n", data);
    udpsocket_send_data(data);
}

/*==================================================================================
* 函 数 名： udpsocket_ack_data_code
* 参    数：
* 功能描述:  udp socket 应答
* 返 回 值： 创建成功返回0
* 备    注： None
* 作    者： lc
* 创建时间： 2023/03/30
==================================================================================*/
static void udpsocket_ack_data_code(int status, char *msg,  char *dataStr)
{
    //创建根节点JSON(最外面大括号的JSON对象)
    cJSON *json_root=cJSON_CreateObject();
    cJSON_AddBoolToObject(json_root, "success", status); 
    cJSON_AddStringToObject(json_root, "message", msg);
    cJSON_AddStringToObject(json_root, "data", dataStr);
    char *data = cJSON_Print(json_root);
    cJSON_Delete(json_root); 
    debug_print("udp send data = %s\n", data);
    udpsocket_send_data(data);
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
void udpsocket_data_processing(char *json_string)
{
    int i =0,num=0;
    int status;
    char *buf =NULL;
    char data[512] = {0};
    char msg_buf[512] = {0};
    char temp_str[256] = {0};
    char numStr[256] = {0};
    char hardVerStr[256] = {0};
    char softVerStr[256] = {0};

    _Tag_Info cardInfoSt = {0};//卡片信息结构体
    android_rcv_uart_Msg pVerMsg = {0};//版本号结构体
    cJSON* rootJson = NULL;
    cJSON *dataJson = NULL;
    cJSON *cmdJson = NULL;   
    cJSON *actionJson = NULL;
    //设备状态json对象
    cJSON *cjson_ack_data = NULL;
    cJSON *json_root = NULL;
    
    
    //JSON字符串到cJSON格式
    rootJson = cJSON_Parse(json_string); 
    //判断cJSON_Parse函数返回值确定是否打包成功
    if(rootJson == NULL) {
        debug_print("json pack into cjson error...\n");
        goto err;
    } else {//打包成功调用cJSON_Print打印输出
        cJSON_Print(rootJson);
    }
     //获取字段值
    //cJSON_GetObjectItemCaseSensitive返回的是一个cJSON结构体所以我们可以通过函数返回结构体的方式选择返回类型！
    cmdJson = cJSON_GetObjectItem(rootJson, "cmd");
    if(cmdJson==NULL)
    {
        goto err;
    }
    //获取字段值
    //cJSON_GetObjectItemCaseSensitive返回的是一个cJSON结构体所以我们可以通过函数返回结构体的方式选择返回类型！
    char *cmdStr = cJSON_GetObjectItemCaseSensitive(rootJson, "cmd")->valuestring;
    //打印输出
    debug_print("cmd = %s\n", cmdStr);
    for(i=getVersion; i<endCmd; i++)
    {
        if(strcmp(cmdStr, choices[i]) == 0)
        {
            break;
        }
    }
    switch(i)
    {
        case getVersion://获取设备版本号
            debug_print("get dev version msg cmd\n");
            pVerMsg = read_bufei_version();
            
            memset(hardVerStr,0,sizeof(hardVerStr));
            memset(softVerStr,0,sizeof(softVerStr));
            sprintf(hardVerStr,"%02X", pVerMsg.data[6] );
            sprintf(softVerStr,"%02X", pVerMsg.data[7]);

            debug_print("get gd32 version %s %s \n", hardVerStr, softVerStr);
            memset(msg_buf, 0, sizeof(data)); 
            //创建根节点JSON(最外面大括号的JSON对象)
            json_root=cJSON_CreateObject();  
            if(pVerMsg.byte_count > 0)//查询成功
            {
                /* 添加一个嵌套的JSON数据（添加一个链表节点） */
                cjson_ack_data = cJSON_CreateObject();
                cJSON *cjson_ack_weigh_ver   = cJSON_CreateObject();
                cJSON *cjson_ack_master_ver  = cJSON_CreateObject();
                cJSON_AddStringToObject(cjson_ack_weigh_ver,  "hardVersion", hardVerStr);
                cJSON_AddStringToObject(cjson_ack_weigh_ver,  "softVersion", softVerStr);
                cJSON_AddStringToObject(cjson_ack_master_ver, "hardVersion", APP_HARD_VER);
                cJSON_AddStringToObject(cjson_ack_master_ver, "softVersion", APP_SOFT_VER);

                cJSON_AddItemToObject(cjson_ack_data, "weighBoard",  cjson_ack_weigh_ver);
                cJSON_AddItemToObject(cjson_ack_data, "masterBoard", cjson_ack_master_ver);

                cJSON_AddBoolToObject(json_root, "success", cJSON_True); 
                cJSON_AddStringToObject(json_root, "message", "success");
                cJSON_AddItemToObject(json_root, "data", cjson_ack_data);
            } else {//查询失败
                cJSON_AddBoolToObject(json_root, "success", cJSON_False); 
                cJSON_AddStringToObject(json_root, "message", "gd32 version query fail");
            }  
                 
            char *data = cJSON_Print(json_root);
            cJSON_Delete(json_root); 
            debug_print("udp send data version= %s\n", data);
            udpsocket_send_data(data);
            return;
        break;

        case setLed://设置led
            debug_print("set led cmd\n");
            dataJson = cJSON_GetObjectItem(rootJson, "data");
            if(dataJson==NULL)
            {
                goto err;
            } 
            //json数据转换成字符串并打印输出
            char *str = cJSON_PrintUnformatted(dataJson);
            debug_print("dataJson  = %s\n", str);
            actionJson = cJSON_GetObjectItem(dataJson, "led");
            if(actionJson==NULL)
            {
                debug_print("data led cmd err\n");
                goto err;
            } 
            str = cJSON_PrintUnformatted(actionJson);
            //获取元素个数
            num =  cJSON_GetArraySize(actionJson);
            debug_print("actionJson = %s number = %d\n", str, num);
            if(num == 0)//没有查询到要控制的led
            {
                goto err;
                return;
            }
            for(int i=0; i<=MAX_LED_NUM; i++)
            {
                memset(numStr, 0, sizeof(numStr));
                sprintf(numStr,"%d",i);
                /*判断是否有key键值是string的项，如果有返回1，否则返回0*/
                if(cJSON_HasObjectItem(actionJson, numStr) > 0)
                {
                    cJSON *numJson = cJSON_GetObjectItem(actionJson, numStr); 
                    if(strcmp("open", numJson->valuestring) == 0)
                    {
                        debug_print("LED-%d open\n", i);
                        open_close_led_device(i, true); 
                    } else {
                        debug_print("LED-%d close\n", i);
                        open_close_led_device(i, false); 
                    }
                }
            }  
            //delete rootJson
            cJSON_Delete(rootJson);
            memset(data, 0, sizeof(data)); 
            sprintf(data, "%s", "success");
            udpsocket_ack_result_code(cJSON_True, data);
            return;
        break;

        case getWeigh://获取当前重量用户卡信息
            debug_print("get current weigh msg cmd\n");
            buf = get_weigh_uuid_from_gd32(); 
            memset(udpSendBuf, 0, sizeof(udpSendBuf));
            memcpy(udpSendBuf, buf, strlen(buf));
            debug_print("udp send data = %s\n", udpSendBuf);
            //delete rootJson
            cJSON_Delete(rootJson);
            udpsocket_send_data(udpSendBuf);
            return;
        break;

        case getCard://获取用户卡片信息
            debug_print("get usr card msg cmd\n");
             //获取重量以及卡片信息
            cardInfoSt = get_weigh_card_uid();
            //创建根节点JSON(最外面大括号的JSON对象)
            json_root=cJSON_CreateObject();  
            /* 添加一个嵌套的JSON数据（添加一个链表节点） */
            cjson_ack_data = cJSON_CreateObject();
            if(cardInfoSt.toalNum > 0)//有卡存在
            {
                cJSON *cjson_card_array   = cJSON_CreateArray();//创建一个json数组
                cJSON *cjson_card  = cJSON_CreateObject();

                memset(temp_str,0,sizeof(temp_str));
                sprintf(temp_str,"%02X %02X %02X %02X %02X %02X %02X %02X",\
                    cardInfoSt.uid[0],cardInfoSt.uid[1],cardInfoSt.uid[2],cardInfoSt.uid[3],\
                    cardInfoSt.uid[4],cardInfoSt.uid[5],cardInfoSt.uid[6],cardInfoSt.uid[7]);
                cJSON_AddStringToObject(cjson_card,  "uid", temp_str);
                cJSON_AddNumberToObject(cjson_card,  "code", cardInfoSt.meicanCode);
                cJSON_AddItemToArray(cjson_card_array, cjson_card);

                cJSON_AddItemToObject(cjson_ack_data, "card", cjson_card_array);
            }
            cJSON_AddBoolToObject(json_root, "success", cJSON_True); 
            cJSON_AddStringToObject(json_root, "message", "success");
            cJSON_AddItemToObject(json_root, "data", cjson_ack_data);
            
            char *cardData = cJSON_Print(json_root);
            cJSON_Delete(json_root); 
            debug_print("udp send data getCard = %s\n", cardData);
            udpsocket_send_data(cardData);
            return;
        break;

        case getCardWeigh:
            //获取重量以及卡片信息
            cardInfoSt = get_weigh_card_uid();
            //创建根节点JSON(最外面大括号的JSON对象)
            json_root=cJSON_CreateObject();  
            /* 添加一个嵌套的JSON数据（添加一个链表节点） */
            cjson_ack_data = cJSON_CreateObject();
            if(cardInfoSt.toalNum > 0)//有卡存在
            {
                cJSON *cjson_card_array   = cJSON_CreateArray();//创建一个json数组
                cJSON *cjson_card  = cJSON_CreateObject();

                memset(temp_str,0,sizeof(temp_str));
                sprintf(temp_str,"%02X %02X %02X %02X %02X %02X %02X %02X",\
                    cardInfoSt.uid[0],cardInfoSt.uid[1],cardInfoSt.uid[2],cardInfoSt.uid[3],\
                    cardInfoSt.uid[4],cardInfoSt.uid[5],cardInfoSt.uid[6],cardInfoSt.uid[7]);
                cJSON_AddStringToObject(cjson_card,  "uid", temp_str);
                cJSON_AddNumberToObject(cjson_card,  "code", cardInfoSt.meicanCode);
                cJSON_AddItemToArray(cjson_card_array, cjson_card);

                cJSON_AddItemToObject(cjson_ack_data, "card", cjson_card_array);
            }
            cJSON_AddNumberToObject(cjson_ack_data, "weigh", cardInfoSt.weighValue);
            cJSON_AddBoolToObject(json_root, "success", cJSON_True); 
            cJSON_AddStringToObject(json_root, "message", "success");
            cJSON_AddItemToObject(json_root, "data", cjson_ack_data);
            
            char *weighCardData = cJSON_Print(json_root);
            cJSON_Delete(json_root); 
            debug_print("udp send data getCardWeigh = %s\n", weighCardData);
            udpsocket_send_data(weighCardData);
            return;
        break;

        case getDevUUID://获取设备唯一UUID
            debug_print("get dev uuid cmd\n");
             //创建根节点JSON(最外面大括号的JSON对象)
            json_root=cJSON_CreateObject();  
            /* 添加一个嵌套的JSON数据（添加一个链表节点） */
            cjson_ack_data = cJSON_CreateObject();
            cJSON_AddStringToObject(cjson_ack_data, "uuid", "11 22 33 44 55 66 77 88");
            cJSON_AddBoolToObject(json_root, "success", cJSON_True); 
            cJSON_AddStringToObject(json_root, "message", "success");
            cJSON_AddItemToObject(json_root, "data", cjson_ack_data);
            
            char *uuidData = cJSON_Print(json_root);
            cJSON_Delete(json_root); 
            debug_print("udp send data uuid= %s\n", uuidData);
            udpsocket_send_data(uuidData);
            return;
        break;

        case ota://固件升级
            debug_print("ota cmd\n");
        break;

        default:
            goto err;
        break;
    }
    err://json错误应答
        //delete rootJson
        cJSON_Delete(rootJson);
        //udp socket 应答
        memset(data, 0, sizeof(data));
        sprintf(data, "%s", "err json cmd");
        udpsocket_ack_result_code(cJSON_False, data);
        return;
}
