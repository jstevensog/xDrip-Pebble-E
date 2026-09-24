//Set up Platform specific values and global variables.
#ifndef __CONSTANT_H__
#define __CONSTANT_H__

#include <pebble.h>

/**
 * Settings values and other constants
 */
#define CGM_ICON_KEY			0	// TUPLE_CSTRING, MAX 2 BYTES (10)
#define CGM_BG_KEY			    1	// TUPLE_CSTRING, MAX 4 BYTES (253 OR 22.2)
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
#define SET_NO_VIBE			    104	// Setting key - No Vibrations
#define SET_LIGHT_ON_CHG		105	// Setting key - Backlight on when charging
#define SET_SAMECOLOUR			106	// Setting key - Same Colours top and bottom
#define SET_NO_DELTA			107	// Setting key - Do not display the Delta value
#define SET_NO_ARROWS			108	// Setting key - Do not show arrows
#define SET_HIGH_LINE			110	// Setting key - Enable High line on graph.
#define SET_LOW_LINE			111	// Setting key - Enable Low line on graph.
#define SET_COLLECT_HEALTH		112	// setting key - Enable Health collection
#define SET_MESSAGE_TIMEOUT		113	// Setting key - Message timeout
#define SET_BOLD_TIMEAGO		114	// Setting key - Meke the TimeAgo text bold if true
#define SET_BOTTOM_LEFT_TEXT	115	// Setting key - What to display in the bottom left text field
#define SET_BOTTOM_RIGHT_TEXT	116	// Setting key - What to display in the bottom right text field
#define SET_USE_PNG             117     // Set the use of PNG images from xDrip or local rendered
#define SET_SHOW_UNIT           118     // Show unit in delta screen
#define SET_SHOW_DELTA          119     // Show the delta
#define SET_SHOW_SLOPE          120     // Show the slope icon
#define SET_SHOW_TREND          121     // Show the trend
#define CGM_SYNC_KEY			1000	// key pebble will use to request an update.	This should probably include the "capabilities" bits
#define PBL_PLATFORM			1001	// key pebble will use to send it's platform	This is probably not required under the new famework.
#define PBL_APP_VER			    1002	// key pebble will use to send the face/app version.	This is probably not required under the new framework.
#define PBL_TREND_SIZE			1003	// key pebble will use to send trend image size.
#define PBL_TREND_LINES		 	1004	// key pebble will use to send trend line options.
#define PBL_TREND_PERIOD		1005	// key pebble will use to send the trend period it wants.
#define PBL_DISP_OPTS			1006	// key pebble will use to send display options (delta/arrows).
#define PBL_VIBE_OPTS			1007	// key pebble will use to send vibration options (alerts, missed signal, no bluetooth)

// Metric Display defines
#define METRIC_NONE		     0
#define METRIC_PHONEBATT	 1
#define METRIC_WATCHBATT	 2
#define METRIC_STEPS		 3
#define METRIC_HEARTRATE	 4
#define METRIC_SENSOR_EXPIRY 5

// platform defines
#ifdef PBL_PLATFORM_APLITE
#define PLATFORM 0
#elif PBL_PLATFORM_BASALT
#define PLATFORM 1
#elif PBL_PLATFORM_CHALK
#define PLATFORM 2
#elif PBL_PLATFORM_DIORITE
#define PLATFORM 3
#elif PBL_PLATFORM_EMERY
#define PLATFORM 4
#elif PBL_PLATFORM_FLINT
#define PLATFORM 5
#elif PBL_PLATFORM_GABBRO
#define PLATFORM 6
#else
#error Platform not defined or not supported
#endif

// time defines
#define WATCH_MSGSEND_SECS      60
#define LOADING_MSGSEND_SECS    2


// resource and index definitions
#define NONE_SPECVALUE_ICON    RESOURCE_ID_IMAGE_NONE
#define BROKEN_ANTENNA_ICON    RESOURCE_ID_IMAGE_BROKEN_ANTENNA
#define BLOOD_DROP_ICON        RESOURCE_ID_IMAGE_BLOOD_DROP 
#define STOP_LIGHT_ICON        RESOURCE_ID_IMAGE_STOP_LIGHT 
#define HOURGLASS_ICON         RESOURCE_ID_IMAGE_HOURGLASS
#define QUESTION_MARKS_ICON    RESOURCE_ID_IMAGE_QUESTION_MARKS
#define LOGO_SPECVALUE_ICON    RESOURCE_ID_IMAGE_LOGO

// ICON ASSIGNMENTS OF ARROW DIRECTIONS

// INDEX FOR ARRAY OF ARROW ICON IMAGES
#define NONE_ARROW_ICON    RESOURCE_ID_IMAGE_NONE
#define UPUP_ICON          RESOURCE_ID_IMAGE_UPUP
#define UP_ICON            RESOURCE_ID_IMAGE_UP
#define UP45_ICON          RESOURCE_ID_IMAGE_UP45
#define FLAT_ICON          RESOURCE_ID_IMAGE_FLAT
#define DOWN45_ICON        RESOURCE_ID_IMAGE_DOWN45
#define DOWN_ICON          RESOURCE_ID_IMAGE_DOWN
#define DOWNDOWN_ICON      RESOURCE_ID_IMAGE_DOWNDOWN
#define LOGO_ARROW_ICON    RESOURCE_ID_IMAGE_LOGO
#define ERR_ARROW_ICON     RESOURCE_ID_IMAGE_ERR

#define NO_ARROW 0
#define DOUBLEUP_ARROW 1
#define SINGLEUP_ARROW 2
#define UP45_ARROW 3
#define FLAT_ARROW 4
#define DOWN45_ARROW 5
#define SINGLEDOWN_ARROW 6
#define DOUBLEDOWN_ARROW 7
#define NOTCOMPUTE 8
#define OUTOFRANGE 9
#define NO_ANTENNA 10
#define NOT_CALIBRATED 11
#define SENSOR_NOT_ACTIVE 12
#define HOURGLASS 13
#define QUESTIONMARK 14
#define SPECIAL_VALUE 15

// message size constants
#define ICON_MSGSTR_SIZE 4
#define BG_MSGSTR_SIZE 6
#define BGDELTA_MSGSTR_SIZE 13
#define BATTLEVEL_MSGSTR_SIZE 5


// BATTERY LEVEL FORMATTED SIZE used for Bridge/Phone and Watch battery indications
#if defined(PBL_PLATFORM_EMERY) || defined(PBL_PLATFORM_GABBRO)
#define BATTLEVEL_FORMATTED_SIZE 12
#else
#define BATTLEVEL_FORMATTED_SIZE 9
#endif

// global constants for time durations
#define MINUTEAGO ((uint8_t) 60)
#define HOURAGO ((uint16_t) 60*(60))
#define DAYAGO ((uint32_t) 24*(60*60))
#define WEEKAGO ((uint32_t) 7*(24*60*60))
#define MS_IN_A_SECOND ((uint16_t) 1000)

// Constants for string buffers
// If add month to date, buffer size needs to increase to 12; also need to reformat date_app_text init string

// global constants for t init string
#define TIME_TEXTBUFF_SIZE ((uint8_t) 10)
#define DATE_TEXTBUFF_SIZE ((uint8_t) 11)
#define LABEL_BUFFER_SIZE ((uint8_t) 6)
#define TIMEAGO_BUFFER_SIZE ((uint8_t) 10)

// * START OF CONSTANTS THAT CAN BE CHANGED; DO NOT CHANGE IF YOU DO NOT KNOW WHAT YOU ARE DOING **
// * FOR MMOL, ALL VALUES ARE STORED AS INTEGER; LAST DIGIT IS USED AS DECIMAL **
// * BE EXTRA CAREFUL OF CHANGING SPECIAL VALUES OR TIMERS; DO NOT CHANGE WITHOUT EXPERT HELP **

// Vibration Levels; 0 = NONE; 1 = LOW; 2 = MEDIUM; 3 = HIGH
// IF YOU DO NOT WANT A SPECIFIC VIBRATION, SET TO 0
#define APPSYNC_ERR_VIBE ((uint8_t) 1)
#define APPMSG_INDROP_VIBE ((uint8_t) 1)
#define APPMSG_OUTFAIL_VIBE ((uint8_t) 1)
#define BTOUT_VIBE ((uint8_t) 1)
#define LOWBATTERY_VIBE ((uint8_t) 1)

// Control Messages
// IF YOU DO NOT WANT A SPECIFIC MESSAGE, SET TO true
#define TurnOff_NOBLUETOOTH_Msg ((bool) false)
#define TurnOff_CHECKPHONE_Msg ((bool) false)

// Bluetooth Timer Wait Time, in Seconds
// RANGE 0-240
// THIS IS ONLY FOR BAD BLUETOOTH CONNECTIONS
// TRY EXTENDING THIS TIME TO SEE IF IT WILL HELP SMOOTH CONNECTION
// CGM DATA RECEIVED EVERY 60 SECONDS, GOING BEYOND THAT MAY RESULT IN MISSED DATA
#define BT_ALERT_WAIT_SECS ((uint8_t) 10)

// * END OF CONSTANTS THAT CAN BE CHANGED; DO NOT CHANGE IF YOU DO NOT KNOW WHAT YOU ARE DOING **


/** 
 * constants only used in some functions
 * collected here to avoid redefines
 */
// load_bg
#define SENSOR_NOT_ACTIVE_VALUE "?SN"
#define MINIMAL_DEVIATION_VALUE	"?MD"
#define NO_ANTENNA_VALUE "?NA"
#define SENSOR_NOT_CALIBRATED_VALUE "?NC"
#define STOP_LIGHT_VALUE "?CD"
#define HOURGLASS_VALUE "hourglass"
#define QUESTION_MARKS_VALUE "???"
#define BAD_RF_VALUE "?RF"

// load_bg_delta
#define MSGLAYER_BUFFER_SIZE 14
#define BGDELTA_LABEL_SIZE 14
#define BGDELTA_FORMATTED_SIZE 14

#define BWP_SYMBOL "😐"

#define TRUE 1
#define FALSE 0
#endif // __CONSTANT_H__
