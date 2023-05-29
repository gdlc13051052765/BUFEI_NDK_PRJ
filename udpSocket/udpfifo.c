#include "udpfifo.h"
#include "udpSocket.h"
#include "../devConfig.h"
#include "../debug.h"

udp_send_queue_t	pudp_queue;//udp socket接收队列

void msg_udp_queue_init(p_udp_send_queue_t p_queue_buff)
{
  memset(p_queue_buff, 0, sizeof(udp_send_queue_t));
}

unsigned int msg_udp_queue_num(p_udp_send_queue_t p_queue_buff)
{
	return p_queue_buff->count;
}

static volatile unsigned char queue_mutex=0;
void msg_udp_queue_push(p_udp_send_queue_t p_queue_buff, _udp_Msg *data)
{
	if(queue_mutex == 0)
	{
		queue_mutex = 1;
		if (p_queue_buff->count >= MAX_UDP_CACHE_NUM)
		{
			memset(&p_queue_buff->queue[p_queue_buff->rd], 0, sizeof(_udp_Msg));
			p_queue_buff->rd = (p_queue_buff->rd + 1) % MAX_UDP_CACHE_NUM;
			//log_printf("Delete the oldest member \r\n");
		} else {
			p_queue_buff->count++;
			if(p_queue_buff->count>MAX_UDP_CACHE_NUM)
			{
				
			}
		}
		memcpy(&p_queue_buff->queue[p_queue_buff->wp], data, sizeof(_udp_Msg));

		p_queue_buff->wp++;
		if(p_queue_buff->wp >= MAX_UDP_CACHE_NUM)
			p_queue_buff->wp = 0;
		queue_mutex = 0;
	}
	else
	{
	//log_printf("queue is locked by others.\r\n");
	}
}

void msg_udp_queue_pop(p_udp_send_queue_t p_queue_buff, uint8_t msg_id)
{
	memset(&p_queue_buff->queue[p_queue_buff->rd], 0, sizeof(_udp_Msg));
	p_queue_buff->count--;
	p_queue_buff->rd++;
	if(p_queue_buff->rd >= MAX_UDP_CACHE_NUM)
		p_queue_buff->rd = 0;
}

/*==================================================================================
* 函 数 名： udp_fifo_instance_init
* 参    数： None
* 功能描述:  udp接收队列初始化
* 返 回 值： None
* 备    注： None
* 作    者：  
* 创建时间： 2019-10-28 170617
=================================================================================*/
void udp_fifo_instance_init(void)
{
	msg_udp_queue_init(&pudp_queue);
}

/*==================================================================================
* 函 数 名： udp_fifo_push_data_msg
* 参    数： None
* 功能描述:  主控T1113 接收到数据后调用此函数把接收的数据添加的队列里面
* 返 回 值： None
* 备    注： 
* 作    者： lc
* 创建时间： 2021-09-29 170658
==================================================================================*/
void udp_fifo_push_data_msg(void* ret_msg)
{
	_pUdp_Msg pmsg = ret_msg;  
	
	msg_udp_queue_push(&pudp_queue, pmsg);
}

/*==================================================================================
* 函 数 名： udp_fifo_pop_one_frame
* 参    数： _pRet_Msg
* 功能描述:  UDP socket数据接收
* 返 回 值： None
* 备    注： None
* 作    者： lc
* 创建时间： 2021-06-20 154449
==================================================================================*/ 
uint8_t udp_fifo_pop_one_frame(void )
{
    int num;
    p_udp_send_queue_t p_queue_buff = &pudp_queue;

    #ifdef UDP_FIFO_MODE
        //检查队列里面是否有数据
        num = msg_udp_queue_num(p_queue_buff);
        if(num)
        {
            for(int i=0; i<MAX_UDP_CACHE_LEN; i++)
            {
                if(p_queue_buff->queue[i].byte_count)
                {
                    debug_print("队列里面的消息长度 = %d \r\n",p_queue_buff->queue[i].byte_count);
                    udpsocket_data_processing(&(p_queue_buff->queue[i].data));
                    msg_udp_queue_pop(p_queue_buff, 0);
                    return 0;
                }	
            }
            //debug_print("队列里面的消息个数 =%d \n",num);
        }
       
    #else

    #endif

    return 0xff;
}

