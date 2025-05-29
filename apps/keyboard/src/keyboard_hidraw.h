#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "pgs_modules.h"

extern pthread_mutex_t hidraw_mutex;
extern chry_ringbuffer_t hidraw_rxrb;

enum escreen_report_ids {
    /* Reserved */
    ESCREEN_REPORT_ID_RESERVED,

    /* Keyboard input report. */
    ESCREEN_REPORT_ID_KEYBOARD,
    /* Mouse input report. */
    ESCREEN_REPORT_ID_MOUSE,
    /* System control input report. */
    ESCREEN_REPORT_ID_SYSTEM_CTRL,
    /* Consumer control input report. */
    ESCREEN_REPORT_ID_CONSUMER_CTRL,
    /* Programmable button input report. */
    ESCREEN_REPORT_ID_PROGRAMMABLE_BUTTON,
    /* Keyboard NKRO input report. */
    ESCREEN_REPORT_ID_NKRO,
    /* Joystick input report. */
    ESCREEN_REPORT_ID_JOYSTICK,
    /* Digitizer input report. */
    ESCREEN_REPORT_ID_DIGITIZER,
    /* Dial input report. */
    ESCREEN_REPORT_ID_DIAL,
    /* Keyboard output report. */
    ESCREEN_REPORT_ID_KEYBOARD_LEDS,

    /* via input report */
    ESCREEN_REPORT_ID_VIA_INPUT,
    /* via input output */
    ESCREEN_REPORT_ID_VIA_OUTPUT,

    /* escreen input report */
    ESCREEN_REPORT_ID_ESCREEN_INPUT,
    /* escreen output report */
    ESCREEN_REPORT_ID_ESCREEN_OUTPUT,

    /* escreen monitor report */
    ESCREEN_REPORT_ID_MONITOR_INPUT,
};

enum escreen_command_id {
    id_scr_get_protocol_version = 0x01,

    id_scr_get_macro  = 0x20,
    id_scr_get_layer  = 0x21,
    id_scr_get_led    = 0x22,
    id_scr_get_output = 0x23,
    id_scr_get_bat    = 0x24,
    id_scr_get_wpm    = 0x25,
    id_scr_get_sleep  = 0x26,

    id_scr_set_record = 0x80,

};

typedef union
{
    uint8_t raw;
    struct
    {
        bool num_lock : 1;
        bool caps_lock : 1;
        bool scroll_lock : 1;
        bool compose : 1;
        bool kana : 1;
        uint8_t reserved : 3;
    };
} keyboard_led_t;

void keyboard_hidraw_init(void);
