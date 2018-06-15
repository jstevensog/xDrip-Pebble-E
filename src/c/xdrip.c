#include "pebble.h"


/* The line below will set the debug message level.
Make sure you set this to 0 before building a release. */

#define DEBUG_LEVEL 1

// global window variables
// ANYTHING THAT IS CALLED BY PEBBLE API HAS TO BE NOT STATIC

const char FACE_VERSION[] = "xDrip-Pebble2";
#ifdef PBL_PLATFORM_APLITE
const uint8_t PLATFORM = 0;
#else
const uint8_t PLATFORM = 1;
#endif

Window *window_cgm = NULL;

TextLayer *bg_layer = NULL;
TextLayer *cgmtime_layer = NULL;
TextLayer *delta_layer = NULL;		// BG DELTA LAYER
TextLayer *message_layer = NULL;	// MESSAGE LAYER
TextLayer *battlevel_layer = NULL;
TextLayer *watch_battlevel_layer = NULL;
TextLayer *time_watch_layer = NULL;
TextLayer *date_app_layer = NULL;



BitmapLayer *icon_layer = NULL;
BitmapLayer *bg_trend_layer = NULL;
BitmapLayer *upper_face_layer = NULL;
BitmapLayer *lower_face_layer = NULL;


GBitmap *icon_bitmap = NULL;
GBitmap *appicon_bitmap = NULL;
GBitmap *specialvalue_bitmap = NULL;
GBitmap *bg_trend_bitmap = NULL;

#ifdef PBL_COLOR
static GColor8 fg_colour;
static GColor8 bg_colour;
#else
static GColor fg_colour;
static GColor bg_colour;
#endif

#if defined(PBL_HEALTH)
void health_handler(HealthEventType event, void *context);
static void start_data_log();
static void stop_data_log();
static void restart_data_log();
static DataLoggingSessionRef s_session_heartrate;
static DataLoggingSessionRef s_session_movement;
static HealthValue laststeps = 0;
static HealthValue lastbpm = 0;
static time_t last_movement_time = 0;
#define HEARTRATE_LOG 101
#define MOVEMENT_LOG 103
#endif

static void health_subscribe();
static void health_unsubscribe();


// Defines to do with Time display
#define TIME_24H_FORMAT "%H:%M"
#define TIME_12H_FORMAT "%l:%M"
#define TIME_24HS_FORMAT "%H:%M:%S"
#define TIME_12HS_FORMAT "%l:%M:%S"
static char time_watch_format[9] = TIME_24H_FORMAT;
static char time_watch_text[] = "00:00:00";
static char date_app_text[] = "Wed 13 Jan";
static GFont time_font;
static GFont time_font_small;
static GFont time_font_normal;

// Boolean to allow/prevent re-raise of NO BLUETOOTH vibration
static bool vibe_repeat = false;
// variables for AppSync
AppSync sync_cgm;
#ifdef PBL_PLATFORM_APLITE
#define CHUNK_SIZE 256
static void bitmapLayerUpdate(struct Layer *layer, GContext *ctx);
#else
#define CHUNK_SIZE 1024
#endif
// CGM message is 57 bytes
// Pebble needs additional 62 Bytes?!? Pad with additional 20 bytes
//#define SYNC_BUFFER_SIZE 1024
//static uint8_t *sync_buffer_cgm;
//static uint8_t sync_buffer_cgm[CHUNK_SIZE];
//static uint8_t trend_buffer[4096];
static bool handling_second = false;
static bool doing_trend = false;
static bool global_lock = false;
//#ifdef PBL_PLATFORM_BASALT
uint8_t *trend_buffer = NULL;
static uint16_t trend_buffer_length = 0;
static uint16_t expected_trend_buffer_length = 0;
//#endif
bool display_message = false;

// variables for settings from Pebble Phone App.
//static GColor8 background_colour = 0;
//static GColor8 foreground_color = 0;
static bool display_seconds = false;
static bool old_state = false;
// variables for timers and time
AppTimer *timer_cgm = NULL;
AppTimer *BT_timer = NULL;
time_t time_now = 0;

// global variable for bluetooth connection
bool bluetooth_connected_cgm = true;

// BATTERY LEVEL FORMATTED SIZE used for Bridge/Phone and Watch battery indications
const uint8_t BATTLEVEL_FORMATTED_SIZE = 8;

// global variables for sync tuple functions
// buffers have to be static and hardcoded
static char current_icon[2];
static char last_bg[6];
//static int current_bg = 0;
static bool currentBG_isMMOL = false;
static char last_battlevel[4];
static uint32_t current_cgm_time = 0;
static uint32_t current_app_time = 0;
static char current_bg_delta[14];
//static int converted_bgDelta = 0;

// global BG snooze timer
static uint8_t lastAlertTime = 0;

// global special value alert
static bool specvalue_alert = false;

// global variables for vibrating in special conditions
static bool DoubleDownAlert = false;
static bool AppSyncErrAlert = false;
static bool AppMsgInDropAlert = false;
static bool AppMsgOutFailAlert = false;
static bool BluetoothAlert = false;
static bool BT_timer_pop = false;
//static bool CGMOffAlert = false;
static bool PhoneOffAlert = false;
static bool LowBatteryAlert = false;

// global constants for time durations
static const uint8_t MINUTEAGO = 60;
static const uint16_t HOURAGO = 60*(60);
static const uint32_t DAYAGO = 24*(60*60);
static const uint32_t WEEKAGO = 7*(24*60*60);
static const uint16_t MS_IN_A_SECOND = 1000;

// Constants for string buffers
// If add month to date, buffer size needs to increase to 12; also need to reformat date_app_text init string
static const uint8_t TIME_TEXTBUFF_SIZE = 10;
static const uint8_t DATE_TEXTBUFF_SIZE = 11;
static const uint8_t LABEL_BUFFER_SIZE = 6;
static const uint8_t TIMEAGO_BUFFER_SIZE = 10;

// ** START OF CONSTANTS THAT CAN BE CHANGED; DO NOT CHANGE IF YOU DO NOT KNOW WHAT YOU ARE DOING **
// ** FOR MMOL, ALL VALUES ARE STORED AS INTEGER; LAST DIGIT IS USED AS DECIMAL **
// ** BE EXTRA CAREFUL OF CHANGING SPECIAL VALUES OR TIMERS; DO NOT CHANGE WITHOUT EXPERT HELP **

// FOR BG RANGES
// DO NOT SET ANY BG RANGES EQUAL TO ANOTHER; LOW CAN NOT EQUAL MIDLOW
// LOW BG RANGES MUST BE IN ASCENDING ORDER; SPECVALUE < HYPOLOW < BIGLOW < MIDLOW < LOW
// HIGH BG RANGES MUST BE IN ASCENDING ORDER; HIGH < MIDHIGH < BIGHIGH
// DO NOT ADJUST SPECVALUE UNLESS YOU HAVE A VERY GOOD REASON
// DO NOT USE NEGATIVE NUMBERS OR DECIMAL POINTS OR ANYTHING OTHER THAN A NUMBER

// BG Ranges, MG/DL
static const uint16_t SPECVALUE_BG_MGDL = 20;
static const uint16_t SHOWLOW_BG_MGDL = 40;
static const uint16_t SHOWHIGH_BG_MGDL = 400;

// BG Ranges, MMOL
// VALUES ARE IN INT, NOT FLOATING POINT, LAST DIGIT IS DECIMAL
// FOR EXAMPLE : SPECVALUE IS 1.1, BIGHIGH IS 16.6
// ALWAYS USE ONE AND ONLY ONE DECIMAL POINT FOR LAST DIGIT
// GOOD : 5.0, 12.2 // BAD : 7 , 14.44
static const uint16_t SPECVALUE_BG_MMOL = 11;
static const uint16_t SHOWLOW_BG_MMOL = 22;
static const uint16_t SHOWHIGH_BG_MMOL = 220;

// BG Snooze Times, in Minutes; controls when vibrate again
// RANGE 0-240
static const uint8_t SPECVALUE_SNZ_MIN = 30;

// Vibration Levels; 0 = NONE; 1 = LOW; 2 = MEDIUM; 3 = HIGH
// IF YOU DO NOT WANT A SPECIFIC VIBRATION, SET TO 0
static const uint8_t SPECVALUE_VIBE = 2;
static const uint8_t DOUBLEDOWN_VIBE = 3;
static const uint8_t APPSYNC_ERR_VIBE = 1;
static const uint8_t APPMSG_INDROP_VIBE = 1;
static const uint8_t APPMSG_OUTFAIL_VIBE = 1;
static const uint8_t BTOUT_VIBE = 1;
static const uint8_t CGMOUT_VIBE = 1;
static const uint8_t PHONEOUT_VIBE = 1;
static const uint8_t LOWBATTERY_VIBE = 1;

// Icon Cross Out & Vibrate Once Wait Times, in Minutes
// RANGE 0-240
// IF YOU WANT TO WAIT LONGER TO GET CONDITION, INCREASE NUMBER
static const uint8_t CGMOUT_WAIT_MIN = 15;
static const uint8_t PHONEOUT_WAIT_MIN = 8;

// Control Messages
// IF YOU DO NOT WANT A SPECIFIC MESSAGE, SET TO true
static const bool TurnOff_NOBLUETOOTH_Msg = false;
static const bool TurnOff_CHECKCGM_Msg = false;
static const bool TurnOff_CHECKPHONE_Msg = false;

// Control Vibrations
// IF YOU WANT NO VIBRATIONS, SET TO true
static bool TurnOffAllVibrations = false;
// IF YOU WANT LESS INTENSE VIBRATIONS, SET TO true
static const bool TurnOffStrongVibrations = false;

//Control Backlight
static bool BacklightOnCharge = false;

//All one background colour
static bool MonochromeBackground =false;

// Bluetooth Timer Wait Time, in Seconds
// RANGE 0-240
// THIS IS ONLY FOR BAD BLUETOOTH CONNECTIONS
// TRY EXTENDING THIS TIME TO SEE IF IT WILL HELP SMOOTH CONNECTION
// CGM DATA RECEIVED EVERY 60 SECONDS, GOING BEYOND THAT MAY RESULT IN MISSED DATA
static const uint8_t BT_ALERT_WAIT_SECS = 10;

// ** END OF CONSTANTS THAT CAN BE CHANGED; DO NOT CHANGE IF YOU DO NOT KNOW WHAT YOU ARE DOING **

// Message Timer Wait Times, in Seconds
static const uint16_t WATCH_MSGSEND_SECS = 60;
static const uint8_t LOADING_MSGSEND_SECS = 2;
static uint8_t minutes_cgm = 0;
static bool CollectHealth = false;
static uint8_t alternator = 0;


#define	CGM_ICON_KEY 	0		// TUPLE_CSTRING, MAX 2 BYTES (10)
#define	CGM_BG_KEY 	1		// TUPLE_CSTRING, MAX 4 BYTES (253 OR 22.2)
#define	CGM_TCGM_KEY	2		// TUPLE_INT, 4 BYTES (CGM TIME)
#define	CGM_TAPP_KEY 	3		// TUPLE_INT, 4 BYTES (APP / PHONE TIME)
#define	CGM_DLTA_KEY 	4		// TUPLE_CSTRING, MAX 5 BYTES (BG DELTA, -100 or -10.0)
#define	CGM_UBAT_KEY 	5		// TUPLE_CSTRING, MAX 3 BYTES (UPLOADER BATTERY, 100)
#define	CGM_NAME_KEY 	6		// TUPLE_CSTRING, MAX 9 BYTES (Christine)
#define	CGM_TREND_BEGIN_KEY 	7		// TUPLE_INT, 4 BYTES (length of CGM_TREND_DATA_KEY
#define	CGM_TREND_DATA_KEY 	8		// TUPLE_BYTE[], No Maximum, based on value found in CGM_TREND_DATA_KEY
#define	CGM_TREND_END_KEY 	9		// TUPLE_INT, always 0.
#define CGM_MESSAGE_KEY		10
#define CGM_VIBE_KEY		11
#define CGM_COLLECT_HEALTH_KEY        112    // whether to log health data
#define SET_DISP_SECS		100	// Setting key - Display Seconds
#define SET_FG_COLOUR		101	// Setting key - Foreground Colour
#define SET_BG_COLOUR		102	// Setting key - Background Colour
#define SET_VIBE_REPEAT		103	// Setting key - Vibration Repeat
#define SET_NO_VIBE		104	// Setting key - No Vibrations
#define SET_LIGHT_ON_CHG	105	// Setting key - Backlight on when charging
#define SET_MONOCHROME	106	// Setting key - Same top and bottom colour
#define CGM_SYNC_KEY		1000	// key pebble will use to request an update.
#define PBL_PLATFORM		1001	// key pebble will use to send it's platform
#define PBL_APP_VER		1002	// key pebble will use to send the face/app version.
#define PBL_TREND_SIZE		1003	// key pebble will use to send trend image size.
#define PBL_TREND_LINES		1004	// key pebble will use to send trend line options.
#define PBL_DISP_OPTS		1005	// key pebble will use to send display options (delta/arrows).

// TOTAL MESSAGE DATA 4x3+2+5+3+9 = 31 BYTES
// TOTAL KEY HEADER DATA (STRINGS) 4x6+2 = 26 BYTES
// TOTAL MESSAGE 57 BYTES

// ARRAY OF SPECIAL VALUE ICONS
static const uint8_t SPECIAL_VALUE_ICONS[] =
{
		RESOURCE_ID_IMAGE_NONE,			//0
		RESOURCE_ID_IMAGE_BROKEN_ANTENNA,	//1
		RESOURCE_ID_IMAGE_BLOOD_DROP,		//2
		RESOURCE_ID_IMAGE_STOP_LIGHT,		//3
		RESOURCE_ID_IMAGE_HOURGLASS,		//4
		RESOURCE_ID_IMAGE_QUESTION_MARKS,	//5
		RESOURCE_ID_IMAGE_LOGO,			//6
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
static const uint8_t ERR_SPECVALUE_ICON_INDX = 7;

/*
// ARRAY OF TIMEAGO ICONS
static const uint8_t TIMEAGO_ICONS[] = {
	RESOURCE_ID_IMAGE_NONE,			//0
	RESOURCE_ID_IMAGE_PHONEON,		//1
	RESOURCE_ID_IMAGE_PHONEOFF,	 	//2
};

// INDEX FOR ARRAY OF TIMEAGO ICONS
static const uint8_t NONE_TIMEAGO_ICON_INDX = 0;
static const uint8_t PHONEON_ICON_INDX = 1;
static const uint8_t PHONEOFF_ICON_INDX = 2;
 */
#ifdef DEBUG_LEVEL
static char *translate_app_error(AppMessageResult result)
{
	switch (result)
	{
	case APP_MSG_OK:
		return "APP_MSG_OK";
	case APP_MSG_SEND_TIMEOUT:
		return "APP_MSG_SEND_TIMEOUT";
	case APP_MSG_SEND_REJECTED:
		return "APP_MSG_SEND_REJECTED";
	case APP_MSG_NOT_CONNECTED:
		return "APP_MSG_NOT_CONNECTED";
	case APP_MSG_APP_NOT_RUNNING:
		return "APP_MSG_APP_NOT_RUNNING";
	case APP_MSG_INVALID_ARGS:
		return "APP_MSG_INVALID_ARGS";
	case APP_MSG_BUSY:
		return "APP_MSG_BUSY";
	case APP_MSG_BUFFER_OVERFLOW:
		return "APP_MSG_BUFFER_OVERFLOW";
	case APP_MSG_ALREADY_RELEASED:
		return "APP_MSG_ALREADY_RELEASED";
	case APP_MSG_CALLBACK_ALREADY_REGISTERED:
		return "APP_MSG_CALLBACK_ALREADY_REGISTERED";
	case APP_MSG_CALLBACK_NOT_REGISTERED:
		return "APP_MSG_CALLBACK_NOT_REGISTERED";
	case APP_MSG_OUT_OF_MEMORY:
		return "APP_MSG_OUT_OF_MEMORY";
	case APP_MSG_CLOSED:
		return "APP_MSG_CLOSED";
	case APP_MSG_INTERNAL_ERROR:
		return "APP_MSG_INTERNAL_ERROR";
	default:
		return "APP UNKNOWN ERROR";
	}
}

static char *translate_dict_error(DictionaryResult result)
{
	switch (result)
	{
	case DICT_OK:
		return "DICT_OK";
	case DICT_NOT_ENOUGH_STORAGE:
		return "DICT_NOT_ENOUGH_STORAGE";
	case DICT_INVALID_ARGS:
		return "DICT_INVALID_ARGS";
	case DICT_INTERNAL_INCONSISTENCY:
		return "DICT_INTERNAL_INCONSISTENCY";
	case DICT_MALLOC_FAILED:
		return "DICT_MALLOC_FAILED";
	default:
		return "DICT UNKNOWN ERROR";
	}
}
#endif

int myAtoi(char *str)
{

	// VARIABLES
	int res = 0; // Initialize result

	// CODE START
#if DEBUG_LEVEL > 1
	APP_LOG(APP_LOG_LEVEL_INFO, "MYATOI: ENTER CODE");
#endif
	// Iterate through all characters of input string and update result
	for (int i = 0; str[i] != '\0'; ++i)
	{

#if DEBUG_LEVEL > 2
		APP_LOG(APP_LOG_LEVEL_DEBUG, "MYATOI, STRING IN: %s", &str[i] );
#endif

		if ( (str[i] >= ('0')) && (str[i] <= ('9')) )
		{
			res = res*10 + str[i] - '0';
		}
		//APP_LOG(APP_LOG_LEVEL_DEBUG, "MYATOI, FOR RESULT OUT: %i", res );
	}
#if DEBUG_LEVEL > 1
	APP_LOG(APP_LOG_LEVEL_DEBUG, "MYATOI, FINAL RESULT OUT: %i", res );
#endif
	return res;
} // end myAtoi


int myBGAtoi(char *str)
{

	// VARIABLES
	int res = 0; // Initialize result

	// CODE START

	// If we have the "???" special value, return 0
	if (strcmp(str, "???") == 0) return res;
	if (strcmp(str, "LOW") == 0) return 13;
	if (strcmp(str, "HIGH") == 0) return 410;

	// initialize currentBG_isMMOL flag
	currentBG_isMMOL = false;
#ifdef DEBUG_LEVEL
	APP_LOG(APP_LOG_LEVEL_INFO, "myBGAtoi, START str is MMOL: %s", str );
#endif
	// Iterate through all characters of input string and update result
	for (int i = 0; str[i] != '\0'; ++i)
	{
#if DEBUG_LEVEL >2
		APP_LOG(APP_LOG_LEVEL_DEBUG, "myBGAtoi, STRING IN: %s", &str[i] );
#endif
		if (str[i] == ('.')||str[i] == (','))
		{
			currentBG_isMMOL = true;
		}
		else if ( (str[i] >= ('0')) && (str[i] <= ('9')) )
		{
			res = res*10 + str[i] - '0';
		}

		//APP_LOG(APP_LOG_LEVEL_DEBUG, "myBGAtoi, FOR RESULT OUT: %i", res );
	}
#ifdef DEBUG_LEVEL
	APP_LOG(APP_LOG_LEVEL_INFO, "myBGAtoi, currentBG is MMOL: %i", currentBG_isMMOL );
	APP_LOG(APP_LOG_LEVEL_INFO, "myBGAtoi, FINAL RESULT OUT: %i", res );
#endif
	return res;
} // end myBGAtoi

static void destroy_null_GBitmap(GBitmap **GBmp_image)
{
	//APP_LOG(APP_LOG_LEVEL_INFO, "DESTROY NULL GBITMAP: ENTER CODE");

	if (*GBmp_image != NULL)
	{
		//APP_LOG(APP_LOG_LEVEL_INFO, "DESTROY NULL GBITMAP: POINTER EXISTS, DESTROY BITMAP IMAGE");
		gbitmap_destroy(*GBmp_image);
		if (*GBmp_image != NULL)
		{
			//APP_LOG(APP_LOG_LEVEL_INFO, "DESTROY NULL GBITMAP: POINTER EXISTS, SET POINTER TO NULL");
			*GBmp_image = NULL;
		}
	}

	//APP_LOG(APP_LOG_LEVEL_INFO, "DESTROY NULL GBITMAP: EXIT CODE");
} // end destroy_null_GBitmap

static void destroy_null_BitmapLayer(BitmapLayer **bmp_layer)
{
	//APP_LOG(APP_LOG_LEVEL_INFO, "DESTROY NULL BITMAP: ENTER CODE");

	if (*bmp_layer != NULL)
	{
		//APP_LOG(APP_LOG_LEVEL_INFO, "DESTROY NULL BITMAP: POINTER EXISTS, DESTROY BITMAP LAYER");
		bitmap_layer_destroy(*bmp_layer);
		if (*bmp_layer != NULL)
		{
			//APP_LOG(APP_LOG_LEVEL_INFO, "DESTROY NULL BITMAP: POINTER EXISTS, SET POINTER TO NULL");
			*bmp_layer = NULL;
		}
	}

	//APP_LOG(APP_LOG_LEVEL_INFO, "DESTROY NULL BITMAP: EXIT CODE");
} // end destroy_null_BitmapLayer

static void destroy_null_TextLayer(TextLayer **txt_layer)
{
	//APP_LOG(APP_LOG_LEVEL_INFO, "DESTROY NULL TEXT LAYER: ENTER CODE");

	if (*txt_layer != NULL)
	{
		//APP_LOG(APP_LOG_LEVEL_INFO, "DESTROY NULL TEXT LAYER: POINTER EXISTS, DESTROY TEXT LAYER");
		text_layer_destroy(*txt_layer);
		if (*txt_layer != NULL)
		{
			//APP_LOG(APP_LOG_LEVEL_INFO, "DESTROY NULL TEXT LAYER: POINTER EXISTS, SET POINTER TO NULL");
			*txt_layer = NULL;
		}
	}
	//APP_LOG(APP_LOG_LEVEL_INFO, "DESTROY NULL TEXT LAYER: EXIT CODE");
} // end destroy_null_TextLayer

static void create_update_bitmap(GBitmap **bmp_image, BitmapLayer *bmp_layer, const int resource_id)
{
	//APP_LOG(APP_LOG_LEVEL_INFO, " CREATE UPDATE BITMAP: ENTER CODE");

	// if bitmap pointer exists, destroy and set to NULL
	destroy_null_GBitmap(bmp_image);

	// create bitmap and pointer
	//APP_LOG(APP_LOG_LEVEL_INFO, " CREATE UPDATE BITMAP: CREATE BITMAP");
	*bmp_image = gbitmap_create_with_resource(resource_id);

	if (*bmp_image == NULL)
	{
		// couldn't create bitmap, return so don't crash
		//APP_LOG(APP_LOG_LEVEL_INFO, " CREATE UPDATE BITMAP: COULDNT CREATE BITMAP, RETURN");
		return;
	}
	else
	{
		// set bitmap
		//APP_LOG(APP_LOG_LEVEL_INFO, " CREATE UPDATE BITMAP: SET BITMAP");
		bitmap_layer_set_bitmap(bmp_layer, *bmp_image);
	}
	//APP_LOG(APP_LOG_LEVEL_INFO, " CREATE UPDATE BITMAP: EXIT CODE");
} // end create_update_bitmap


// battery_handler - updates the pebble battery percentage.
static void battery_handler(BatteryChargeState charge_state)
{

	static char watch_battlevel_percent[9];
#ifdef PBL_COLOR
	snprintf(watch_battlevel_percent, BATTLEVEL_FORMATTED_SIZE, "W:%i%% ", charge_state.charge_percent);
#else
	snprintf(watch_battlevel_percent, BATTLEVEL_FORMATTED_SIZE, "W:%i%%", charge_state.charge_percent);
#endif
	if(BacklightOnCharge)
	{
		if(charge_state.is_plugged)
		{
			light_enable(true);
		}
		else
		{
			light_enable(false);
		}
	}
	else
	{
		light_enable(false);
	}

	if(charge_state.is_charging)
	{
#ifdef DEBUG_LEVEL
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Charging.  BacklightOnCharge:%u", BacklightOnCharge);
#endif
#ifdef PBL_COLOR
		//APP_LOG(APP_LOG_LEVEL_INFO, "COLOR DETECTED");
		text_layer_set_text_color(watch_battlevel_layer, GColorDukeBlue);
		text_layer_set_background_color(watch_battlevel_layer, GColorGreen);
#else
		//APP_LOG(APP_LOG_LEVEL_INFO, "BW DETECTED");
		text_layer_set_text_color(watch_battlevel_layer, GColorBlack);
		text_layer_set_background_color(watch_battlevel_layer, GColorWhite);
#endif
	}
	else
	{
#ifdef DEBUG_LEVEL
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Not Charging.  BacklightOnCharge:%u", BacklightOnCharge);
#endif
#ifdef PBL_COLOR
		//APP_LOG(APP_LOG_LEVEL_INFO, "COLOR DETECTED");
		if(charge_state.charge_percent > 40)
		{
			//APP_LOG(APP_LOG_LEVEL_INFO, "BATTERY > 40");
			text_layer_set_text_color(watch_battlevel_layer, GColorGreen);
		}
		else if (charge_state.charge_percent > 20)
		{
			//APP_LOG(APP_LOG_LEVEL_INFO, "BATTERY > 20");
			text_layer_set_text_color(watch_battlevel_layer, GColorYellow);
		}
		else
		{
			//APP_LOG(APP_LOG_LEVEL_INFO, "BATTERY <= 20");
			text_layer_set_text_color(watch_battlevel_layer, GColorRed);
		}
		text_layer_set_background_color(watch_battlevel_layer, GColorClear);
#else
		//APP_LOG(APP_LOG_LEVEL_INFO, "BW DETECTED");
		text_layer_set_text_color(watch_battlevel_layer, GColorWhite);
		text_layer_set_background_color(watch_battlevel_layer, GColorBlack);
#endif
	}
	text_layer_set_text(watch_battlevel_layer, watch_battlevel_percent);


} // end battery_handler

static void alert_handler_cgm(uint8_t alertValue)
{
	//APP_LOG(APP_LOG_LEVEL_INFO, "ALERT HANDLER");
#ifdef DEBUG_LEVEL
	APP_LOG(APP_LOG_LEVEL_DEBUG, "ALERT CODE: %d", alertValue);
#endif
	// CONSTANTS
	// constants for vibrations patterns; has to be uint32_t, measured in ms, maximum duration 10000ms
	// Vibe pattern: ON, OFF, ON, OFF; ON for 500ms, OFF for 100ms, ON for 100ms;

	// CURRENT PATTERNS
	const uint32_t highalert_fast[] = { 300,100,50,100,300,100,50,100,300,100,50,100,300,100,50,100,300,100,50,100,300,100,50,100,300,100,50,100,300,100,50,100,300 };
	const uint32_t medalert_long[] = { 500,100,100,100,500,100,100,100,500,100,100,100,500,100,100,100,500 };
	const uint32_t lowalert_beebuzz[] = { 75,50,50,50,75,50,50,50,75,50,50,50,75,50,50,50,75,50,50,50,75,50,50,50,75 };

	// PATTERN DURATION
	const uint8_t HIGHALERT_FAST_STRONG = 33;
	const uint8_t HIGHALERT_FAST_SHORT = (33/2);
	const uint8_t MEDALERT_LONG_STRONG = 17;
	const uint8_t MEDALERT_LONG_SHORT = (17/2);
	const uint8_t LOWALERT_BEEBUZZ_STRONG = 25;
	const uint8_t LOWALERT_BEEBUZZ_SHORT = (25/2);


	// CODE START

	if (TurnOffAllVibrations)
	{
		//turn off all vibrations is set, return out here
		return;
	}

	switch (alertValue)
	{

	case 0:
		//No alert
		//Normal (new data, in range, trend okay)
		break;

	case 1:
		;
		//Low
#ifdef DEBUG_LEVEL
		APP_LOG(APP_LOG_LEVEL_INFO, "ALERT HANDLER: LOW ALERT");
#endif
		VibePattern low_alert_pat =
		{
				.durations = lowalert_beebuzz,
				.num_segments = LOWALERT_BEEBUZZ_STRONG,
		};
		if (TurnOffStrongVibrations)
		{
			low_alert_pat.num_segments = LOWALERT_BEEBUZZ_SHORT;
		};
		vibes_enqueue_custom_pattern(low_alert_pat);
		break;

	case 2:
		;
		// Medium Alert
#ifdef DEBUG_LEVEL
		APP_LOG(APP_LOG_LEVEL_INFO, "ALERT HANDLER: MEDIUM ALERT");
#endif
		VibePattern med_alert_pat =
		{
				.durations = medalert_long,
				.num_segments = MEDALERT_LONG_STRONG,
		};
		if (TurnOffStrongVibrations)
		{
			med_alert_pat.num_segments = MEDALERT_LONG_SHORT;
		};
		vibes_enqueue_custom_pattern(med_alert_pat);
		break;

	case 3:
		;
		// High Alert
#ifdef DEBUG_LEVEL
		APP_LOG(APP_LOG_LEVEL_INFO, "ALERT HANDLER: HIGH ALERT");
#endif
		VibePattern high_alert_pat =
		{
				.durations = highalert_fast,
				.num_segments = HIGHALERT_FAST_STRONG,
		};
		if (TurnOffStrongVibrations)
		{
			high_alert_pat.num_segments = HIGHALERT_FAST_SHORT;
		};
		vibes_enqueue_custom_pattern(high_alert_pat);
		break;

	} // switch alertValue

} // end alert_handler_cgm

void BT_timer_callback(void *data);

void handle_bluetooth_cgm(bool bt_connected)
{
	//APP_LOG(APP_LOG_LEVEL_INFO, "HANDLE BT: ENTER CODE");

	if (bt_connected == false)
	{

		// Check BluetoothAlert for extended Bluetooth outage; if so, do nothing
		if (BluetoothAlert)
		{
			//Already vibrated and set message; out
			return;
		}

		// Check to see if the BT_timer needs to be set; if BT_timer is not null we're still waiting
		if (BT_timer == NULL)
		{
			// check to see if timer has popped
			if (!BT_timer_pop)
			{
				//set timer
				BT_timer = app_timer_register((BT_ALERT_WAIT_SECS*MS_IN_A_SECOND), BT_timer_callback, NULL);
				// have set timer; next time we come through we will see that the timer has popped
				return;
			}
		}
		else
		{
			// BT_timer is not null and we're still waiting
			return;
		}

		// timer has popped
		// Vibrate; BluetoothAlert takes over until Bluetooth connection comes back on
#ifdef DEBUG_LEVEL
		APP_LOG(APP_LOG_LEVEL_INFO, "BT HANDLER: TIMER POP, NO BLUETOOTH");
#endif
		alert_handler_cgm(BTOUT_VIBE);
		BluetoothAlert = true;

		// Reset timer pop
		if(vibe_repeat)
		{
			BT_timer_pop = false;
		}

		//APP_LOG(APP_LOG_LEVEL_INFO, "NO BLUETOOTH");
		if (!TurnOff_NOBLUETOOTH_Msg)
		{
#ifdef PBL_COLOR
			text_layer_set_text_color(delta_layer, GColorRed);
#endif
			text_layer_set_text(delta_layer, "NO BLUETOOTH");
		}

		// erase cgm and app ago times
		text_layer_set_text(cgmtime_layer, "");
	}

	else
	{
		// Bluetooth is on, reset BluetoothAlert
		//APP_LOG(APP_LOG_LEVEL_INFO, "HANDLE BT: BLUETOOTH ON");
		BluetoothAlert = false;
		if (BT_timer == NULL)
		{
			// no timer is set, so need to reset timer pop
			BT_timer_pop = false;
		}
#ifdef PBL_COLOR
		text_layer_set_text_color(delta_layer, fg_colour);
#endif

	}

	//APP_LOG(APP_LOG_LEVEL_INFO, "BluetoothAlert: %i", BluetoothAlert);
} // end handle_bluetooth_cgm

void BT_timer_callback(void *data)
{
	//APP_LOG(APP_LOG_LEVEL_INFO, "BT TIMER CALLBACK: ENTER CODE");

	// reset timer pop and timer
	BT_timer_pop = true;
	if (BT_timer != NULL)
	{
		BT_timer = NULL;
	}

	// check bluetooth and call handler
	bluetooth_connected_cgm = bluetooth_connection_service_peek();
	handle_bluetooth_cgm(bluetooth_connected_cgm);

} // end BT_timer_callback

static void draw_date_from_app()
{

	// VARIABLES
	time_t d_app = time(NULL);
	struct tm *current_d_app = localtime(&d_app);
	size_t draw_return = 0;

	// CODE START

	// format current date from app
	//if (strcmp(time_watch_text, "00:00") == 0)
	//	{
	draw_return = strftime(time_watch_text, TIME_TEXTBUFF_SIZE, time_watch_format , current_d_app);
	if (draw_return != 0)
	{
		text_layer_set_text(time_watch_layer, time_watch_text);
	}
	//	}

	draw_return = strftime(date_app_text, DATE_TEXTBUFF_SIZE, "%a %d %b", current_d_app);
	if (draw_return != 0)
	{
		text_layer_set_text(date_app_layer, date_app_text);
	}

} // end draw_date_from_app

void sync_error_callback_cgm(DictionaryResult appsync_dict_error, AppMessageResult appsync_error, void *context)
{

	// VARIABLES
	DictionaryIterator *iter = NULL;
	AppMessageResult appsync_err_openerr = APP_MSG_OK;
	AppMessageResult appsync_err_senderr = APP_MSG_OK;

	// CODE START

	// APPSYNC ERROR debug logs
#ifdef DEBUG_LEVEL
	APP_LOG(APP_LOG_LEVEL_INFO, "APP SYNC ERROR");
	APP_LOG(APP_LOG_LEVEL_DEBUG, "APP SYNC MSG ERR CODE: %i RES: %s", appsync_error, translate_app_error(appsync_error));
	APP_LOG(APP_LOG_LEVEL_DEBUG, "APP SYNC DICT ERR CODE: %i RES: %s", appsync_dict_error, translate_dict_error(appsync_dict_error));
#endif

	bluetooth_connected_cgm = bluetooth_connection_service_peek();

	if (!bluetooth_connected_cgm)
	{
		// bluetooth is out, BT message already set; return out
		return;
	}

	appsync_err_openerr = app_message_outbox_begin(&iter);

#ifdef DEBUG_LEVEL
	APP_LOG(APP_LOG_LEVEL_DEBUG, "APP SYNC OPEN ERR CODE: %i RES: %s", appsync_err_openerr, translate_app_error(appsync_err_openerr));
#endif

	if (appsync_err_openerr == APP_MSG_OK)
	{
		// reset AppSyncErrAlert to flag for vibrate
		AppSyncErrAlert = false;

		// send message
		appsync_err_senderr = app_message_outbox_send();
		//APP_LOG(APP_LOG_LEVEL_DEBUG, "APP SYNC SEND ERR CODE: %i RES: %s", appsync_err_senderr, translate_app_error(appsync_err_senderr));
		if (appsync_err_senderr != APP_MSG_OK  && appsync_err_senderr != APP_MSG_BUSY && appsync_err_senderr != APP_MSG_SEND_REJECTED)
		{
#ifdef DEBUG_LEVEL
			APP_LOG(APP_LOG_LEVEL_INFO, "APP SYNC SEND ERROR");
			APP_LOG(APP_LOG_LEVEL_DEBUG, "APP SYNC SEND ERR CODE: %i RES: %s", appsync_err_senderr, translate_app_error(appsync_err_senderr));
#endif
		}
		else
		{
			return;
		}
	}
#if DEBUG_LEVEL > 1
	APP_LOG(APP_LOG_LEVEL_INFO, "APP SYNC RESEND ERROR");
	APP_LOG(APP_LOG_LEVEL_DEBUG, "APP SYNC RESEND ERR CODE: %i RES: %s", appsync_err_openerr, translate_app_error(appsync_err_openerr));
	APP_LOG(APP_LOG_LEVEL_DEBUG, "AppSyncErrAlert:	%i", AppSyncErrAlert);
#endif
	bluetooth_connected_cgm = bluetooth_connection_service_peek();

	if (!bluetooth_connected_cgm || appsync_err_openerr == APP_MSG_BUSY)
	{
		// bluetooth is out, BT message already set; return out
		return;
	}

	// set message to RESTART WATCH -> PHONE
#ifdef DEBUG
	text_layer_set_text(delta_layer, translate_app_error(appsync_err_openerr));
#else
	text_layer_set_text(delta_layer, "RSTRT WCH/PH");
#endif

	// erase cgm and app ago times
	text_layer_set_text(cgmtime_layer, "");
	//text_layer_set_text(time_app_layer, "");

	// erase cgm icon
	//create_update_bitmap(&cgmicon_bitmap,cgmicon_layer,TIMEAGO_ICONS[NONE_TIMEAGO_ICON_INDX]);

	// turn phone icon off
	//create_update_bitmap(&appicon_bitmap,appicon_layer,TIMEAGO_ICONS[PHONEOFF_ICON_INDX]);

	// check if need to vibrate
	if (!AppSyncErrAlert)
	{
#ifdef DEBUG_LEVEL
		APP_LOG(APP_LOG_LEVEL_INFO, "APPSYNC ERROR: VIBRATE");
#endif
		alert_handler_cgm(APPSYNC_ERR_VIBE);
		AppSyncErrAlert = true;
	}

} // end sync_error_callback_cgm

void inbox_dropped_handler_cgm(AppMessageResult appmsg_indrop_error, void *context)
{
	// incoming appmessage send back from Pebble app dropped; no data received

	// VARIABLES
	DictionaryIterator *iter = NULL;
	AppMessageResult appmsg_indrop_openerr = APP_MSG_OK;
	AppMessageResult appmsg_indrop_senderr = APP_MSG_OK;

	// CODE START

	// APPMSG IN DROP debug logs
#ifdef DEBUG_LEVEL
	APP_LOG(APP_LOG_LEVEL_INFO, "APPMSG IN DROP ERROR");
	APP_LOG(APP_LOG_LEVEL_DEBUG, "APPMSG IN DROP ERR CODE: %i RES: %s", appmsg_indrop_error, translate_app_error(appmsg_indrop_error));
#endif
	bluetooth_connected_cgm = bluetooth_connection_service_peek();

	if (!bluetooth_connected_cgm)
	{
		// bluetooth is out, BT message already set; return out
		return;
	}

	appmsg_indrop_openerr = app_message_outbox_begin(&iter);

	if (appmsg_indrop_openerr == APP_MSG_OK )
	{
		// reset AppMsgInDropAlert to flag for vibrate
		AppMsgInDropAlert = false;

		// send message
		appmsg_indrop_senderr = app_message_outbox_send();
		if (appmsg_indrop_senderr != APP_MSG_OK || appmsg_indrop_senderr == APP_MSG_BUSY || appmsg_indrop_senderr == APP_MSG_SEND_REJECTED)
		{
#ifdef DEBUG_LEVEL
			APP_LOG(APP_LOG_LEVEL_INFO, "APPMSG IN DROP SEND ERROR");
			APP_LOG(APP_LOG_LEVEL_DEBUG, "APPMSG IN DROP SEND ERR CODE: %i RES: %s", appmsg_indrop_senderr, translate_app_error(appmsg_indrop_senderr));
#endif
		}
		else
		{
			return;
		}
	}
#ifdef DEBUG_LEVEL
	APP_LOG(APP_LOG_LEVEL_INFO, "APPMSG IN DROP RESEND ERROR");
	APP_LOG(APP_LOG_LEVEL_DEBUG, "APPMSG IN DROP RESEND ERR CODE: %i RES: %s", appmsg_indrop_openerr, translate_app_error(appmsg_indrop_openerr));
	APP_LOG(APP_LOG_LEVEL_DEBUG, "AppMsgInDropAlert:	%i", AppMsgInDropAlert);
#endif

	bluetooth_connected_cgm = bluetooth_connection_service_peek();

	if (!bluetooth_connected_cgm)
	{
		// bluetooth is out, BT message already set; return out
		return;
	}

	// set message to RESTART WATCH -> PHONE
	text_layer_set_text(delta_layer, "RSTRT WCH/PHN");

	// erase cgm and app ago times
	text_layer_set_text(cgmtime_layer, "");
	//text_layer_set_text(time_app_layer, "");

	// erase cgm icon
	//create_update_bitmap(&cgmicon_bitmap,cgmicon_layer,TIMEAGO_ICONS[NONE_TIMEAGO_ICON_INDX]);

	// turn phone icon off
	//create_update_bitmap(&appicon_bitmap,appicon_layer,TIMEAGO_ICONS[PHONEOFF_ICON_INDX]);

	// check if need to vibrate
	if (!AppMsgInDropAlert)
	{
#ifdef DEBUG_LEVEL
		APP_LOG(APP_LOG_LEVEL_INFO, "APPMSG IN DROP ERROR: VIBRATE");
#endif
		alert_handler_cgm(APPMSG_INDROP_VIBE);
		AppMsgInDropAlert = true;
	}

} // end inbox_dropped_handler_cgm

void outbox_failed_handler_cgm(DictionaryIterator *failed, AppMessageResult appmsg_outfail_error, void *context)
{
	// outgoing appmessage send failed to deliver to Pebble

	// VARIABLES
	DictionaryIterator *iter = NULL;
	AppMessageResult appmsg_outfail_openerr = APP_MSG_OK;
	AppMessageResult appmsg_outfail_senderr = APP_MSG_OK;

	// CODE START

	// APPMSG OUT FAIL debug logs
#ifdef DEBUG_LEVEL
	APP_LOG(APP_LOG_LEVEL_INFO, "APPMSG OUT FAIL ERROR");
	APP_LOG(APP_LOG_LEVEL_DEBUG, "APPMSG OUT FAIL ERR CODE: %i RES: %s", appmsg_outfail_error, translate_app_error(appmsg_outfail_error));
#endif

	bluetooth_connected_cgm = bluetooth_connection_service_peek();

	if (!bluetooth_connected_cgm)
	{
		// bluetooth is out, BT message already set; return out
		return;
	}

	appmsg_outfail_openerr = app_message_outbox_begin(&iter);

	if (appmsg_outfail_openerr == APP_MSG_OK)
	{
		// reset AppMsgOutFailAlert to flag for vibrate
		AppMsgOutFailAlert = false;

		// send message
		return;
	}

#ifdef DEBUG_LEVEL
	APP_LOG(APP_LOG_LEVEL_INFO, "APPMSG OUT FAIL RESEND ERROR");
	APP_LOG(APP_LOG_LEVEL_DEBUG, "APPMSG OUT FAIL RESEND ERR CODE: %i RES: %s", appmsg_outfail_openerr, translate_app_error(appmsg_outfail_openerr));
	APP_LOG(APP_LOG_LEVEL_DEBUG, "AppMsgOutFailAlert:	%i", AppMsgOutFailAlert);
#endif

	bluetooth_connected_cgm = bluetooth_connection_service_peek();

	if (!bluetooth_connected_cgm || appmsg_outfail_senderr != APP_MSG_SEND_REJECTED)
	{
		// bluetooth is out, BT message already set; return out
		return;
	}

	// set message to RESTART WATCH -> PHONE
#ifdef DEBUG
	text_layer_set_text(delta_layer, translate_app_error(appmsg_outfail_openerr));
#else
	text_layer_set_text(delta_layer, "RSTRT WCH/PH");
#endif

	// erase cgm and app ago times
	text_layer_set_text(cgmtime_layer, "");
	//text_layer_set_text(time_app_layer, "");

	// check if need to vibrate
	if (!AppMsgOutFailAlert)
	{
#ifdef DEBUG_LEVEL
		APP_LOG(APP_LOG_LEVEL_INFO, "APPMSG OUT FAIL ERROR: VIBRATE");
#endif
		alert_handler_cgm(APPMSG_OUTFAIL_VIBE);
		AppMsgOutFailAlert = true;
	}

} // end outbox_failed_handler_cgm

static void load_icon()
{
	//APP_LOG(APP_LOG_LEVEL_INFO, "LOAD ICON ARROW FUNCTION START");

	// CONSTANTS

	// ICON ASSIGNMENTS OF ARROW DIRECTIONS
	const char NO_ARROW[] = "0";
	const char DOUBLEUP_ARROW[] = "1";
	const char SINGLEUP_ARROW[] = "2";
	const char UP45_ARROW[] = "3";
	const char FLAT_ARROW[] = "4";
	const char DOWN45_ARROW[] = "5";
	const char SINGLEDOWN_ARROW[] = "6";
	const char DOUBLEDOWN_ARROW[] = "7";
	const char NOTCOMPUTE_ICON[] = "8";
	const char OUTOFRANGE_ICON[] = "9";

	// ARRAY OF ARROW ICON IMAGES
	const uint8_t ARROW_ICONS[] =
	{
			RESOURCE_ID_IMAGE_NONE,		 //0
			RESOURCE_ID_IMAGE_UPUP,		 //1
			RESOURCE_ID_IMAGE_UP,			 //2
			RESOURCE_ID_IMAGE_UP45,		 //3
			RESOURCE_ID_IMAGE_FLAT,		 //4
			RESOURCE_ID_IMAGE_DOWN45,	 //5
			RESOURCE_ID_IMAGE_DOWN,		 //6
			RESOURCE_ID_IMAGE_DOWNDOWN, //7
			RESOURCE_ID_IMAGE_LOGO,		 //8
			RESOURCE_ID_IMAGE_ERR			 //9
	};

	// INDEX FOR ARRAY OF ARROW ICON IMAGES
	const uint8_t NONE_ARROW_ICON_INDX = 0;
	const uint8_t UPUP_ICON_INDX = 1;
	const uint8_t UP_ICON_INDX = 2;
	const uint8_t UP45_ICON_INDX = 3;
	const uint8_t FLAT_ICON_INDX = 4;
	const uint8_t DOWN45_ICON_INDX = 5;
	const uint8_t DOWN_ICON_INDX = 6;
	const uint8_t DOWNDOWN_ICON_INDX = 7;
	const uint8_t LOGO_ARROW_ICON_INDX = 8;
	const uint8_t ERR_ARROW_ICON_INDX = 9;

	//APP_LOG(APP_LOG_LEVEL_INFO, "LOAD ARROW ICON, BEFORE CHECK SPEC VALUE BITMAP");
	// check if special value set
	if (specvalue_alert == false)
	{

		// no special value, set arrow
		// check for arrow direction, set proper arrow icon
		//APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD ICON, CURRENT ICON: %s", current_icon);
		if ( (strcmp(current_icon, NO_ARROW) == 0) || (strcmp(current_icon, NOTCOMPUTE_ICON) == 0) || (strcmp(current_icon, OUTOFRANGE_ICON) == 0) )
		{
			create_update_bitmap(&icon_bitmap,icon_layer,ARROW_ICONS[NONE_ARROW_ICON_INDX]);
			DoubleDownAlert = false;
		}
		else if (strcmp(current_icon, DOUBLEUP_ARROW) == 0)
		{
			create_update_bitmap(&icon_bitmap,icon_layer,ARROW_ICONS[UPUP_ICON_INDX]);
			DoubleDownAlert = false;
		}
		else if (strcmp(current_icon, SINGLEUP_ARROW) == 0)
		{
			create_update_bitmap(&icon_bitmap,icon_layer,ARROW_ICONS[UP_ICON_INDX]);
			DoubleDownAlert = false;
		}
		else if (strcmp(current_icon, UP45_ARROW) == 0)
		{
			create_update_bitmap(&icon_bitmap,icon_layer,ARROW_ICONS[UP45_ICON_INDX]);
			DoubleDownAlert = false;
		}
		else if (strcmp(current_icon, FLAT_ARROW) == 0)
		{
			create_update_bitmap(&icon_bitmap,icon_layer,ARROW_ICONS[FLAT_ICON_INDX]);
			DoubleDownAlert = false;
		}
		else if (strcmp(current_icon, DOWN45_ARROW) == 0)
		{
			create_update_bitmap(&icon_bitmap,icon_layer,ARROW_ICONS[DOWN45_ICON_INDX]);
			DoubleDownAlert = false;
		}
		else if (strcmp(current_icon, SINGLEDOWN_ARROW) == 0)
		{
			create_update_bitmap(&icon_bitmap,icon_layer,ARROW_ICONS[DOWN_ICON_INDX]);
			DoubleDownAlert = false;
		}
		else if (strcmp(current_icon, DOUBLEDOWN_ARROW) == 0)
		{
			/*				if (!DoubleDownAlert) {
										APP_LOG(APP_LOG_LEVEL_INFO, "LOAD ICON, ICON ARROW: DOUBLE DOWN");
										alert_handler_cgm(DOUBLEDOWN_VIBE);
										DoubleDownAlert = true;
									}
			 */				create_update_bitmap(&icon_bitmap,icon_layer,ARROW_ICONS[DOWNDOWN_ICON_INDX]);
		}
		else
		{
			// check for special cases and set icon accordingly
			// check bluetooth
			bluetooth_connected_cgm = bluetooth_connection_service_peek();

			// check to see if we are in the loading screen
			if (!bluetooth_connected_cgm)
			{
				// Bluetooth is out; in the loading screen so set logo
				create_update_bitmap(&icon_bitmap,icon_layer,ARROW_ICONS[LOGO_ARROW_ICON_INDX]);
			}
			else
			{
				// unexpected, set error icon
				create_update_bitmap(&icon_bitmap,icon_layer,ARROW_ICONS[ERR_ARROW_ICON_INDX]);
			}
			DoubleDownAlert = false;
		}
	} // if specvalue_alert == false
	else   // this is just for log when need it
	{
		//APP_LOG(APP_LOG_LEVEL_INFO, "LOAD ICON, SPEC VALUE ALERT IS TRUE, DONE");
	} // else specvalue_alert == true

} // end load_icon

static void load_bg()
{
	//APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, FUNCTION START");

	// CONSTANTS

	// ARRAY OF BG CONSTANTS; MGDL
	/*	uint16_t BG_MGDL[] = {
			SPECVALUE_BG_MGDL,	//0
			SHOWLOW_BG_MGDL,	//1
			SHOWHIGH_BG_MGDL	//2
		};

		// ARRAY OF BG CONSTANTS; MMOL
		uint16_t BG_MMOL[] = {
			SPECVALUE_BG_MMOL,	//0
			SHOWLOW_BG_MMOL,	//1
			SHOWHIGH_BG_MMOL	//2
		};
	 */
	// INDEX FOR ARRAYS OF BG CONSTANTS
	/*	const uint8_t SPECVALUE_BG_INDX = 0;
		const uint8_t SHOWLOW_BG_INDX = 1;
		const uint8_t SHOWHIGH_BG_INDX = 2;
	 */
	// MG/DL SPECIAL VALUE CONSTANTS ACTUAL VALUES
	// mg/dL = mmol / .0555 OR mg/dL = mmol * 18.0182
	/*	const uint8_t SENSOR_NOT_ACTIVE_VALUE_MGDL = 1;		// show stop light, ?SN
		const uint8_t MINIMAL_DEVIATION_VALUE_MGDL = 2; 		// show stop light, ?MD
		const uint8_t NO_ANTENNA_VALUE_MGDL = 3; 			// show broken antenna, ?NA
		const uint8_t SENSOR_NOT_CALIBRATED_VALUE_MGDL = 5;	// show blood drop, ?NC
		const uint8_t STOP_LIGHT_VALUE_MGDL = 6;				// show stop light, ?CD
		const uint8_t HOURGLASS_VALUE_MGDL = 9;				// show hourglass, hourglass
		const uint8_t QUESTION_MARKS_VALUE_MGDL = 10;		// show ???, ???
		const uint8_t BAD_RF_VALUE_MGDL = 12;				// show broken antenna, ?RF

		// MMOL SPECIAL VALUE CONSTANTS ACTUAL VALUES
		// mmol = mg/dL / 18.0182 OR mmol = mg/dL * .0555
		const uint8_t SENSOR_NOT_ACTIVE_VALUE_MMOL = 1;		// show stop light, ?SN (.06 -> .1)
		const uint8_t MINIMAL_DEVIATION_VALUE_MMOL = 1;		// show stop light, ?MD (.11 -> .1)
		const uint8_t NO_ANTENNA_VALUE_MMOL = 2;				// show broken antenna, ?NA (.17 -> .2)
		const uint8_t SENSOR_NOT_CALIBRATED_VALUE_MMOL = 3;	// show blood drop, ?NC (.28 -> .3)
		const uint8_t STOP_LIGHT_VALUE_MMOL = 4;				// show stop light, ?CD (.33 -> .3, set to .4 here)
		const uint8_t HOURGLASS_VALUE_MMOL = 5;				// show hourglass, hourglass (.50 -> .5)
		const uint8_t QUESTION_MARKS_VALUE_MMOL = 6;			// show ???, ??? (.56 -> .6)
		const uint8_t BAD_RF_VALUE_MMOL = 7;					// show broken antenna, ?RF (.67 -> .7)
	 */
#define SENSOR_NOT_ACTIVE_VALUE "?SN"
#define MINIMAL_DEVIATION_VALUE	"?MD"
#define NO_ANTENNA_VALUE "?NA"
#define SENSOR_NOT_CALIBRATED_VALUE "?NC"
#define STOP_LIGHT_VALUE "?CD"
#define HOURGLASS_VALUE "hourglass"
#define QUESTION_MARKS_VALUE "???"
#define BAD_RF_VALUE "?RF"
	// ARRAY OF SPECIAL VALUES CONSTANTS; MGDL
	/*	static uint8_t SPECVALUE[] = {
			SENSOR_NOT_ACTIVE_VALUE,		//0
			MINIMAL_DEVIATION_VALUE,		//1
			NO_ANTENNA_VALUE,			//2
			SENSOR_NOT_CALIBRATED_VALUE,		//3
			STOP_LIGHT_VALUE,			//4
			HOURGLASS_VALUE,			//5
			QUESTION_MARKS_VALUE,			//6
			BAD_RF_VALUE				//7
		};
	 */
	// INDEX FOR ARRAYS OF SPECIAL VALUES CONSTANTS
	/*	const uint8_t SENSOR_NOT_ACTIVE_VALUE_INDX = 0;
		const uint8_t MINIMAL_DEVIATION_VALUE_INDX = 1;
		const uint8_t NO_ANTENNA_VALUE_INDX = 2;
		const uint8_t SENSOR_NOT_CALIBRATED_VALUE_INDX = 3;
		const uint8_t STOP_LIGHT_VALUE_INDX = 4;
		const uint8_t HOURGLASS_VALUE_INDX = 5;
		const uint8_t QUESTION_MARKS_VALUE_INDX = 6;
		const uint8_t BAD_RF_VALUE_INDX = 7;
	 */
	// VARIABLES

	// pointers to be used to MGDL or MMOL values for parsing

	// CODE START

	// if special value set, erase anything in the icon field
	if (specvalue_alert == true)
	{
		create_update_bitmap(&specialvalue_bitmap,icon_layer,SPECIAL_VALUE_ICONS[NONE_SPECVALUE_ICON_INDX]);
	}

	// set special value alert to false no matter what
	specvalue_alert = false;

#if DEBUG_LEVEL > 1
	APP_LOG(APP_LOG_LEVEL_DEBUG, "LAST BG: %s", last_bg);
	//APP_LOG(APP_LOG_LEVEL_DEBUG, "CURRENT BG: %i", current_bg);
#endif

	// BG parse, check snooze, and set text

	// check for init code or error code
	if (last_bg[0] == '-')
	{
		lastAlertTime = 0;

		// check bluetooth
		bluetooth_connected_cgm = bluetooth_connection_service_peek();

		if (!bluetooth_connected_cgm)
		{
			// Bluetooth is out; set BT message
			//APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, BG INIT: NO BT, SET NO BT MESSAGE");
			if (!TurnOff_NOBLUETOOTH_Msg)
			{
				text_layer_set_text(delta_layer, "NO BLUETOOTH");
			} // if turnoff nobluetooth msg
		}// if !bluetooth connected
		else
		{
			// if init code, we will set it right in message layer
			//APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, UNEXPECTED BG: SET ERR ICON");
			//APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD BG, UNEXP BG, CURRENT_BG: %d LAST_BG: %s ", current_bg, last_bg);
			text_layer_set_text(bg_layer, "ERR");
			create_update_bitmap(&icon_bitmap,icon_layer,SPECIAL_VALUE_ICONS[NONE_SPECVALUE_ICON_INDX]);
			specvalue_alert = true;
		}

	} // if current_bg <= 0

	else
	{
		// valid BG
		// check for special value, if special value, then replace icon and blank BG; else send current BG
		//APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, BEFORE CREATE SPEC VALUE BITMAP");
		if (strcmp(last_bg, NO_ANTENNA_VALUE) == 0 || strcmp(last_bg, BAD_RF_VALUE) == 0)
		{
			//APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, SPECIAL VALUE: SET BROKEN ANTENNA");
			text_layer_set_text(bg_layer, "");
			create_update_bitmap(&specialvalue_bitmap,icon_layer, SPECIAL_VALUE_ICONS[BROKEN_ANTENNA_ICON_INDX]);
			specvalue_alert = true;
		}
		else if (strcmp(last_bg, SENSOR_NOT_CALIBRATED_VALUE) == 0)
		{
			//APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, SPECIAL VALUE: SET BLOOD DROP");
			text_layer_set_text(bg_layer, "");
			create_update_bitmap(&specialvalue_bitmap,icon_layer,SPECIAL_VALUE_ICONS[BLOOD_DROP_ICON_INDX]);
			specvalue_alert = true;
		}
		else if (strcmp(last_bg, SENSOR_NOT_ACTIVE_VALUE) == 0 || strcmp(last_bg, MINIMAL_DEVIATION_VALUE) == 0
				|| strcmp(last_bg, STOP_LIGHT_VALUE) == 0)
		{
			//APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, SPECIAL VALUE: SET STOP LIGHT");
			text_layer_set_text(bg_layer, "");
			create_update_bitmap(&specialvalue_bitmap,icon_layer,SPECIAL_VALUE_ICONS[STOP_LIGHT_ICON_INDX]);
			specvalue_alert = true;
		}
		else if (strcmp(last_bg, HOURGLASS_VALUE) == 0)
		{
			//APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, SPECIAL VALUE: SET HOUR GLASS");
			text_layer_set_text(bg_layer, "");
			create_update_bitmap(&specialvalue_bitmap,icon_layer,SPECIAL_VALUE_ICONS[HOURGLASS_ICON_INDX]);
			specvalue_alert = true;
		}
		else if (strcmp(last_bg, QUESTION_MARKS_VALUE) == 0)
		{
			//PP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, SPECIAL VALUE: SET QUESTION MARKS, CLEAR TEXT");
			text_layer_set_text(bg_layer, "");
			//APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, SPECIAL VALUE: SET QUESTION MARKS, SET BITMAP");
			create_update_bitmap(&specialvalue_bitmap,icon_layer,SPECIAL_VALUE_ICONS[QUESTION_MARKS_ICON_INDX]);
			//APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, SPECIAL VALUE: SET QUESTION MARKS, DONE");
			specvalue_alert = true;
		}

		//APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, AFTER CREATE SPEC VALUE BITMAP");

		if (specvalue_alert == false)
		{
			// we didn't find a special value, so set BG instead
			// arrow icon already set separately
			//APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD BG, SET TO BG: %s ", last_bg);
			text_layer_set_text(bg_layer, last_bg);
		} // end bg checks (if special_value_bitmap)


	}

	//APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, FUNCTION OUT");
	//APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD BG, FUNCTION OUT, SNOOZE VALUE: %d", lastAlertTime);
#ifdef DEBUG_LEVEL
	APP_LOG(APP_LOG_LEVEL_INFO, "LOAD_BG: bg_layer is \"%s\"", text_layer_get_text(bg_layer));
#endif


} // end load_bg

// Gets the UTC offset of the local time in seconds
// (pass in an existing localtime struct tm to save creating another one, or else pass NULL)
time_t get_UTC_offset(struct tm *t)
{
	if (t == NULL)
	{
		time_t temp;
		temp = time(NULL);
		t = localtime(&temp);
	}

	return t->tm_gmtoff + ((t->tm_isdst > 0) ? 3600 : 0);
}

static void load_cgmtime()
{
	//APP_LOG(APP_LOG_LEVEL_INFO, "LOAD CGMTIME FUNCTION START");

	// VARIABLES
	// NOTE: buffers have to be static and hardcoded
	uint32_t current_cgm_timeago = 0;
	int cgm_timeago_diff = 0;
	static char formatted_cgm_timeago[10];
	static char cgm_label_buffer[6];

	// CODE START

	// initialize label buffer
	strncpy(cgm_label_buffer, "", LABEL_BUFFER_SIZE);

	//APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD CGMTIME, NEW CGM TIME: %s", new_cgm_time);

	if (current_cgm_time == 0)
	{
		// Init code or error code; set text layer & icon to empty value
		//APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD CGMTIME, CGM TIME AGO INIT OR ERROR CODE: %s", cgm_label_buffer);
		text_layer_set_text(cgmtime_layer, "");
		//create_update_bitmap(&cgmicon_bitmap,cgmicon_layer,TIMEAGO_ICONS[NONE_TIMEAGO_ICON_INDX]);
	}
	else
	{
		// set rcvr on icon
		//create_update_bitmap(&cgmicon_bitmap,cgmicon_layer,TIMEAGO_ICONS[RCVRON_ICON_INDX]);

		time_now = time(NULL);
		time_now = abs(time_now + get_UTC_offset(localtime(&time_now)));

		//APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD CGMTIME, CURRENT CGM TIME: %lu", current_cgm_time);
#ifdef DEBUG_LEVEL
		APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD CGMTIME, time_now: %lu, current_cgm_time: %lu", time_now, current_cgm_time);
#endif

		current_cgm_timeago = abs(time_now - current_cgm_time);

		//APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD CGMTIME, CURRENT CGM TIMEAGO: %lu", current_cgm_timeago);

		//APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD CGMTIME, GM TIME AGO LABEL IN: %s", cgm_label_buffer);

		if (current_cgm_timeago < MINUTEAGO)
		{
			cgm_timeago_diff = 0;
			strncpy (formatted_cgm_timeago, "now", TIMEAGO_BUFFER_SIZE);
		}
		else if (current_cgm_timeago < HOURAGO)
		{
			cgm_timeago_diff = (current_cgm_timeago / MINUTEAGO);
			snprintf(formatted_cgm_timeago, TIMEAGO_BUFFER_SIZE, "%i", cgm_timeago_diff);
			strncpy(cgm_label_buffer, "m", LABEL_BUFFER_SIZE);
			strcat(formatted_cgm_timeago, cgm_label_buffer);
		}
		else if (current_cgm_timeago < DAYAGO)
		{
			cgm_timeago_diff = (current_cgm_timeago / HOURAGO);
			snprintf(formatted_cgm_timeago, TIMEAGO_BUFFER_SIZE, "%i", cgm_timeago_diff);
			strncpy(cgm_label_buffer, "h", LABEL_BUFFER_SIZE);
			strcat(formatted_cgm_timeago, cgm_label_buffer);
		}
		else if (current_cgm_timeago < WEEKAGO)
		{
			cgm_timeago_diff = (current_cgm_timeago / DAYAGO);
			snprintf(formatted_cgm_timeago, TIMEAGO_BUFFER_SIZE, "%i", cgm_timeago_diff);
			strncpy(cgm_label_buffer, "d", LABEL_BUFFER_SIZE);
			strcat(formatted_cgm_timeago, cgm_label_buffer);
		}
		else
		{
			strncpy (formatted_cgm_timeago, "---", TIMEAGO_BUFFER_SIZE);
			//create_update_bitmap(&cgmicon_bitmap,cgmicon_layer,TIMEAGO_ICONS[NONE_TIMEAGO_ICON_INDX]);
		}

		text_layer_set_text(cgmtime_layer, formatted_cgm_timeago);
	} // else init code

	//#ifdef DEBUG_LEVEL
	//APP_LOG(APP_LOG_LEVEL_INFO, "LOAD_CGMTIME: time_app_layer is \"%s\"", text_layer_get_text(cgmtime_layer));
	//#endif
	//APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD CGMTIME, CGM TIMEAGO LABEL OUT: %s", cgm_label_buffer);
} // end load_cgmtime

static void load_bg_delta()
{
#ifdef DEBUG_LEVEL
	APP_LOG(APP_LOG_LEVEL_INFO, "BG DELTA FUNCTION START");
#endif
#ifdef DEBUG_LEVEL
	APP_LOG(APP_LOG_LEVEL_INFO, "LOAD_BG_DELTA: current_bg_delta is \"%s\"", current_bg_delta);
#endif


	// CONSTANTS
#define MSGLAYER_BUFFER_SIZE 14
#define BGDELTA_LABEL_SIZE 14
#define BGDELTA_FORMATTED_SIZE 14
	// VARIABLES
	// NOTE: buffers have to be static and hardcoded
	//static char delta_label_buffer[BGDELTA_LABEL_SIZE];
	static char formatted_bg_delta[BGDELTA_FORMATTED_SIZE];

	// CODE START

	// check bluetooth connection
	bluetooth_connected_cgm = bluetooth_connection_service_peek();

	if (!bluetooth_connected_cgm)
	{
		// Bluetooth is out; BT message already set, so return
		return;
	}

	// check for CHECK PHONE condition, if true set message
	if ((PhoneOffAlert) && (!TurnOff_CHECKPHONE_Msg))
	{
		text_layer_set_text(delta_layer, "CHECK PHONE");
		return;
	}

	// check for special messages; if no string, set no message
	if (strcmp(current_bg_delta, "") == 0)
	{
		strncpy(formatted_bg_delta, "", MSGLAYER_BUFFER_SIZE);
		text_layer_set_text(delta_layer, formatted_bg_delta);
		return;
	}


	// check if LOADING.., if true set message
	// put " " (space) in bg field so logo continues to show
	if (strcmp(current_bg_delta, "LOAD") == 0)
	{
#ifdef DEBUG_LEVEL
		APP_LOG(APP_LOG_LEVEL_INFO, "LOAD_BG_DELTA: Found LOAD, current_bg_delta is \"%s\"", current_bg_delta);
#endif

		strncpy(formatted_bg_delta, "LOADING...", MSGLAYER_BUFFER_SIZE);
		text_layer_set_text(delta_layer, formatted_bg_delta);
		text_layer_set_text(bg_layer, " ");
		create_update_bitmap(&icon_bitmap,icon_layer,SPECIAL_VALUE_ICONS[LOGO_SPECVALUE_ICON_INDX]);
		specvalue_alert = false;
		return;
	}

	//check for "--" indicating an indeterminate delta.  Display it.
	if (strcmp(current_bg_delta, "???") == 0)
	{
		strncpy(formatted_bg_delta, current_bg_delta, BGDELTA_FORMATTED_SIZE);
		text_layer_set_text(delta_layer, formatted_bg_delta);
		return;
	}

	//check for "ERR" indicating an indeterminate delta.  Display it.
	if (strcmp(current_bg_delta, "ERR") == 0)
	{
		strncpy(formatted_bg_delta, current_bg_delta, BGDELTA_FORMATTED_SIZE);
		text_layer_set_text(delta_layer, formatted_bg_delta);
		return;
	}

	// Bluetooth is good, Phone is good, CGM connection is good, no special message
	// set delta BG message

#ifdef DEBUG_LEVEL
	//APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD BG DELTA, DELTA STRING: %s", &current_bg_delta[i]);
#endif
	//strcat(formatted_bg_delta, delta_label_buffer);
	strncpy(formatted_bg_delta, current_bg_delta, BGDELTA_FORMATTED_SIZE);

#ifdef DEBUG_LEVEL
	APP_LOG(APP_LOG_LEVEL_INFO, "LOAD_BG_DELTA: All good. Setting \"%s\"", formatted_bg_delta);
#endif

	text_layer_set_text(delta_layer, formatted_bg_delta);
#ifdef PBL_COLOR
	text_layer_set_text_color(delta_layer,fg_colour);
#endif
#ifdef DEBUG_LEVEL
	APP_LOG(APP_LOG_LEVEL_INFO, "LOAD_BG_DELTA: delta_layer is \"%s\"", text_layer_get_text(delta_layer));
#endif

} // end load_bg_delta

static void load_battlevel()
{
	//APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BATTLEVEL, FUNCTION START");

	// CONSTANTS


	// VARIABLES
	// NOTE: buffers have to be static and hardcoded
	int current_battlevel = 0;
	static char battlevel_percent[7];

	// CODE START

#if DEBUG_LEVEL >0
	APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD BATTLEVEL, LAST BATTLEVEL: %s", last_battlevel);
#endif
	if (strcmp(last_battlevel, " ") == 0)
	{
		// Init code or no battery, can't do battery; set text layer & icon to empty value
#if DEBUG_LEVEL > 1
		APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BATTLEVEL, NO BATTERY");
#endif
		text_layer_set_text(battlevel_layer, "");
		LowBatteryAlert = false;
		return;
	}

	if (strcmp(last_battlevel, "0") == 0)
	{
		// Zero battery level; set here, so if we get zero later we know we have an error instead
#if DEBUG_LEVEL > 1
		APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BATTLEVEL, ZERO BATTERY, SET STRING");
#endif
		text_layer_set_text(battlevel_layer, "0%");
		if (!LowBatteryAlert)
		{
#if DEBUG_LEVEL > 1
			APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BATTLEVEL, ZERO BATTERY, VIBRATE");
#endif
			alert_handler_cgm(LOWBATTERY_VIBE);
			LowBatteryAlert = true;
		}
		return;
	}

	current_battlevel = myAtoi(last_battlevel);

#if DEBUG_LEVEL > 1
	APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD BATTLEVEL, CURRENT BATTLEVEL: %i", current_battlevel);
#endif

	if ((current_battlevel <= 0) || (current_battlevel > 100) || (last_battlevel[0] == '-'))
	{
		// got a negative or out of bounds or error battery level
#if DEBUG_LEVEL > 1
		APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BATTLEVEL, UNKNOWN, ERROR BATTERY");
#endif
		text_layer_set_text(battlevel_layer, "ERR");
		return;
	}

	// get current battery level and set battery level text with percent
#ifdef PBL_COLOR
	snprintf(battlevel_percent, BATTLEVEL_FORMATTED_SIZE, " B:%i%%", current_battlevel);
#else
	snprintf(battlevel_percent, BATTLEVEL_FORMATTED_SIZE, "B:%i%%", current_battlevel);
#endif
#ifdef DEBUG_LEVEL
	APP_LOG(APP_LOG_LEVEL_DEBUG, "SETTING BATTLEVEL to %s", battlevel_percent);
#endif
	text_layer_set_text(battlevel_layer, battlevel_percent);
#ifdef PBL_COLOR
	if ( (current_battlevel > 0) && (current_battlevel <= 30) )
	{
		text_layer_set_text_color(battlevel_layer, GColorRed);
		if (!LowBatteryAlert)
		{
#if DEBUG_LEVEL > 1
			APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BATTLEVEL, LOW BATTERY, 5 OR LESS, VIBRATE");
#endif
			alert_handler_cgm(LOWBATTERY_VIBE);
			LowBatteryAlert = true;
		}
	}
	else if ( (current_battlevel > 30) && (current_battlevel <= 50) )
	{
		text_layer_set_text_color(battlevel_layer, GColorYellow);
	}
	else
	{
		text_layer_set_text_color(battlevel_layer, GColorGreen);
	}
#endif
#ifdef DEBUG_LEVEL
	APP_LOG(APP_LOG_LEVEL_INFO, "LOAD_BATTLEVEL: battlevel_layer is \"%s\"", text_layer_get_text(battlevel_layer));
#endif
	//APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BATTLEVEL, END FUNCTION");
} // end load_battlevel

static void send_cmd_cgm(void)
{

	DictionaryIterator *iter = NULL;
#ifdef DEBUG_LEVEL
	APP_LOG(APP_LOG_LEVEL_INFO, "send_cmd_cgm called.");
#endif
	AppMessageResult sendcmd_openerr = APP_MSG_OK;
	AppMessageResult sendcmd_senderr = APP_MSG_OK;

	sendcmd_openerr = app_message_outbox_begin(&iter);
	if(BluetoothAlert)
	{
		//BT is down rignt now, so don't do anything.
		//Note, we cannot log this, as BT must be up in order to log it.
		return;
	}
	if (sendcmd_openerr != APP_MSG_OK)
	{
#ifdef DEBUG_LEVEL
		APP_LOG(APP_LOG_LEVEL_INFO, "WATCH SENDCMD OPEN ERROR");
		APP_LOG(APP_LOG_LEVEL_DEBUG, "WATCH SENDCMD OPEN ERR CODE: %i RES: %s", sendcmd_openerr, translate_app_error(sendcmd_openerr));
#endif
		return;
	}

#ifdef DEBUG_LEVEL
	APP_LOG(APP_LOG_LEVEL_INFO, "SEND CMD, MSG OUTBOX OPEN, NO ERROR, Creating Dictionary.");
#endif

	dict_write_uint32(iter, CGM_SYNC_KEY, CGM_SYNC_KEY);
	dict_write_uint8(iter, PBL_PLATFORM, (uint8_t) PLATFORM);
	dict_write_cstring(iter, PBL_APP_VER, FACE_VERSION);
	dict_write_end(iter);
	//APP_LOG(APP_LOG_LEVEL_INFO, "SEND CMD IN, ABOUT TO OPEN APP MSG OUTBOX");


	sendcmd_senderr = app_message_outbox_send();

	if (sendcmd_senderr != APP_MSG_OK && sendcmd_senderr != APP_MSG_BUSY && sendcmd_senderr != APP_MSG_SEND_REJECTED)
	{
#ifdef DEBUG_LEVEL
		APP_LOG(APP_LOG_LEVEL_INFO, "WATCH SENDCMD SEND ERROR");
		APP_LOG(APP_LOG_LEVEL_DEBUG, "WATCH SENDCMD SEND ERR CODE: %i RES: %s", sendcmd_senderr, translate_app_error(sendcmd_senderr));
#endif
	}
	//free(iter);
	//APP_LOG(APP_LOG_LEVEL_INFO, "SEND CMD OUT, SENT MSG TO APP");

} // end send_cmd_cgm

// updateColours - called when fg_colour or bg_colour is changed.
#ifdef PBL_COLOR
void updateColours()
{
	if(MonochromeBackground)
	{
		bitmap_layer_set_background_color(upper_face_layer, bg_colour);
		text_layer_set_text_color(delta_layer, fg_colour);
		text_layer_set_text_color(message_layer, fg_colour);
		text_layer_set_text_color(bg_layer, fg_colour);
		text_layer_set_text_color(cgmtime_layer, fg_colour);
	}
	else
	{
		bitmap_layer_set_background_color(upper_face_layer, fg_colour);
		text_layer_set_text_color(delta_layer, bg_colour);
		text_layer_set_text_color(message_layer, bg_colour);
		text_layer_set_text_color(bg_layer, bg_colour);
		text_layer_set_text_color(cgmtime_layer, bg_colour);
	}
	bitmap_layer_set_background_color(lower_face_layer, bg_colour);
	text_layer_set_text_color(time_watch_layer, fg_colour);
	text_layer_set_text_color(date_app_layer, fg_colour);
}
// end updateColours
#endif

void inbox_received_handler_cgm(DictionaryIterator *iterator, void *context)
{
	Tuple *data = dict_read_first(iterator);
#ifdef DEBUG_LEVEL
	//APP_LOG(APP_LOG_LEVEL_INFO, "SYNC TUPLE");
	APP_LOG(APP_LOG_LEVEL_INFO, "inbox_received_callback_cgm: got dictionary");
#endif

	if (global_lock)
	{

		APP_LOG(APP_LOG_LEVEL_ERROR, "GLOBALLY LOCKED");
		return;
	}

	// CONSTANTS
#define ICON_MSGSTR_SIZE 4
#define BG_MSGSTR_SIZE 6
#define BGDELTA_MSGSTR_SIZE 13
#define BATTLEVEL_MSGSTR_SIZE 5

	// CODE START

	while ((data != NULL) && (!global_lock))
	{
#ifdef DEBUG_LEVEL
		APP_LOG(APP_LOG_LEVEL_INFO, "inbox_received_callback_cgm: key is %lu", data->key);
#endif
		switch (data->key)
		{

		case CGM_ICON_KEY:
			;
#ifdef DEBUG_LEVEL
			APP_LOG(APP_LOG_LEVEL_INFO, "SYNC TUPLE: ICON ARROW");
#endif
			strncpy(current_icon, data->value->cstring, ICON_MSGSTR_SIZE);
			load_icon();
			break; // break for CGM_ICON_KEY

		case CGM_BG_KEY:
			;
#ifdef DEBUG_LEVEL
			APP_LOG(APP_LOG_LEVEL_INFO, "SYNC TUPLE: BG CURRENT");
#endif
			strncpy(last_bg, data->value->cstring, BG_MSGSTR_SIZE);
			load_bg();
#if defined(PBL_HEALTH)
			if (CollectHealth) restart_data_log(); // flush the data
#endif
			break; // break for CGM_BG_KEY

		case CGM_TCGM_KEY:
			;
#ifdef DEBUG_LEVEL
			APP_LOG(APP_LOG_LEVEL_INFO, "SYNC TUPLE: READ CGM TIME");
#endif
			current_cgm_time = data->value->uint32;
			load_cgmtime();
			// as long as current_cgm_time is not zero, we know we have gotten an update from the app,
			// so we reset minutes_cgm to 6, so the pebble doesn't request another one.
			if (current_cgm_time != 0)
			{
				minutes_cgm = 6;
			}
			break; // break for CGM_TCGM_KEY

		case CGM_DLTA_KEY:
			;
			strncpy(current_bg_delta, data->value->cstring, BGDELTA_MSGSTR_SIZE);
#ifdef DEBUG_LEVEL
			APP_LOG(APP_LOG_LEVEL_INFO, "SYNC TUPLE: BG DELTA - %s", current_bg_delta);
#endif
			load_bg_delta();
			break; // break for CGM_DLTA_KEY

		case CGM_UBAT_KEY:
			;
#ifdef DEBUG_LEVEL
			APP_LOG(APP_LOG_LEVEL_INFO, "SYNC TUPLE: UPLOADER BATTERY LEVEL");
#endif
			//APP_LOG(APP_LOG_LEVEL_INFO, "SYNC TUPLE: BATTERY LEVEL IN, COPY LAST BATTLEVEL");
			strncpy(last_battlevel, data->value->cstring, BATTLEVEL_MSGSTR_SIZE);
			//APP_LOG(APP_LOG_LEVEL_INFO, "SYNC TUPLE: BATTERY LEVEL, CALL LOAD BATTLEVEL");
			load_battlevel();
			//APP_LOG(APP_LOG_LEVEL_INFO, "SYNC TUPLE: BATTERY LEVEL OUT");
			break; // break for CGM_UBAT_KEY


			//		#ifdef PBL_PLATFORM_BASALT
		case CGM_TREND_BEGIN_KEY:
			expected_trend_buffer_length = data->value->uint16;
#ifdef DEBUG_LEVEL
			APP_LOG(APP_LOG_LEVEL_INFO, "TREND_BEGIN; About to receive Trend Image of %i size.", expected_trend_buffer_length);
#endif
			if(trend_buffer)
			{
#ifdef DEBUG_LEVEL
				APP_LOG(APP_LOG_LEVEL_INFO, "TREND_BEGIN; Freeing trend_buffer.");
#endif
				free(trend_buffer);
			}
#ifdef DEBUG_LEVEL
			APP_LOG(APP_LOG_LEVEL_INFO, "TREND_BEGIN; Allocating trend_buffer.");
#endif
			trend_buffer = malloc(expected_trend_buffer_length);
			trend_buffer_length = 0;
#if DEBUG_LEVEL > 1
			if(trend_buffer == NULL)
			{
				APP_LOG(APP_LOG_LEVEL_DEBUG, "TREND_BEGIN: Could not allocate trend_buffer");
				break;
			}
			APP_LOG(APP_LOG_LEVEL_DEBUG, "TREND_BEGIN: trend_buffer is %lx, trend_buffer_length is %i", (uint32_t)trend_buffer, trend_buffer_length);
#endif
			break;
		case CGM_TREND_DATA_KEY:
#ifdef DEBUG_LEVEL
			APP_LOG(APP_LOG_LEVEL_INFO, "TREND_DATA: receiving Trend Image chunk");
#endif
			if(trend_buffer)
			{
				if ((trend_buffer_length + data->length) <= expected_trend_buffer_length)
				{
					memcpy((trend_buffer+trend_buffer_length), data->value->data, data->length);
					trend_buffer_length += data->length;
#ifdef DEBUG_LEVEL
					APP_LOG(APP_LOG_LEVEL_INFO, "TREND_DATA: received %u of %u so far", trend_buffer_length, expected_trend_buffer_length);
#endif
				}
				else
				{
#ifdef DEBUG_LEVEL
					APP_LOG(APP_LOG_LEVEL_INFO, "TREND_DATA: EXCEEDED BUFFER received %u of %u so far", trend_buffer_length, expected_trend_buffer_length);
#endif

				}
			}
#if DEBUG_LEVEL > 1
			else
			{
				APP_LOG(APP_LOG_LEVEL_DEBUG, "TREND_DATA: trend_buffer not allocated, ignoring");
			}
#endif
			if(trend_buffer_length == expected_trend_buffer_length) doing_trend = true;
			break;

		case CGM_TREND_END_KEY:
			if(!doing_trend)
			{
#ifdef DEBUG_LEVEL
				APP_LOG(APP_LOG_LEVEL_INFO, "Got a TREND_END without TREND_START");
#endif
				break;
			}
#ifdef DEBUG_LEVEL
			APP_LOG(APP_LOG_LEVEL_INFO, "Finished receiving Trend Image");
#endif
			if(bg_trend_bitmap != NULL)
			{
#if DEBUG_LEVEL > 1
				APP_LOG(APP_LOG_LEVEL_INFO, "Destroying bg_trend_bitmap");
#endif
				gbitmap_destroy(bg_trend_bitmap);
				bg_trend_bitmap = NULL;
			}

#ifdef DEBUG_LEVEL
			APP_LOG(APP_LOG_LEVEL_INFO, "Creating Trend Image");

			APP_LOG(APP_LOG_LEVEL_DEBUG, "TREND_END: trend_buffer is %lx, trend_buffer_length is %i", (uint32_t)trend_buffer, trend_buffer_length);
#endif
			if ((trend_buffer != NULL) && (trend_buffer_length > 0) && (trend_buffer_length == expected_trend_buffer_length))
			{
				bg_trend_bitmap = gbitmap_create_from_png_data(trend_buffer, trend_buffer_length);
			}
			else
			{
				break;
			}

			if(bg_trend_bitmap != NULL)
			{
#ifdef DEBUG_LEVEL
				APP_LOG(APP_LOG_LEVEL_INFO, "bg_trend_bitmap created, setting to layer");
#endif
				bitmap_layer_set_bitmap(bg_trend_layer, bg_trend_bitmap);
			}

#ifdef DEBUG_LEVEL
			else
			{
				APP_LOG(APP_LOG_LEVEL_INFO, "bg_trend_bitmap creation FAILED!");
			}
#endif
			if (trend_buffer)
			{
#ifdef DEBUG_LEVEL
				APP_LOG(APP_LOG_LEVEL_INFO, "Free trend buffer 2");
#endif
				free(trend_buffer);
				trend_buffer = NULL;
			}
			break;
			//		#endif
		case CGM_MESSAGE_KEY:
#ifdef DEBUG_LEVEL
			APP_LOG(APP_LOG_LEVEL_INFO, "Got Message Key, message is \"%s\"", data->value->cstring);
#endif
			text_layer_set_text(message_layer,data->value->cstring);
			if(strcmp(data->value->cstring, "")==0)
			{
#ifdef DEBUG_LEVEL
				APP_LOG(APP_LOG_LEVEL_INFO, "Setting Delta Only");
#endif
				display_message = false;
				layer_set_hidden((Layer *)message_layer, true);
				layer_set_hidden((Layer *)delta_layer, false);
			}
			else
			{
#ifdef DEBUG_LEVEL
				APP_LOG(APP_LOG_LEVEL_INFO, "Setting Message");
#endif
				display_message = true;
				layer_set_hidden((Layer *)message_layer, false);
				layer_set_hidden((Layer *)delta_layer, true);
			}
			break;

		case CGM_COLLECT_HEALTH_KEY:
			old_state = CollectHealth;
			CollectHealth = (data->value->uint8 > 0);
#ifdef DEBUG_LEVEL
			APP_LOG(APP_LOG_LEVEL_INFO, "Got COLLECT_HEALTH_KEY value: %u", data->value->uint8);
#endif
			if (!old_state && CollectHealth) {
				health_subscribe(); // was just switched on
			} else if (old_state && !CollectHealth) {
				health_unsubscribe(); // was just switched off;
			}
			break;


		case CGM_VIBE_KEY:
#ifdef DEBUG_LEVEL
			APP_LOG(APP_LOG_LEVEL_INFO, "Got Vibe Key, message is \"%u\"", data->value->uint8);
#endif
			if((data->value->uint8 > 0 || data->value->uint8 <4) && ! BluetoothAlert)
			{
				alert_handler_cgm(data->value->uint8);
			}
			break;

		case CGM_SYNC_KEY:
#ifdef DEBUG_LEVEL
			APP_LOG(APP_LOG_LEVEL_INFO, "Got Sync Key, message is \"%u\"", data->value->uint8);
#endif
			send_cmd_cgm();
			break;

		case SET_FG_COLOUR:
#ifdef DEBUG_LEVEL
			APP_LOG(APP_LOG_LEVEL_INFO, "Got foreground Key, message is \"%lx\"", data->value->uint32);
#endif
#ifdef PBL_COLOR
			fg_colour = GColorFromHEX(data->value->uint32);
			persist_write_int(SET_FG_COLOUR, data->value->uint32);
			updateColours();
#endif
			break;

		case SET_BG_COLOUR:
#ifdef DEBUG_LEVEL
			APP_LOG(APP_LOG_LEVEL_INFO, "Got background Key, message is \"%lx\"", data->value->uint32);
#endif
#ifdef PBL_COLOR
			bg_colour = GColorFromHEX(data->value->uint32);
			persist_write_int(SET_BG_COLOUR, data->value->uint32);
			updateColours();
#endif
			break;
		case SET_DISP_SECS:
#ifdef DEBUG_LEVEL
			APP_LOG(APP_LOG_LEVEL_INFO, "Got dispsecs Key, message is \"%u\"", data->value->uint8);
#endif
			if(data->value->uint8 > 0)
			{
				display_seconds = true;
				time_font = time_font_small;
			}
			else
			{

				display_seconds = false;
				time_font = time_font_normal;
			}
			persist_write_bool(SET_DISP_SECS, display_seconds);
			text_layer_set_font(time_watch_layer,time_font);
			if(clock_is_24h_style() == true)
			{
				if(display_seconds)
				{
					snprintf(time_watch_format, 10, "%s", TIME_24HS_FORMAT);
				}
				else
				{
					snprintf(time_watch_format, 6, "%s", TIME_24H_FORMAT);
				}
			}
			else
			{
				if(display_seconds)
				{
					snprintf(time_watch_format, 10, "%s", TIME_12HS_FORMAT);
				}
				else
				{
					snprintf(time_watch_format, 6, "%s", TIME_12H_FORMAT);
				}
			}
			draw_date_from_app();
			break;

		case SET_VIBE_REPEAT:
#ifdef DEBUG_LEVEL
			APP_LOG(APP_LOG_LEVEL_INFO, "Got background Key, message is \"%lx\"", data->value->uint32);
#endif
			if(data->value->uint8 > 0)
			{
				vibe_repeat = true;
			}
			else
			{
				vibe_repeat = false;
			}
			persist_write_bool(SET_VIBE_REPEAT, vibe_repeat);
			break;

		case SET_NO_VIBE:
#ifdef DEBUG_LEVEL
			APP_LOG(APP_LOG_LEVEL_INFO, "Got No Vibe Key, message is \"%lx\"", data->value->uint32);
#endif
			if(data->value->uint8 > 0)
			{
				TurnOffAllVibrations = true;
			}
			else
			{
				TurnOffAllVibrations = false;
			}
			persist_write_bool(SET_NO_VIBE, TurnOffAllVibrations);
			break;

		case SET_LIGHT_ON_CHG:
#ifdef DEBUG_LEVEL
			APP_LOG(APP_LOG_LEVEL_INFO, "Got Backlight on Charge key, message is \"%lx\"", data->value->uint32);
#endif
			if(data->value->uint8 > 0)
			{
				BacklightOnCharge = true;
			}
			else
			{
				BacklightOnCharge = false;
			}
			persist_write_bool(SET_LIGHT_ON_CHG, BacklightOnCharge);
			break;

		case SET_MONOCHROME:
#ifdef DEBUG_LEVEL
			APP_LOG(APP_LOG_LEVEL_INFO, "Got Monochrome key, message is \"%lx\"", data->value->uint32);
#endif
			if(data->value->uint8 == 0)
			{
				MonochromeBackground = true;
				bitmap_layer_set_background_color(upper_face_layer, fg_colour);
				text_layer_set_text_color(delta_layer, bg_colour);
				text_layer_set_text_color(message_layer, bg_colour);
				text_layer_set_text_color(bg_layer, bg_colour);
				text_layer_set_text_color(cgmtime_layer, bg_colour);
			}
			else
			{
				MonochromeBackground = false;
				bitmap_layer_set_background_color(upper_face_layer, bg_colour);
				text_layer_set_text_color(delta_layer, fg_colour);
				text_layer_set_text_color(message_layer, fg_colour);
				text_layer_set_text_color(bg_layer, fg_colour);
				text_layer_set_text_color(cgmtime_layer, fg_colour);
			}
			persist_write_bool(SET_MONOCHROME, MonochromeBackground);

			break;

		default:
#ifdef DEBUG_LEVEL
			APP_LOG(APP_LOG_LEVEL_INFO, "sync_tuple_cgm_callback: Dictionary Key not recognised");
#endif
			break;
		}
		// end switch(key)
		data = dict_read_next(iterator);
	}
} // end sync_tuple_changed_callback_cgm()

void timer_callback_cgm(void *data)
{

	//APP_LOG(APP_LOG_LEVEL_INFO, "TIMER CALLBACK IN, TIMER POP, ABOUT TO CALL SEND CMD");
	// reset msg timer to NULL
	if (timer_cgm != NULL)
	{
		timer_cgm = NULL;
	}
	//minutes_cgm tracks the minutes since the last update sent from the pebble.
	// if it reaches 0, we have not received an update in 6 minutes, so we ask for one.
	//This limits the number of messages the pebble sends, and therefore improves battery life.

	//APP_LOG(APP_LOG_LEVEL_INFO, "minutes_cgm: %d", minutes_cgm);
	if (minutes_cgm == 0)
	{
		minutes_cgm = 6;
		// send message
#ifdef DEBUG_LEVEL
		APP_LOG(APP_LOG_LEVEL_INFO, "send_cmd_cgm: minutes_cgm is zero, setting to 6 and sending for data.");
#endif
		send_cmd_cgm();
	}
	else
	{
		minutes_cgm--;
		load_cgmtime();
		load_bg_delta();
	}
#ifdef DEBUG_LEVEL
	APP_LOG(APP_LOG_LEVEL_INFO, "minutes_cgm: %d", minutes_cgm);
#endif
	//APP_LOG(APP_LOG_LEVEL_INFO, "TIMER CALLBACK, SEND CMD DONE, ABOUT TO REGISTER TIMER");
	// set msg timer
	timer_cgm = app_timer_register((WATCH_MSGSEND_SECS*MS_IN_A_SECOND), timer_callback_cgm, NULL);

	//APP_LOG(APP_LOG_LEVEL_INFO, "TIMER CALLBACK, REGISTER TIMER DONE");

} // end timer_callback_cgm

// format current time from watch

void handle_second_tick_cgm(struct tm* tick_time_cgm, TimeUnits units_changed_cgm)
{
	// VARIABLES
	size_t tick_return_cgm = 0;
	// CODE START
	handling_second = true;
	if (SECOND_UNIT && display_seconds)
	{
		tick_return_cgm = strftime(time_watch_text, TIME_TEXTBUFF_SIZE, time_watch_format, tick_time_cgm);
		if (tick_return_cgm != 0)
		{
			text_layer_set_text(time_watch_layer, time_watch_text);
		}
#if DEBUG_LEVEL >0

		APP_LOG(APP_LOG_LEVEL_INFO, "Seconds Tick: display_seconds = %i, time_watch_text = %s, time_watch_format = %s", display_seconds, time_watch_text, time_watch_format);
#endif
	}
	if ((units_changed_cgm & SECOND_UNIT) && ((tick_time_cgm->tm_sec & 0x01)==1))
	{

#if DEBUG_LEVEL > 1
		APP_LOG(APP_LOG_LEVEL_INFO, "Handling 2 second tick, display_message is %i", display_message);
#endif
		if(display_message)
		{
#if DEBUG_LEVEL > 1
			APP_LOG(APP_LOG_LEVEL_DEBUG, "message_layer toggling %i", (tick_time_cgm->tm_sec & 0x01)==1);
#endif

			layer_set_hidden((Layer *)delta_layer, !(layer_get_hidden((Layer *)delta_layer)));
			layer_set_hidden((Layer *)message_layer, !(layer_get_hidden((Layer *)message_layer)));
		}
		else
		{
			if(!layer_get_hidden((Layer *)message_layer))
			{
				layer_set_hidden((Layer *)message_layer, true);
			}
			if(layer_get_hidden((Layer *)delta_layer))
			{
				layer_set_hidden((Layer *)delta_layer, false);
			}
		}
	}
	if (units_changed_cgm & MINUTE_UNIT)
	{
#if DEBUG_LEVEL >0
		APP_LOG(APP_LOG_LEVEL_INFO, "TICK TIME MINUTE CODE");
#endif
		if(!display_seconds)
		{
			tick_return_cgm = strftime(time_watch_text, TIME_TEXTBUFF_SIZE, time_watch_format, tick_time_cgm);
			if (tick_return_cgm != 0)
			{
				text_layer_set_text(time_watch_layer, time_watch_text);
			}
		}

#if DEBUG_LEVEL >0
		APP_LOG(APP_LOG_LEVEL_INFO, "Minute Tick: display_seconds = %i, time_watch_text = %s", display_seconds, time_watch_text);
#endif 
		++lastAlertTime;
	}
	if (units_changed_cgm & DAY_UNIT)
	{
#if DEBUG_LEVEL >1
		APP_LOG(APP_LOG_LEVEL_INFO, "TICK TIME DAY CODE");
#endif
		tick_return_cgm = strftime(date_app_text, DATE_TEXTBUFF_SIZE, "%a %d %b", tick_time_cgm);
		if (tick_return_cgm != 0)
		{
			text_layer_set_text(date_app_layer, date_app_text);
		}
	}
	handling_second = false;

} // end handle_minute_tick_cgm

#ifdef PBL_PLATFORM_APLITE

static uint8_t breverse(uint8_t b);
static uint8_t breverse(uint8_t b)
{
	b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
	b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
	b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
	return b;
}

static void bitmapLayerUpdate(struct Layer *layer, GContext *ctx)
{
	GBitmap *framebuffer;
	const GBitmap *graphic = bitmap_layer_get_bitmap((BitmapLayer *)layer);
	int height;
	uint8_t finalBits;
	uint8_t *bfr, *bitmap;

	if (global_lock) return;
	global_lock = true;

	framebuffer = graphics_capture_frame_buffer(ctx);
	if (framebuffer == NULL)
	{
		APP_LOG(APP_LOG_LEVEL_DEBUG, "capture frame buffer failed!!");
	}
	else
	{
		//  APP_LOG(APP_LOG_LEVEL_DEBUG, "capture frame buffer succeeded %i vs %i and %i vs %i with bpw: %i vs %i",gbitmap_get_bounds(graphic).size.w,gbitmap_get_bounds(framebuffer).size.w,gbitmap_get_bounds(graphic).size.h,gbitmap_get_bounds(framebuffer).size.h, gbitmap_get_bytes_per_row(graphic),gbitmap_get_bytes_per_row(framebuffer));

		if (graphic == NULL)
		{
			APP_LOG(APP_LOG_LEVEL_DEBUG, "GRAPHIC IS NULL!!");
		}
		else
		{

			height = gbitmap_get_bounds(graphic).size.h;

			uint8_t* bfstart=(uint8_t*)gbitmap_get_data(framebuffer);
			uint8_t* bitmapstart=(uint8_t*)gbitmap_get_data(graphic);
			if (bitmapstart == NULL)
			{
				APP_LOG(APP_LOG_LEVEL_WARNING, "bitmap start went to null!!");
				graphics_release_frame_buffer(ctx, framebuffer);
				global_lock = false;
				return;
			}
			if (bfstart == NULL)
			{
				APP_LOG(APP_LOG_LEVEL_WARNING, "framebuffer start went to null!!");
				graphics_release_frame_buffer(ctx, framebuffer);
				global_lock = false;
				return;
			}
			unsigned int fb_bytes_per_row = gbitmap_get_bytes_per_row(framebuffer);
			unsigned int gbitmap_bytes_per_row = gbitmap_get_bytes_per_row(graphic);

			bfstart += fb_bytes_per_row*34; // how far down screen to start

			for (int yindex =0; yindex < height; yindex++)
			{

				int fb_yoffset = yindex * fb_bytes_per_row;

				bfr = (uint8_t*)(bfstart+fb_yoffset);
				bitmap = (uint8_t*)(bitmapstart+(yindex * gbitmap_bytes_per_row));
				for ( unsigned int xindex = 0; xindex < gbitmap_bytes_per_row; xindex++)
				{

					finalBits = breverse(*bitmap++) ^ *bfr;
					*bfr++ = finalBits;
					// APP_LOG(APP_LOG_LEVEL_DEBUG, "bfr: %0x, bitmsp: %0x, finalBits: %x", (unsigned int)bfr, (unsigned int)bitmap, finalBits );

				}
			}
		}
		graphics_release_frame_buffer(ctx, framebuffer);
	}
	global_lock = false;
}
#endif

void window_load_cgm(Window *window_cgm)
{
	//APP_LOG(APP_LOG_LEVEL_INFO, "WINDOW LOAD");

	// VARIABLES
	Layer *window_layer_cgm = NULL;

	// CODE START

	window_layer_cgm = window_get_root_layer(window_cgm);

	//Paint the backgrounds for upper and lower halves of the watch face.
#ifdef DEBUG_LEVEL
	APP_LOG(APP_LOG_LEVEL_INFO, "Creating Upper and Lower face panels");
#endif

#ifdef PBL_PLATFORM_APLITE
	upper_face_layer = bitmap_layer_create(GRect(0,0,144,88));
#else
	upper_face_layer = bitmap_layer_create(GRect(0,0,144,83));
#endif

	//	bitmap_layer_set_background_color(upper_face_layer, GColorWhite);
	if(MonochromeBackground)
	{
		bitmap_layer_set_background_color(upper_face_layer, bg_colour);
	}
	else
	{
		bitmap_layer_set_background_color(upper_face_layer, fg_colour);
	}
#ifdef PBL_PLATFORM_APLITE
	lower_face_layer = bitmap_layer_create(GRect(0,89,144,165));
#else
	lower_face_layer = bitmap_layer_create(GRect(0,84,144,165));
#endif

	bitmap_layer_set_background_color(lower_face_layer, bg_colour);
	layer_add_child(window_layer_cgm, bitmap_layer_get_layer(upper_face_layer));
	layer_add_child(window_layer_cgm, bitmap_layer_get_layer(lower_face_layer));

	// ARROW OR SPECIAL VALUE
#ifdef DEBUG_LEVEL
	APP_LOG(APP_LOG_LEVEL_INFO, "Creating Arrow Bitmap layer");
#endif
#ifdef PBL_COLOR
	icon_layer = bitmap_layer_create(GRect(85, -9, 78, 49));
	bitmap_layer_set_compositing_mode(icon_layer, GCompOpSet);
#else
	icon_layer = bitmap_layer_create(GRect(85, -7, 78, 51));
#endif
	bitmap_layer_set_alignment(icon_layer, GAlignTopLeft);
	bitmap_layer_set_background_color(icon_layer, GColorClear);
	layer_add_child(window_layer_cgm, bitmap_layer_get_layer(icon_layer));

	//create the bg_trend_layer
#ifdef DEBUG_LEVEL
	APP_LOG(APP_LOG_LEVEL_INFO, "Creating BG Trend Bitmap layer");
#endif
#ifdef PBL_PLATFORM_BASALT
	bg_trend_layer = bitmap_layer_create(GRect(0,0,144,84));
	bitmap_layer_set_compositing_mode(bg_trend_layer, GCompOpSet);
	layer_add_child(window_layer_cgm, bitmap_layer_get_layer(bg_trend_layer));
#endif
#ifdef PBL_PLATFORM_APLITE
	bg_trend_layer = bitmap_layer_create(GRect(0,24,144,64));
	layer_set_update_proc(bitmap_layer_get_layer(bg_trend_layer),bitmapLayerUpdate);
#endif

	// DELTA BG
#ifdef DEBUG_LEVEL
	APP_LOG(APP_LOG_LEVEL_INFO, "Creating Delta BG Text layer");
#endif
#ifdef PBL_PLATFORM_APLITE
	delta_layer = text_layer_create(GRect(0, 36, 143, 50));
#else
	delta_layer = text_layer_create(GRect(0, 55, 143, 50));
#endif
	if(MonochromeBackground)
	{
		text_layer_set_text_color(delta_layer, fg_colour);
	}
	else
	{
		text_layer_set_text_color(delta_layer, bg_colour);
	}
	text_layer_set_background_color(delta_layer, GColorClear);
	text_layer_set_font(delta_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28));

#ifdef PBL_PLATFORM_APLITE
	text_layer_set_text_alignment(delta_layer, GTextAlignmentRight);
#else
	text_layer_set_text_alignment(delta_layer, GTextAlignmentLeft);
#endif

	layer_add_child(window_layer_cgm, text_layer_get_layer(delta_layer));

	// MESSAGE
#ifdef DEBUG_LEVEL
	APP_LOG(APP_LOG_LEVEL_INFO, "Creating Message Text layer");
#endif
	message_layer = text_layer_create(GRect(0, 36, 143, 50));
	if(MonochromeBackground)
	{
		text_layer_set_text_color(message_layer, fg_colour);
	}
	else
	{
		text_layer_set_text_color(message_layer, bg_colour);
	}
	text_layer_set_background_color(message_layer, GColorClear);
	text_layer_set_font(message_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28));
	text_layer_set_text_alignment(message_layer, GTextAlignmentCenter);
	text_layer_set_text(message_layer, "");
	layer_set_hidden((Layer *)message_layer, true);
	layer_add_child(window_layer_cgm, text_layer_get_layer(message_layer));

	// BG
#ifdef DEBUG_LEVEL
	APP_LOG(APP_LOG_LEVEL_INFO, "Creating BG Text layer");
#endif
#ifdef PBL_COLOR
	bg_layer = text_layer_create(GRect(0, -5, 95, 42));
#else
	bg_layer = text_layer_create(GRect(0, -5, 95, 47));
#endif
	if(MonochromeBackground)
	{
		text_layer_set_text_color(bg_layer, fg_colour);
	}
	else
	{
		text_layer_set_text_color(bg_layer, bg_colour);
	}
	text_layer_set_background_color(bg_layer, GColorClear);
	text_layer_set_font(bg_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
	text_layer_set_text_alignment(bg_layer, GTextAlignmentCenter);
	layer_add_child(window_layer_cgm, text_layer_get_layer(bg_layer));


	// CGM TIME AGO READING
#ifdef DEBUG_LEVEL
	APP_LOG(APP_LOG_LEVEL_INFO, "Creating CGM Time Ago Bitmap layer");
#endif
#ifdef PBL_PLATFORM_APLITE
	cgmtime_layer = text_layer_create(GRect(104, 58, 40, 24));
#else
	cgmtime_layer = text_layer_create(GRect(104, 58, 40, 24));
#endif

	if(MonochromeBackground)
	{
		text_layer_set_text_color(cgmtime_layer, fg_colour);
	}
	else
	{
		text_layer_set_text_color(cgmtime_layer, bg_colour);
	}
	text_layer_set_background_color(cgmtime_layer, GColorClear);
	text_layer_set_font(cgmtime_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
#ifdef PBL_PLATFORM_APLITE
	text_layer_set_text_alignment(cgmtime_layer, GTextAlignmentCenter);
#else
	text_layer_set_text_alignment(cgmtime_layer, GTextAlignmentRight);
#endif
	layer_add_child(window_layer_cgm, text_layer_get_layer(cgmtime_layer));

#ifdef PBL_PLATFORM_APLITE
	// top layer on pebble classic
	layer_add_child(window_layer_cgm, bitmap_layer_get_layer(bg_trend_layer));
#endif

	// CURRENT ACTUAL TIME FROM WATCH
#ifdef DEBUG_LEVEL
	APP_LOG(APP_LOG_LEVEL_INFO, "Creating Watch Time Text layer");
#endif
#ifdef PBL_COLOR
	time_watch_layer = text_layer_create(GRect(0, 82, 143, 44));
#else

#ifdef PBL_PLATFORM_APLITE
	time_watch_layer = text_layer_create(GRect(0, 84, 143, 44));
#else
	time_watch_layer = text_layer_create(GRect(0, 82, 143, 44));
#endif

#endif
	text_layer_set_text_color(time_watch_layer, fg_colour);
	text_layer_set_background_color(time_watch_layer, GColorClear);
	text_layer_set_font(time_watch_layer,time_font);
	text_layer_set_text_alignment(time_watch_layer, GTextAlignmentCenter);
	layer_add_child(window_layer_cgm, text_layer_get_layer(time_watch_layer));

	// CURRENT ACTUAL DATE FROM APP
#ifdef DEBUG_LEVEL
	APP_LOG(APP_LOG_LEVEL_INFO, "Creating Watch Date Text layer");
#endif
#ifdef PBL_COLOR
	date_app_layer = text_layer_create(GRect(0, 124, 143, 29));
#else

#ifdef PBL_PLATFORM_APLITE
	date_app_layer = text_layer_create(GRect(0, 122, 143, 29));
#else
	date_app_layer = text_layer_create(GRect(0, 120, 143, 29));
#endif
#endif
	text_layer_set_text_color(date_app_layer, fg_colour);
	text_layer_set_background_color(date_app_layer, GColorClear);
	text_layer_set_font(date_app_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
	text_layer_set_text_alignment(date_app_layer, GTextAlignmentCenter);
	layer_add_child(window_layer_cgm, text_layer_get_layer(date_app_layer));
	draw_date_from_app();

	// PHONE BATTERY LEVEL
#ifdef DEBUG_LEVEL
	APP_LOG(APP_LOG_LEVEL_INFO, "Creating Phone Battery Text layer");
#endif
#ifdef PBL_COLOR
	battlevel_layer = text_layer_create(GRect(0, 150, 72, 18));
#else
	battlevel_layer = text_layer_create(GRect(0, 148, 59, 18));
#endif
	text_layer_set_text_color(battlevel_layer, GColorGreen);
	text_layer_set_background_color(battlevel_layer, GColorClear);
	text_layer_set_font(battlevel_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
	text_layer_set_text_alignment(battlevel_layer, GTextAlignmentLeft);
	layer_add_child(window_layer_cgm, text_layer_get_layer(battlevel_layer));
#ifdef DEBUG_LEVEL
	APP_LOG(APP_LOG_LEVEL_INFO, "battlevel_layer; %s", text_layer_get_text(battlevel_layer));
#endif


	// WATCH BATTERY LEVEL
#ifdef DEBUG_LEVEL
	APP_LOG(APP_LOG_LEVEL_INFO, "Creating Watch Battery Text layer");
#endif
	BatteryChargeState charge_state=battery_state_service_peek();
#ifdef PBL_COLOR
	watch_battlevel_layer = text_layer_create(GRect(72, 150, 72, 18));
	APP_LOG(APP_LOG_LEVEL_INFO, "COLOR DETECTED");
#else
	APP_LOG(APP_LOG_LEVEL_INFO, "BW DETECTED");
	watch_battlevel_layer = text_layer_create(GRect(81, 148, 59, 18));
#endif
	text_layer_set_font(watch_battlevel_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
	text_layer_set_text_alignment(watch_battlevel_layer, GTextAlignmentRight);
#ifdef PBL_COLOR
	if(charge_state.is_charging)
	{
		text_layer_set_text_color(watch_battlevel_layer, GColorDukeBlue);
		text_layer_set_background_color(watch_battlevel_layer, GColorGreen);
	}
	else
	{
		text_layer_set_text_color(watch_battlevel_layer, GColorGreen);
		text_layer_set_background_color(watch_battlevel_layer, GColorClear);
	}
#else
	if(charge_state.is_charging)
	{
		text_layer_set_text_color(watch_battlevel_layer, GColorBlack);
		text_layer_set_background_color(watch_battlevel_layer, GColorWhite);
	}
	else
	{
		text_layer_set_text_color(watch_battlevel_layer, GColorWhite);
		text_layer_set_background_color(watch_battlevel_layer, GColorBlack);
	}
#endif
	layer_add_child(window_layer_cgm, text_layer_get_layer(watch_battlevel_layer));
	battery_handler(charge_state);
#ifdef DEBUG_LEVEL
	APP_LOG(APP_LOG_LEVEL_INFO, "watch_battlevel_layer; %s", text_layer_get_text(watch_battlevel_layer));
#endif

	// put " " (space) in bg field so logo continues to show
	// " " (space) also shows these are init values, not bad or null values
	snprintf(current_icon, 1, " ");
	load_icon();
	snprintf(last_bg, BG_MSGSTR_SIZE, " ");
	load_bg();
	current_cgm_time = 0;
	load_cgmtime();
	current_app_time = 0;
	snprintf(current_bg_delta, BGDELTA_MSGSTR_SIZE, "LOAD");
	load_bg_delta();
	snprintf(last_battlevel, BATTLEVEL_MSGSTR_SIZE, " ");
	load_battlevel();

	// init timer to null if needed, and register timer
	//APP_LOG(APP_LOG_LEVEL_INFO, "WINDOW LOAD, APP INIT DONE, ABOUT TO REGISTER TIMER");
	if (timer_cgm != NULL)
	{
		timer_cgm = NULL;
	}
	timer_cgm = app_timer_register((LOADING_MSGSEND_SECS*MS_IN_A_SECOND), timer_callback_cgm, NULL);
	//APP_LOG(APP_LOG_LEVEL_INFO, "WINDOW LOAD, TIMER REGISTER DONE");

} // end window_load_cgm

void window_unload_cgm(Window *window_cgm)
{
	//APP_LOG(APP_LOG_LEVEL_INFO, "WINDOW UNLOAD IN");

	//APP_LOG(APP_LOG_LEVEL_INFO, "WINDOW UNLOAD, APP SYNC DEINIT");
	app_sync_deinit(&sync_cgm);

	//destroy the trend bitmap and layer
	if(bg_trend_bitmap != NULL) destroy_null_GBitmap(&bg_trend_bitmap);
	if(bg_trend_layer != NULL) destroy_null_BitmapLayer(&bg_trend_layer);
	//APP_LOG(APP_LOG_LEVEL_INFO, "WINDOW UNLOAD, DESTROY GBITMAPS IF EXIST");
	if(icon_bitmap != NULL) destroy_null_GBitmap(&icon_bitmap);
	if(appicon_bitmap != NULL) destroy_null_GBitmap(&appicon_bitmap);
	if(specialvalue_bitmap != NULL) destroy_null_GBitmap(&specialvalue_bitmap);

	//APP_LOG(APP_LOG_LEVEL_INFO, "WINDOW UNLOAD, DESTROY BITMAPS IF EXIST");
	if(icon_layer != NULL) destroy_null_BitmapLayer(&icon_layer);

	//APP_LOG(APP_LOG_LEVEL_INFO, "WINDOW UNLOAD, DESTROY TEXT LAYERS IF EXIST");
	if(bg_layer != NULL) destroy_null_TextLayer(&bg_layer);
	if(cgmtime_layer != NULL) destroy_null_TextLayer(&cgmtime_layer);
	if(delta_layer != NULL) destroy_null_TextLayer(&delta_layer);
	if(message_layer != NULL) destroy_null_TextLayer(&message_layer);
	if(battlevel_layer != NULL) destroy_null_TextLayer(&battlevel_layer);
	if(watch_battlevel_layer != NULL) destroy_null_TextLayer(&watch_battlevel_layer);
	if(time_watch_layer != NULL) destroy_null_TextLayer(&time_watch_layer);
	if(date_app_layer != NULL) destroy_null_TextLayer(&date_app_layer);

	//destroy the face background layers.
	if(lower_face_layer != NULL) destroy_null_BitmapLayer(&lower_face_layer);
	if(upper_face_layer != NULL) destroy_null_BitmapLayer(&upper_face_layer);

	//APP_LOG(APP_LOG_LEVEL_INFO, "WINDOW UNLOAD OUT");
} // end window_unload_cgm

static void init_cgm(void)
{
#ifdef DEBUG_LEVEL
	APP_LOG(APP_LOG_LEVEL_INFO, "init_cgm");
#endif
	//Load persistent settings
	display_seconds = persist_exists(SET_DISP_SECS)? persist_read_bool(SET_DISP_SECS) : false;
	vibe_repeat = persist_exists(SET_VIBE_REPEAT)? persist_read_bool(SET_VIBE_REPEAT) : true;
#ifdef PBL_COLOR
	fg_colour = persist_exists(SET_FG_COLOUR)? GColorFromHEX(persist_read_int(SET_FG_COLOUR)) : COLOR_FALLBACK(GColorWhite,GColorWhite);
	bg_colour = persist_exists(SET_BG_COLOUR)? GColorFromHEX(persist_read_int(SET_BG_COLOUR)) : COLOR_FALLBACK(GColorDukeBlue,GColorBlack);
#else
	// set the default colours to use.
	fg_colour = COLOR_FALLBACK(GColorWhite,GColorWhite);
	bg_colour = COLOR_FALLBACK(GColorDukeBlue,GColorBlack);
#endif
	TurnOffAllVibrations = persist_exists(SET_NO_VIBE)? persist_read_bool(SET_NO_VIBE) : true;
	BacklightOnCharge = persist_exists(SET_LIGHT_ON_CHG)? persist_read_bool(SET_LIGHT_ON_CHG) : false;
	MonochromeBackground = persist_exists(SET_MONOCHROME)? persist_read_bool(SET_MONOCHROME) : false;
#ifdef DEBUG_LEVEL
	APP_LOG(APP_LOG_LEVEL_INFO, "display_seconds: %i", display_seconds);
#endif
	//initialise the Time Fonts
	time_font_normal = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_GOTHAM_BOLD_40));
	time_font_small = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_GOTHAM_BOLD_30));
	//Initialise the time format string.  No seconds here.
	if(clock_is_24h_style() == true)
	{
		if(display_seconds)
		{
			snprintf(time_watch_format, 9, "%s", TIME_24HS_FORMAT);
			time_font = time_font_small;
		}
		else
		{
			snprintf(time_watch_format, 6, "%s", TIME_24H_FORMAT);
			time_font = time_font_normal;
		}
	}
	else
	{
		if(display_seconds)
		{
			snprintf(time_watch_format, 9, "%s", TIME_12HS_FORMAT);
			time_font = time_font_small;
		}
		else
		{
			snprintf(time_watch_format, 6, "%s", TIME_12H_FORMAT);
			time_font = time_font_normal;
		}
	}
#ifdef DEBUG_LEVEL
	APP_LOG(APP_LOG_LEVEL_INFO, "time_watch_format: %s", time_watch_format);
#endif

	//APP_LOG(APP_LOG_LEVEL_INFO, "INIT CODE IN");

	// subscribe to the tick timer service
	tick_timer_service_subscribe(SECOND_UNIT, &handle_second_tick_cgm);

	// subscribe to the bluetooth connection service
	bluetooth_connection_service_subscribe(handle_bluetooth_cgm);

	//subscribe to the battery handler
	battery_state_service_subscribe(battery_handler);

	// init the window pointer to NULL if it needs it
	if (window_cgm != NULL)
	{
		window_cgm = NULL;
	}

	// create the windows
	window_cgm = window_create();
	window_set_background_color(window_cgm, GColorBlack);
	window_set_window_handlers(window_cgm, (WindowHandlers) 
			{
		.load = window_load_cgm,
				.unload = window_unload_cgm
			});

	//APP_LOG(APP_LOG_LEVEL_INFO, "INIT CODE, REGISTER APP MESSAGE ERROR HANDLERS");
	app_message_register_inbox_dropped(inbox_dropped_handler_cgm);
	app_message_register_outbox_failed(outbox_failed_handler_cgm);
	app_message_register_inbox_received(inbox_received_handler_cgm);

	//APP_LOG(APP_LOG_LEVEL_INFO, "INIT CODE, ABOUT TO CALL APP MSG OPEN");
#ifdef PBL_PLATFORM_APLITE
	app_message_open(512, 1024);
#else
	app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
#endif
	//APP_LOG(APP_LOG_LEVEL_INFO, "INIT CODE, APP MSG OPEN DONE");

	const bool animated_cgm = true;
	window_stack_push(window_cgm, animated_cgm);

#if defined(PBL_HEALTH)
	start_data_log();
	if (CollectHealth) health_subscribe();
#else
	APP_LOG(APP_LOG_LEVEL_ERROR, "Health service not supported!");
#endif

#ifdef DEBUG_LEVEL
	APP_LOG(APP_LOG_LEVEL_INFO, "init_cgm done.");
#endif
}	// end init_cgm

static void health_subscribe() {
	// Attempt to subscribe
#if defined(PBL_HEALTH)
	if (!health_service_events_subscribe(health_handler, NULL)) {
		APP_LOG(APP_LOG_LEVEL_ERROR, "Health not available!");
	} else {
		//#ifdef DEBUG_LEVEL
		APP_LOG(APP_LOG_LEVEL_INFO, "subscribed to health");
		//#endif
		// health ok
	}
#endif
}

static void health_unsubscribe() {
#if defined(PBL_HEALTH)
	// Finish the session and sync data if appropriate
	health_service_events_unsubscribe();
	//#ifdef DEBUG_LEVEL
	APP_LOG(APP_LOG_LEVEL_INFO, "unsubscribed from health");
	//#endif

#endif
}
static void deinit_cgm(void)
{
#if defined(PBL_HEALTH)
	// Finish the session and sync data if appropriate
	health_unsubscribe();
	stop_data_log();
#endif

	//APP_LOG(APP_LOG_LEVEL_INFO, "DEINIT CODE IN");
	// Make sure we are not handling a second tick.
	while (handling_second) {};

	// unsubscribe to the tick timer service
	//APP_LOG(APP_LOG_LEVEL_INFO, "DEINIT, UNSUBSCRIBE TICK TIMER");
	tick_timer_service_unsubscribe();

	// unsubscribe to the bluetooth connection service
	//APP_LOG(APP_LOG_LEVEL_INFO, "DEINIT, UNSUBSCRIBE BLUETOOTH");
	bluetooth_connection_service_unsubscribe();

	battery_state_service_unsubscribe();

	// cancel timers if they exist
	//APP_LOG(APP_LOG_LEVEL_INFO, "DEINIT, CANCEL APP TIMER");
	if (timer_cgm != NULL)
	{
		app_timer_cancel(timer_cgm);
		timer_cgm = NULL;
	}

	//APP_LOG(APP_LOG_LEVEL_INFO, "DEINIT, CANCEL BLUETOOTH TIMER");
	if (BT_timer != NULL)
	{
		app_timer_cancel(BT_timer);
		BT_timer = NULL;
	}

	// destroy the window if it exists
	//APP_LOG(APP_LOG_LEVEL_INFO, "DEINIT, CHECK WINDOW POINTER FOR DESTROY");
	//APP_LOG(APP_LOG_LEVEL_INFO, "DEINIT, CHECK WINDOW POINTER FOR NULL");
	if (window_cgm != NULL)
	{
		//APP_LOG(APP_LOG_LEVEL_INFO, "DEINIT, WINDOW POINTER NOT NULL, SET TO NULL");
		window_cgm = NULL;
	}
	//unload the custom time font.
	fonts_unload_custom_font(time_font_normal);
	fonts_unload_custom_font(time_font_small);


	//APP_LOG(APP_LOG_LEVEL_INFO, "DEINIT CODE OUT");
} // end deinit_cgm

#if defined(PBL_HEALTH)

static void write_log(int id, int value) {

	if ((id == MOVEMENT_LOG) && ((time(NULL) - last_movement_time) < 60)) {
		return;
	}
	last_movement_time = time(NULL);

	uint32_t d[2] = {(uint32_t) last_movement_time, (uint32_t) value};

	APP_LOG(APP_LOG_LEVEL_INFO, "Logging data");

	DataLoggingResult result = data_logging_log((id == HEARTRATE_LOG) ? s_session_heartrate : s_session_movement, &d,
			2);
	if (result != DATA_LOGGING_SUCCESS) {
		APP_LOG(APP_LOG_LEVEL_ERROR, "Error logging to queue: %d value %d data: err:%d", (int) id, (int) value,
				(int) result);
	}
}

void health_handler(HealthEventType event, void *context) {
	if (!CollectHealth) return;
	// Which type of event occurred?
	switch (event) {
	case HealthEventSignificantUpdate:
		APP_LOG(APP_LOG_LEVEL_DEBUG,
				"New HealthService HealthEventSignificantUpdate event");
		break;
	case HealthEventMovementUpdate:
		APP_LOG(APP_LOG_LEVEL_DEBUG,
				"New HealthService HealthEventMovementUpdate event");

		HealthValue steps = health_service_sum_today(HealthMetricStepCount);

		if ((steps != laststeps) && (abs(steps - laststeps) > 20)) {
			APP_LOG(APP_LOG_LEVEL_INFO, "New Steps daily total: %d", (int) steps);
			laststeps = steps;
			write_log(MOVEMENT_LOG, steps);
		} // if new data
		break;

	case HealthEventSleepUpdate:
		APP_LOG(APP_LOG_LEVEL_DEBUG,
				"New HealthService HealthEventSleepUpdate event");
		break;
	case HealthEventHeartRateUpdate:
		APP_LOG(APP_LOG_LEVEL_INFO,
				"New HealthService HealthEventHeartRateUpdate event");

		HealthServiceAccessibilityMask hr = health_service_metric_accessible(HealthMetricHeartRateBPM, time(NULL),
				time(NULL));
		if (hr & HealthServiceAccessibilityMaskAvailable) {
			HealthValue value = health_service_peek_current_value(HealthMetricHeartRateBPM);

			if ((value > 0) && (value != lastbpm)) {
				lastbpm = value;
				APP_LOG(APP_LOG_LEVEL_INFO, "New BPM: %d", (int) value);
				write_log(HEARTRATE_LOG, value);
			}
		}
		break;
	case HealthEventMetricAlert:
		APP_LOG(APP_LOG_LEVEL_DEBUG,
				"New HealthService HealthEventMetricAlert event");
		break;
	default:
		APP_LOG(APP_LOG_LEVEL_INFO, "Received other health event not handled %d", event);
		break;
	}
}

static void start_data_log() {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "starting log");
	alternator = alternator ^ 0x8;
	s_session_heartrate = data_logging_create(HEARTRATE_LOG | alternator, DATA_LOGGING_UINT, sizeof(uint32_t), true);
	s_session_movement = data_logging_create(MOVEMENT_LOG | alternator, DATA_LOGGING_UINT, sizeof(uint32_t), true);
}

static void stop_data_log() {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Stopping log");
	data_logging_finish(s_session_heartrate);
	data_logging_finish(s_session_movement);
}

static void restart_data_log() {
	stop_data_log();
	start_data_log();
}

#endif

int main(void)
{
	init_cgm();
	app_event_loop();
	deinit_cgm();

} // end main

