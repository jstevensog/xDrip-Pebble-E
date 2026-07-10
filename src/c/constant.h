//Set up Platform specific values and global variables.
#ifndef __CONSTANT_H__
#define __CONSTANT_H__

#include <pebble.h>

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

// message size constants
#define ICON_MSGSTR_SIZE 4
#define BG_MSGSTR_SIZE 6
#define BGDELTA_MSGSTR_SIZE 13
#define BATTLEVEL_MSGSTR_SIZE 5


// BATTERY LEVEL FORMATTED SIZE used for Bridge/Phone and Watch battery indications
#define BATTLEVEL_FORMATTED_SIZE 8


// global constants for time durations
#define MINUTEAGO ((uint8_t) 60)
#define HOURAGO ((uint16_t) 60*(60))
#define DAYAGO ((uint32_t) 24*(60*60))
#define WEEKAGO ((uint32_t) 7*(24*60*60))
#define MS_IN_A_SECOND ((uint16_t) 1000)

// Constants for string buffers
// If add month to date, buffer size needs to increase to 12; also need to reformat date_app_text init string
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


#endif // __CONSTANT_H__
