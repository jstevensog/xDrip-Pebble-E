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
    void (*seconds_tick)(struct tm* tick_time_cgm, TimeUnits units_changed);
    void (*minutes_tick)(struct tm *tick_time_cgm, TimeUnits units_changed);
    void (*update_battery_state)(void);
    void (*update_colours)(void);
    void (*update_seconds_timer)(bool swap);
    void (*update_message_timeout)(uint32_t timeout);
    void (*update_timeago)(void);
    void (*update_left_field)(void);
    void (*update_right_field)(void);
    void (*update_trend)(void);
    void (*update_collect_health)(void);
    void (*update_message)(void);
    /* Note: this nasty undef is needed due to how pebble compiles everything icw GRect being both a typedef and macro (bad practice) */
#undef GRect
    GRect (*trend_bounds)(void);
#define GRect(x, y, w, h) ((GRect){{(x), (y)}, {(w), (h)}})
} WatchfaceCallbacks;

typedef struct {
    void (*alert_handler)(uint8_t alertvalue);
} GlobalCallbacks;

/*
 * Callback struct
 *
 * All callbacks are registered on comm_init, this way a user
 * can provide which to support and all others are ignored
 */
typedef struct comm_callback_t {
    void (*phonebat)(comm_phonebat value);
    void (*low_limit)(comm_low_limit value);
    void (*high_limit)(comm_high_limit value);
    void (*slopeval)(comm_slopeval value);
    void (*vibe)(comm_vibe value);
    void (*message)(comm_message message);
    void (*bgl_data)(comm_bgl_data *value);
    void (*bgl_series)(comm_bgl_series *value);
    void (*bgl_delta)(comm_bgl_delta value);
    void (*bgl_value)(comm_bgl_value value);
    void (*bgl_timestamp)(uint32_t timestamp);
    void (*bwp_value)(comm_bwp_value value);
    void (*sensor_info)(comm_sensor_info *value);
    void (*png)(comm_png_data *data);
    void (*health)(comm_health value);
} CommunicationCallbacks;

#define CALLBACK(name, ...)  \
{ \
    if (name != NULL) { name (__VA_ARGS__); }\
    else { LOG("Callback " #name " called but not set"); }\
}

#endif
