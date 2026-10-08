//xdrip.h - file for all common defines and function prototypes used in xdrip.c
#ifndef __XDRIP_H__
#define __XDRIP_H__

#include <pebble.h>
#include <math.h>
#include "constant.h"

/**
 * Feature flags
 */
#define ENABLE_COMM_FRAMEWORK
#define ENABLE_TREND_RENDERER

/** 
 * Face name
 */

#define FACE_VERSION "xDrip-Pebble2"

// Defines to do with Time display
#define TIME_24H_FORMAT "%H:%M"
#define TIME_12H_FORMAT "%l:%M"
#define TIME_24HS_FORMAT "%H:%M:%S"
#define TIME_12HS_FORMAT "%l:%M:%S"

#ifndef PBL_COLOR
#define CHUNK_SIZE 256
#else
#define CHUNK_SIZE 1024
#endif

#define HIGH_RES() (PBL_PLATFORM_TYPE_CURRENT == PlatformTypeEmery || PBL_PLATFORM_TYPE_CURRENT == PlatformTypeGabbro)


#define SET_DISP_SECS                100    // Setting key - Display Seconds
#define SET_FG_COLOUR                101    // Setting key - Foreground Colour
#define SET_BG_COLOUR                102    // Setting key - Background Colour
#define SET_VIBE_REPEAT              103    // Setting key - Vibration Repeat
#define SET_NO_VIBE                  104    // Setting key - No Vibrations
#define SET_LIGHT_ON_CHG             105    // Setting key - Backlight on when charging
#define SET_SAMECOLOUR               106    // Setting key - Same Colours top and bottom
#define SET_NO_DELTA                 107    // Setting key - Do not display the Delta value
#define SET_NO_ARROWS                108    // Setting key - Do not show arrows
#define SET_HIGH_LINE                110    // Setting key - Enable High line on graph.
#define SET_LOW_LINE                 111    // Setting key - Enable Low line on graph.
#define SET_COLLECT_HEALTH           112    // setting key - Enable Health collection
#define SET_MESSAGE_TIMEOUT          113    // Setting key - Message timeout
#define SET_BOLD_TIMEAGO             114    // Setting key - Meke the TimeAgo text bold if true
#define SET_BOTTOM_LEFT_TEXT         115    // Setting key - What to display in the bottom left text field
#define SET_BOTTOM_RIGHT_TEXT        116    // Setting key - What to display in the bottom right text field
#define STALE_DATA_ALERT_TIMEOUT     122    // Setting key - How long to wait on data from xDrip before alerting through vibration.
#define PBL_PLATFORM                 1001    // key pebble will use to send it's platform    This is probably not required under the new famework.
#define PBL_APP_VER                  1002    // key pebble will use to send the face/app version.    This is probably not required under the new framework.
#define PBL_TREND_SIZE               1003    // key pebble will use to send trend image size.
#define PBL_TREND_LINES              1004    // key pebble will use to send trend line options.
#define PBL_TREND_PERIOD             1005    // key pebble will use to send the trend period it wants.
#define PBL_DISP_OPTS                1006    // key pebble will use to send display options (delta/arrows).
#define PBL_VIBE_OPTS                1007    // key pebble will use to send vibration options (alerts, missed signal, no bluetooth)


// TOTAL MESSAGE DATA 4x3+2+5+3+9 = 31 BYTES
// TOTAL KEY HEADER DATA (STRINGS) 4x6+2 = 26 BYTES
// TOTAL MESSAGE 57 BYTES

//Trend layer dimensions
#if defined(PBL_PLATFORM_FLINT)
#define TREND_HEIGHT    88
#elif defined(PBL_PLATFORM_CHALK)
#define TREND_HEIGHT    84
#elif defined(PBL_PLATFORM_EMERY)
#define TREND_HEIGHT    114
#elif defined(PBL_PLATFORM_GABBRO)
#define TREND_HEIGHT    122
#else
#define TREND_HEIGHT    64
#endif

#define MGDL_TO_MMOL(x)      ((int) ((float) x / 18.016)) 
#define MGDL_TO_MMOL_DEC(x)  ((int) round(10.0 * (((float) x / 18.016) - MGDL_TO_MMOL(x))))

// Function Prototypes
// These two are only used if DEBUG_LEVEL is defined.  The code is conditinally compiled otherwise there are warnings.
int myAtoi(char *str);
int myBGAtoi(char *str);
//Handler functions and callbacks
void handle_bluetooth_cgm(bool bt_connected);
void handle_message_tick(void *data);
void handle_stale_data_tick(void *data);
void handle_minute_tick_cgm(struct tm* tick_time_cgm, TimeUnits units_changed_cgm);
void handle_second_tick_cgm(struct tm* tick_time_cgm, TimeUnits units_changed_cgm);
void inbox_dropped_handler_cgm(AppMessageResult appmsg_indrop_error, void *context);
void inbox_received_handler_cgm(DictionaryIterator *iterator, void *context);
void outbox_failed_handler_cgm(DictionaryIterator *failed, AppMessageResult appmsg_outfail_error, void *context);
void BT_timer_callback(void *data);
void timer_callback_cgm(void *data);
void sync_error_callback_cgm(DictionaryResult appsync_dict_error, AppMessageResult appsync_error, void *context);
//determine the UTC offset in the time.
time_t get_UTC_offset(struct tm *t);
//updates the display colours when they are changed in settings
void updateColours();
void reset_timer_callback_cgm(int32_t seconds);

//Health and Metric Display functions
#ifdef PBL_HEALTH
void update_health_metric_displays();
void health_handler(HealthEventType event, void *context);
#endif
#endif // __XDRIP_H__
