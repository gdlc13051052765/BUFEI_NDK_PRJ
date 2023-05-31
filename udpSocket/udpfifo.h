#ifndef __PUDPFIFO_H
#define __PUDPFIFO_H

#include <stdint.h>
#include <string.h>


#define MAX_UDP_CACHE_LEN					1024	//UDP发送缓存
#define MAX_UDP_CACHE_NUM				  10	//最大缓存池


typedef struct 
{ 
	uint8_t byte_count;
	char  data[MAX_UDP_CACHE_LEN];
}_udp_Msg, *_pUdp_Msg;


typedef struct udp_send_queue {
	_udp_Msg queue[MAX_UDP_CACHE_NUM];
	char wp;
	char rd;
	char count;
}udp_send_queue_t, * p_udp_send_queue_t;

void msg_udp_queue_pop(p_udp_send_queue_t p_queue_buff, uint8_t msg_id);
/*==================================================================================
* 函 数 名： udp_fifo_instance_init
* 参    数： None
* 功能描述:  udp接收队列初始化
* 返 回 值： None
* 备    注： None
* 作    者：  
* 创建时间： 2019-10-28 170617
=================================================================================*/
void udp_fifo_instance_init(void);

/*==================================================================================
* 函 数 名： udp_fifo_push_data_msg
* 参    数： None
* 功能描述:  主控T1113 接收到数据后调用此函数把接收的数据添加的队列里面
* 返 回 值： None
* 备    注： 
* 作    者： lc
* 创建时间： 2021-09-29 170658
==================================================================================*/
void udp_fifo_push_data_msg(void* ret_msg);

/*==================================================================================
* 函 数 名： udp_fifo_pop_one_frame
* 参    数： _pRet_Msg
* 功能描述:  UDP socket数据接收
* 返 回 值： None
* 备    注： None
* 作    者： lc
* 创建时间： 2021-06-20 154449
==================================================================================*/ 
uint8_t udp_fifo_pop_one_frame(void );

#endif
