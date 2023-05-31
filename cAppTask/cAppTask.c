/*****************************************************************************************
 * 文件说明：
 * C线程任务管理
 * 为了减少CPU占有率，每个任务里面加入相应的延时
 *****************************************************************************************/

#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>
#include <sys/time.h>
#include <string.h>

#include "../udpSocket/udpsocket.h"
#include "../json/cJSON.h"

#include "../udpSocket/udpfifo.h"
#include "../devConfig.h"
#include "cAppTask.h"

static pthread_t thread[10]; // 两个线程

_Tag_Info pTag = {};

#define TASK_DELAY 1000 // 线程间延时
// 串口设备名字
#define devname "/dev/ttyS1"

static bool tempFlag = false;
static int abs_weigh = 0; // 绝对重量
static int init_weigh = 0;

/*==================================================================================
* 函 数 名： udp_socket_thread
* 参    数： Non
* 功能描述:  udp socket 任务
* 返 回 值：
* 备    注：
* 作    者： lc
* 创建时间： 2021/4/16
==================================================================================*/
static void *udp_recv_socket_thread(void *args)
{
	int status = -1;

	if (!make_udpsocket_connect())
	{
		debug_print("udp socket make ok\n");
	}

	while (1)
	{
		udpsocket_receive_data();
		// usleep(50000);
	}
}

/*==================================================================================
* 函 数 名： udp_ack_socket_thread
* 参    数： Non
* 功能描述:  udp socket 应答任务
* 返 回 值：
* 备    注：
* 作    者： lc
* 创建时间： 2021/4/16
==================================================================================*/
static void *udp_ack_socket_thread(void *args)
{
#ifdef UDP_FIFO_MODE
	// 初始护士udp fifo
	udp_fifo_instance_init();
#endif
	while (1)
	{
		udp_fifo_pop_one_frame();
		usleep(5000);
	}
}

/*==================================================================================
* 函 数 名： create_weigh_json
* 参    数： Non
* 功能描述:  创建重量用户卡信息结构体
* 返 回 值：
* 备    注：
* 作    者： lc
* 创建时间： 2021/4/16
==================================================================================*/
static char *create_weigh_json(_Tag_Info pTag)
{
	char *data = NULL;
	char tempbuf[200] = {};

	// 创建根节点JSON(最外面大括号的JSON对象)
	cJSON *json_root = cJSON_CreateObject();
	if (pTag.toalNum > 0) // 有卡存在
		cJSON_AddStringToObject(json_root, "name", "张三");
	else
		cJSON_AddStringToObject(json_root, "name", "");
	sprintf(tempbuf, "%02X %02X %02X %02X %02X %02X %02X %02X",
			pTag.uid[0], pTag.uid[1], pTag.uid[2], pTag.uid[3],
			pTag.uid[4], pTag.uid[5], pTag.uid[6], pTag.uid[7]);
	cJSON_AddStringToObject(json_root, "uuid", tempbuf);
	cJSON_AddNumberToObject(json_root, "weight", abs_weigh);

	data = cJSON_Print(json_root);
	cJSON_Delete(json_root);
	return data;
}

/*==================================================================================
* 函 数 名： get_weigh_card_uid
* 参    数： Non
* 功能描述:  创建重量用户卡信息结构体
* 返 回 值：
* 备    注：
* 作    者： lc
* 创建时间： 2021/4/16
==================================================================================*/
_Tag_Info get_weigh_card_uid(void)
{
	return pTag;
}

/*==================================================================================
* 函 数 名： get_weigh_gd32_thread
* 参    数： Non
* 功能描述:  从gd32获取重量跟用户卡信息 任务
* 返 回 值：
* 备    注：
* 作    者： lc
* 创建时间： 2021/4/16
==================================================================================*/
static void *get_weigh_gd32_thread(void *args)
{
	int status = -1;
	int fd = -1;

	fd = init_gd32_uart_api(devname, 115200);

	while (1)
	{
		pTag = read_bufei_weigh_uuid() ;
		debug_print("卡片数量 = %d, 美餐标识 = %d, uuid = %02x %02x %02x %02x %02x %02x %02x %02x, 重量 = %d\n",
		                    pTag.toalNum, pTag.meicanCode,  pTag.uid[0], pTag.uid[1], pTag.uid[2], pTag.uid[3],
		                    pTag.uid[4], pTag.uid[5], pTag.uid[6], pTag.uid[7], pTag.weighValue);

		if(pTag.toalNum==0)
		{
			if(tempFlag == false)
			{
				tempFlag = true;
				init_weigh = pTag.weighValue;
			}
		} else {
			if(tempFlag == true)
			{
				tempFlag = false;
				init_weigh = 0;
			}
		}
		abs_weigh = abs(pTag.weighValue - init_weigh);
		usleep(50000);
	}
}

/*==================================================================================
* 函 数 名： get_weigh_uuid_from_gd32
* 参    数： Non
* 功能描述:  从gd32获取重量跟用户卡信息
* 返 回 值：
* 备    注：
* 作    者： lc
* 创建时间： 2021/4/16
==================================================================================*/
char *get_weigh_uuid_from_gd32(void)
{
	char *sendData = NULL;

	// json数据形式发送
	sendData = create_weigh_json(pTag);
	// printf("get_weigh_uuid_from_gd32 = %s\n", sendData);
	return sendData;
}

/*==================================================================================
* 函 数 名： app_wrplate_create_thread
* 参    数： Non
* 功能描述:  创建多线程任务
* 返 回 值：
* 备    注：
* 作    者： lc
* 创建时间： 2021/5/6
==================================================================================*/
void app_bufei_create_thread(void)
{
	int temp;
	memset(&thread, 0, sizeof(thread));

	if ((temp = pthread_create(&thread[0], NULL, udp_ack_socket_thread, NULL)) != 0) // udp socket应答线程任务
		debug_print("thread 0 make fail\n");
	else
		debug_print("thread 0 make ok\n");

	if ((temp = pthread_create(&thread[1], NULL, udp_recv_socket_thread, NULL)) != 0) // udp socket接收线程任务
		debug_print("thread 1 make fail\n");
	else
		debug_print("thread 1 make ok\n");

	if ((temp = pthread_create(&thread[2], NULL, get_weigh_gd32_thread, NULL)) != 0) // 从单片机读取重量，用户卡信息任务
		debug_print("thread 2 make fail\n");
	else
		debug_print("thread 2 make ok\n");

	pthread_join(thread[0], NULL);
	pthread_join(thread[1], NULL);
	pthread_join(thread[2], NULL);
}
