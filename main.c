#include <stdio.h>  
#include <fcntl.h>  
#include <linux/ioctl.h>
#include "cAppTask/cAppTask.h"
#include "sqlite/sqliteTask.h"
#include "log/cLog.h"


int main(void)
{
    //创建基础配置数据库
    // sqlite_create_config_db();
   // write_log_test();
    //创建C进程任务
    app_bufei_create_thread();
}
