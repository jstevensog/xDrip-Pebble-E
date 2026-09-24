#ifndef __SETTINGS_H__
#define __SETTINGS_H__
#include <stdint.h>
#include <pebble.h>
#include "callbacks.h"

/**
 * Dirty markers
 */
typedef struct {
	uint32_t delta : 1;	 // mark delta layer as dirty and update 
	uint32_t need_cgm : 1;  // make the heartbeat request delta and slope
	uint32_t step_count : 1;
	uint32_t hbm : 1;
    uint32_t sensor_info : 1;
} dirty_markers;

#pragma pack(push, a, 1)
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
    uint32_t vibrate_strong_off : 1;
    uint32_t backlight_on_charge : 1;
    uint32_t collect_health : 1;

    // UX
    uint32_t fields_same_colour : 1;
    uint32_t bold_timeago : 1;
    uint32_t battery_is_charging : 1;
    uint32_t use_analogue_wf : 1;

    // Values
    GColor foreground_colour;
    GColor background_colour;

    uint32_t message_timeout;
    uint8_t left_text_field;
    uint8_t right_text_field;

    // state
    union {
        struct {
            uint32_t stored_time;
            uint8_t storage_marker;
            uint32_t cgm_time;
            uint32_t app_time;
            uint32_t sensor_end_time;
            uint32_t stale_data_timeout;
        
            int32_t step_count;
            int32_t hbm;

            uint32_t special_value_alert : 1;
            uint32_t double_up_down_alert : 1;
            uint32_t app_sync_error_alert : 1;
            uint32_t app_msg_in_drop_alert : 1;
            uint32_t app_msg_out_fail_alert : 1;
            uint32_t bluetooth_alert : 1;
            uint32_t bluetooth_timer_pop : 1;
            uint32_t bluetooth_message_off : 1;
            uint32_t bluetooth_is_connected : 1;
            uint32_t phone_off_alert : 1;
            uint32_t battery_low_alert : 1;

            uint8_t icon;
            uint8_t battery_level;
            uint8_t phone_battery_level;
            comm_bgl_delta delta;
            comm_bgl_value bgl_value;
        };
        uint8_t state_blob[(sizeof(uint32_t) * 8) + (sizeof(uint8_t) * 4) + sizeof(comm_bgl_value) + sizeof(comm_bgl_delta)];
    };

    // global dirty markers
    dirty_markers dirty;

    // Callbacks
    CommunicationCallbacks comm_callbacks;
    GlobalCallbacks gl_cb;
    WatchfaceCallbacks wf_cb;
    
} AppState;
#pragma pack(pop, a)

void settings_init(AppState *state);
void settings_handle(Tuple *data);
void settings_deinit(void);

#define STORAGE_MARKER 0x02
#endif // __SETTINGS_H__
