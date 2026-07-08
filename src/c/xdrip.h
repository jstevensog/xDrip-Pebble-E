//xdrip.h - file for all common defines and function prototypes used in xdrip.c
#ifndef __XDRIP_H__
#define __XDRIP_H
// global window variables
// ANYTHING THAT IS CALLED BY PEBBLE API HAS TO BE NOT STATIC

const char FACE_VERSION[] = "xDrip-Pebble2";

// windows definition.
Window *window_cgm = NULL;

// text layer definitions.
TextLayer *bg_layer = NULL;
TextLayer *cgmtime_layer = NULL;
TextLayer *delta_layer = NULL;          // BG DELTA LAYER
TextLayer *message_layer = NULL;        // MESSAGE LAYER
TextLayer *bottom_left_text_layer = NULL;
TextLayer *bottom_right_text_layer = NULL;
TextLayer *time_watch_layer = NULL;
TextLayer *date_app_layer = NULL;

// bitmap layer definitions
BitmapLayer *icon_layer = NULL;
BitmapLayer *bg_trend_layer = NULL;
BitmapLayer *upper_face_layer = NULL;
BitmapLayer *lower_face_layer = NULL;

#ifdef PBL_COLOR
static GColor8 fg_colour;
static GColor8 bg_colour;
#else
static GColor fg_colour;
static GColor bg_colour;
#endif
//Set up Platform specific values and global variables.
#ifdef PBL_PLATFORM_APLITE
const uint8_t PLATFORM = 0;
#elif PBL_PLATFORM_BASALT
const uint8_t PLATFORM = 1;
#elif PBL_PLATFORM_CHALK
const uint8_t PLATFORM = 2;
#elif PBL_PLATFORM_DIORITE
const uint8_t PLATFORM = 3;
#elif PBL_PLATFORM_EMERY
const uint8_t PLATFORM = 4;
#elif PBL_PLATFORM_FLINT
const uint8_t PLATFORM = 5;
#elif PBL_PLATFORM_GABBRO
const uint8_t PLATFORM = 6;
#endif

#define HIGH_RES() (PBL_PLATFORM_TYPE_CURRENT >= PlatformTypeEmery)

GBitmap *icon_bitmap = NULL;
GBitmap *appicon_bitmap = NULL;
GBitmap *specialvalue_bitmap = NULL;
GBitmap *bg_trend_bitmap = NULL;

// Defines to do with Time display
#define TIME_24H_FORMAT "%H:%M"
#define TIME_12H_FORMAT "%l:%M"
#define TIME_24HS_FORMAT "%H:%M:%S"
#define TIME_12HS_FORMAT "%l:%M:%S"
static char time_watch_format[9] = TIME_24H_FORMAT;
static char time_watch_text[] = "00:00:00";
static char date_app_text[] = "Wed 13 Jan";
static char message_layer_text[13];
static GFont time_font;
static char message_layer_text[13];
static GFont time_font_small;
static GFont time_font_normal;

#ifndef PBL_COLOR
#define CHUNK_SIZE 256

// function definition to update monochrome bitmap layers
static void bitmapLayerUpdate(struct Layer *layer, GContext *ctx);
#else
#define CHUNK_SIZE 1024
#endif

// Message Timer Wait Times, in Seconds
static const uint16_t WATCH_MSGSEND_SECS = 60;
static const uint8_t LOADING_MSGSEND_SECS = 2;
static uint8_t minutes_cgm = 0;


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
#define SET_MESSAGE_TIMEOUT		113	// Setting key - Message timeout
#define SET_BOLD_TIMEAGO		114	// Setting key - Meke the TimeAgo text bold if true
#define SET_BOTTOM_LEFT_TEXT		115	// Setting key - What to display in the bottom left text field
#define SET_BOTTOM_RIGHT_TEXT		116	// Setting key - What to display in the bottom right text field
#define CGM_SYNC_KEY			1000	// key pebble will use to request an update.	This should probably include the "capabilities" bits
#define PBL_PLATFORM			1001	// key pebble will use to send it's platform	This is probably not required under the new famework.
#define PBL_APP_VER			1002	// key pebble will use to send the face/app version.	This is probably not required under the new framework.
#define PBL_TREND_SIZE			1003	// key pebble will use to send trend image size.
#define PBL_TREND_LINES		 	1004	// key pebble will use to send trend line options.
#define PBL_DISP_OPTS			1005	// key pebble will use to send display options (delta/arrows).

// TOTAL MESSAGE DATA 4x3+2+5+3+9 = 31 BYTES
// TOTAL KEY HEADER DATA (STRINGS) 4x6+2 = 26 BYTES
// TOTAL MESSAGE 57 BYTES

// ARRAY OF SPECIAL VALUE ICONS
static const uint8_t SPECIAL_VALUE_ICONS[] =
{
	RESOURCE_ID_IMAGE_NONE,		 	//0
	RESOURCE_ID_IMAGE_BROKEN_ANTENNA,	//1
	RESOURCE_ID_IMAGE_BLOOD_DROP,		//2
	RESOURCE_ID_IMAGE_STOP_LIGHT,		//3
	RESOURCE_ID_IMAGE_HOURGLASS,		//4
	RESOURCE_ID_IMAGE_QUESTION_MARKS,	//5
	RESOURCE_ID_IMAGE_LOGO,		 	//6
	RESOURCE_ID_IMAGE_ERR			//7
};

// INDEX FOR ARRAY OF SPECIAL VALUE ICONS
static const uint8_t NONE_SPECVALUE_ICON_INDX = 0;
static const uint8_t BROKEN_ANTENNA_ICON_INDX = 1;
static const uint8_t BLOOD_DROP_ICON_INDX = 2;
static const uint8_t STOP_LIGHT_ICON_INDX = 3;
static const uint8_t HOURGLASS_ICON_INDX = 4;
static const uint8_t QUESTION_MARKS_ICON_INDX = 5;
static const uint8_t LOGO_SPECVALUE_ICON_INDX = 6;

// Metric Display defines
#define METRIC_NONE_STR		"no"
#define METRIC_PHONEBATT_STR	"pb"
#define METRIC_WATCHBATT_STR	"wb"
#define METRIC_STEPS_STR	"sc"
#define METRIC_HEARTRATE_STR	"hr"
#define METRIC_NONE		0
#define METRIC_PHONEBATT	1
#define METRIC_WATCHBATT	2
#define METRIC_STEPS		3
#define METRIC_HEARTRATE	4

//Metric Display Left/Right
static uint8_t bottom_left_metric = 1;
static uint8_t bottom_right_metric = 1;
static char  bottom_left_metric_str[] = "pb";
static char  bottom_right_metric_str[] = "wb";
#ifdef PBL_HEALTH
TextLayer *step_count_text_layer = NULL;
#if defined(PBL_PLATFORM_EMERY) || defined(PBL_PLATFORM_FLINT) || defined(PBL_PLATFORM_GABBRO)
TextLayer *heart_rate_text_layer = NULL;
#endif
#endif

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

//Health and Metric Display functions
#ifdef PBL_HEALTH
static void update_health_metric_displays();
static void health_handler(HealthEventType event, void *context);
#endif
#endif // __XDRIP_H__
