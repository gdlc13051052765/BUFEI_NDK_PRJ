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
#include<stdbool.h>
#include "../debug.h"

//文件号
//int fd = false; 
void uart_close(int fd);
int uart_recv(int fd, char *rcv_buf, int data_len);
int uart_send(int fd, char *send_buf, int data_len);


/*****************************************************************
* 名称： UART0_Open
* 功能： 打开串口并返回串口设备文件描述
* 入口参数： fd :文件描述符 port :串口号(ttyS0,ttyS1,ttyS2)
* 出口参数： 正确返回为1，错误返回为0
*****************************************************************/
static int UART_Open(int fd, char* port)
{
    struct termios options;

    fd = open( port, O_RDWR|O_NOCTTY|O_NDELAY);
    if (false == fd){
        perror("Can't Open Serial Port");
        return(false);
    }
    /*问题:在Linux应用层，直接从/dev/tty***使用read()函数读数据，无法读到，只有在数据末尾加上0a/0d才可以读到数据
    (这里是发送十六进制的数据，ASCLL码同理，增加回车才可以读到数据)
    原因:在linux内核中增加了行缓存的机制，必须出现一个结束符read函数才能从缓冲区里读出数据 */
    // tcgetattr(fd, &options);                 //获得属性
    // options.c_lflag &= ~(ICANON | ECHO | ECHOE);
    // tcsetattr(fd, TCSANOW, &options);        //设置属性
    tcgetattr(fd, &options);

    options.c_cflag |= (CLOCAL | CREAD); //disenable dcd, start read func
    options.c_cflag &= ~CRTSCTS; //close hard stream control
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8; //set data bit 8 (8N1)
    options.c_cflag &= ~(PARENB | CSTOPB); //no use parity check bit and stop bit 1
    options.c_cflag &= ~(ICRNL | IXON);
    options.c_oflag = 0;
    options.c_lflag = 0;
    options.c_iflag &= ~(BRKINT | INPCK | ISTRIP | ICRNL | IXON);
    tcsetattr(fd, TCSANOW, &options);

    //判断串口的状态是否为阻塞状态
    if(fcntl(fd, F_SETFL, 0) < 0){
        debug_print("fcntl failed!\n");
        return(false);
    } else {
        //    debug_print("fcntl=%d\n",fcntl(fd, F_SETFL,0));
    }

    //测试是否为终端设备
    if(0 == isatty(STDIN_FILENO)){
        debug_print("standard input is not a terminal device\n");
        return(false);
    }

    return fd;
}

/*******************************************************************
* 名称： uart_close
* 功能： 关闭串口
* 入口参数： fd :文件描述符
*******************************************************************/
void uart_close(int fd)
{
    close(fd);
}

/*******************************************************************
* 名称： UART0_Set
* 功能： 设置串口数据位，停止位和效验位
* 入口参数： fd 串口文件描述符
* speed 串口速度
* flow_ctrl 数据流控制
* databits 数据位 取值为 7 或者8
* stopbits 停止位 取值为 1 或者2
* parity 效验类型 取值为N,E,O,,S
*出口参数： 正确返回为1，错误返回为0
*******************************************************************/
static int UART_Set(int fd, int speed, int flow_ctrl, int databits, int stopbits, int parity)
{  
    int i;
    int speed_arr[] = { B115200, B57600, B38400, B19200, B9600, B4800, B2400, B1200, B300,
        B38400, B19200, B9600, B4800, B2400, B1200, B300
    };
    int name_arr[] = { 115200, 57600, 38400, 19200, 9600, 4800, 2400, 1200, 300, 38400,
        19200, 9600, 4800, 2400, 1200, 300
    };
    struct termios options;

    /*tcgetattr(fd,&options)得到与fd指向对象的相关参数，并将它们保存于options,该函数,还可以测试配置是否正确，该串口是否可用等。若调用成功，函数返回值为0，若调用失败，函数返回值为1.*/
    if(tcgetattr( fd,&options) != 0){
        perror("SetupSerial 1");
        return(false);
    }  
    //设置串口输入波特率和输出波特率
    for(i= 0;i < sizeof(speed_arr) / sizeof(int);i++) {     
        if (speed == name_arr[i]) {
            cfsetispeed(&options, speed_arr[i]);
            cfsetospeed(&options, speed_arr[i]);
        }
    }    
    //修改控制模式，保证程序不会占用串口        
    options.c_cflag |= CLOCAL;
    //修改控制模式，使得能够从串口中读取输入数据
    options.c_cflag |= CREAD;
    //设置数据流控制
    switch(flow_ctrl) {
        case 0 : //不使用流控制
            options.c_cflag &= ~CRTSCTS;
        break;    
        case 1 : //使用硬件流控制
            options.c_cflag |= CRTSCTS;
        break;
        case 2 : //使用软件流控制
            options.c_cflag |= IXON | IXOFF | IXANY;
        break;
    }
    //设置数据位
    options.c_cflag &= ~CSIZE; //屏蔽其他标志位
    switch (databits){
        case 5 :
            options.c_cflag |= CS5;
        break;
        case 6    :
            options.c_cflag |= CS6;
        break;
        case 7    :
            options.c_cflag |= CS7;
        break;
        case 8:
            options.c_cflag |= CS8;
        break;
        default:
            fprintf(stderr,"Unsupported data size\n");
        return (false);
    }
    //设置校验位
    switch (parity) {
        case 'n':
        case 'N': //无奇偶校验位。
            options.c_cflag &= ~PARENB;
            options.c_iflag &= ~INPCK;
        break;
        case 'o':
        case 'O': //设置为奇校验
            options.c_cflag |= (PARODD | PARENB);
            options.c_iflag |= INPCK;
        break;
        case 'e':
        case 'E': //设置为偶校验
            options.c_cflag |= PARENB;
            options.c_cflag &= ~PARODD;
            options.c_iflag |= INPCK;
        break;
        case 's':
        case 'S': //设置为空格
            options.c_cflag &= ~PARENB;
            options.c_cflag &= ~CSTOPB;
        break;
        default:
            fprintf(stderr,"Unsupported parity\n");
            return (false);
    }
    // 设置停止位
    switch (stopbits){
        case 1:
            options.c_cflag &= ~CSTOPB;
        break;
        case 2:
            options.c_cflag |= CSTOPB;
        break;
        default:
            fprintf(stderr,"Unsupported stop bits\n");
            return (false);
    }
    //修改输出模式，原始数据输出
    options.c_oflag &= ~OPOST;
    //设置等待时间和最小接收字符
    options.c_cc[VTIME] = 1; /* 读取一个字符等待1*(1/10)s */
    options.c_cc[VMIN] = 1; /* 读取字符的最少个数为1 */
   
    //如果发生数据溢出，接收数据，但是不再读取
    tcflush(fd,TCIFLUSH);
   
    //激活配置 (将修改后的termios数据设置到串口中）
    if (tcsetattr(fd,TCSANOW,&options) != 0)
    {
        perror("com set error!/n");
        return (false);
    }
    return (true);
}

static int UART_Init(int fd, int speed, int flow_ctrlint, int databits, int stopbits, char parity)
{
    //设置串口数据帧格式
    if (false == UART_Set(fd,speed,flow_ctrlint,databits,stopbits,parity)) {         
        return false;
    } else {
        return true;
    }
}

/*******************************************************************
* 名称： uart_init
* 功能： 串口初始化
* 入口参数： dev 设备号"/dev/ttyS0"; baud 波特率
* 出口参数： 正确返回为设备号，错误返回为-1
*******************************************************************/
int uart_init(char *dev, int baud)
{
    int fd = false; 
    int ret;

    fd = UART_Open(fd, dev);
    if(false == fd) {    
        debug_print("open error\n");    
        return false;
        //exit(1);     
    }
    ret = UART_Init(fd,baud,0,8,1,'N');
    if(false == fd) {    
        debug_print("Set Port Error\n");    
        return false;
        //exit(1);
    }
    return fd;
}

/*******************************************************************
* 名称： uart_recv
* 功能： 接收串口数据
* 入口参数： fd :文件描述符
* rcv_buf :接收串口中数据存入rcv_buf缓冲区中
* data_len :一帧数据的长度
* 出口参数： 正确返回接收数据长度len，错误返回为-1
*******************************************************************/
int uart_recv(int fd, char *rcv_buf, int data_len)
{
    int len,fs_sel;
    fd_set fs_read;
    struct timeval time;
    
    FD_ZERO(&fs_read);
    FD_SET(fd,&fs_read);
    
    time.tv_sec = 1;//超时时间1S
    time.tv_usec = 0;
    
    //使用select实现串口的多路通信
    fs_sel = select(fd+1,&fs_read,NULL,NULL,&time);
    if(fs_sel) {
        len = read(fd,rcv_buf,data_len);    
        return len;
    } else {
        return false;
    }    
}

/*******************************************************************
* 名称： uart_send
* 功能： 发送数据
* 入口参数： fd :文件描述符
* send_buf :存放串口发送数据
* data_len :一帧数据的个数
* 出口参数： 正确返回为1，错误返回为0
*******************************************************************/
int uart_send(int fd, char *send_buf, int data_len)
{
    int ret;
    
    ret = write(fd,send_buf,data_len);
    if (data_len == ret ) {    
        return ret;
    } else {
        tcflush(fd,TCOFLUSH);
        return false;  
    } 
}


