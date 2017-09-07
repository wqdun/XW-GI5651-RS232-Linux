#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/signal.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>

#define FALSE -1
#define TRUE 0
#define flag 1
#define noflag 0

int wait_flag = noflag;
int STOP = 0;
int res;

int speed_arr[] = {
    B38400, B19200, B9600, B4800, B2400, B1200, B300, B38400, B19200, B9600, B4800, B2400, B1200, B300
};
int name_arr[] = {
    38400, 19200, 9600, 4800, 2400, 1200, 300, 38400, 19200, 9600, 4800, 2400, 1200, 300
};

void set_speed(int fd, int speed) {
    int status;
    struct termios Opt;
    tcgetattr(fd, &Opt);
    for(int i = 0; i < sizeof(speed_arr) / sizeof(speed_arr[0]); ++i) {
        if(speed != name_arr[i]) {
            continue;
        }

        tcflush(fd, TCIOFLUSH);
        cfsetispeed(&Opt, speed_arr[i]);
        cfsetospeed(&Opt, speed_arr[i]);
        status = tcsetattr(fd, TCSANOW, &Opt);
        if(0 != status) {
            perror("Com set speed error.");
            return;
        }
        tcflush(fd, TCIOFLUSH);
    }
}

int set_parity(int fd, int databits, int stopbits, int parity) {
    struct termios options;
    if(0 != tcgetattr(fd, &options)) {
        perror("Setup Serial 1.");
        return FALSE;
    }
    options.c_cflag &= ~CSIZE;

    switch(databits) {
    case 7:
        options.c_cflag |= CS7;
        break;
    case 8:
        options.c_cflag |= CS8;
        break;
    default:
        fprintf(stderr, "Unsupported data size.\n");
        return FALSE;
    }

    switch(parity) {
    case 'n':
    case 'N':
        options.c_cflag &= ~PARENB; /* Clear parity enable */
        options.c_iflag &= ~INPCK;  /* Enable parity checking */
        break;
    case 'o':
    case 'O':
        options.c_cflag |= (PARODD | PARENB);
        options.c_iflag |= INPCK;   /* Disnable parity checking */
        break;
    case 'e':
    case 'E':
        options.c_cflag |= PARENB;  /* Enable parity */
        options.c_cflag &= ~PARODD;
        options.c_iflag |= INPCK;   /* Disnable parity checking */
        break;
    case 'S':
    case 's':                       /* as no parity */
        options.c_cflag &= ~PARENB;
        options.c_cflag &= ~CSTOPB;
        options.c_iflag |= INPCK;
        break;
    default:
        fprintf(stderr, "Unsupported parity.\n");
        return FALSE;
    }

    switch(stopbits) {
    case 1:
        options.c_cflag &= ~CSTOPB;
        break;
    case 2:
        options.c_cflag |= CSTOPB;
        break;
    default:
        fprintf(stderr, "Unsupported stop bits.\n");
        return FALSE;
    }

    tcflush(fd, TCIFLUSH);
    options.c_cc[VTIME] = 150;
    options.c_cc[VMIN] = 0;         /* Update the options and do it NOW */
    if(0 != tcsetattr (fd, TCSANOW, &options)) {
        perror("Com set parity error.");
        return FALSE;
    }
    return TRUE;
}

void signal_handler_IO(int status) {
    printf("Received SIGIO signal.\n");
    wait_flag = noflag;
}

int main(){
    printf("This program updates last time at %s   %s\n", __TIME__, __DATE__);
    printf("STDIO COM1\n");
    int fd;
    struct sigaction saio;

    fd = open("/dev/ttyS0", O_RDONLY | O_NONBLOCK);
    if(-1 == fd) {
        perror("Failed to open serial port.\n");
        exit 1;
    }
    printf("Open ");
    printf("%s", ttyname(fd));
    printf("successfully.\n");

    saio.sa_handler = signal_handler_IO;
    sigemptyset(&saio.sa_mask);
    saio.sa_flags = 0;
    saio.sa_restorer = NULL;
    sigaction(SIGIO, &saio, NULL);

    //allow the process to receive SIGIO
    fcntl(fd, F_SETOWN, getpid());
    //make the file descriptor asynchronous
    fcntl(fd, F_SETFL, FASYNC);

    set_speed(fd, 115200);
    if(FALSE == set_parity(fd, 8, 1, 'N')) {
        printf("Failed to set parity error.\n");
        exit 1;
    }

    char buf[1024];
    while(0 == STOP) {
        usleep(100000); // ms
        // after receiving SIGIO, wait_flag = FALSE, input is available and can be read
        if(0 != wait_flag) {
            continue;
        }

        memset(buf, 0, sizeof(buf));
        res = read(fd, buf, 1024);
        if(res <= 0) {
            cout << "Failed to read Com port." << endl;
            continue;
        }
        printf("Got %d char:\"%s\"\n", res, buf);
        // if(1 == res)
            // STOP = 1;        /* stop loop if only a CR was input */
        wait_flag = flag;       /* wait for new input */
    }

    close(fd);
    return 0;
}
