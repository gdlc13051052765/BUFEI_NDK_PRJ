#ifndef _GETWEIGHAPI_H__
#define _GETWEIGHAPI_H__

#include <stdio.h>  
#include <stdlib.h>  
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "../crc/gdCrc32.h"
 

#define MAX_READ_ZERO_NUM		5
#define MAX_TAG_NUM		    	1
#define TAG_MAX_TRY_TIME		3			//最大尝试次数，超过则认为标签离开
#define TAG_UID_LENS			8
#define TAG_LENGTH		    	TAG_UID_LENS

typedef unsigned char u8;
typedef unsigned int  u32;

#define UPDATE_DATA_LENS	106		
#define READ_CMD  2//单片机板读命令
#define WRITE_CMD 3//单片机板写命令



#define T113_RCV_LENGTH			100
//串口收发数据结构体
typedef struct 
{ 
    uint8_t byte_count;
    char data[T113_RCV_LENGTH];
}android_rcv_uart_Msg, *_pandroid_rcv_uart_Msg;

//版本信息结构体
typedef struct 
{ 
    char hardVer[100]; 
    char softVer[100];
}android_ver_Msg, *_pandroid_ver_Msg;

typedef enum
{
	//安卓发起到CC的命令 
	ANDROID_CC_GET_SW_AND_HW_VER = 0xA1,//获取版本号
	ANDROID_CC_GET_WEIGH_UID,//获取当前重量值跟卡号
	ANDROID_CC_GET_WEIGH,//获取当前重量值
	ANDROID_CC_GET_CARD_UID,//获取当卡号

	//固件升级命令
	ANDROID_CC_UPDATE_INFO  = 0x80,//下发升级包信息
	ANDROID_CC_UPDATE_DATA  = 0x81,//下发升级包数据
	ANDROID_CC_GET_VERSION  = 0x82,//获取软硬件版本
	ANDROID_CC_UPDATE_CHECK = 0x84,//检查确认升级
	ANDROID_CC_UPDATE_RESET = 0x88,//重启
	ANDROID_CC_UPDATE_LOST  = 0x8A,//下发缺失升级包
	
}android_Cmd;

//发送数据结构体
typedef struct
{
	//读卡参数
	uint8_t id;		//ID号
	uint8_t len;//数据长度
	uint8_t slid;
	uint8_t main_cmd;//主命令
	uint8_t nodeID;
	uint8_t slave_cmd;//从指令
	uint8_t data[UPDATE_DATA_LENS];	//数据
	uint32_t crc32;
}_Update_Send_Info,*_pUpdate_Send_Info;

//接收数据结构体
typedef struct
{
	//读卡参数
	uint16_t header;		//数据头
	uint8_t id;		//ID号
	uint8_t len;//数据长度
	uint8_t slid;
	uint8_t nodeID;
	uint8_t data[UPDATE_DATA_LENS];	//数据
	uint32_t crc32;
}_Update_Rcv_Info,*_pUpdate_Rcv_Info;

typedef struct
{
	//读卡参数
	uint8_t  toalNum;//卡片数量
	uint8_t meicanCode;			//美餐标识符
	uint8_t try_time;		//尝试次数，如果超过最大次数则认为标签离开
	//标签信息
	uint8_t uid[TAG_UID_LENS];	//存放标签的uid
	uint8_t block[TAG_LENGTH];	//block数据
    //重量信息
    int32_t weighValue;//重量值
	//重量绝对值
	int32_t absoluteValue;
	//当前消费额
	int32_t currentMoney;
	//消费总额
	int32_t toalMoney;
}_Tag_Info, *_pTag_Info;


typedef struct
{
	//读卡参数
	uint8_t meican_code;			//美餐标识符
	uint8_t try_time;		//尝试次数，如果超过最大次数则认为标签离开
	//标签信息
	uint8_t uid[TAG_LENGTH];		//存放标签的uid
	uint8_t block[TAG_LENGTH];	//block数据
}_Card_Info,*_pCard_Info;

typedef struct
{
	uint8_t total_tag_num;	//总标签数量，包含EXIT_STA状态的标签
	uint8_t read_tag_num;	//读取到的标签数量，每一次读取的标签数量
	uint8_t use_state;		//每一位代表一个存储， 0：当前为空  1：代表有标签数据

	_Card_Info new_tag_info[MAX_TAG_NUM];
	_Card_Info old_tag_info[MAX_TAG_NUM];
}_Tag_Context,*_pTag_Context;

/*==================================================================================
* 函 数 名： init_gd32_uart_api
* 参    数：* 入口参数： devName 设备号"/dev/ttyS0"; baud 波特率
* 功能描述:  初始化获取重量接口
* 返 回 值：
* 备    注： None
* 作    者： lc
* 创建时间： 2021/6/29
==================================================================================*/
int init_gd32_uart_api(char *devName,int baud );

/*==================================================================================
* 函 数 名： send_data_to_gd32
* 参    数： None
* 功能描述:  发送数据到GD32
* 返 回 值： None
* 备    注： 
* 作    者： lc
* 创建时间： 2022-12-08 170658 
==================================================================================*/  
android_rcv_uart_Msg send_data_to_gd32(void* ret_msg) ;

/*==================================================================================
* 函 数 名： read_bufei_version
* 参    数： None
* 功能描述:  读取单片机板版本号
* 返 回 值： None
* 备    注： 
* 作    者： lc
* 创建时间： 2022-12-08 170658
==================================================================================*/  
android_rcv_uart_Msg read_bufei_version(void) ;

/*==================================================================================
* 函 数 名： read_bufei_weigh_uuid
* 参    数： None
* 功能描述:  读取单片机板当前重量跟卡uuid
* 返 回 值： None
* 备    注： 
* 作    者： lc
* 创建时间： 2022-12-08 170658
==================================================================================*/  
_Tag_Info read_bufei_weigh_uuid(void)  ;

/*==================================================================================
* 函 数 名： read_bufei_weigh
* 参    数： None
* 功能描述:  读取单片机板当前重量
* 返 回 值： None
* 备    注： 
* 作    者： lc
* 创建时间： 2022-12-08 170658
==================================================================================*/  
_Tag_Info read_bufei_weigh(void) ; 

/*==================================================================================
* 函 数 名： read_bufei_card_uid
* 参    数： None
* 功能描述:  读取单片机用户卡uid
* 返 回 值： None
* 备    注： 
* 作    者： lc
* 创建时间： 2022-12-08 170658
==================================================================================*/  
_Tag_Info read_bufei_card_uid(void) ;

#endif
