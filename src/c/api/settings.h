#ifndef __SETTINGS_H__
#define __SETTINGS_H__
#include <stdint.h>
#include <pebble.h>
#include "communication.h"

typedef struct {

    // boolean settings

    // enable seconds
    uint32_t enable_seconds : 1;

    // trend type to use
    uint32_t use_png : 1;

    // show fields
    uint32_t show_trend : 1;
    uint32_t show_delta : 1;
    uint32_t show_slope : 1;
    uint32_t show_message : 1;

    // other settings
    uint32_t vibrate_repeat : 1;
    uint32_t vibrate_off : 1;
    uint32_t backlight_on_charge : 1;
    uint32_t collect_health : 1;

    // UX
    uint32_t fields_same_colour : 1;
    uint32_t bold_timeago : 1;


    // Values
    GColor foreground_colour;
    GColor background_colour;

    uint32_t message_timeout;
    uint8_t left_text_field;
    uint8_t right_text_field;

    // Callbacks
    comm_callback *comm_callbacks;
    void (*update_colours)(void);
    void (*update_seconds_timer)(bool swap);
    void (*update_message_timeout)(uint32_t timeout);
    void (*update_timeago)(void);
    void (*update_left_field)(void);
    void (*update_right_field)(void);
    void (*update_trend)(void);
    void (*update_collect_health)(void);
    
} globals;

void settings_init(globals *values);
void settings_handle(Tuple *data);
#endif // __SETTINGS_H__
