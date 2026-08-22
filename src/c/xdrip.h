//xdrip.h - file for all common defines and function prototypes used in xdrip.c
#ifndef __XDRIP_H__
#define __XDRIP_H__

#include <math.h>
#include "constant.h"
/**
 * Defines for testing modes
 */

/*
 * Set debug text to show and compile DEBUG_APP_[NONE,INFO,DEBUG,TRACE]
 * Note: Aplite will not go above INFO logging.
 */
#define DEBUG_APP_TRACE 3
#define DEBUG_APP_DEBUG 2
#define DEBUG_APP_INFO 1
#define DEBUG_APP_NONE 0

/* #define DEBUG_LEVEL DEBUG_APP_TRACE  */

/* The line below, if defined, will only indicate test values on the display.
this is for testing purposes only until I can get the PebbleKit.JS code operating with the emulator.
Make sure you udefine this before building a release.
*/
/* #define TEST_MODE */

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

// function definition to update monochrome bitmap layers
static void bitmapLayerUpdate(struct Layer *layer, GContext *ctx);
#else
#define CHUNK_SIZE 1024
#endif

#define HIGH_RES() (PBL_PLATFORM_TYPE_CURRENT >= PlatformTypeEmery)


#define CGM_ICON_KEY			0	// TUPLE_CSTRING, MAX 2 BYTES (10)
#define CGM_BG_KEY			1	// TUPLE_CSTRING, MAX 4 BYTES (253 OR 22.2)
#define CGM_TCGM_KEY			2	// TUPLE_INT, 4 BYTES (CGM TIME)
#define CGM_TAPP_KEY			3	// TUPLE_INT, 4 BYTES (APP / PHONE TIME)
#define CGM_DLTA_KEY			4	// TUPLE_CSTRING, MAX 5 BYTES (BG DELTA, -100 or -10.0)
#define CGM_UBAT_KEY			5	// TUPLE_CSTRING, MAX 3 BYTES (UPLOADER BATTERY, 100)
#define CGM_NAME_KEY			6	// TUPLE_CSTRING, MAX 9 BYTES (Christine)
#define CGM_TREND_BEGIN_KEY		7	// TUPLE_INT, 4 BYTES (length of CGM_TREND_DATA_KEY
#define CGM_TREND_DATA_KEY		8	// TUPLE_BYTE[], No Maximum, based on value found in CGM_TREND_DATA_KEY
#define CGM_TREND_END_KEY		9	// TUPLE_INT, always 0.
#define CGM_MESSAGE_KEY		 	10	// TUPLE_CSTRING, Message to display flashing in mid screen
#define CGM_VIBE_KEY			11	// TUPLE_INT, Vibe pattern to alert with
#define SET_DISP_SECS			100	// Setting key - Display Seconds
#define SET_FG_COLOUR			101	// Setting key - Foreground Colour
#define SET_BG_COLOUR			102	// Setting key - Background Colour
#define SET_VIBE_REPEAT		 	103	// Setting key - Vibration Repeat
#define SET_NO_VIBE			104	// Setting key - No Vibrations
#define SET_LIGHT_ON_CHG		105	// Setting key - Backlight on when charging
#define SET_SAMECOLOUR			106	// Setting key - Same Colours top and bottom
#define SET_NO_DELTA			107	// Setting key - Do not display the Delta value
#define SET_NO_ARROWS			108	// Setting key - Do not show arrows
#define SET_HIGH_LINE			110	// Setting key - Enable High line on graph.
#define SET_LOW_LINE			111	// Setting key - Enable Low line on graph.
#define SET_COLLECT_HEALTH      112
#define SET_MESSAGE_TIMEOUT		113	// Setting key - Message timeout
#define SET_BOLD_TIMEAGO		114	// Setting key - Meke the TimeAgo text bold if true
#define SET_BOTTOM_LEFT_TEXT	        115	// Setting key - What to display in the bottom left text field
#define SET_BOTTOM_RIGHT_TEXT	        116	// Setting key - What to display in the bottom right text field
#define SET_USE_PNG             117
#define SET_SHOW_UNIT           118
#define SET_SHOW_DELTA          119
#define SET_SHOW_SLOPE          120
#define SET_SHOW_TREND          121
#define SET_BGL_CRITICAL_COLOUR  301
#define SET_BGL_HIGH_COLOUR      302
#define SET_BGL_AVERAGE_COLOUR   303
#define SET_BGL_GOOD_COLOUR      304
#define SET_BGL_LOW_COLOUR       305
#define SET_BGL_LOW             306
#define SET_BGL_AVERAGE         307
#define SET_BGL_HIGH            308
#define SET_BGL_CRITICAL        309
#define SET_LOW_LINE_COLOUR      310
#define SET_HIGH_LINE_COLOUR     311
#define SET_LINE_STYLE          312
#define SET_LINE_WIDTH          313
#define SET_TREND_STYLE         314
#define SET_TREND_WIDTH         315
#define SET_HIGH_LIMIT          316
#define SET_HIGH_LINE_VALUE     317
#define SET_LOW_LIMIT           318
#define SET_LOW_LINE_VALUE      319
#define CGM_SYNC_KEY			1000	// key pebble will use to request an update.	This should probably include the "capabilities" bits
#define PBL_PLATFORM			1001	// key pebble will use to send it's platform	This is probably not required under the new famework.
#define PBL_APP_VER			1002	// key pebble will use to send the face/app version.	This is probably not required under the new framework.
#define PBL_TREND_SIZE			1003	// key pebble will use to send trend image size.
#define PBL_TREND_LINES		 	1004	// key pebble will use to send trend line options.
#define PBL_TREND_PERIOD		1005	// key pebble will use to send the trend period it wants.
#define PBL_DISP_OPTS			1006	// key pebble will use to send display options (delta/arrows).
#define PBL_VIBE_OPTS			1007	// key pebble will use to send vibration options (alerts, missed signal, no bluetooth)


// TOTAL MESSAGE DATA 4x3+2+5+3+9 = 31 BYTES
// TOTAL KEY HEADER DATA (STRINGS) 4x6+2 = 26 BYTES
// TOTAL MESSAGE 57 BYTES

//Trend layer dimensions
#if defined(PBL_PLATFORM_FLINT)
#define TREND_HEIGHT	88
#elif defined(PBL_PLATFORM_CHALK)
#define TREND_HEIGHT	84
#elif defined(PBL_PLATFORM_EMERY)
#define TREND_HEIGHT	114
#elif defined(PBL_PLATFORM_GABBRO)
#define TREND_HEIGHT	122
#else
#define TREND_HEIGHT	64
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
static void update_health_metric_displays();
static void health_handler(HealthEventType event, void *context);
#endif
#endif // __XDRIP_H__
