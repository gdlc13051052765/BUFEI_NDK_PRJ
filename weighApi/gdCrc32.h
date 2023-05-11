#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<termios.h>
#include<errno.h>
#include<string.h>
#include<stdint.h>


/*==================================================================================
* 函 数 名： CRC32Software
* 参    数： 查表法
* 功能描述:  软件实现STM32硬件CRC32
* 返 回 值： None
* 备    注： 
* 作    者： lc
* 创建时间： 2022-06-08 170658
==================================================================================*/  
uint32_t CRC32Software(uint8_t *pData, uint16_t Length);

int crc32_test(void);
