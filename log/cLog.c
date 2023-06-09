// /** log.c **/
// #include "cLog.h"

// // log文件路径
// #define filepath "/data/meican/ps_com_log.log"

// //设定时间
// static char * settime(char * time_s)
// {
//     time_t timer=time(NULL);
//     strftime(time_s, 20, "%Y-%m-%d %H:%M:%S",localtime(&timer));
//     return time_s;
// }

// /*
// *打印
// * */
// static int PrintfLog(char * logText, char * string)
// {
//     FILE * fd = NULL;
//     char s[1024];
//     char tmp[256];

//     //使用追加方式打开文件
//     fd = fopen(filepath,"a+");
//     if(fd == NULL)
//     {
//         return -1;
//     }
//     memset(s, 0, sizeof(s));
//     memset(tmp, 0,sizeof(tmp));
//     sprintf(tmp, "*****[pid=%d]:[", getpid());
//     strcpy(s, tmp);
//     memset(tmp, 0,sizeof(tmp));
//     settime(tmp);
//     strcat(s, tmp);
//     strcat(s, "]*****");
//     fprintf(fd, "%s", s);
//     fprintf(fd, "*[%s]*****:\n",logText);
//     fprintf(fd, "%s\n",string);
//     fclose(fd);
//     return 0;
// }

// /*==================================================================================
// * 函 数 名： LogWrite
// * 参    数：
// * 功能描述:  日志写入
// * 返 回 值： 创建成功返回0
// * 备    注： LogWrite("INFO","Hello World!");
//             LogWrite("error","H.e.l.l.o W.o.r.l.d!");
//             LogWrite("mint","H e l l o W o r l d!");
//             LogWrite("iout","Hallo World!");
// * 作    者： lc
// * 创建时间： 2023/03/30
// ==================================================================================*/
// void LogWrite(char *logText,char *string)
// {
//     //[为支持多线程需要加锁] pthread_mutex_lock(&mutex_log); //lock.
//     //打印日志信息
//     PrintfLog(logText, string);
//     //[为支持多线程需要加锁] pthread_mutex_unlock(&mutex_log); //unlock.
// }

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>		// linux下头文件
#endif
 
#define FILE_MAX_SIZE (1024*1024)
 
/*
获得当前时间字符串
@param buffer [out]: 时间字符串
@return 空
*/
void get_local_time(char* buffer)
{
	time_t rawtime; 
	struct tm* timeinfo;
 
	time(&rawtime); 
	timeinfo = localtime(&rawtime); 
	sprintf(buffer, "%04d-%02d-%02d %02d:%02d:%02d = ", 
		(timeinfo->tm_year+1900), timeinfo->tm_mon, timeinfo->tm_mday,
		timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
}
 
/*
获得文件大小
@param filename [in]: 文件名
@return 文件大小
*/
long get_file_size(char* filename)
{
	long length = 0;
	FILE *fp = NULL;
 
	fp = fopen(filename, "rb");
	if (fp != NULL)
	{
		fseek(fp, 0, SEEK_END);
		length = ftell(fp);
	}
 
	if (fp != NULL)
	{
		fclose(fp);
		fp = NULL;
	}
 
	return length;
}
 
/*
写入日志文件
@param filename [in]: 日志文件名
@param max_size [in]: 日志文件大小限制
@param buffer [in]: 日志内容
@param buf_size [in]: 日志内容大小
@return 空
*/
void write_log_file(char* filename, long max_size, char* buffer, unsigned buf_size)
{
	if (filename != NULL && buffer != NULL)
	{
		// 文件超过最大限制, 删除
		long length = get_file_size(filename);
 
		if (length > max_size)
		{
			unlink(filename); // 删除文件
		}
 
		// 写日志
		{
			FILE *fp;
			fp = fopen(filename, "at+");
			if (fp != NULL)
			{
				char now[32];
				memset(now, 0, sizeof(now));
				get_local_time(now);
				fwrite(now, strlen(now)+1, 1, fp);
				fwrite(buffer, buf_size, 1, fp);
 
				fclose(fp);
				fp = NULL;
			}
		}
	}
}

/*==================================================================================
* 函 数 名： LogWrite
* 参    数： filename==log文件名字；buffer==写入内容
* 功能描述:  日志写入
* 返 回 值： 创建成功返回0
* 备    注： 
* 作    者： lc
* 创建时间： 2023/03/30
==================================================================================*/
void app_Log_Write(char* filename, char* buffer)
{
    write_log_file(filename, FILE_MAX_SIZE, buffer, strlen(buffer));
}

int write_log_test(void)
{
	int i;
	for (i=0; i<10; ++i)
	{
		char buffer[32];
		memset(buffer, 0, sizeof(buffer));
		sprintf(buffer, "====> %s\n", "write_log_test\n");
		app_Log_Write("log.txt",  buffer);
        usleep(500000);	// 500m秒
	}
	// system("pause");
	return 0;
}
