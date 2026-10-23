#ifndef __CALLBACKS_H__
#define __CALLBACKS_H__
#include <pebble.h>
#include "communication.h"

typedef struct {
    void (*set_message)(char *message, size_t length);
    void (*set_icon)(uint8_t icon);
    void (*set_delta)(char *message, size_t length);
    void (*set_delta_colour)(GColor colour);
    void (*set_cgmtime)(char *message, size_t length);
    void (*set_png)(comm_png_data *data);
    void (*seconds_tick)(struct tm* tick_time_cgm, TimeUnits units_changed);
    void (*minutes_tick)(struct tm *tick_time_cgm, TimeUnits units_changed);
    void (*update_battery_state)(void);
    void (*update_phone_battery_state)(void);
    void (*update_colours)(void);
    void (*update_seconds_timer)(bool swap);
    void (*update_message_timeout)(uint32_t timeout);
    void (*update_timeago)(void);
    void (*update_left_field)(void);
    void (*update_right_field)(void);
    void (*update_trend)(void);
    void (*update_collect_health)(void);
    void (*update_message)(void);
    void (*update_touch)(void);
    void (*update_sensor_info)(void);
    void (*update_timestamp)(void);
    void (*update_bgl_value)(void);
    /* Note: this nasty undef is needed due to how pebble compiles everything icw GRect being both a typedef and macro (bad practice) */
#undef GRect
    GRect (*trend_bounds)(void);
#define GRect(x, y, w, h) ((GRect){{(x), (y)}, {(w), (h)}})
    void (*update_touch_methods)(void);
    void (*update_health_displays)(void);
} WatchfaceCallbacks;

typedef struct {
    void (*alert_handler)(uint8_t alertvalue);
    void (*update_stale_timeout)(void);
    void (*health_poll)(void);
    void (*health_schedule_send)(void);
    void (*minutes_tick)(struct tm* tick_time_cgm, TimeUnits units_changed_cgm);
    void (*second_tick)(struct tm* tick_time_cgm, TimeUnits units_changed_cgm);
} GlobalCallbacks;

#define CALLBACK(name, ...)  \
{ \
    if (name != NULL) { name (__VA_ARGS__); }\
    else { LOG("Callback " #name " called but not set"); }\
}

#endif
