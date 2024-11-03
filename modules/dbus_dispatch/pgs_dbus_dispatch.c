#include "pgs_modules.h"

#include <stdio.h>
#include <signal.h>
#include <pthread.h>
#include <dbus/dbus.h>
#include "pgs_dbus_dispatch.h"

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static int state             = 1;

static void set_lvgl_foreground(int states)
{
    state = states;
}

static void set_lvgl_suspend(void)
{
    pthread_mutex_lock(&mutex);
}

static void set_lvgl_resume(void)
{
    pthread_mutex_unlock(&mutex);
}

int pgs_get_lvgl_foreground(void)
{
    return state;
}

void pgs_wait_become_foreground(void)
{
    pthread_mutex_lock(&mutex);
    pthread_mutex_unlock(&mutex);
}

void pgs_dispatch_dbus_message(int data)
{
    if(0 == data) {
        set_lvgl_suspend();
        set_lvgl_foreground(0); /* Background */
    } else if(1 == data) {
        set_lvgl_foreground(1); /* Foreground */
        set_lvgl_resume();
    }
}
