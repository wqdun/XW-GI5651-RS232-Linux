#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>
#include <error.h>
#include <time.h>

using std::cout;
using std::endl;

int set_opt(int fd, int nSpeed, int nBits, char nEvent, int nStop) {
    struct termios newtio, oldtio;
    if(tcgetattr(fd, &oldtio) != 0) {
        perror("Setup Serial 1.");
        return -1;
    }
    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag |= (CLOCAL | CREAD);
    newtio.c_cflag &= ~CSIZE;

    switch(nBits) {
    case 7:
        newtio.c_cflag |= CS7;
        break;
    case 8:
        newtio.c_cflag |= CS8;
        break;
    }

    switch(nEvent) {
    case 'O':
        newtio.c_cflag |= PARENB;
        newtio.c_cflag |= PARODD;
        newtio.c_iflag |= (INPCK | ISTRIP);
        break;
    case 'E':
        newtio.c_iflag |= (INPCK | ISTRIP);
        newtio.c_cflag |= PARENB;
        newtio.c_cflag &= ~PARODD;
        break;
    case 'N':
        newtio.c_cflag &= ~PARENB;
        break;
    }

    switch(nSpeed) {
    case 2400:
        cfsetispeed(&newtio, B2400);
        cfsetospeed(&newtio, B2400);
        break;
    case 4800:
        cfsetispeed(&newtio, B4800);
        cfsetospeed(&newtio, B4800);
        break;
    case 9600:
        cfsetispeed(&newtio, B9600);
        cfsetospeed(&newtio, B9600);
        break;
    case 115200:
        cfsetispeed(&newtio, B115200);
        cfsetospeed(&newtio, B115200);
        break;
    case 460800:
        cfsetispeed(&newtio, B460800);
        cfsetospeed(&newtio, B460800);
        break;
    default:
        cfsetispeed(&newtio, B9600);
        cfsetospeed(&newtio, B9600);
        break;
    }

    if(1 == nStop) {
        newtio.c_cflag &= ~CSTOPB;
    }
    else
    if(2 == nStop) {
        newtio.c_cflag |= CSTOPB;
    }
    else {
        perror("Setup nStop unavailable.");
        return -1;
    }

    tcflush(fd, TCIFLUSH);

    newtio.c_cc[VTIME] = 100; // time out 15s重要
    newtio.c_cc[VMIN] = 0; // Update the option and do it now 返回的最小值  重要

    if(0 != tcsetattr(fd, TCSANOW, &newtio)) {
        perror("Com setup error.");
        return -1;
    }

    // printf("set done!\n\r");
    return 0;
}

int main(void) {
    char buf[1024];

    // open serial port: ttyS0
    int fd1 = open("/dev/ttyS0", O_RDONLY | O_NONBLOCK); // 打开串口 // fd1 = open("/dev/ttyUSB0", O_RDWR);
    if(-1 == fd1) {
        printf("fd1 == -1\n");
        exit 1;
    }

    // setup port properties
    int nset1 = set_opt(fd1, 115200, 8, 'N', 1); // 设置串口属性
    if(-1 == nset1) {
        exit 1;
    }

    std::string frameBuf;
    long freq = 0;
    timespec t1, t2;
    long last_time_ns;
    while(1) {
        memset(buf, 0, 1024);
        int nread = read(fd1, buf, 1024); // 读串口
        if(nread <= 0) {
            continue;
        }

        for(int i = 0; i < nread; ++i) {
            switch(buf[i]) {
            case '$':
                clock_gettime(CLOCK_MONOTONIC, &t2);
                // cout << "time_end  :" << t2.tv_nsec << endl;
                freq = 1000000000 / (t2.tv_nsec - last_time_ns);
                cout << "The Freq is: " << freq << endl;
                frameBuf = buf[i];

                clock_gettime(CLOCK_MONOTONIC, &t1);
                // cout << "time_start:" << t1.tv_nsec << endl;
                last_time_ns = t1.tv_nsec;
                break;
            case '\n':
                cout << frameBuf << endl;
                break;
            default:
                frameBuf += buf[i];
            }
        }
    }


    close(fd1);
    return 0;
}
