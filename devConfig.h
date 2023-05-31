/*****************************************************************************************
 * 文件说明：
 * 工程的基础参数配置文件
 *****************************************************************************************/
#ifndef _DEVCONFIG_H__
#define _DEVCONFIG_H__

#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

//布菲台硬件管理模块程序配置
#define UDP_FIFO_MODE //udp fifo 队列缓存模式

//管理模块版本
#define APP_HARD_VER    "wr_01"
#define APP_SOFT_VER    "v0.0.0"


/**
 * 简单打印调试信息
 */
#define DEBUG        //打印开关
#ifdef DEBUG 
    #define debug_print(format, args...)  fprintf(stderr, format, ##args)
#else
    #define debug_print(fmt, args...)
#endif

//error code defines
typedef enum {
		SUCCESS = 0,
		FAIL,	
	
}error_code;

#endif