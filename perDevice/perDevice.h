#ifndef PERDEVICE_H
#define PERDEVICE_H

#include <stdio.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>

#define MAX_LED_NUM      4//LED个数

/*==================================================================================
* 函 数 名： open_close_led_device
* 参    数： num == led设备号;flag==true打开设备
* 功能描述:  打开关闭LED
* 返 回 值：
* 备    注： 创建成功返回0
* 作    者： lc
* 创建时间： 2021-05-25
==================================================================================*/
int open_close_led_device(uint8_t num, bool flag);

#endif 		//_SWITCH_CONFIG_H__