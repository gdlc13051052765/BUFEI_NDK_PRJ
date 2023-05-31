 
/*****************************************************************************************
 * 文件说明：
 * 调用串口，从GD32单片机获取重量以及读卡信息
 *****************************************************************************************/
#include <stdio.h>  
#include <stdlib.h>  
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "getWeighApi.h"
#include "../devConfig.h"
#include "../usart/usart.h"


//串口设备名字gd32串口
#define devname "/dev/ttyS1"
//串口文件句柄
int gd_usart_fd = -1;
//gd32固件目录
char *src_bin = "/root/bufei_app_01.mcu";  


void delay_ms(int i)
{
	usleep(i*1000);
}

/*==================================================================================
* 函 数 名： init_gd32_uart_api
* 参    数：* 入口参数： devName 设备号"/dev/ttyS0"; baud 波特率
* 功能描述:  初始化获取重量接口
* 返 回 值：
* 备    注： None
* 作    者： lc
* 创建时间： 2021/6/29
==================================================================================*/
int init_gd32_uart_api(char *devName,int baud )
{
    gd_usart_fd = usart_init(devName, baud);
    return gd_usart_fd;
}

/*==================================================================================
* 函 数 名： gd32_uart_send_data
* 参    数：
* 功能描述:  gd32串口数据发送
* 返 回 值：
* 备    注： None
* 作    者： lc
* 创建时间： 2021/6/29
==================================================================================*/
android_rcv_uart_Msg gd32_uart_send_data(char *send_buf, int data_len)
{
    int ret, i;
    android_rcv_uart_Msg pmsg={0};

    uart_send(gd_usart_fd, send_buf, data_len);
    ret = uart_recv(gd_usart_fd, pmsg.data , 100);   
    if(ret > 0) 
    {
        debug_print("rcv len  = %d, data =", ret);  
        for(i=0; i<ret; i++)
        {
            debug_print(" %02X", pmsg.data[i]);
        }
        debug_print("\n");
        pmsg.byte_count = ret;
        return pmsg;
    }
	return pmsg;
}

/*==================================================================================
* 函 数 名： file_read_bin_data
* 参    数： None
* 功能描述:  读取二进制文件
* 返 回 值： None
* 备    注： 
* 作    者： lc
* 创建时间： 2022-12-08 170658
==================================================================================*/ 
static int file_read_bin_data(char *file, u8 *buf, int size)  
{  
    FILE *fp;  
      
    if((fp=fopen(file, "rb")) == NULL) {  
        debug_print("Can not open the file: %s \n", file);  
        return -1;  
    }  
    fread(buf, sizeof(u8), size, fp);	
    fclose(fp); 
	return 0;	
} 

/*==================================================================================
* 函 数 名： file_get_bin_size
* 参    数： None
* 功能描述:  获取二进制文件的大小
* 返 回 值： None
* 备    注： 
* 作    者： lc
* 创建时间： 2022-12-08 170658
==================================================================================*/ 
static u32 file_get_bin_size(char *file)  
{     
    u32  size = 0;     
    FILE  *fp = fopen(file, "rb");     
    if (fp) {        
        fseek(fp, 0, SEEK_END);        
        size = ftell(fp);        
        fclose(fp);     
    }     
    return size;  
} 

/*==================================================================================
* 函 数 名： file_out_handle
* 参    数： None
* 功能描述:  生成新的二进制文件
* 返 回 值： None
* 备    注： 
* 作    者： lc
* 创建时间： 2022-12-08 170658
==================================================================================*/ 
int file_out_handle(char *out_file, u8 *buf, u32 size)  
{  
    FILE *fp; 
    int i,j,k,n;  
    int fd ;
    char pbuf[10]={0};  
    char mfgimage[4096*2];
 
    if((fp=fopen(out_file, "wa+"))==NULL) {  
        debug_print( "\nCan not open the path: %s \n", out_file);  
        return -1;  
    }  
    k=0; 
    for(i = 0; i < size; i++) {  
		k++;  
		sprintf(pbuf,"0x%02x", buf[i]);  
		fwrite(pbuf,strlen(pbuf),1,fp); 
		
		if(k != 16)  
			fwrite(", ", strlen(", "),1,fp);  
		else  
			fwrite(",", strlen(","),1,fp);
		
		if(k == 16) {  
			k=0;  
			fwrite("\n", strlen("\n"),1,fp);  
		}  
    }  
	fwrite("\n", strlen("\n"),1,fp);
    fclose(fp); 
	return 0;
}   

/*==================================================================================
* 函 数 名： send_data_to_gd32
* 参    数： None
* 功能描述:  发送数据到GD32
* 返 回 值： None
* 备    注： 
* 作    者： lc
* 创建时间： 2022-12-08 170658 
==================================================================================*/  
android_rcv_uart_Msg send_data_to_gd32(void* ret_msg)  
{
	_pUpdate_Send_Info pmsg = ret_msg;
	android_rcv_uart_Msg uart_msg;
	char buf[200];
	uint8_t len = 0,i;

	buf[len++] = 0x55;
	buf[len++] = 0xAA;
	buf[len++] = 0x14;
	buf[len++] = pmsg->len;
	buf[len++] = pmsg->slid;
	buf[len++] = pmsg->main_cmd;
	buf[len++] = pmsg->nodeID;
	buf[len++] = pmsg->slave_cmd;
	memcpy(buf+len, pmsg->data, pmsg->len-8);
	len += pmsg->len-8;
	pmsg->crc32 = CRC32Software(buf+2, len-2);
	//debug_print("crc32 = %8x \n", pmsg->crc32);
	memcpy(buf+len, &pmsg->crc32, 4);
	len += 4;

	debug_print("android 发送数据 = ");
	for(i=0; i<len; i++)
	{
		debug_print("%02x ",buf[i]);
	}
	debug_print("\n");
	uart_msg = gd32_uart_send_data(buf, len);
	return uart_msg;
}

/*==================================================================================
* 函 数 名： read_bin_data_send_to_gd32
* 参    数： None
* 功能描述:  从bin文件读取数据并发送数据到GD32
* 返 回 值： None
* 备    注： 
* 作    者： lc
* 创建时间： 2022-12-08 170658 
==================================================================================*/  
static int read_bin_data_send_to_gd32(void) 
{
	u8 *buf = NULL;  
	u32 size, i;  
	_Update_Send_Info pmsg;
 
	size = file_get_bin_size(src_bin);	
	buf = (u8 *)malloc(sizeof(u8) * size); 
	debug_print("固件大小 = %d\n", size/106);
	if(buf) 
	{	
		file_read_bin_data(src_bin, buf, size);			
		for(i=0; i<size/106; i++)
		{
			debug_print("包号 = %d\n", i);
			pmsg.len  = 0x72;
			pmsg.slid = 0x00;
			pmsg.main_cmd = WRITE_CMD;
			pmsg.nodeID = 0x00;
			if(i==0)
				pmsg.slave_cmd = ANDROID_CC_UPDATE_INFO; //信息包
			else
				pmsg.slave_cmd = ANDROID_CC_UPDATE_DATA; //升级包数据
			memcpy(pmsg.data, buf+i*UPDATE_DATA_LENS, UPDATE_DATA_LENS);
			send_data_to_gd32(&pmsg);
			delay_ms(10);
		}
		free(buf);
	}
	//下发确认升级
	pmsg.len  = 0x08;
	pmsg.slid = 0x00;
	pmsg.main_cmd = WRITE_CMD;
	pmsg.nodeID = 0x00;
	pmsg.slave_cmd = ANDROID_CC_UPDATE_CHECK;
	send_data_to_gd32(&pmsg);
	delay_ms(100);
	//下发重启命令
	pmsg.len  = 0x08;
	pmsg.slid = 0x00;
	pmsg.main_cmd = WRITE_CMD;
	pmsg.nodeID = 0x00;
	pmsg.slave_cmd = ANDROID_CC_UPDATE_RESET;
	send_data_to_gd32(&pmsg);
    return 0;  
}

/*==================================================================================
* 函 数 名： read_bin_test
* 参    数： None
* 功能描述:  读取二进制文件测试
* 返 回 值： None
* 备    注： 
* 作    者： lc
* 创建时间： 2022-12-08 170658
==================================================================================*/  
static int read_bin_test(void)  
{ 
	u8 *buf = NULL;  
	u32 size, i;  
 
	size = file_get_bin_size(src_bin);
	buf = (u8 *)malloc(sizeof(u8) * size); 
	if (buf) 
	{	
		file_read_bin_data(src_bin, buf, size);		
		for(i=0; i<size; i++)
		{
			debug_print("%02x ", buf[i]);  
			if(!((i+1)%106) & (i>10))
			{
				debug_print("\n");
			}
		}	
		free(buf);
	}	
    return 0;  
} 

/*==================================================================================
* 函 数 名： read_bufei_version
* 参    数： None
* 功能描述:  读取单片机板版本号
* 返 回 值： None
* 备    注： 
* 作    者： lc
* 创建时间： 2022-12-08 170658
==================================================================================*/   
android_rcv_uart_Msg read_bufei_version(void)  
{
	_Update_Send_Info pmsg;
	android_rcv_uart_Msg uart_msg= {0};

	pmsg.len  = 0x08;
	pmsg.slid = 0x00;
	pmsg.main_cmd = READ_CMD;
	pmsg.nodeID = 0x00;
	pmsg.slave_cmd = ANDROID_CC_GET_SW_AND_HW_VER;

	uart_msg = send_data_to_gd32(&pmsg);
	debug_print("称重板硬件版本 = %02X; 称重板软件版本 = %02X\n", uart_msg.data[6], uart_msg.data[7]);
	return uart_msg;
}

/*==================================================================================
* 函 数 名： read_bufei_weigh_uuid
* 参    数： None
* 功能描述:  读取单片机板当前重量跟卡uuid
* 返 回 值： None
* 备    注： 
* 作    者： lc
* 创建时间： 2022-12-08 170658
==================================================================================*/  
_Tag_Info read_bufei_weigh_uuid(void)  
{
	_Update_Send_Info pmsg;
	android_rcv_uart_Msg uart_msg={0};
    _Tag_Info tag_msg={0};

	pmsg.len  = 0x08;
	pmsg.slid = 0x00;
	pmsg.main_cmd = READ_CMD;
	pmsg.nodeID = 0x00;
	pmsg.slave_cmd = ANDROID_CC_GET_WEIGH_UID;

	uart_msg = send_data_to_gd32(&pmsg);
	tag_msg.toalNum = uart_msg.data[5];//卡片数量
	tag_msg.meicanCode = uart_msg.data[6];//美餐标识符

	memcpy(tag_msg.uid, uart_msg.data+7, TAG_UID_LENS);
	memcpy(&tag_msg.weighValue, uart_msg.data+7+TAG_UID_LENS, 4);

	return tag_msg;
}

/*==================================================================================
* 函 数 名： read_bufei_weigh
* 参    数： None
* 功能描述:  读取单片机板当前重量
* 返 回 值： None
* 备    注： 
* 作    者： lc
* 创建时间： 2022-12-08 170658
==================================================================================*/  
_Tag_Info read_bufei_weigh(void)  
{
	_Update_Send_Info pmsg;
	android_rcv_uart_Msg uart_msg={0};
    _Tag_Info tag_msg={0};

	pmsg.len  = 0x08;
	pmsg.slid = 0x00;
	pmsg.main_cmd = READ_CMD;
	pmsg.nodeID = 0x00;
	pmsg.slave_cmd = ANDROID_CC_GET_WEIGH;

	uart_msg = send_data_to_gd32(&pmsg);
	memcpy(&tag_msg.weighValue, uart_msg.data+6+TAG_UID_LENS, 4);

	return tag_msg;
}

/*==================================================================================
* 函 数 名： read_bufei_card_uid
* 参    数： None
* 功能描述:  读取单片机用户卡uid
* 返 回 值： None
* 备    注： 
* 作    者： lc
* 创建时间： 2022-12-08 170658
==================================================================================*/  
_Tag_Info read_bufei_card_uid(void)  
{
	_Update_Send_Info pmsg;
	android_rcv_uart_Msg uart_msg={0};
    _Tag_Info tag_msg={0};

	pmsg.len  = 0x08;
	pmsg.slid = 0x00;
	pmsg.main_cmd = READ_CMD;
	pmsg.nodeID = 0x00;
	pmsg.slave_cmd = ANDROID_CC_GET_CARD_UID;

	uart_msg = send_data_to_gd32(&pmsg);
	memcpy(tag_msg.uid, uart_msg.data+6, TAG_UID_LENS);

	return tag_msg;
}

// void weightest(void)
// {
//     //初始化串口
// 	init_gd32_uart_api(devname, 115200);

//     read_bufei_version();
//     while(1)
//     {
//         read_bufei_weigh_uuid();
//         usleep(100000);ss
//     }
// }