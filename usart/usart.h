#ifndef _GD32UARTAPI_H__
#define _GD32UARTAPI_H__

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

/*******************************************************************
* 名称： uart_init
* 功能： 串口初始化
* 入口参数： dev 设备号"/dev/ttyS0"; baud 波特率
* 出口参数： 正确返回为设备号，错误返回为-1
*******************************************************************/
int uart_init(char *dev, int baud);

/*******************************************************************
* 名称： uart_send
* 功能： 发送数据
* 入口参数： fd :文件描述符
* send_buf :存放串口发送数据
* data_len :一帧数据的个数
* 出口参数： 正确返回为1，错误返回为0
*******************************************************************/
int uart_send(int fd, char *send_buf, int data_len);

/*******************************************************************
* 名称： uart_recv
* 功能： 接收串口数据
* 入口参数： fd :文件描述符
* rcv_buf :接收串口中数据存入rcv_buf缓冲区中
* data_len :一帧数据的长度
* 出口参数： 正确返回接收数据长度len，错误返回为-1
*******************************************************************/
int uart_recv(int fd, char *rcv_buf, int data_len);

/*******************************************************************
* 名称： uart_close
* 功能： 关闭串口
* 入口参数： fd :文件描述符
*******************************************************************/
void uart_close(int fd);

#endif