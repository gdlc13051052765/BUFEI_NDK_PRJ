#include <stdio.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>

#include <math.h>
#include "perDevice.h"


/*==================================================================================
* 函 数 名： open_close_led_device
* 参    数： num == led设备号;flag==true打开设备
* 功能描述:  打开关闭led
* 返 回 值：
* 备    注： 创建成功返回0
* 作    者： lc
* 创建时间： 2023-05-08
==================================================================================*/
int open_close_led_device(uint8_t num, bool flag)
{

    if(flag == true)
    {
        //return pca9535_set_gpio_value(buf[num], 1);
    } else {
        //return pca9535_set_gpio_value(buf[num], 0);
    }
    return 0;
}
