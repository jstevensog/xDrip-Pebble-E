#include <pebble.h>
#include "../constant.h"
#include "../debug.h"
#include "settings.h"

static AppState *state;

#define LOG_SETTING(s)      LOG("state: " #s " = %d", state-> s) 
#define LOG_SETTING_HEX(s)  LOG("state: " #s " = 0x%X", state-> s) 
#define LOG_SETTING_INT(s)  LOG("state: " #s " = %d", (int32_t) state-> s) 

void settings_init(AppState *values) {
    DEBUG("Initializing state");
    // set local pointer
    state = values;

	// prep for battery display, even if we don't have one.
	state->icon = NOT_CALIBRATED; // no icon set and ignore
	state->cgm_time = 0;
	state->app_time = 0;
    state->dirty.need_cgm = 1;
    state->dirty.delta = 1;
    state->dirty.sensor_info = 1;
	state->phone_battery_level = 255;
	state->battery_level = 255;

    // fetch previous data (if available)
    if (persist_exists(STORED_DATA)) {
        uint8_t *tmp = malloc(sizeof(state->state_blob)); 
        if (tmp != NULL) {
            status_t st = persist_read_data(STORED_DATA, tmp, sizeof(state->state_blob));
            if (st != sizeof(state->state_blob)) ERROR("Could not load data: %d", st);
            uint32_t *tmp_time = (uint32_t *) tmp; 
            if ((uint32_t)time(NULL) - *tmp_time > 5 * SECONDS_PER_MINUTE || tmp[4] != STORAGE_MARKER) {
                persist_delete(STORED_DATA);
                WARNING("Deleting persistent storage");
                // do nothing, values will remain zero
            } else {
                memcpy(state->state_blob, tmp, sizeof(state->state_blob));
            }
            free(tmp);
        }
    }

    // fetch all values from storage
    
	state->use_png = persist_exists(SET_USE_PNG) ? persist_read_bool(SET_USE_PNG) : false;
	state->show_slope = persist_exists(SET_SHOW_SLOPE) ? persist_read_bool(SET_SHOW_SLOPE) : true;
	state->show_delta = persist_exists(SET_SHOW_DELTA) ? persist_read_bool(SET_SHOW_DELTA) : true;
	state->show_trend = persist_exists(SET_SHOW_TREND) ? persist_read_bool(SET_SHOW_TREND) : true;

	//Load persistent state
	state->enable_seconds= persist_exists(SET_DISP_SECS)? persist_read_bool(SET_DISP_SECS) : false;
	state->vibrate_repeat = persist_exists(SET_VIBE_REPEAT)? persist_read_bool(SET_VIBE_REPEAT) : true;
	state->vibrate_off = persist_exists(SET_NO_VIBE)? persist_read_bool(SET_NO_VIBE) : true;
	state->fields_same_colour = persist_exists(SET_SAMECOLOUR)? persist_read_bool(SET_SAMECOLOUR) : false;
	state->message_timeout = persist_exists(SET_MESSAGE_TIMEOUT) ? persist_read_int(SET_MESSAGE_TIMEOUT) * 1000 : 15000;
	state->backlight_on_charge = persist_exists(SET_LIGHT_ON_CHG)? persist_read_bool(SET_LIGHT_ON_CHG) : false;
	state->bold_timeago = persist_exists(SET_BOLD_TIMEAGO)? persist_read_bool(SET_BOLD_TIMEAGO) : false;
	state->left_text_field = persist_exists(SET_BOTTOM_LEFT_TEXT) ? persist_read_int(SET_BOTTOM_LEFT_TEXT) : METRIC_PHONEBATT;
	state->right_text_field = persist_exists(SET_BOTTOM_RIGHT_TEXT) ? persist_read_int(SET_BOTTOM_RIGHT_TEXT) : METRIC_WATCHBATT;
#ifdef PBL_COLOR
	state->foreground_colour = persist_exists(SET_FG_COLOUR)? GColorFromHEX(persist_read_int(SET_FG_COLOUR)) : COLOR_FALLBACK(GColorWhite,GColorWhite);
	state->background_colour = persist_exists(SET_BG_COLOUR)? GColorFromHEX(persist_read_int(SET_BG_COLOUR)) : COLOR_FALLBACK(GColorDukeBlue,GColorBlack);
#endif
    state->stale_data_timeout = persist_exists(STALE_DATA_ALERT_TIMEOUT) ? persist_read_int(STALE_DATA_ALERT_TIMEOUT) : 6 * 60000;
    state->collect_health = persist_exists(SET_COLLECT_HEALTH) ? persist_read_bool(SET_COLLECT_HEALTH) : false;
    state->touch_support = persist_exists(SET_TOUCH_SUPPORT) ? persist_read_bool(SET_TOUCH_SUPPORT) : false;

    state->default_basal = persist_exists(SET_DEFAULT_BASAL) ? persist_read_int(SET_DEFAULT_BASAL) : 16;
    state->default_bolus = persist_exists(SET_DEFAULT_BOLUS) ? persist_read_int(SET_DEFAULT_BOLUS) : 10;
    state->default_carbs = persist_exists(SET_DEFAULT_CARBS) ? persist_read_int(SET_DEFAULT_CARBS) : 60;

    state->snooze_low = persist_exists(SET_SNOOZE_LOW) ? persist_read_int(SET_SNOOZE_LOW) : 30;
    state->snooze_high = persist_exists(SET_SNOOZE_HIGH) ? persist_read_int(SET_SNOOZE_HIGH) : 120;

    state->touch_treatment = persist_exists(SET_TOUCH_TREATMENT) ? persist_read_int(SET_TOUCH_TREATMENT) : TOUCH_TAP;
    state->touch_alert_snooze = persist_exists(SET_TOUCH_ALERT_SNOOZE) ? persist_read_int(SET_TOUCH_ALERT_SNOOZE) : TOUCH_TAP;

    if (state->sensor.interval == 0) state->sensor.interval = 5 * SECONDS_PER_MINUTE; // default to 5 mins unless xdrip tells otherwise

    LOG_SETTING(use_png);
    LOG_SETTING(show_slope);
    LOG_SETTING(show_delta);
    LOG_SETTING(show_trend);
    LOG_SETTING(enable_seconds);
    LOG_SETTING(vibrate_repeat);
    LOG_SETTING(vibrate_off);
    LOG_SETTING(backlight_on_charge);
    LOG_SETTING(bold_timeago);
    LOG_SETTING(fields_same_colour);
    LOG_SETTING(collect_health);
#ifdef PBL_COLOR
    LOG_SETTING_HEX(foreground_colour);
    LOG_SETTING_HEX(background_colour);
#endif
    LOG_SETTING_HEX(left_text_field);
    LOG_SETTING_HEX(right_text_field);
    LOG_SETTING_INT(message_timeout);
    LOG_SETTING_INT(stale_data_timeout);
    
    // check if we actually need a refresh
    if (time(NULL) - state->cgm_time < (uint32_t) state->sensor.interval) {
        state->dirty.need_cgm = 0;
    }

    if (state->sensor.sensor_type == 0) state->dirty.sensor_info = 1;
}

void settings_deinit(void) {
    state->stored_time = time(NULL);
    state->storage_marker = STORAGE_MARKER;
    persist_write_data(STORED_DATA, state->state_blob, sizeof(state->state_blob));
}

#define SETTING_BOOL(name, value, field) \
{\
    state-> name = value != 0; \
    persist_write_bool(field, value); \
    LOG_SETTING(name);\
}
#define SETTING_BOOL_CB(name, value, field, callback, ...) \
{\
    SETTING_BOOL(name, value, field);\
    CALLBACK(state->wf_cb.callback, __VA_ARGS__);\
}
#define SETTING_INT(name, value, field) \
{\
    state-> name = value; \
    persist_write_int(field, value); \
    LOG_SETTING(name);\
}
#define SETTING_INT_CB(name, value, field, callback, ...) \
{\
    SETTING_INT(name, value, field);\
    CALLBACK(state->wf_cb.callback, __VA_ARGS__);\
}
#define SETTING_INT_CB_GL(name, value, field, callback, ...) \
{\
    SETTING_INT(name, value, field);\
    CALLBACK(state->gl_cb.callback, __VA_ARGS__);\
}

#define SETTING_INT_CB_WF(name, value, field, callback, ...) \
{\
    SETTING_INT(name, value, field);\
    CALLBACK(state->wf_cb.callback, __VA_ARGS__);\
}

bool settings_receiver(Tuple *data) {
    bool rv = true;

    switch (data->key)
    {
        case SET_SAMECOLOUR:
            SETTING_BOOL_CB(fields_same_colour, data->value->uint8, SET_SAMECOLOUR, update_colours);
            break;

        case SET_FG_COLOUR:
#ifdef PBL_COLOR
            state->foreground_colour = GColorFromHEX(data->value->uint32);
            persist_write_int(SET_FG_COLOUR, data->value->uint32);
            LOG_SETTING(foreground_colour);
            CALLBACK(state->wf_cb.update_colours);
#endif
            break;

        case SET_BG_COLOUR:
#ifdef PBL_COLOR
            state->background_colour = GColorFromHEX(data->value->uint32);
            persist_write_int(SET_BG_COLOUR, data->value->uint32);
            LOG_SETTING(background_colour);
            CALLBACK(state->wf_cb.update_colours);
#endif
            break;

        case SET_DISP_SECS:
            bool sw = state->enable_seconds;
            SETTING_BOOL_CB(enable_seconds, data->value->uint8, SET_DISP_SECS, update_seconds_timer, sw);
            rv = true;
            break;


        case SET_VIBE_REPEAT:
            SETTING_BOOL(vibrate_repeat, data->value->uint8, SET_VIBE_REPEAT);
            break;

        case SET_NO_VIBE:
            SETTING_BOOL(vibrate_off, data->value->uint8, SET_NO_VIBE);
            break;

        case SET_LIGHT_ON_CHG:
            SETTING_BOOL(backlight_on_charge, data->value->uint8, SET_LIGHT_ON_CHG);
            break;

        case SET_MESSAGE_TIMEOUT:
            SETTING_INT_CB(message_timeout, data->value->uint8, SET_MESSAGE_TIMEOUT, update_message_timeout, state->message_timeout);
            break;

        case SET_BOLD_TIMEAGO:
            SETTING_BOOL_CB(bold_timeago, data->value->uint8, SET_BOLD_TIMEAGO, update_timeago);
            break;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wzero-length-bounds"
        //Bottom left metric to display
        case SET_BOTTOM_LEFT_TEXT:
            SETTING_INT_CB(left_text_field, data->value->data[0] - 0x30, SET_BOTTOM_LEFT_TEXT, update_left_field);
            break;

        //Bottom right metric to display
        case SET_BOTTOM_RIGHT_TEXT:
            SETTING_INT_CB(right_text_field, data->value->data[0] - 0x30, SET_BOTTOM_RIGHT_TEXT, update_right_field);
            break;
        case SET_TOUCH_TREATMENT:
            SETTING_INT_CB_WF(touch_treatment, data->value->data[0] - 0x30, SET_TOUCH_TREATMENT, update_touch_methods);
            break;
        case SET_TOUCH_ALERT_SNOOZE:
            SETTING_INT_CB_WF(touch_alert_snooze, data->value->data[0] - 0x30, SET_TOUCH_ALERT_SNOOZE, update_touch_methods);
            break;
#pragma GCC diagnostic pop

        case SET_USE_PNG:
            SETTING_BOOL_CB(use_png, data->value->uint8, SET_USE_PNG, update_trend);
            break;

        case SET_COLLECT_HEALTH:
#ifdef PBL_HEALTH
            SETTING_BOOL_CB(collect_health, data->value->uint8, SET_COLLECT_HEALTH, update_collect_health);
#endif
            break;
        case STALE_DATA_ALERT_TIMEOUT:
            if (data->value->uint32 >= 6) {
                SETTING_INT_CB_GL(stale_data_timeout, data->value->int32 * 60000, STALE_DATA_ALERT_TIMEOUT, update_stale_timeout);
            }
            rv = true;
            break;
        case SET_TOUCH_SUPPORT:
            SETTING_BOOL_CB(touch_support, data->value->int8, SET_TOUCH_SUPPORT, update_touch);
            break;
        case SET_DEFAULT_BASAL:
            SETTING_INT(default_basal, data->value->uint16, SET_DEFAULT_BASAL);
            break;
        case SET_DEFAULT_BOLUS:
            SETTING_INT(default_bolus, data->value->uint16, SET_DEFAULT_BOLUS);
            break;
        case SET_DEFAULT_CARBS:
            SETTING_INT(default_carbs, data->value->uint16, SET_DEFAULT_CARBS);
            break;
        case SET_SNOOZE_LOW:
            SETTING_INT(snooze_low, data->value->uint16, SET_SNOOZE_LOW);
            break;
        case SET_SNOOZE_HIGH:
            SETTING_INT(snooze_high, data->value->uint16, SET_SNOOZE_HIGH);
            break;
        default:
            rv = false;
            break;
    }
    return rv;
}
