#include <pebble.h>
#include "settings.h"
#include "../constant.h"
#include "../debug.h"

static globals *settings;

#define LOG_SETTING(s)      LOG("settings: " #s " = %d", settings-> s) 
#define LOG_SETTING_HEX(s)  LOG("settings: " #s " = 0x%X", settings-> s) 

#define CALLBACK(name, ...)  \
{ \
    if (settings-> name != NULL) { settings-> name (__VA_ARGS__); }\
    else { LOG("Callback " #name " called but not set"); }\
}

void settings_init(globals *values) {
    DEBUG("Initializing settings");
    // set local pointer
    settings = values;

    // fetch all values from storage
    
	settings->use_png = persist_exists(SET_USE_PNG) ? persist_read_bool(SET_USE_PNG) : false;
	settings->show_slope = persist_exists(SET_SHOW_SLOPE) ? persist_read_bool(SET_SHOW_SLOPE) : true;
	settings->show_delta = persist_exists(SET_SHOW_DELTA) ? persist_read_bool(SET_SHOW_DELTA) : true;
	settings->show_trend = persist_exists(SET_SHOW_TREND) ? persist_read_bool(SET_SHOW_TREND) : true;

	//Load persistent settings
	settings->enable_seconds= persist_exists(SET_DISP_SECS)? persist_read_bool(SET_DISP_SECS) : false;
	settings->vibrate_repeat = persist_exists(SET_VIBE_REPEAT)? persist_read_bool(SET_VIBE_REPEAT) : true;
	settings->vibrate_off = persist_exists(SET_NO_VIBE)? persist_read_bool(SET_NO_VIBE) : true;
	settings->fields_same_colour = persist_exists(SET_SAMECOLOUR)? persist_read_bool(SET_SAMECOLOUR) : false;
	settings->message_timeout = persist_exists(SET_MESSAGE_TIMEOUT) ? persist_read_int(SET_MESSAGE_TIMEOUT) * 1000 : 15000;
	settings->backlight_on_charge = persist_exists(SET_LIGHT_ON_CHG)? persist_read_bool(SET_LIGHT_ON_CHG) : false;
	settings->bold_timeago = persist_exists(SET_BOLD_TIMEAGO)? persist_read_bool(SET_BOLD_TIMEAGO) : false;
	settings->left_text_field = persist_exists(SET_BOTTOM_LEFT_TEXT) ? persist_read_int(SET_BOTTOM_LEFT_TEXT) : METRIC_PHONEBATT;
	settings->right_text_field = persist_exists(SET_BOTTOM_RIGHT_TEXT) ? persist_read_int(SET_BOTTOM_RIGHT_TEXT) : METRIC_WATCHBATT;
#ifdef PBL_COLOR
	settings->foreground_colour = persist_exists(SET_FG_COLOUR)? GColorFromHEX(persist_read_int(SET_FG_COLOUR)) : COLOR_FALLBACK(GColorWhite,GColorWhite);
	settings->background_colour = persist_exists(SET_BG_COLOUR)? GColorFromHEX(persist_read_int(SET_BG_COLOUR)) : COLOR_FALLBACK(GColorDukeBlue,GColorBlack);
#endif

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
#ifdef PBL_COLOR
    LOG_SETTING_HEX(foreground_colour);
    LOG_SETTING_HEX(background_colour);
#endif
    LOG_SETTING_HEX(left_text_field);
    LOG_SETTING_HEX(right_text_field);
    
    // set all callback to null
}

#define SETTING_BOOL(name, value, field) \
{\
    settings-> name = value != 0; \
    persist_write_bool(field, value); \
    LOG_SETTING(name);\
}
#define SETTING_BOOL_CB(name, value, field, callback, ...) \
{\
    SETTING_BOOL(name, value, field);\
    CALLBACK(callback, __VA_ARGS__);\
}
#define SETTING_INT(name, value, field) \
{\
    settings-> name = value; \
    persist_write_bool(field, value); \
    LOG_SETTING(name);\
}
#define SETTING_INT_CB(name, value, field, callback, ...) \
{\
    SETTING_INT(name, value, field);\
    CALLBACK(callback, __VA_ARGS__);\
}

void settings_handle(Tuple *data) {
    switch (data->key)
    {
        case SET_SAMECOLOUR:
            SETTING_BOOL_CB(fields_same_colour, data->value->uint8, SET_SAMECOLOUR, update_colours);
            break;

        case SET_FG_COLOUR:
#ifdef PBL_COLOR
            settings->foreground_colour = GColorFromHEX(data->value->uint32);
            persist_write_int(SET_FG_COLOUR, data->value->uint32);
            LOG_SETTING(foreground_colour);
            CALLBACK(update_colours);
#endif
        break;

        case SET_BG_COLOUR:
#ifdef PBL_COLOR
            settings->background_colour = GColorFromHEX(data->value->uint32);
            persist_write_int(SET_BG_COLOUR, data->value->uint32);
            LOG_SETTING(background_colour);
            CALLBACK(update_colours);
#endif
        break;

        case SET_DISP_SECS:
            bool sw = settings->enable_seconds;
            SETTING_BOOL_CB(enable_seconds, data->value->uint8, SET_DISP_SECS, update_seconds_timer, sw);
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
            SETTING_INT_CB(message_timeout, data->value->uint8, SET_MESSAGE_TIMEOUT, update_message_timeout, settings->message_timeout);
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
#pragma GCC diagnostic pop

        case SET_USE_PNG:
            SETTING_BOOL_CB(use_png, data->value->uint8, SET_USE_PNG, update_trend);
            break;

        case SET_COLLECT_HEALTH:
#ifdef PBL_HEALTH
            SETTING_BOOL_CB(collect_health, data->value->uint8, SET_COLLECT_HEALTH, update_collect_health);
#endif
            break;
    }
}
