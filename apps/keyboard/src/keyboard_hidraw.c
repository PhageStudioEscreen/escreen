#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/hidraw.h>
#include <sys/ioctl.h>
#include <string.h>
#include <errno.h>
#include <poll.h>
#include "keyboard_hidraw.h"

pthread_mutex_t hidraw_mutex = PTHREAD_MUTEX_INITIALIZER;
chry_ringbuffer_t hidraw_rxrb;
static uint8_t keyboard_hidraw_rx_mempool[4096];

static void * write_thread(void * arg)
{
    int fd;
    const char * path = "/dev/hidraw0";
    struct hidraw_devinfo info;
    uint8_t buff[2];

    fd = open(path, O_WRONLY);
    if(fd < 0) {
        printf("write open %s failed\n", path);
        return NULL;
    }

    if(ioctl(fd, HIDIOCGRAWINFO, &info) < 0) {
        printf("write get %s info failed\n", path);
        close(fd);
        return NULL;
    }

    printf("write %s vendor=0x%04hx, product=0x%04hx\n", path, info.vendor, info.product);

    buff[0] = ESCREEN_REPORT_ID_ESCREEN_OUTPUT;
    buff[1] = id_scr_get_protocol_version;

    if(sizeof(buff) != write(fd, buff, sizeof(buff))) {
        printf("write %d %s failed %d\n", sizeof(buff), path, errno);
    } else {
        printf("write %d %s success\n", sizeof(buff), path);
    }

    usleep(100 * 1000);

    buff[1] = id_scr_get_macro;
    write(fd, buff, sizeof(buff));
    usleep(100 * 1000);

    buff[1] = id_scr_get_layer;
    write(fd, buff, sizeof(buff));
    usleep(100 * 1000);

    buff[1] = id_scr_get_led;
    write(fd, buff, sizeof(buff));
    usleep(100 * 1000);

    buff[1] = id_scr_get_output;
    write(fd, buff, sizeof(buff));
    usleep(100 * 1000);

    buff[1] = id_scr_get_bat;
    write(fd, buff, sizeof(buff));
    usleep(100 * 1000);

    buff[1] = id_scr_get_wpm;
    write(fd, buff, sizeof(buff));
    usleep(100 * 1000);

    buff[1] = id_scr_get_sleep;
    write(fd, buff, sizeof(buff));
    usleep(100 * 1000);

    close(fd);

    return NULL;
}

static void * poll_thread(void * arg)
{
    int fd;
    int ret;
    const char * path = "/dev/hidraw0";
    struct hidraw_devinfo info;
    unsigned char buf[512];
    struct pollfd fds[1];

    fd = open(path, O_RDONLY);
    if(fd < 0) {
        printf("poll open %s failed\n", path);
        return NULL;
    }

    if(ioctl(fd, HIDIOCGRAWINFO, &info) < 0) {
        printf("poll get %s info failed\n", path);
        close(fd);
        return NULL;
    }

    printf("poll %s vendor=0x%04hx, product=0x%04hx\n", path, info.vendor, info.product);

    fds[0].fd     = fd;
    fds[0].events = POLLIN;

    while(1) {
        ret = poll(fds, 1, -1);
        if(ret < 0) {
            printf("poll %s failed\n", path);
            break;
        }

        if(fds[0].revents & POLLIN) {
            ssize_t len = read(fd, buf, sizeof(buf));
            if(len < 0) {
                printf("poll read %s failed\n", path);
                continue;
            } else if(len == 0) {
                continue;
            }

            // for(uint8_t i = 0; i < len; i++) {
            //     printf("%02x ", buf[i]);
            // }
            // printf("\n");

            pthread_mutex_lock(&hidraw_mutex);
            chry_ringbuffer_write(&hidraw_rxrb, buf, len);
            pthread_mutex_unlock(&hidraw_mutex);
        }
    }

    return NULL;
}

void keyboard_hidraw_init(void)
{
    pthread_t tid;
    pthread_t tid2;

    chry_ringbuffer_init(&hidraw_rxrb, keyboard_hidraw_rx_mempool, sizeof(keyboard_hidraw_rx_mempool));

    pthread_create(&tid, NULL, poll_thread, NULL);

    pthread_create(&tid2, NULL, write_thread, NULL);
}
