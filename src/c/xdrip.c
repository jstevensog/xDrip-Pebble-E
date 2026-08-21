#include <pebble.h>
#include <stdarg.h>
#include "xdrip.h" // set DEBUG_LEVEL in here or on the pebble build command line
#include "debug.h" // must be included after xdrip.h
#include "api/communication.h"
#ifdef ENABLE_TREND_RENDERER
#include "api/trend.h"
#endif
/**
 * Variables
 */

// Boolean to allow/prevent re-raise of NO BLUETOOTH vibration
static bool vibe_repeat = false;
// variables for AppSync
AppSync sync_cgm;

static bool handling_second = false;
static bool doing_trend = false;
static bool global_lock = false;
static bool use_png = false;

//#ifdef PBL_PLATFORM_BASALT
uint8_t *trend_buffer = NULL;
static uint16_t trend_buffer_length = 0;
static uint16_t expected_trend_buffer_length = 0;
//#endif
bool display_message = false;

// variables for settings from Pebble Phone App.
static bool display_seconds = false;
// variables for timers and time
AppTimer *timer_cgm = NULL;
AppTimer *BT_timer = NULL;
time_t time_now = 0;

// global variable for bluetooth connection
bool bluetooth_connected_cgm = true;

// global variables for sync tuple functions
// buffers have to be static and hardcoded
static uint32_t current_icon = 0;
static char last_bg[6];
static bool currentBG_isMMOL = false;
static uint32_t last_battlevel = 100;
static uint32_t current_cgm_time = 0;
static uint32_t current_app_time = 0;
static char current_bg_delta[14];
static int current_step_count = 0;
static int current_hbm = 0;

// global BG snooze timer
static uint8_t lastAlertTime = 0;

// global special value alert
static bool specvalue_alert = false;
// global flag to set the top and bottom colours the same
static bool SameColourTopAndBottom = false;

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

// Control Vibrations
// IF YOU WANT NO VIBRATIONS, SET TO true
static bool TurnOffAllVibrations = false;
// IF YOU WANT LESS INTENSE VIBRATIONS, SET TO true
static bool TurnOffStrongVibrations = false;

//Control Backlight
static bool BacklightOnCharge = false;

//Control TimeAgo text boldness
static bool TimeAgoBold = false;

/**
 * Message timeout indicator, this is separate from the other timers to allow users 
 * to control the update rate and thus battery life.
 */
static uint32_t message_tick_timeout = 15000; // default of 15s
static AppTimer *message_tick_timer = NULL;

/**
 * Global window and UI variables
 */
// ANYTHING THAT IS CALLED BY PEBBLE API HAS TO BE NOT STATIC

// windows definition.
Window *window_cgm = NULL;

// text layer definitions.
TextLayer *bg_layer = NULL;
TextLayer *cgmtime_layer = NULL;
TextLayer *delta_layer = NULL;	 	// BG DELTA LAYER
TextLayer *message_layer = NULL;	// MESSAGE LAYER
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


GBitmap *icon_bitmap = NULL;
GBitmap *appicon_bitmap = NULL;
GBitmap *specialvalue_bitmap = NULL;
GBitmap *bg_trend_bitmap = NULL;

static char time_watch_format[9] = TIME_24H_FORMAT;
static char time_watch_text[] = "00:00:00";
static char date_app_text[] = "Wed 13 Jan";
static char message_layer_text[13];
static GFont time_font;
static char message_layer_text[13];
static GFont time_font_small;
static GFont time_font_normal;

// Message Timer Wait Times, in Seconds
static uint8_t minutes_cgm = 0;

//Metric Display Left/Right
static uint8_t bottom_left_metric = 1;
static uint8_t bottom_right_metric = 1;

#ifdef PBL_HEALTH
TextLayer *step_count_text_layer = NULL;
#if defined(PBL_PLATFORM_EMERY) || defined(PBL_PLATFORM_FLINT) || defined(PBL_PLATFORM_GABBRO)
TextLayer *heart_rate_text_layer = NULL;
#endif
#endif

// comms framework
#ifdef ENABLE_COMM_FRAMEWORK
comm_callback comm_callbacks;

void set_icon(comm_slopeval value);
void set_phone_battery(comm_phonebat value);
void set_vibrate(comm_vibe value);
void set_bgl_delta(comm_bgl_delta value);
void set_bgl_timestamp(uint32_t timestamp); 
void set_bgl_value(comm_bgl_value value);
void set_bgl_data(comm_bgl_data *value); 
void set_png(comm_png_data data);
#endif


/**
 * Dirty markers
 */
typedef struct {
    uint32_t delta : 1;     // mark delta layer as dirty and update 
    uint32_t need_cgm : 1;  // make the heartbeat request delta and slope
    uint32_t step_count : 1;
    uint32_t hbm : 1;
} dirty_markers;

dirty_markers dirty = {
    .delta = 1,
    .need_cgm = 1,
    .step_count = 0,
    .hbm = 1,
}; // init one 

/**
 * predefines
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
	INFO("MYATOI: ENTER CODE");
	// Iterate through all characters of input string and update result
	for (int i = 0; str[i] != '\0'; ++i)
	{

	DEBUG("MYATOI, STRING IN: %s", &str[i] );

		if ( (str[i] >= ('0')) && (str[i] <= ('9')) )
		{
			res = res*10 + str[i] - '0';
		}
		TRACE("MYATOI, FOR RESULT OUT: %i", res );
	}
	INFO("MYATOI, FINAL RESULT OUT: %i", res );
	return res;
} // end myAtoi

static void destroy_null_GBitmap(GBitmap **GBmp_image)
{
	TRACE("DESTROY NULL GBITMAP: ENTER CODE");

	if (*GBmp_image != NULL)
	{
		TRACE("DESTROY NULL GBITMAP: POINTER EXISTS, DESTROY BITMAP IMAGE");
		gbitmap_destroy(*GBmp_image);
		if (*GBmp_image != NULL)
		{
			TRACE("DESTROY NULL GBITMAP: POINTER EXISTS, SET POINTER TO NULL");
			*GBmp_image = NULL;
		}
	}

	TRACE("DESTROY NULL GBITMAP: EXIT CODE");
} // end destroy_null_GBitmap

static void destroy_null_BitmapLayer(BitmapLayer **bmp_layer)
{
	TRACE("DESTROY NULL BITMAP: ENTER CODE");

	if (*bmp_layer != NULL)
	{
		TRACE("DESTROY NULL BITMAP: POINTER EXISTS, DESTROY BITMAP LAYER");
		bitmap_layer_destroy(*bmp_layer);
		if (*bmp_layer != NULL)
		{
			TRACE("DESTROY NULL BITMAP: POINTER EXISTS, SET POINTER TO NULL");
			*bmp_layer = NULL;
		}
	}

	TRACE("DESTROY NULL BITMAP: EXIT CODE");
} // end destroy_null_BitmapLayer *

static void destroy_null_TextLayer(TextLayer **txt_layer)
{
	TRACE("DESTROY NULL TEXT LAYER: ENTER CODE");

	if (*txt_layer != NULL)
	{
		TRACE("DESTROY NULL TEXT LAYER: POINTER EXISTS, DESTROY TEXT LAYER");
		text_layer_destroy(*txt_layer);
		if (*txt_layer != NULL)
		{
			TRACE("DESTROY NULL TEXT LAYER: POINTER EXISTS, SET POINTER TO NULL");
			*txt_layer = NULL;
		}
	}
TRACE("DESTROY NULL TEXT LAYER: EXIT CODE");
} // end destroy_null_TextLayer

static void create_update_bitmap(GBitmap **bmp_image, BitmapLayer *bmp_layer, const int resource_id)
{
	TRACE(" CREATE UPDATE BITMAP: ENTER CODE");

	// if bitmap pointer exists, destroy and set to NULL
	destroy_null_GBitmap(bmp_image);

	// create bitmap and pointer
	TRACE(" CREATE UPDATE BITMAP: CREATE BITMAP");
	*bmp_image = gbitmap_create_with_resource(resource_id);

	if (*bmp_image == NULL)
	{
		// couldn't create bitmap, return so don't crash
		TRACE(" CREATE UPDATE BITMAP: COULDNT CREATE BITMAP, RETURN");
		return;
	}
	else
	{
		// set bitmap
		TRACE(" CREATE UPDATE BITMAP: SET BITMAP");
		bitmap_layer_set_bitmap(bmp_layer, *bmp_image);
	}
	TRACE(" CREATE UPDATE BITMAP: EXIT CODE");
} // end create_update_bitmap

#ifdef PBL_HEALTH
// health_handler - handler to deal with health events
static void health_handler(HealthEventType event, void *context) {
	// Which type of event occurred?
	switch(event) {
		case HealthEventSignificantUpdate:
			LOG("health_handler: Significant Update");
		break;

		case HealthEventMovementUpdate:
 			LOG("health_handler: Movement Update");
		break;

		case HealthEventMetricAlert:
 			LOG("health_handler: Metric Alert");
		break;

		case HealthEventSleepUpdate:
			//LOG("health_handler: Sleep Update");
		break;
		
		case HealthEventHeartRateUpdate:
			LOG("health_handler: Heart rate Update");
		break;
		case HealthEventHRVUpdate:
			LOG("health_handler: Heart rate HRV Update");

	}
	update_health_metric_displays();
} //end health_handler

// update_health_metric_displays - Updates the bottom left and right metrics displays if they are displaying health metrics
static void update_health_metric_displays() {
	static char step_count_text[9];
	int step_count;

	// If there are no health metrics to display, do nothing and return.
	if(bottom_left_metric != METRIC_STEPS && bottom_right_metric != METRIC_STEPS && bottom_left_metric != METRIC_HEARTRATE && bottom_right_metric != METRIC_HEARTRATE) return;

	if(bottom_left_metric == METRIC_STEPS || bottom_right_metric == METRIC_STEPS) {
		HealthMetric metric = HealthMetricStepCount;
		time_t start = time_start_of_today();
		time_t end = time(NULL);

		// Check the metric has data available for today
		HealthServiceAccessibilityMask mask = health_service_metric_accessible(metric, start, end);

		if(mask & HealthServiceAccessibilityMaskAvailable) {
			// Data is available!
			step_count = health_service_sum_today(metric);
            if (step_count != current_step_count)  {
                dirty.step_count = 1;
                current_step_count = step_count;
                LOG("Steps today: %d", step_count);
                snprintf(step_count_text,8, "%i s", step_count);
            }
		} else {
			// No data recorded yet today
			LOG("Data unavailable!");
		}
		if(bottom_left_metric == METRIC_STEPS && dirty.step_count) {
		 	text_layer_set_text(bottom_left_text_layer, step_count_text);
            dirty.step_count = 0;
		}
		if(bottom_right_metric == METRIC_STEPS && dirty.step_count) {
		    text_layer_set_text(bottom_right_text_layer, step_count_text);
            dirty.step_count = 0;
		}
	}	
#if defined(PBL_PLATFORM_EMERY) || defined(PBL_PLATFORM_FLINT) || defined(PBL_PLATFORM_GABBRO)
	if(bottom_left_metric == METRIC_HEARTRATE || bottom_right_metric == METRIC_HEARTRATE) {
		static char s_hrm_buffer[16] = "Wait.. \U0001F493";
		HealthValue val = 0;
		HealthServiceAccessibilityMask hr = health_service_metric_accessible(HealthMetricHeartRateBPM, time(NULL), time(NULL));
		if (hr & HealthServiceAccessibilityMaskAvailable) {
			val = health_service_peek_current_value(HealthMetricHeartRateBPM);
			LOG("Heart Rate data is \"%lu\"", (uint32_t)val);
			if(val > 0 && val != current_hbm) {
				// Display HRM value
                current_hbm = val;
                dirty.hbm = 1;
				snprintf(s_hrm_buffer, sizeof(s_hrm_buffer), "%lu \U0001F493", (uint32_t)val);
			}
		} else if (current_hbm == 0 && dirty.hbm) {
			snprintf(s_hrm_buffer, sizeof(s_hrm_buffer), "Wait.. \U0001F493");
		}
		if(bottom_left_metric == METRIC_HEARTRATE && dirty.hbm) {
			DEBUG("Setting bottom left metric to \"%lu\"", (uint32_t)val);
			text_layer_set_text(bottom_left_text_layer, s_hrm_buffer);
		}
		if(bottom_right_metric == METRIC_HEARTRATE && dirty.hbm) {
			DEBUG("Setting bottom right metric to \"%lu\"", (uint32_t)val);
			text_layer_set_text(bottom_right_text_layer, s_hrm_buffer);
		}
	}
#endif

}
#endif

// battery_handler - updates the pebble battery percentage.
static void battery_handler(BatteryChargeState charge_state)
{

	static char watch_battlevel_percent[9];
	// If there are no battery level metric display elements, exit
	if(bottom_left_metric != METRIC_WATCHBATT && bottom_right_metric != METRIC_WATCHBATT) {
		return;
	}

#ifdef PBL_COLOR 
	#ifdef PBL_ROUND
	snprintf(watch_battlevel_percent, BATTLEVEL_FORMATTED_SIZE, "%i%% ", charge_state.charge_percent);
	#else
	snprintf(watch_battlevel_percent, BATTLEVEL_FORMATTED_SIZE, "W:%i%% ", charge_state.charge_percent);
	#endif
#else
	snprintf(watch_battlevel_percent, BATTLEVEL_FORMATTED_SIZE, "W:%i%%", charge_state.charge_percent);
#endif
	LOG(" battery_handler: watch_battlevel_percent: %s", watch_battlevel_percent);
	LOG(" battery_handler: BackLightOnCharge: %u", BacklightOnCharge);
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
			
	if((bottom_left_metric == METRIC_WATCHBATT || bottom_right_metric == METRIC_WATCHBATT) && charge_state.is_charging)
	{
		LOG("Charging.  BacklightOnCharge:%u", BacklightOnCharge);
		if(bottom_left_metric == METRIC_WATCHBATT) {
#ifdef PBL_COLOR
			TRACE("COLOR DETECTED");
			text_layer_set_text_color(bottom_left_text_layer, bg_colour);
			text_layer_set_background_color(bottom_left_text_layer, GColorGreen);
#else
			TRACE("BW DETECTED");
			text_layer_set_text_color(bottom_left_text_layer, bg_colour);
			text_layer_set_background_color(bottom_left_text_layer, fg_colour);
#endif
		}
		if(bottom_right_metric == METRIC_WATCHBATT) {
#ifdef PBL_COLOR
			TRACE("COLOR DETECTED");
			text_layer_set_text_color(bottom_right_text_layer, bg_colour);
			text_layer_set_background_color(bottom_right_text_layer, GColorGreen);
#else
			TRACE("BW DETECTED");
			text_layer_set_text_color(bottom_right_text_layer, bg_colour);
			text_layer_set_background_color(bottom_right_text_layer, fg_colour);
#endif
		}
	}
	else if(bottom_left_metric == METRIC_WATCHBATT || bottom_right_metric == METRIC_WATCHBATT)
	{
		LOG("battery_handler: Not Charging.  BacklightOnCharge:%u", BacklightOnCharge);
#ifdef PBL_COLOR
		TRACE("battery_handler: COLOR DETECTED");
		if(charge_state.charge_percent > 40)
		{
			TRACE("battery_handler: BATTERY > 40");
			if(bottom_left_metric == METRIC_WATCHBATT) {
				LOG("battery_handler: >40%% bottom_left_text_layer: GColorGreen");
				text_layer_set_text_color(bottom_left_text_layer, fg_colour);
			}
			if(bottom_right_metric == METRIC_WATCHBATT) {
				LOG("battery_handler: >40%% bottom_right_text_layer: GColorGreen");
				text_layer_set_text_color(bottom_right_text_layer, fg_colour);
			}
		}
		else if (charge_state.charge_percent > 20)
		{
			TRACE("battery_handler: BATTERY > 20");
			if(bottom_left_metric == METRIC_WATCHBATT) {
				LOG("battery_handler: >20%% bottom_left_text_layer: GColorYellow");
				text_layer_set_text_color(bottom_left_text_layer, GColorYellow);
			}
			if(bottom_right_metric == METRIC_WATCHBATT) {
				LOG("battery_handler: >20%% bottom_right_text_layer: GColorYellow");
				text_layer_set_text_color(bottom_right_text_layer, GColorYellow);
			}
		}
		else
		{
			TRACE("battery_handler: BATTERY <= 20");
			if(bottom_left_metric == METRIC_WATCHBATT) {
				LOG("battery_handler: <=20%% bottom_left_text_layer: GColorRed");
				text_layer_set_text_color(bottom_left_text_layer, GColorRed);
			}
			if(bottom_right_metric == METRIC_WATCHBATT) {
				LOG("battery_handler: <=20%% bottom_right_text_layer: GColorRed");
				text_layer_set_text_color(bottom_right_text_layer, GColorRed);
			}
		}
		if(bottom_left_metric == METRIC_WATCHBATT) {
			LOG("battery_handler: Normal, bottom_left_text_layer: GColorClear");
			text_layer_set_background_color(bottom_left_text_layer, GColorClear);
		}
		if(bottom_right_metric == METRIC_WATCHBATT) {
			LOG("battery_handler: Normal, bottom_right_text_layer: GColorClear");
			text_layer_set_background_color(bottom_right_text_layer, GColorClear);
		}
#else
		TRACE("battery_handler: BW DETECTED");
		if(bottom_left_metric == METRIC_WATCHBATT) {
			text_layer_set_text_color(bottom_left_text_layer, GColorWhite);
			text_layer_set_background_color(bottom_left_text_layer, GColorBlack);
		}
		if(bottom_right_metric == METRIC_WATCHBATT) {
			text_layer_set_text_color(bottom_right_text_layer, GColorWhite);
			text_layer_set_background_color(bottom_right_text_layer, GColorBlack);
		}
#endif
	}
	if(bottom_left_metric == METRIC_WATCHBATT) {
		text_layer_set_text(bottom_left_text_layer, watch_battlevel_percent);
	}
	if(bottom_right_metric == METRIC_WATCHBATT) {
		text_layer_set_text(bottom_right_text_layer, watch_battlevel_percent);
	}


} // end battery_handler

static void alert_handler_cgm(uint8_t alertValue)
{
//	TRACE("ALERT HANDLER");
	LOG("alert_handler: alertValue: %d", alertValue);
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
			LOG("alert_handler: LOW ALERT");
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
			LOG("alert_handler: MEDIUM ALERT");
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
			LOG("alert_handler: HIGH ALERT");
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
	TRACE("HANDLE BT: ENTER CODE");

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
		LOG("BT HANDLER: TIMER POP, NO BLUETOOTH");
		alert_handler_cgm(BTOUT_VIBE);
		BluetoothAlert = true;

		// Reset timer pop
		if(vibe_repeat) 
		{
			BT_timer_pop = false;
		}

		TRACE("NO BLUETOOTH");
		if (!TurnOff_NOBLUETOOTH_Msg)
		{
#ifdef PBL_COLOR
			text_layer_set_text_color(delta_layer, GColorRed);
#endif
			text_layer_set_text(delta_layer, "NO BLUETOOTH");
            // make sure we get the data we need
            dirty.need_cgm = 1;
            reset_timer_callback_cgm(2);
		}

		// erase cgm and app ago times
		text_layer_set_text(cgmtime_layer, "");
	}

	else
	{
		// Bluetooth is on, reset BluetoothAlert
		TRACE("HANDLE BT: BLUETOOTH ON");
		BluetoothAlert = false;
		if (BT_timer == NULL)
		{
			// no timer is set, so need to reset timer pop
			BT_timer_pop = false;
		}
#ifdef PBL_COLOR
		if(SameColourTopAndBottom) {
			text_layer_set_text_color(delta_layer, fg_colour);
		} else {
			text_layer_set_text_color(delta_layer, bg_colour);
		}
#endif

	}

	TRACE("BluetoothAlert: %i", BluetoothAlert);
} // end handle_bluetooth_cgm

void BT_timer_callback(void *data)
{
	TRACE("BT TIMER CALLBACK: ENTER CODE");

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
	LOG("sync_error_callback_cgm: MSG ERR CODE: %i RES: %s", appsync_error, translate_app_error(appsync_error));
	LOG("sync_error_callback_cgm: DICT ERR CODE: %i RES: %s", appsync_dict_error, translate_dict_error(appsync_dict_error));

	bluetooth_connected_cgm = bluetooth_connection_service_peek();

	if (!bluetooth_connected_cgm)
	{
		// bluetooth is out, BT message already set; return out
		return;
	}

	appsync_err_openerr = app_message_outbox_begin(&iter);

	LOG("APP SYNC OPEN ERR CODE: %i RES: %s", appsync_err_openerr, translate_app_error(appsync_err_openerr));

	if (appsync_err_openerr == APP_MSG_OK)
	{
		// reset AppSyncErrAlert to flag for vibrate
		AppSyncErrAlert = false;

		// send message
		appsync_err_senderr = app_message_outbox_send();
		TRACE("APP SYNC SEND ERR CODE: %i RES: %s", appsync_err_senderr, translate_app_error(appsync_err_senderr));
		if (appsync_err_senderr != APP_MSG_OK  && appsync_err_senderr != APP_MSG_BUSY && appsync_err_senderr != APP_MSG_SEND_REJECTED)
		{
			INFO("APP SYNC SEND ERROR");
			DEBUG("APP SYNC SEND ERR CODE: %i RES: %s", appsync_err_senderr, translate_app_error(appsync_err_senderr));
		}
		else
		{
			return;
		}
	}

//	INFO("APP SYNC RESEND ERROR");
	DEBUG("APP SYNC RESEND ERR CODE: %i RES: %s", appsync_err_openerr, translate_app_error(appsync_err_openerr));
	DEBUG("AppSyncErrAlert:	%i", AppSyncErrAlert);

	bluetooth_connected_cgm = bluetooth_connection_service_peek();

	if (!bluetooth_connected_cgm || appsync_err_openerr == APP_MSG_BUSY)
	{
		// bluetooth is out, BT message already set; return out
		return;
	}

	// set message to RESTART WATCH -> PHONE
#if DEBUG
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
		LOG("APPSYNC ERROR: VIBRATE");
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
	INFO("inbox_dropped_handler_cgm");
	DEBUG("APPMSG IN DROP ERR CODE: %i RES: %s", appmsg_indrop_error, translate_app_error(appmsg_indrop_error));
	
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
//			INFO("inbox_drepped_handler_cgm: SEND ERROR");
			DEBUG("inbox_drepped_handler_cgm: SEND ERROR : %i RES: %s", appmsg_indrop_senderr, translate_app_error(appmsg_indrop_senderr));
		}
		else
		{
			return;
		}
	}
//	INFO("APPMSG IN DROP RESEND ERROR");
	DEBUG("inbox_drepped_handler_cgm: RESEND ERR CODE: %i RES: %s", appmsg_indrop_openerr, translate_app_error(appmsg_indrop_openerr));
	DEBUG("AppMsgInDropAlert:	%i", AppMsgInDropAlert);

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
	LOG("inbox_drepped_handler_cgm: VIBRATE");
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
//	INFO("APPMSG OUT FAIL ERROR");
	DEBUG("outbox_failed_handler_cgm: ERR CODE: %i RES: %s", appmsg_outfail_error, translate_app_error(appmsg_outfail_error));

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

//	INFO("APPMSG OUT FAIL RESEND ERROR");
	DEBUG("outbox_failed_handler_cgm RESEND ERR CODE: %i RES: %s", appmsg_outfail_openerr, translate_app_error(appmsg_outfail_openerr));
	DEBUG("AppMsgOutFailAlert: %i", AppMsgOutFailAlert);

	bluetooth_connected_cgm = bluetooth_connection_service_peek();

	if (!bluetooth_connected_cgm || appmsg_outfail_senderr != APP_MSG_SEND_REJECTED)
	{
		// bluetooth is out, BT message already set; return out
		return;
	}

	// set message to RESTART WATCH -> PHONE
#if DEBUG
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
		LOG("outbox_failed_handler_cgm: VIBRATE");
		alert_handler_cgm(APPMSG_OUTFAIL_VIBE);
		AppMsgOutFailAlert = true;
	}

} // end outbox_failed_handler_cgm

static void load_icon()
{
	TRACE("load_icon: Start");

	// check if special value set
	if (specvalue_alert == false)
	{

		// no special value, set arrow
		// check for arrow direction, set proper arrow icon
		TRACE("load_icon: CURRENT ICON: %lu", current_icon);
		switch (current_icon) {
			case NO_ARROW:
			case NOTCOMPUTE:
			case OUTOFRANGE:
			{
				create_update_bitmap(&icon_bitmap,icon_layer, NONE_ARROW_ICON);
				DoubleDownAlert = false;
			}
			break;

			case DOUBLEUP_ARROW:
			{
				create_update_bitmap(&icon_bitmap,icon_layer, UPUP_ICON);
				DoubleDownAlert = false;
			}
			break;

			case SINGLEUP_ARROW:
			{
				create_update_bitmap(&icon_bitmap,icon_layer, UP_ICON);
				DoubleDownAlert = false;
			}
			break;

			case UP45_ARROW:
			{
				create_update_bitmap(&icon_bitmap,icon_layer, UP45_ICON);
				DoubleDownAlert = false;
			}
			break;

			case FLAT_ARROW:
			{
				create_update_bitmap(&icon_bitmap,icon_layer, FLAT_ICON);
				DoubleDownAlert = false;
			}
			break;
			case DOWN45_ARROW:
			{
				create_update_bitmap(&icon_bitmap,icon_layer, DOWN45_ICON);
				DoubleDownAlert = false;
			}
			break;
			case SINGLEDOWN_ARROW:
			{
				create_update_bitmap(&icon_bitmap,icon_layer, DOWN_ICON);
				DoubleDownAlert = false;
			}
			break;
			case DOUBLEDOWN_ARROW:
			{
				create_update_bitmap(&icon_bitmap,icon_layer, DOWNDOWN_ICON);
				DoubleDownAlert = true; // does nothing
			}
			break;
			default:
			{
				// check for special cases and set icon accordingly
				// check bluetooth
				bluetooth_connected_cgm = bluetooth_connection_service_peek();

				// check to see if we are in the loading screen
				if (!bluetooth_connected_cgm)
				{
					// Bluetooth is out; in the loading screen so set logo
					create_update_bitmap(&icon_bitmap,icon_layer, LOGO_ARROW_ICON);
				}
				else
				{
					// unexpected, set error icon
					create_update_bitmap(&icon_bitmap,icon_layer, ERR_ARROW_ICON);
				}
				DoubleDownAlert = false;
			}
			break;
		}
	} // if specvalue_alert == false
	else   // this is just for log when need it
	{
		TRACE("load_icon: DONE");
	} // else specvalue_alert == true

} // end load_icon

static void load_bg()
{
	TRACE("load_bg: start");

	// CODE START

	// if special value set, erase anything in the icon field
	if (specvalue_alert == true)
	{
		create_update_bitmap(&specialvalue_bitmap,icon_layer, NONE_SPECVALUE_ICON);
	}

	// set special value alert to false no matter what
	specvalue_alert = false;
    dirty.need_cgm = 0;

	INFO("load_bg: last_bg: %s", last_bg);

#ifdef TEST_MODE
	snprintf(last_bg,sizeof(last_bg),"%s","100");
#endif
	// BG parse, check snooze, and set text

	// check for init code or error code
	if (last_bg[0] == '-')
	{
		lastAlertTime = 0;

		// check bluetooth
	 	bluetooth_connected_cgm = bluetooth_connection_service_peek();
#ifdef TEST_MODE
		bluetooth_connected_cgm = true;
#endif
		if (!bluetooth_connected_cgm)
		{
			// Bluetooth is out; set BT message
			TRACE("load_bg: NO BT, SET NO BT MESSAGE");
			if (!TurnOff_NOBLUETOOTH_Msg)
			{
				text_layer_set_text(delta_layer, "NO BLUETOOTH");
                
                // make sure we get the data we need
                dirty.need_cgm = 1;
                reset_timer_callback_cgm(2);
			} // if turnoff nobluetooth msg
		}
		else
		{
			// if init code, we will set it right in message layer
			TRACE("load_bg: UNEXPECTED BG: SET ERR ICON");
			text_layer_set_text(bg_layer, "ERR");
			create_update_bitmap(&icon_bitmap,icon_layer, NONE_SPECVALUE_ICON);
			specvalue_alert = true;
		}

	} 
	// if current_bg <= 0

	else
	{
		// valid BG
		// check for special value, if special value, then replace icon and blank BG; else send current BG
		TRACE("load_bg: BEFORE CREATE SPEC VALUE BITMAP");
		if (strcmp(last_bg, NO_ANTENNA_VALUE) == 0 || strcmp(last_bg, BAD_RF_VALUE) == 0)
		{
			TRACE("load_bg: last_bg: \"%s\"", last_bg);
			text_layer_set_text(bg_layer, "");
			create_update_bitmap(&specialvalue_bitmap,icon_layer,  BROKEN_ANTENNA_ICON);
			specvalue_alert = true;
		}
		else if (strcmp(last_bg, SENSOR_NOT_CALIBRATED_VALUE) == 0)
		{
			TRACE("load_bg: SET BLOOD DROP");
			text_layer_set_text(bg_layer, "");
			create_update_bitmap(&specialvalue_bitmap,icon_layer, BLOOD_DROP_ICON);
			specvalue_alert = true;
		}
		else if (strcmp(last_bg, SENSOR_NOT_ACTIVE_VALUE) == 0 || strcmp(last_bg, MINIMAL_DEVIATION_VALUE) == 0
				 || strcmp(last_bg, STOP_LIGHT_VALUE) == 0)
		{
			TRACE("load_bg: SET STOP LIGHT");
			text_layer_set_text(bg_layer, "");
			create_update_bitmap(&specialvalue_bitmap,icon_layer, STOP_LIGHT_ICON);
			specvalue_alert = true;
		}
		else if (strcmp(last_bg, HOURGLASS_VALUE) == 0)
		{
			TRACE("load_bg: SET HOUR GLASS");
			text_layer_set_text(bg_layer, "");
			create_update_bitmap(&specialvalue_bitmap,icon_layer, HOURGLASS_ICON);
			specvalue_alert = true;
		}
		else if (strcmp(last_bg, QUESTION_MARKS_VALUE) == 0)
		{
			//INFO("LOAD BG, SPECIAL VALUE: SET QUESTION MARKS, CLEAR TEXT");
			text_layer_set_text(bg_layer, "");
			TRACE("load_bg: SET QUESTION MARKS, SET BITMAP");
			create_update_bitmap(&specialvalue_bitmap,icon_layer, QUESTION_MARKS_ICON);
			TRACE("load_bg: SET QUESTION MARKS, DONE");
			specvalue_alert = true;
		}

		TRACE("load_bg: AFTER CREATE SPEC VALUE BITMAP");

		if (specvalue_alert == false)
		{
			// we didn't find a special value, so set BG instead
			// arrow icon already set separately
			TRACE("load_bg: SET BG: %s ", last_bg);
			text_layer_set_text(bg_layer, last_bg);
		} // end bg checks (if special_value_bitmap)


	}

	TRACE("load_bg: SNOOZE VALUE: %d", lastAlertTime);
	LOG("load_bg: bg_layer is \"%s\"", text_layer_get_text(bg_layer));


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
	TRACE("load_cgmtime: START");

	// VARIABLES
	// NOTE: buffers have to be static and hardcoded
	uint32_t current_cgm_timeago = 0;
	int cgm_timeago_diff = 0;
	static char formatted_cgm_timeago[10];
	static char cgm_label_buffer[6];

	// CODE START
#ifdef TEST_MODE
	current_cgm_time = time(NULL);
#endif

	// initialize label buffer
	strncpy(cgm_label_buffer, "", LABEL_BUFFER_SIZE);

	if (current_cgm_time == 0)
	{
		// Init code or error code; set text layer & icon to empty value
		TRACE("load_cgmtime, CGM TIME AGO INIT OR ERROR CODE: %s", cgm_label_buffer);
		text_layer_set_text(cgmtime_layer, "");
		//create_update_bitmap(&cgmicon_bitmap,cgmicon_layer,TIMEAGO_ICONS[NONE_TIMEAGO_ICON_INDX]);
	}
	else
	{
		// set rcvr on icon
		//create_update_bitmap(&cgmicon_bitmap,cgmicon_layer,TIMEAGO_ICONS[RCVRON_ICON_INDX]);

		time_now = time(NULL);
		/*
		* Since 4.17 (or maybe 4.16) get_UTC_offset accepts isdst in addition 
		* to CEST/CEDT and adds 3600s.
		* This results in 1h offset. 
		* Note: Setting isdst should not offset time. It's indicative only.
		*
		* To avoid this issue we use the tm_gmtoff which should always be the
		* local offset from UTC.
		* Thanks Tristan for the fix.
		*/
		// Leaving this as per Tristan's work.  Should probably be #if defined, to reduce code on older pebbles, but this is easier while it works.
#ifndef ENABLE_COMM_FRAMEWORK
        // new framework sends trend/bgl timestamps as UTC

		if (watch_info_get_model() > WATCH_INFO_MODEL_PEBBLE_TIME_2) {
			//this code should only run on core devices models.  Hopefully this will not change.
			struct tm *lc_tm = localtime(&time_now);
			time_now = abs(time_now + lc_tm->tm_gmtoff);
			TRACE("load_cgmtime UTC OFFSET: %lu", lc_tm->tm_gmtoff);
		} else {
			// old models can use get_UTC_offset
			time_now = abs(time_now + get_UTC_offset(localtime(&time_now)));
		}
#endif
		LOG("load_cgmtime:  time_now: %lu, current_cgm_time: %lu", time_now, current_cgm_time);

		//current_cgm_timeago = abs(time_now - current_cgm_time);
		current_cgm_timeago = (time_now - current_cgm_time);

		TRACE("load_cgmtime: cgm_label_buffer: %s, current_cgm_timeago\"%lu\"", cgm_label_buffer);

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

	LOG("load_cgmtime: cgmtime_layer is \"%s\"", text_layer_get_text(cgmtime_layer));
	TRACE("load_cgmtime: cgm_label_buffer: %s", cgm_label_buffer);
} // end load_cgmtime

static void load_bg_delta()
{
	LOG("load_bg_delta: current_bg_delta is \"%s\"", current_bg_delta);

    if (!dirty.delta) {
        TRACE("Delta not dirty, not changing");
        return;
    }

	// VARIABLES
	// NOTE: buffers have to be static and hardcoded
	static char formatted_bg_delta[BGDELTA_FORMATTED_SIZE];

	// CODE START

	// check bluetooth connection
	bluetooth_connected_cgm = bluetooth_connection_service_peek();

	if (!bluetooth_connected_cgm)
	{
		// Bluetooth is out; BT message already set, so return
		return;
	}

    dirty.delta = 0; // all next escapes are dirty

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
		LOG("load_bg_delta: Found \"LOAD\"");

		strncpy(formatted_bg_delta, "LOADING...", MSGLAYER_BUFFER_SIZE);
		text_layer_set_text(delta_layer, formatted_bg_delta);
		text_layer_set_text(bg_layer, " ");
		create_update_bitmap(&icon_bitmap,icon_layer, LOGO_SPECVALUE_ICON);
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

	strncpy(formatted_bg_delta, current_bg_delta, BGDELTA_FORMATTED_SIZE);

	LOG("load_bg_delta: All good. Setting \"%s\"", formatted_bg_delta);

	text_layer_set_text(delta_layer, formatted_bg_delta);
#ifdef PBL_COLOR
	if(SameColourTopAndBottom) {
		text_layer_set_text_color(delta_layer,fg_colour);
	} else {
		text_layer_set_text_color(delta_layer,bg_colour);
	}
#endif
	LOG("load_bg_delta: delta_layer is \"%s\"", text_layer_get_text(delta_layer));

} // end load_bg_delta

static void load_battlevel()
{
	TRACE("load_battlevel: START");

	// CONSTANTS


	// VARIABLES
	// NOTE: buffers have to be static and hardcoded
	uint32_t current_battlevel = 0;
	static char battlevel_percent[9];

	// CODE START
	//Deterime if a metric text layer is configured for phone battery
	if(bottom_left_metric != METRIC_PHONEBATT && bottom_right_metric == METRIC_PHONEBATT) {
		LOG("load_battlevel: No phone battery displays, exiting.");
		return;
	}
	LOG("load_battlevel: last_battlevel: %lu", last_battlevel);
	if (last_battlevel == 255)
	{
		// Init code or no battery, can't do battery; set text layer & icon to empty value
		INFO("load_battlevel: NO BATTERY");
		if(bottom_left_metric == METRIC_PHONEBATT) text_layer_set_text(bottom_left_text_layer, "");
		if(bottom_right_metric == METRIC_PHONEBATT) text_layer_set_text(bottom_right_text_layer, "");
		LowBatteryAlert = false;
		return;
	}

	if (last_battlevel == 0)
	{
		// Zero battery level; set here, so if we get zero later we know we have an error instead
		INFO("load_battlevel: 0 value");
		if(bottom_left_metric == METRIC_PHONEBATT) text_layer_set_text(bottom_left_text_layer, "0%");
		if(bottom_right_metric == METRIC_PHONEBATT) text_layer_set_text(bottom_right_text_layer, "0%");
		if (!LowBatteryAlert)
		{
			INFO("load_battlevel: 0 value, vibe");
			alert_handler_cgm(LOWBATTERY_VIBE);
			LowBatteryAlert = true;
		}
		return;
	}

    if (current_battlevel == last_battlevel) {
        TRACE("Battery level early exit to not mark layers dirty");
        return;
    }

	current_battlevel = last_battlevel;

	INFO("load_battlevel: current_battlevel: %i", current_battlevel);

	if ((current_battlevel <= 0) || (current_battlevel > 100) || ((last_battlevel > 100 && last_battlevel != 255)))
	{
		// got a negative or out of bounds or error battery level
		INFO("load_battlevel: error");
		if(bottom_left_metric == METRIC_PHONEBATT) text_layer_set_text(bottom_left_text_layer, "ERR");
		if(bottom_right_metric == METRIC_PHONEBATT) text_layer_set_text(bottom_right_text_layer, "ERR");
		return;
	}

	// get current battery level and set battery level text with percent
#ifdef PBL_ROUND
	snprintf(battlevel_percent, BATTLEVEL_FORMATTED_SIZE, " %lu%%", current_battlevel);
#elif PBL_COLOR
	snprintf(battlevel_percent, BATTLEVEL_FORMATTED_SIZE, " B:%lu%%", current_battlevel);
#else
	snprintf(battlevel_percent, BATTLEVEL_FORMATTED_SIZE, "B:%lu%%", current_battlevel);
#endif
	LOG("load_battlevel: %s\%", battlevel_percent);
#ifndef PBL_ROUND
	if(bottom_left_metric == METRIC_PHONEBATT) text_layer_set_text(bottom_left_text_layer, battlevel_percent);
	if(bottom_right_metric == METRIC_PHONEBATT) text_layer_set_text(bottom_right_text_layer, battlevel_percent);
#endif
#ifdef PBL_COLOR
	// if neither bottom metric is battery indication, then return immediately and don't process the colours.
	if(bottom_left_metric != METRIC_PHONEBATT && bottom_right_metric != METRIC_PHONEBATT) {
		TRACE("load_battlevel: No watch battery displays, done");
		return;
	}
	if ( (current_battlevel > 0) && (current_battlevel <= 30) && (bottom_left_metric == METRIC_PHONEBATT || bottom_right_metric == METRIC_PHONEBATT) )
	{
		if(bottom_left_metric == METRIC_PHONEBATT) {
			LOG("load_battlevel: Setting bottom_left_text_layer to GColorRed");
			text_layer_set_text_color(bottom_left_text_layer, GColorRed);
		}
		if(bottom_right_metric == METRIC_PHONEBATT) {
			LOG("load_battlevel: Setting bottom_right_text_layer to GColorRed");
			text_layer_set_text_color(bottom_right_text_layer, GColorRed);
		}
		if (!LowBatteryAlert)
		{
			INFO("load_battlevel: low battery VIBRATE");
			alert_handler_cgm(LOWBATTERY_VIBE);
			LowBatteryAlert = true;
		}
	}
	else if ( (current_battlevel > 30) && (current_battlevel <= 50) && (bottom_left_metric == METRIC_PHONEBATT || bottom_right_metric == METRIC_PHONEBATT) )
	{
		if(bottom_left_metric == METRIC_PHONEBATT) {
			LOG("load_battlevel: Setting bottom_left_text_layer to GColorYellow");
			text_layer_set_text_color(bottom_left_text_layer, GColorYellow);
		}
		if(bottom_right_metric == METRIC_PHONEBATT) {
			LOG("load_battlevel: Setting bottom_right_text_layer to GColorYellow");
			text_layer_set_text_color(bottom_right_text_layer, GColorYellow);
		}
	}
	else
	{
		if(bottom_left_metric == METRIC_PHONEBATT) {
			LOG("load_battlevel: Setting bottom_left_text_layer to GColorGreen");
//			text_layer_set_text_color(bottom_left_text_layer, GColorGreen);
			text_layer_set_text_color(bottom_left_text_layer, fg_colour);
		}
		if(bottom_right_metric == METRIC_PHONEBATT) {
//			text_layer_set_text_color(bottom_right_text_layer, GColorGreen);
			text_layer_set_text_color(bottom_right_text_layer, fg_colour);
			LOG("load_battlevel: Setting bottom_right_text_layer to GColorGreen");
		}
	}
#endif
	TRACE("load_battlevel: done");
} // end load_battlevel

// send_cmd_cgm - Function to send dat to xDrip to cause a refresh/update of data.
// Needs to include configuration values that xDrip can read and respond to.
static void send_cmd_cgm(void)
{
	AppMessageResult sendcmd_openerr = APP_MSG_OK;
	AppMessageResult sendcmd_senderr = APP_MSG_OK;
	DictionaryIterator *iter = NULL;

	sendcmd_openerr = app_message_outbox_begin(&iter);
	if(BluetoothAlert)
	{
		//BT is down rignt now, so don't do anything.
		//Note, we cannot log this, as BT must be up in order to log it.
		return;
	}
	if (sendcmd_openerr != APP_MSG_OK)
	{
		LOG("send_cmd_cgm: ERR CODE: %i RES: %s", sendcmd_openerr, translate_app_error(sendcmd_openerr));
		return;
	}
    comm_heartbeat hb;
    hb.raw = 0; // reset

#ifdef PBL_COLOR
    hb.colour = 1;
#else 
    hb.colour = 0;
#endif

    hb.time_series = use_png ? 0 : 1;
#ifdef PBL_PLATFORM_GABBRO
    hb.time_period = 1;
#else
    hb.time_period = 3;
#endif

    // trend values
    if (!trend_isinitialized() && !use_png) { 
        hb.high_limit = 1;
        hb.low_limit = 1;
    }

    // delta + pump values
    /* hb.send_iob = 1; */
    /* hb.send_pump_state = 1; */
    /* hb.send_pump_battery = 1; */
   
    // function is called when BGL times out, send data if more than 5 mins ago
    if (dirty.need_cgm) {
        hb.send_slope_arrow = 1;
        hb.send_delta_value = 1;
        dict_write_uint32(iter, FRAMEWORK_BGL_VALUE, current_cgm_time); // request update
        if (use_png) {
            comm_request_png(iter, layer_get_bounds(bitmap_layer_get_layer(bg_trend_layer)));
        }
    }

    if (bottom_right_metric == METRIC_PHONEBATT || bottom_left_metric == METRIC_PHONEBATT) hb.send_phone_battery = 1;

	dict_write_uint32(iter, FRAMEWORK_HEARTBEAT, hb.raw);

	TRACE("send_cmd_cgm: Opening outbox");
	sendcmd_senderr = app_message_outbox_send();

	if (sendcmd_senderr != APP_MSG_OK && sendcmd_senderr != APP_MSG_BUSY && sendcmd_senderr != APP_MSG_SEND_REJECTED)
	{
		LOG("send_cmd_cgm: ERR CODE: %i RES: %s", sendcmd_senderr, translate_app_error(sendcmd_senderr));
	}
	//free(iter);
	TRACE("send_cmd_cgm: done");
} // end send_cmd_cgm

// updateColours - called when fg_colour or bg_colour is changed.
#ifdef PBL_COLOR
void updateColours()
{
	if(SameColourTopAndBottom) {
		bitmap_layer_set_background_color(upper_face_layer, bg_colour);
		text_layer_set_text_color(delta_layer, fg_colour);
		text_layer_set_text_color(message_layer, fg_colour);
		text_layer_set_text_color(bg_layer, fg_colour);
		text_layer_set_text_color(cgmtime_layer, fg_colour);
		text_layer_set_text_color(bottom_left_text_layer, fg_colour);
		text_layer_set_text_color(bottom_right_text_layer, fg_colour);
	} else {
		bitmap_layer_set_background_color(upper_face_layer, fg_colour);
		text_layer_set_text_color(delta_layer, bg_colour);
		text_layer_set_text_color(message_layer, bg_colour);
		text_layer_set_text_color(bg_layer, bg_colour);
		text_layer_set_text_color(cgmtime_layer, bg_colour);
		text_layer_set_text_color(bottom_left_text_layer, bg_colour);
		text_layer_set_text_color(bottom_right_text_layer, bg_colour);
	}
	bitmap_layer_set_background_color(lower_face_layer, bg_colour);
	text_layer_set_text_color(time_watch_layer, fg_colour);
	text_layer_set_text_color(date_app_layer, fg_colour);
	text_layer_set_background_color(bottom_right_text_layer, GColorClear);
	// update the watch battery colours etc.
	battery_handler(battery_state_service_peek());
}
// end updateColours
#endif

void inbox_received_handler_cgm(DictionaryIterator *iterator, void *context)
{
	Tuple *data = dict_read_first(iterator);
//	TRACE("SYNC TUPLE");
	LOG("inbox_received_callback_cgm: got dictionary");

	if (global_lock)
	{
		LOG("inbox_received_handler_cgm: GLOBALLY LOCKED");
		return;
	}


	// CODE START

	while ((data != NULL) && (!global_lock))
	{
		LOG("inbox_received_handler_cgm: key is %lu", data->key);
		switch (data->key)
		{


			case CGM_TREND_BEGIN_KEY:
#ifndef ENABLE_TREND_RENDERER
				expected_trend_buffer_length = data->value->uint16;
				LOG("TREND_BEGIN; About to receive Trend Image of %i size.", expected_trend_buffer_length);
				if(trend_buffer)
				{
					LOG("TREND_BEGIN; Freeing trend_buffer.");
					free(trend_buffer);
				}
				LOG("TREND_BEGIN; Allocating trend_buffer.");
				trend_buffer = malloc(expected_trend_buffer_length);
				trend_buffer_length = 0;
				if(trend_buffer == NULL)
				{
					DEBUG("TREND_BEGIN: Could not allocate trend_buffer");
					break;
				}
				DEBUG("TREND_BEGIN: trend_buffer is %lx, trend_buffer_length is %i", (uint32_t)trend_buffer, trend_buffer_length);
#endif
			break;
			case CGM_TREND_DATA_KEY:
#ifndef ENABLE_TREND_RENDERER
				LOG("TREND_DATA: receiving Trend Image chunk");
				if(trend_buffer)
				{
					if ((trend_buffer_length + data->length) <= expected_trend_buffer_length)
					{
						memcpy((trend_buffer+trend_buffer_length), data->value->data, data->length);
						trend_buffer_length += data->length;
						LOG("TREND_DATA: received %u of %u so far", trend_buffer_length, expected_trend_buffer_length);
					}
					else
					{
						LOG("TREND_DATA: EXCEEDED BUFFER received %u of %u so far", trend_buffer_length, expected_trend_buffer_length);

					}
				}
				else
				{
					DEBUG("TREND_DATA: trend_buffer not allocated, ignoring");
				}
				if(trend_buffer_length == expected_trend_buffer_length) doing_trend = true;
#endif
			break;

			case CGM_TREND_END_KEY:
#ifndef ENABLE_TREND_RENDERER
				if(!doing_trend)
				{
					LOG("Got a TREND_END without TREND_START");
					break;
				}
				LOG("Finished receiving Trend Image");
				if(bg_trend_bitmap != NULL)
				{
					INFO("Destroying bg_trend_bitmap");
					gbitmap_destroy(bg_trend_bitmap);
					bg_trend_bitmap = NULL;
				}

				LOG("Creating Trend Image");
				LOG("TREND_END: trend_buffer is %lx, trend_buffer_length is %i", (uint32_t)trend_buffer, trend_buffer_length);

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
					LOG("bg_trend_bitmap created, setting to layer");
					bitmap_layer_set_bitmap(bg_trend_layer, bg_trend_bitmap);
				}

				else
				{
					INFO("bg_trend_bitmap creation FAILED!");
				}
				if (trend_buffer)
				{
					LOG("Free trend buffer 2");
					free(trend_buffer);
					trend_buffer = NULL;
				}
#endif
			break;

			case CGM_MESSAGE_KEY:
				LOG("Got Message Key, message is \"%s\"", data->value->cstring);
				snprintf(message_layer_text,sizeof(message_layer_text),"%s",data->value->cstring);
				//text_layer_set_text(message_layer,data->value->cstring);
				text_layer_set_text(message_layer,message_layer_text);
				if(strcmp(data->value->cstring, "")==0)
				{
					LOG("Setting message_layer hidden");
					display_message = false;
					layer_set_hidden((Layer *)message_layer, true);
#ifdef PBL_ROUND
					layer_set_hidden((Layer *)delta_layer, false);
#endif
				}
				else
				{
					LOG("Setting message_layer visible");
					display_message = true;
					layer_set_hidden((Layer *)message_layer, false);
#ifdef PBL_ROUND
					layer_set_hidden((Layer *)delta_layer, true);
#endif
					if (!app_timer_reschedule(message_tick_timer, message_tick_timeout)) {
						message_tick_timer = app_timer_register(message_tick_timeout, handle_message_tick, NULL);
					}
				}
				
			break;

			case SET_SAMECOLOUR:
				LOG("Got SET_SAMECOLOUR Key, message is \"%u\"", data->value->uint8);
				SameColourTopAndBottom = data->value->uint8;
				persist_write_int(SET_SAMECOLOUR, data->value->uint8);
#ifdef PBL_COLOR
				updateColours();
#else 
				if(SameColourTopAndBottom) {
					bitmap_layer_set_background_color(upper_face_layer, bg_colour);
				} else {
					bitmap_layer_set_background_color(upper_face_layer, fg_colour);
				}
#endif
			break;

			case SET_FG_COLOUR:
				LOG("Got foreground Key, message is \"%lx\"", data->value->uint32);
#ifdef PBL_COLOR
				fg_colour = GColorFromHEX(data->value->uint32);
				persist_write_int(SET_FG_COLOUR, data->value->uint32);
				updateColours();
#endif
			break;

			case SET_BG_COLOUR:
				LOG("Got background Key, message is \"%lx\"", data->value->uint32);
#ifdef PBL_COLOR
				bg_colour = GColorFromHEX(data->value->uint32);
				persist_write_int(SET_BG_COLOUR, data->value->uint32);
				updateColours();
#endif
			break;

			case SET_DISP_SECS:
				LOG("Got dispsecs Key, message is \"%u\"", data->value->uint8);
				if(data->value->uint8 > 0)
				{
					if (!display_seconds) {
						// register second timer
						tick_timer_service_subscribe(SECOND_UNIT, &handle_second_tick_cgm);
						// resize time and date layer iff PT2
						if (HIGH_RES()) {
							layer_set_frame((Layer *) time_watch_layer, GRect(0, 121, 200, 60));
							layer_set_frame((Layer *) date_app_layer, GRect(0, 168, 200, 39));
						}
					}
					display_seconds = true;
					time_font = time_font_small;
				}
				else
				{
					if (display_seconds) {
						// unsub seconds timer to save power
						tick_timer_service_unsubscribe();
						tick_timer_service_subscribe(MINUTE_UNIT, &handle_minute_tick_cgm);
						// reset layers

						if (HIGH_RES()) {
							layer_set_frame((Layer *) time_watch_layer, GRect(0, 111, 200, 60));
							layer_set_frame((Layer *) date_app_layer, GRect(0, 176, 200, 39));
						}
					}
					display_seconds = false;
					time_font = time_font_normal;
				}
				persist_write_bool(SET_DISP_SECS, display_seconds);
				text_layer_set_font(time_watch_layer, time_font);
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
				LOG("Got background Key, message is \"%lx\"", data->value->uint32);
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
				LOG("Got No Vibe Key, message is \"%lx\"", data->value->uint32);
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
				LOG("Got Backlight on Charge key, message is \"%lx\"", data->value->uint32);
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

			case SET_MESSAGE_TIMEOUT:
				LOG("Got message timeout, message is \"%lx\"", data->value->uint32);
				message_tick_timeout = data->value->uint32 * 1000;
				if (!app_timer_reschedule(message_tick_timer, message_tick_timeout)) {
					message_tick_timer = app_timer_register(message_tick_timeout, handle_message_tick, NULL);
				}
				persist_write_int(SET_MESSAGE_TIMEOUT, data->value->uint32);
			break;

			case SET_BOLD_TIMEAGO:
				LOG("Got timeago bold, message is \"%lx\"", data->value->uint32);
				if(data->value->uint8 > 0) {
					TimeAgoBold = true;
				}
				else
				{
					TimeAgoBold = false;
				}
				persist_write_bool(SET_BOLD_TIMEAGO, TimeAgoBold);
				LOG("Setting TimeAgoBold to \"%lx\"", TimeAgoBold);
				if(TimeAgoBold) {
					text_layer_set_font(cgmtime_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
				}
				else
				{
					text_layer_set_font(cgmtime_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28));
				}
			break;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wzero-length-bounds"
			//Bottom left metric to display
			case SET_BOTTOM_LEFT_TEXT:
				LOG("Got bottom_left_metric message is \"%s\"", data->value->cstring);
                bottom_left_metric = data->value->data[0] - 0x30;
				if(data->value->data[0]-0x30 == METRIC_PHONEBATT) {
					text_layer_set_text(bottom_left_text_layer, "Wait..");
				}
				LOG("Set bottom_left_metric to \"%u\"", bottom_left_metric);
				persist_write_int(SET_BOTTOM_LEFT_TEXT, data->value->data[0] - 0x30);
				if(bottom_left_metric == METRIC_NONE) {
					text_layer_set_text(bottom_left_text_layer, "");
				} else {
					text_layer_set_text_color(bottom_left_text_layer, fg_colour);
#ifdef PBL_HEALTH
					update_health_metric_displays();
#endif
					battery_handler(battery_state_service_peek());
					//load_battlevel();
				}
			break;

			//Bottom right metric to display
			case SET_BOTTOM_RIGHT_TEXT:
				LOG("Got bottom_right_metric message is \"%s\"", data->value->cstring);
                bottom_right_metric = data->value->data[0] - 0x30;
				if(data->value->data[0]-0x30 == METRIC_PHONEBATT) {
					text_layer_set_text(bottom_right_text_layer, "Wait..");
				}
				LOG("Set bottom_right_metric to \"%u\"", bottom_right_metric);
				persist_write_int(SET_BOTTOM_RIGHT_TEXT, data->value->data[0] - 0x30);
				if(bottom_right_metric == METRIC_NONE) {
					text_layer_set_text(bottom_right_text_layer, "");
				} else {
					text_layer_set_text_color(bottom_right_text_layer, fg_colour);
#ifdef PBL_HEALTH
					update_health_metric_displays();
#endif
					battery_handler(battery_state_service_peek());
					//load_battlevel();
				}
			break;
#pragma GCC diagnostic pop

            case SET_USE_PNG:
                LOG("Switching PNG settings");
                use_png = data->value->int8 != 0;
                DEBUG("PNG set to %d %d", use_png, data->value->int32);
                persist_write_bool(SET_USE_PNG, use_png);
                // reinit
                if (!use_png) trend_init(bitmap_layer_get_layer(bg_trend_layer));
                else trend_deinit();
                // reset
                dirty.need_cgm = 1;
                current_cgm_time = 0; // force update all
                reset_timer_callback_cgm(2);
                break;
            /**
             * trend config colors
             */
            case SET_BGL_CRITICAL_COLOUR:
            case SET_BGL_HIGH_COLOUR:
            case SET_BGL_AVERAGE_COLOUR:
            case SET_BGL_GOOD_COLOUR:
            case SET_BGL_LOW_COLOUR:
            case SET_LOW_LINE_COLOUR:
            case SET_HIGH_LINE_COLOUR:
            case SET_LINE_STYLE:
            case SET_TREND_STYLE:
            case SET_LINE_WIDTH:
            case SET_TREND_WIDTH:
            case SET_BGL_LOW:
            case SET_BGL_AVERAGE:
            case SET_BGL_HIGH:
            case SET_BGL_CRITICAL:
#ifdef ENABLE_TREND_RENDERER
                trend_process_config(data);
#endif
                break;

            /**
             * New Comms framework messages
             */
            case FRAMEWORK_HEARTBEAT:
            case FRAMEWORK_VIBE:
            case FRAMEWORK_MESSAGE:
            case FRAMEWORK_LOWLIMIT:
            case FRAMEWORK_HIGHLIMIT:
            case FRAMEWORK_SLOPEVAL:
            case FRAMEWORK_BGL_VALUE:
            case FRAMEWORK_BGL_SERIES:
            case FRAMEWORK_BGL_DELTA:
            case FRAMEWORK_PNG_IMAGE:
#ifdef ENABLE_COMM_FRAMEWORK
                comm_handle(data);
#endif
                break;

			default:
				LOG("inbox_received_handler_cgm: Dictionary Key not recognised: %ld", data->key);
			break;
		}
		// end switch(key)
		data = dict_read_next(iterator);
	}
} // end sync_tuple_changed_callback_cgm()

void reset_timer_callback_cgm(int32_t seconds) {
    int32_t retimer = (seconds) * MS_IN_A_SECOND;
    if (retimer < 0) retimer = 1000; // schedule for 1s
    if (timer_cgm == NULL || !app_timer_reschedule(timer_cgm, retimer)) {
        timer_cgm = app_timer_register(retimer, timer_callback_cgm, NULL);
    }
}

void timer_callback_cgm(void *data)
{
    // set timer to null, as it has beenh called and does not need rescheduling
    timer_cgm = NULL;
	TRACE("timer_callback_cgm: register timer");
    // if we have not received anything for over 6 minutes, keep checking
    if ((long) (current_cgm_time + 360) < time(NULL)) {
        // mark cgm data as dirty, send heartbeat
        dirty.need_cgm = 1;
        send_cmd_cgm();
        // try again in 60 seconds until we get something
        reset_timer_callback_cgm(60);
    } else {
        // schedule normal checkup for 6 minutes from now
        reset_timer_callback_cgm(360);
    }

	TRACE("timer_callback_cgm: done");

} // end timer_callback_cgm

// format current time from watch

// message/delta tick layer
void handle_message_tick(void *data) 
{
	if(display_message == 0) return;
	INFO("handle_message_tick: Handling alert tick, display_message is %i", display_message);
	if(display_message)
	{
		layer_set_hidden((Layer *)message_layer, !(layer_get_hidden((Layer *)message_layer)));
#ifdef PBL_ROUND
		layer_set_hidden((Layer *)delta_layer, !(layer_get_hidden((Layer *)delta_layer)));
#endif
	}
	else
	{
		if(!layer_get_hidden((Layer *)message_layer))
		{
			layer_set_hidden((Layer *)message_layer, true);
		}
#ifdef PBL_ROUND
		if(layer_get_hidden((Layer *)delta_layer))
		{
			layer_set_hidden((Layer *)delta_layer, false);
		}
#endif
	}

	message_tick_timer = app_timer_register(message_tick_timeout, handle_message_tick, NULL);
}

void handle_second_tick_cgm(struct tm* tick_time_cgm, TimeUnits units_changed_cgm)
{
	TRACE("handle_second_tick_cgm:");

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
		INFO("handle_second_tick_cgm: display_seconds = %i, time_watch_text = %s, time_watch_format = %s", display_seconds, time_watch_text, time_watch_format);
	}
	handling_second = false;

} // end handle_second_tick_cgm

/**
 * Check minute and day data
 */
void handle_minute_tick_cgm(struct tm* tick_time_cgm, TimeUnits units_changed_cgm)
{
	TRACE("handle_minute_tick_cgm:");

	// VARIABLES
	size_t tick_return_cgm = 0;
	// CODE START

	if (units_changed_cgm & MINUTE_UNIT)
	{
		LOG("handle_minute_tick_cgm: tick");
		tick_return_cgm = strftime(time_watch_text, TIME_TEXTBUFF_SIZE, time_watch_format, tick_time_cgm);
	}

	if (tick_return_cgm != 0)
	{
		text_layer_set_text(time_watch_layer, time_watch_text);
		++lastAlertTime;
	}

	if (units_changed_cgm & DAY_UNIT)
	{
		INFO("handle_minute_tick_cgm: Day changed");
		tick_return_cgm = strftime(date_app_text, DATE_TEXTBUFF_SIZE, "%a %d %b", tick_time_cgm);
		if (tick_return_cgm != 0)
		{
			text_layer_set_text(date_app_layer, date_app_text);
		}
	}

    // detect some error in rescheduling
    if (time(NULL) - current_cgm_time > (10 * 60)) {
        reset_timer_callback_cgm(2);
    }


    // We wake up every minute anyway and the resolution of all display time items
    // is 1m except for the clock
    load_cgmtime();
    load_bg_delta();

} // end handle_minute_tick_cgm

//#ifdef PBL_PLATFORM_APLITE
#ifndef PBL_COLOR

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
		DEBUG("bitmapLayerUpdate: capture frame buffer failed!!");
	}
	else
	{
		//  DEBUG("capture frame buffer succeeded %i vs %i and %i vs %i with bpw: %i vs %i",gbitmap_get_bounds(graphic).size.w,gbitmap_get_bounds(framebuffer).size.w,gbitmap_get_bounds(graphic).size.h,gbitmap_get_bounds(framebuffer).size.h, gbitmap_get_bytes_per_row(graphic),gbitmap_get_bytes_per_row(framebuffer));

		if (graphic == NULL)
		{
			DEBUG("bitmapLayerUpdate: GRAPHIC IS NULL!!");
		}
		else
		{
			height = gbitmap_get_bounds(graphic).size.h;
			uint8_t* bfstart=(uint8_t*)gbitmap_get_data(framebuffer);
			uint8_t* bitmapstart=(uint8_t*)gbitmap_get_data(graphic);
			if (bitmapstart == NULL)
			{
				WARNING("bitmapLayerUpdate: bitmap start went to null!!");
				graphics_release_frame_buffer(ctx, framebuffer);
				global_lock = false;
				return;
			}
			if (bfstart == NULL)
			{
				WARNING("bitmapLayerUpdate: framebuffer start went to null!!");
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
					// DEBUG("bfr: %0x, bitmsp: %0x, finalBits: %x", (unsigned int)bfr, (unsigned int)bitmap, finalBits );
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
	TRACE("window_load_cgm: start");

	// VARIABLES
	Layer *window_layer_cgm = NULL;
//APLITE (CLASSIC)
#ifdef PBL_PLATFORM_APLITE
	LOG("window_load_cgm: Detected Aplite");
	//monochrome colours
	//static GColor fg_colour;
	//static GColor bg_colour;
	// face layer sizes
	upper_face_layer = bitmap_layer_create(GRect(0,0,144,89));
	lower_face_layer = bitmap_layer_create(GRect(0,89,144,165));
	// icon layer dimensions
	icon_layer = bitmap_layer_create(GRect(85, -7, 78, 51));
	// trend bitmap layer dimensions
	bg_trend_layer = bitmap_layer_create(GRect(0,24,144,64));
	bitmap_layer_set_compositing_mode(bg_trend_layer, GCompOpSet);
	// delta layer dimensions
	delta_layer = text_layer_create(GRect(0, 58, 143, 50));
	text_layer_set_text_alignment(delta_layer, GTextAlignmentRight);
	// message layer dimensions
	message_layer = text_layer_create(GRect(0, 36, 143, 50));
	text_layer_set_text_alignment(message_layer, GTextAlignmentCenter);
	// BG layter dimensions
	bg_layer = text_layer_create(GRect(0, -5, 95, 47));
	// cgmtime layer dimensions
	cgmtime_layer = text_layer_create(GRect(104, 58, 40, 24));
	text_layer_set_text_alignment(cgmtime_layer, GTextAlignmentRight);
	// time watch layer dimenssions
	time_watch_layer = text_layer_create(GRect(0, 84, 143, 44));
	text_layer_set_text_alignment(time_watch_layer, GTextAlignmentCenter);
	// date layer dimenstions
	date_app_layer = text_layer_create(GRect(0, 124, 143, 29));
	text_layer_set_text_alignment(date_app_layer, GTextAlignmentCenter);
	// phone/bridge batter level layer diemnsions
	bottom_left_text_layer = text_layer_create(GRect(0, 148, 59, 18));
	text_layer_set_text_alignment(bottom_left_text_layer, GTextAlignmentLeft);
	//watch battery level layer dimensions
	bottom_right_text_layer = text_layer_create(GRect(81, 148, 59, 18));
	text_layer_set_text_alignment(bottom_right_text_layer, GTextAlignmentRight);

#endif

//BASALT (TIME, TIME STEEL)
#ifdef PBL_PLATFORM_BASALT
	LOG("window_load_cgm: Detected Basalt");
	//collour colours
	//static GColor8 fg_colour;
	//static GColor8 bg_colour;
	// upper and lower face dimensions
	upper_face_layer = bitmap_layer_create(GRect(0,0,144,84));
	lower_face_layer = bitmap_layer_create(GRect(0,84,144,165));
	// icon layer dimensions
	icon_layer = bitmap_layer_create(GRect(85, -9, 78, 49));
	bitmap_layer_set_compositing_mode(icon_layer, GCompOpSet);
	// trend bitmap layer dimensions and composition mode
	bg_trend_layer = bitmap_layer_create(GRect(0,0,144,84));
	bitmap_layer_set_compositing_mode(bg_trend_layer, GCompOpSet);
	// delta layer dimensions
	delta_layer = text_layer_create(GRect(0, 58, 143, 50));
	layer_set_bounds((Layer *) delta_layer, GRect(0, -2, 143, 50)); // fixes bounding box with latest sdk
	text_layer_set_text_alignment(delta_layer, GTextAlignmentLeft);
	// message layer dimensions
	message_layer = text_layer_create(GRect(0, 36, 143, 50));
	layer_set_bounds((Layer *) message_layer, GRect(0, -2, 143, 50)); // fixes bounding box with latest sdk
	text_layer_set_text_alignment(message_layer, GTextAlignmentCenter);
	// BG layer dimensions
	bg_layer = text_layer_create(GRect(0, -5, 95, 42));
	// cgmtime layer dimensions
	cgmtime_layer = text_layer_create(GRect(104, 58, 40, 24));
	layer_set_bounds((Layer *) cgmtime_layer, GRect(0, -2, 40, 24)); // fixes bounding box with latest sdk
	text_layer_set_text_alignment(cgmtime_layer, GTextAlignmentRight);
	// time watch layer dimenssions
	time_watch_layer = text_layer_create(GRect(0, 82, 143, 44));
	text_layer_set_text_alignment(time_watch_layer, GTextAlignmentCenter);
	// date layer dimenstions
	date_app_layer = text_layer_create(GRect(0, 124, 143, 29));
	text_layer_set_text_alignment(date_app_layer, GTextAlignmentCenter);
	// phone/bridge batter level layer diemnsions
	bottom_left_text_layer = text_layer_create(GRect(0, 148, 72, 20));
	layer_set_bounds((Layer *) bottom_left_text_layer, GRect(0, -1, 72, 20)); // fixes bounding box with latest sdk
	text_layer_set_text_alignment(bottom_left_text_layer, GTextAlignmentLeft);
	// watch battery level layer dimensions
	bottom_right_text_layer = text_layer_create(GRect(72, 148, 72, 20));
	layer_set_bounds((Layer *) bottom_right_text_layer, GRect(0, -1, 72, 20)); // fixes bounding box with latest sdk
	text_layer_set_text_alignment(bottom_right_text_layer, GTextAlignmentRight);

#endif

//CHALK (ROUND)
#ifdef PBL_PLATFORM_CHALK
	LOG("window_load_cgm: Detected Chalk");
	//collour colours
	//static GColor8 fg_colour;
	//static GColor8 bg_colour;
	// face layer sizes
	upper_face_layer = bitmap_layer_create(GRect(0,0,180,84));
	lower_face_layer = bitmap_layer_create(GRect(0,84,180,165));
	// icon layer size and composition mode
	icon_layer = bitmap_layer_create(GRect(120, 30, 78, 50));
	bitmap_layer_set_compositing_mode(icon_layer, GCompOpSet);
	// trend bitmap layer dimensions and composition mode
	bg_trend_layer = bitmap_layer_create(GRect(0,0,144,84));
	bitmap_layer_set_compositing_mode(bg_trend_layer, GCompOpSet);
	// delta layer dimensions
	delta_layer = text_layer_create(GRect(0, 36, 180, 50));
	text_layer_set_text_alignment(delta_layer, GTextAlignmentCenter);
	// message layer dimensions
	message_layer = text_layer_create(GRect(0, 36, 180, 50));
	text_layer_set_text_alignment(message_layer, GTextAlignmentCenter);
	// BG layer dimensions
	bg_layer = text_layer_create(GRect(0, -7, 180, 47));
	text_layer_set_text_alignment(bg_layer, GTextAlignmentCenter);
	// cgmtime layer dimensions
	cgmtime_layer = text_layer_create(GRect(5, 58, 40, 24));
	text_layer_set_text_alignment(cgmtime_layer, GTextAlignmentRight);
	// time watch layer dimenssions
	time_watch_layer = text_layer_create(GRect(18, 82, 143, 44));
	text_layer_set_text_alignment(time_watch_layer, GTextAlignmentCenter);
	// date layer dimenstions
	date_app_layer = text_layer_create(GRect(18, 124, 143, 26));
	text_layer_set_text_alignment(date_app_layer, GTextAlignmentCenter);
	// phone/bridge batter level layer diemnsions
	bottom_left_text_layer = text_layer_create(GRect(48, 150, 1, 1));
	text_layer_set_text_alignment(bottom_left_text_layer, GTextAlignmentLeft);
	// watch battery level layer dimensions
	bottom_right_text_layer = text_layer_create(GRect(45, 150, 90, 18));
	text_layer_set_text_alignment(bottom_right_text_layer, GTextAlignmentCenter);

#endif

//DIORITE (PEBBLE 2)
#ifdef PBL_PLATFORM_DIORITE
	LOG("window_load_cgm: Detected Diorite");
	//monochrome colours
	//static GColor fg_colour;
	//static GColor bg_colour;
	upper_face_layer = bitmap_layer_create(GRect(0,0,144,88));
	lower_face_layer = bitmap_layer_create(GRect(0,89,144,165));
	// icon layer dimensions
	icon_layer = bitmap_layer_create(GRect(85, -7, 78, 51));
	// trend bitmap layer dimensions
	bg_trend_layer = bitmap_layer_create(GRect(0,24,144,64));
	bitmap_layer_set_compositing_mode(bg_trend_layer, GCompOpSet);
	// delta layer dimensions
	delta_layer = text_layer_create(GRect(0, 58, 143, 50));
	text_layer_set_text_alignment(delta_layer, GTextAlignmentRight);
	// message layer dimensions
	message_layer = text_layer_create(GRect(0, 36, 143, 50));
	text_layer_set_text_alignment(message_layer, GTextAlignmentCenter);
	// BG layer dimensions
	bg_layer = text_layer_create(GRect(0, -5, 95, 47));
	// cgmtime layer dimensions
	cgmtime_layer = text_layer_create(GRect(104, 58, 40, 24));
	text_layer_set_text_alignment(cgmtime_layer, GTextAlignmentRight);
	// time watch layer dimenssions
	time_watch_layer = text_layer_create(GRect(0, 84, 143, 44));
	text_layer_set_text_alignment(time_watch_layer, GTextAlignmentCenter);
	// date layer dimenstions
	date_app_layer = text_layer_create(GRect(0, 124, 143, 29));
	text_layer_set_text_alignment(date_app_layer, GTextAlignmentCenter);
	// phone/bridge batter level layer diemnsions
	bottom_left_text_layer = text_layer_create(GRect(0, 148, 59, 18));
	text_layer_set_text_alignment(bottom_left_text_layer, GTextAlignmentLeft);
	// watch battery level layer dimensions
	bottom_right_text_layer = text_layer_create(GRect(81, 148, 59, 18));
	text_layer_set_text_alignment(bottom_right_text_layer, GTextAlignmentRight);

#endif

//EMERY (CORE TIME 2)
#ifdef PBL_PLATFORM_EMERY
	LOG("window_load_cgm: Detected Emery");
	//upper and lower face layer dimensions
	upper_face_layer = bitmap_layer_create(GRect(0,0,200,114));
	lower_face_layer = bitmap_layer_create(GRect(0,115,200,228));
	// icon layer diemnsions and composition mode.
	icon_layer = bitmap_layer_create(GRect(146, -9, 78, 49));
	bitmap_layer_set_compositing_mode(icon_layer, GCompOpSet);
	// trend bitmap layer dimensions and composition mode
	bg_trend_layer = bitmap_layer_create(GRect(0,0,200,114));
	bitmap_layer_set_compositing_mode(bg_trend_layer, GCompOpSet);
	// delta layer dimensions
	delta_layer = text_layer_create(GRect(2, 78, 198, 50));
	text_layer_set_text_alignment(delta_layer, GTextAlignmentLeft);
	// message layer dimensions
	message_layer = text_layer_create(GRect(2, 49, 198, 50));
	text_layer_set_text_alignment(message_layer, GTextAlignmentCenter);
	// BG layer dimensions
	bg_layer = text_layer_create(GRect(0, -5, 132, 57));
	// cgmtime layer dimensions
	cgmtime_layer = text_layer_create(GRect(142, 78, 55, 32));
	text_layer_set_text_alignment(cgmtime_layer, GTextAlignmentRight);
	// time watch layer dimenssions
	if (display_seconds) {
		time_watch_layer = text_layer_create(GRect(0, 121, 200, 60));
	} else {
		time_watch_layer = text_layer_create(GRect(0, 111, 200, 60));
	}
	text_layer_set_text_alignment(time_watch_layer, GTextAlignmentCenter);
	// date layer dimenstions
	if (display_seconds) {
		date_app_layer = text_layer_create(GRect(0, 168, 200, 39));
	} else {
		date_app_layer = text_layer_create(GRect(0, 176, 200, 39));
	}
	text_layer_set_text_alignment(date_app_layer, GTextAlignmentCenter);
	// phone/bridge batter level layer diemnsions
	bottom_left_text_layer = text_layer_create(GRect(2, 203, 100, 24));
	text_layer_set_text_alignment(bottom_left_text_layer, GTextAlignmentLeft);
	// watch battery level layer dimensions
	bottom_right_text_layer = text_layer_create(GRect(98, 203, 100, 24));
	text_layer_set_text_alignment(bottom_right_text_layer, GTextAlignmentRight);

#endif

//FLINT (CORE DUO 2)
#ifdef PBL_PLATFORM_FLINT
	LOG("window_load_cgm: Detected Flint");
	//monochrome colours
	//static GColor fg_colour;
	//static GColor bg_colour;
	// upper and lower face layer dimensions
	upper_face_layer = bitmap_layer_create(GRect(0,0,144,88));
	lower_face_layer = bitmap_layer_create(GRect(0,89,144,165));
	// icon layer dimensions
	icon_layer = bitmap_layer_create(GRect(85, -7, 78, 51));
	// trend bitmap layer dimensions
	bg_trend_layer = bitmap_layer_create(GRect(0,24,144,64));
	bitmap_layer_set_compositing_mode(bg_trend_layer, GCompOpSet);
	// delta layer dimensions
	delta_layer = text_layer_create(GRect(0, 58, 143, 50));
	text_layer_set_text_alignment(delta_layer, GTextAlignmentLeft);
	// message layer dimensions
	message_layer = text_layer_create(GRect(0, 36, 143, 50));
	text_layer_set_text_alignment(message_layer, GTextAlignmentCenter);
	// BG layer dimensions
	bg_layer = text_layer_create(GRect(0, -5, 95, 47));
	// cgmtime layer dimensions
	cgmtime_layer = text_layer_create(GRect(104, 58, 40, 24));
	text_layer_set_text_alignment(cgmtime_layer, GTextAlignmentRight);
	// time watch layer dimenssions
	time_watch_layer = text_layer_create(GRect(0, 84, 143, 44));
	text_layer_set_text_alignment(time_watch_layer, GTextAlignmentCenter);
	// date layer dimenstions
	date_app_layer = text_layer_create(GRect(0, 124, 143, 29));
	text_layer_set_text_alignment(date_app_layer, GTextAlignmentCenter);
	// phone/bridge batter level layer diemnsions
	bottom_left_text_layer = text_layer_create(GRect(0, 148, 59, 18));
	text_layer_set_text_alignment(bottom_left_text_layer, GTextAlignmentLeft);
	// watch battery level layer dimensions
	bottom_right_text_layer = text_layer_create(GRect(81, 148, 59, 18));
	text_layer_set_text_alignment(bottom_right_text_layer, GTextAlignmentRight);

#endif

//GABBRO (CORE ROUND 2)
#ifdef PBL_PLATFORM_GABBRO
	// 260x260 (renumerate from original round is 180-180, everything *1.44
	//collour colours
	LOG("window_load_cgm: Detected GABBRO");
	//collour colours
	//static GColor8 fg_colour;
	//static GColor8 bg_colour;
	// face layer sizes
	upper_face_layer = bitmap_layer_create(GRect(0  ,   0, 260, 120));
	lower_face_layer = bitmap_layer_create(GRect(0   ,121, 260, 238));
	// icon layer size and composition mode
	icon_layer = bitmap_layer_create(GRect(173,  43, 112,  72));
	bitmap_layer_set_compositing_mode(icon_layer, GCompOpSet);
	// trend bitmap layer dimensions and composition mode
	bg_trend_layer = bitmap_layer_create(GRect(  0,   0, 260, 121));
	bitmap_layer_set_compositing_mode(bg_trend_layer, GCompOpSet);
	// delta layer dimensions
	delta_layer = text_layer_create(GRect(  0,  52, 260,  72));
	text_layer_set_text_alignment(delta_layer, GTextAlignmentCenter);
	// message layer dimensions
	message_layer = text_layer_create(GRect(  0,  52, 260,  72));
	text_layer_set_text_alignment(message_layer, GTextAlignmentCenter);
	// BG layer dimensions
	bg_layer = text_layer_create(GRect(  0,  -7, 260,  68));
	text_layer_set_text_alignment(bg_layer, GTextAlignmentCenter);
	// cgmtime layer dimensions
	cgmtime_layer = text_layer_create(GRect(  7,  84,  58,  35));
	text_layer_set_text_alignment(cgmtime_layer, GTextAlignmentRight);
	// time watch layer dimenssions
	time_watch_layer = text_layer_create(GRect( 26, 118, 206,  64));
	text_layer_set_text_alignment(time_watch_layer, GTextAlignmentCenter);
	// date layer dimenstions
	date_app_layer = text_layer_create(GRect( 26, 178, 206,  38));
	text_layer_set_text_alignment(date_app_layer, GTextAlignmentCenter);
	// phone/bridge batter level layer diemnsions
	bottom_left_text_layer = text_layer_create(GRect( 69, 236,  130,  26));
	text_layer_set_text_alignment(bottom_left_text_layer, GTextAlignmentLeft);
	// watch battery level layer dimensions
	bottom_right_text_layer = text_layer_create(GRect( 65, 210,  130,  26));
	text_layer_set_text_alignment(bottom_right_text_layer, GTextAlignmentCenter);

#endif

	// CODE START

	window_layer_cgm = window_get_root_layer(window_cgm);
	// Platform Sepcific display objects

	if(SameColourTopAndBottom) {
		bitmap_layer_set_background_color(upper_face_layer, bg_colour);
		text_layer_set_text_color(delta_layer, fg_colour);
		text_layer_set_text_color(message_layer, fg_colour);
		text_layer_set_text_color(bg_layer, fg_colour);
		text_layer_set_text_color(cgmtime_layer, fg_colour);
	} else {
		bitmap_layer_set_background_color(upper_face_layer, fg_colour);
		text_layer_set_text_color(delta_layer, bg_colour);
		text_layer_set_text_color(message_layer, bg_colour);
		text_layer_set_text_color(bg_layer, bg_colour);
		text_layer_set_text_color(cgmtime_layer, bg_colour);
	}
	bitmap_layer_set_background_color(lower_face_layer, bg_colour);
	bitmap_layer_set_alignment(icon_layer, GAlignTopLeft);
	bitmap_layer_set_background_color(icon_layer, GColorClear);
	text_layer_set_background_color(delta_layer, GColorClear);
	text_layer_set_background_color(message_layer, GColorClear);
	text_layer_set_font(message_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28));
	text_layer_set_text_alignment(message_layer, GTextAlignmentCenter);
	text_layer_set_background_color(bg_layer, GColorClear);
	text_layer_set_font(bg_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
	text_layer_set_background_color(cgmtime_layer, GColorClear);
	if(TimeAgoBold) {
		text_layer_set_font(cgmtime_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
	}
	else
	{
		text_layer_set_font(cgmtime_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28));
	}
	text_layer_set_text_color(time_watch_layer, fg_colour);
	text_layer_set_background_color(time_watch_layer, GColorClear);
	text_layer_set_font(time_watch_layer,time_font);

	
	//Paint the backgrounds for upper and lower halves of the watch face.
	LOG("Creating Upper and Lower face panels");
	layer_add_child(window_layer_cgm, bitmap_layer_get_layer(upper_face_layer));
	layer_add_child(window_layer_cgm, bitmap_layer_get_layer(lower_face_layer));


	//create the bg_trend_layer
	INFO("Creating BG Trend Bitmap layer");
#if DEBUG_LEVEL > 0
	text_layer_set_background_color(message_layer, GColorClear);
	text_layer_set_font(message_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28));
	text_layer_set_text_alignment(message_layer, GTextAlignmentCenter);
#endif

	layer_add_child(window_layer_cgm, bitmap_layer_get_layer(bg_trend_layer));

	// ARROW OR SPECIAL VALUE
	LOG("Creating Arrow Bitmap layer");
#if defined(PBL_PLATFORM_EMERY) || defined(PBL_PLATFORM_GABBRO)
	layer_add_child(bitmap_layer_get_layer(bg_trend_layer), bitmap_layer_get_layer(icon_layer));
#else
	layer_add_child(window_layer_cgm, bitmap_layer_get_layer(icon_layer));
#endif

	// DELTA BG
	LOG("Creating Delta BG Text layer");
	text_layer_set_font(delta_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28));
#ifdef TEST_MODE
	text_layer_set_text(delta_layer,"0.5mmol");
#endif

	layer_add_child(window_layer_cgm, text_layer_get_layer(delta_layer));

	// MESSAGE
	LOG("Creating Message Text layer");
#ifdef TEST_MODE
	snprintf(message_layer_text,sizeof(message_layer_text), "Test Mode");
	text_layer_set_text(message_layer, message_layer_text);
	display_message=true;
	layer_set_hidden((Layer *)message_layer, true);
	layer_add_child(window_layer_cgm, text_layer_get_layer(message_layer));
#else
	snprintf(message_layer_text,sizeof(message_layer_text),"%s","");
	text_layer_set_text(message_layer, message_layer_text);
	layer_set_hidden((Layer *)message_layer, true);
	layer_add_child(window_layer_cgm, text_layer_get_layer(message_layer));
#endif
	// BG
	LOG("Creating BG Text layer");
	layer_add_child(window_layer_cgm, text_layer_get_layer(bg_layer));


	// CGM TIME AGO READING
	LOG("Creating CGM Time Ago Bitmap layer");
	layer_add_child(window_layer_cgm, text_layer_get_layer(cgmtime_layer));


	// CURRENT ACTUAL TIME FROM WATCH
	LOG("Creating Watch Time Text layer");
//	text_layer_set_text_alignment(time_watch_layer, GTextAlignmentCenter);
	layer_add_child(window_layer_cgm, text_layer_get_layer(time_watch_layer));

	// CURRENT ACTUAL DATE FROM APP
	LOG("Creating Watch Date Text layer");
//	date_app_layer = text_layer_create(GRect(0, 122, 143, 29));
	text_layer_set_text_color(date_app_layer, fg_colour);
	text_layer_set_background_color(date_app_layer, GColorClear);
	text_layer_set_font(date_app_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
	//text_layer_set_text_alignment(date_app_layer, GTextAlignmentCenter);
	layer_add_child(window_layer_cgm, text_layer_get_layer(date_app_layer));
	draw_date_from_app();

	// Metric Layers
	// left metric layer
	LOG("Creating Left Metric Text layer");
//	text_layer_set_text_color(bottom_left_text_layer, GColorGreen);
	text_layer_set_text_color(bottom_left_text_layer, fg_colour);
	text_layer_set_background_color(bottom_left_text_layer, GColorClear);
	text_layer_set_font(bottom_left_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
	layer_add_child(window_layer_cgm, text_layer_get_layer(bottom_left_text_layer));
	//LOG("bottom_left_text_layer; %s", text_layer_get_text(bottom_left_text_layer));

	// right metric layer
	LOG("Creating Right Metric Text layer");
//	text_layer_set_text_color(bottom_right_text_layer, GColorGreen);
	text_layer_set_text_color(bottom_right_text_layer, fg_colour);
	text_layer_set_background_color(bottom_right_text_layer, GColorClear);
	text_layer_set_font(bottom_right_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
	layer_add_child(window_layer_cgm, text_layer_get_layer(bottom_right_text_layer));

	// prep for battery display, even if we don't have one.
	BatteryChargeState charge_state=battery_state_service_peek();
	battery_handler(charge_state);
	
	// put " " (space) in bg field so logo continues to show
	// " " (space) also shows these are init values, not bad or null values
	current_icon = 255; // no icon set and ignore
#ifdef TEST_MODE
	current_icon = 1;
	specvalue_alert=false;
#endif
	load_icon();
	snprintf(last_bg, BG_MSGSTR_SIZE, " ");
	load_bg();
	current_cgm_time = 0;
	load_cgmtime();
	current_app_time = 0;
	snprintf(current_bg_delta, BGDELTA_MSGSTR_SIZE, "LOAD");
//if it is not for a COLOR platform, it is monochrome
	//text_layer_set_text_alignment(cgmtime_layer, GTextAlignmentRight);
	//text_layer_set_text_alignment(cgmtime_layer, GTextAlignmentCenter);
#ifdef TEST_MODE
	snprintf(current_bg_delta, BGDELTA_MSGSTR_SIZE, "+0.08");
#endif
	load_bg_delta();
	last_battlevel = 255;
#ifdef TEST_MODE
	last_battlevel = 100;
#endif
	load_battlevel();

    // default config
    if (!use_png) trend_init(bitmap_layer_get_layer(bg_trend_layer));

//	TRACE("WINDOW LOAD, ABOUT TO CALL APP SYNC INIT");
	//app_sync_init(&sync_cgm, sync_buffer_cgm, sizeof(sync_buffer_cgm), initial_values_cgm, ARRAY_LENGTH(initial_values_cgm), sync_tuple_changed_callback_cgm, sync_error_callback_cgm, NULL);
	// init timer to null if needed, and register timer
	TRACE("window_load_cgm: build done, init timer");
    // mark dirty and request data
    dirty.need_cgm = 1;
    reset_timer_callback_cgm(LOADING_MSGSEND_SECS);
	TRACE("window_load_cgm: timer registered");

} // end window_load_cgm

void window_unload_cgm(Window *window_cgm)
{
//	TRACE("WINDOW UNLOAD IN");

	TRACE("window_unload_cgm: deinitialise app_sync");
	app_sync_deinit(&sync_cgm);

	//destroy the trend bitmap and layer
	if(bg_trend_bitmap != NULL) destroy_null_GBitmap(&bg_trend_bitmap);
	if(bg_trend_layer != NULL) destroy_null_BitmapLayer(&bg_trend_layer);
	TRACE("window_unload_cgm: destroy existing GBitmaps");
	if(icon_bitmap != NULL) destroy_null_GBitmap(&icon_bitmap);
	if(appicon_bitmap != NULL) destroy_null_GBitmap(&appicon_bitmap);
	if(specialvalue_bitmap != NULL) destroy_null_GBitmap(&specialvalue_bitmap);

	TRACE("window_unload_cgm: destroy existing Bitmaps");
	if(icon_layer != NULL) destroy_null_BitmapLayer(&icon_layer);

	TRACE("window_unload_cgm: destroy existing text layers");
	if(bg_layer != NULL) destroy_null_TextLayer(&bg_layer);
	if(cgmtime_layer != NULL) destroy_null_TextLayer(&cgmtime_layer);
	if(delta_layer != NULL) destroy_null_TextLayer(&delta_layer);
	if(message_layer != NULL) destroy_null_TextLayer(&message_layer);
	if(bottom_left_text_layer != NULL) destroy_null_TextLayer(&bottom_left_text_layer);
	if(bottom_right_text_layer != NULL) destroy_null_TextLayer(&bottom_right_text_layer);
	if(time_watch_layer != NULL) destroy_null_TextLayer(&time_watch_layer);
	if(date_app_layer != NULL) destroy_null_TextLayer(&date_app_layer);

	//destroy the face background layers.
	if(lower_face_layer != NULL) destroy_null_BitmapLayer(&lower_face_layer);
	if(upper_face_layer != NULL) destroy_null_BitmapLayer(&upper_face_layer);

	TRACE("window_unload_cgm: done");
} // end window_unload_cgm

static void init_cgm(void)
{
	LOG("init_cgm");
    use_png = persist_exists(SET_USE_PNG) ? persist_read_bool(SET_USE_PNG) : false;
	//Load persistent settings
	display_seconds = persist_exists(SET_DISP_SECS)? persist_read_bool(SET_DISP_SECS) : false;
	LOG("init_cgm: display_seccongs \"%u\".", display_seconds);
	vibe_repeat = persist_exists(SET_VIBE_REPEAT)? persist_read_bool(SET_VIBE_REPEAT) : true;
	LOG("init_cgm: vibe_repeat \"%u\".", vibe_repeat);
	SameColourTopAndBottom = persist_exists(SET_SAMECOLOUR)? persist_read_bool(SET_SAMECOLOUR) : false;
	LOG("init_cgm: SameColourTopAndBottom \"%u\".", SameColourTopAndBottom);
	message_tick_timeout = persist_exists(SET_MESSAGE_TIMEOUT) ? persist_read_int(SET_MESSAGE_TIMEOUT) * 1000 : 15000;
	LOG("init_cgm:  message_tick_timeout \"%u\".", message_tick_timeout);
#ifdef PBL_COLOR
	fg_colour = persist_exists(SET_FG_COLOUR)? GColorFromHEX(persist_read_int(SET_FG_COLOUR)) : COLOR_FALLBACK(GColorWhite,GColorWhite);
	bg_colour = persist_exists(SET_BG_COLOUR)? GColorFromHEX(persist_read_int(SET_BG_COLOUR)) : COLOR_FALLBACK(GColorDukeBlue,GColorBlack);
#endif
	TurnOffAllVibrations = persist_exists(SET_NO_VIBE)? persist_read_bool(SET_NO_VIBE) : true;
	LOG("init_cgm: TurnOffAllVibrations \"%u\".", TurnOffAllVibrations);
	BacklightOnCharge = persist_exists(SET_LIGHT_ON_CHG)? persist_read_bool(SET_LIGHT_ON_CHG) : false;
	LOG("init_cgm: BacklightOnCharge \"%u\".", BacklightOnCharge);
	TimeAgoBold = persist_exists(SET_BOLD_TIMEAGO)? persist_read_bool(SET_BOLD_TIMEAGO) : false;
	LOG("init_cgm: TimeAgoBold \"%u\".", TimeAgoBold);
	bottom_left_metric = persist_exists(SET_BOTTOM_LEFT_TEXT) ? persist_read_int(SET_BOTTOM_LEFT_TEXT) : METRIC_PHONEBATT;
	bottom_right_metric = persist_exists(SET_BOTTOM_RIGHT_TEXT) ? persist_read_int(SET_BOTTOM_RIGHT_TEXT) : METRIC_WATCHBATT;
	LOG("init_cgm: bottom_left_metric \"%u\".", bottom_left_metric);
	LOG("init_cgm: bottom_right_metric \"%u\".", bottom_right_metric);

	
	LOG("display_seconds: %i", display_seconds);
	//initialise the Time Fonts
	if (HIGH_RES()) {
		time_font_normal = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_GOTHAM_BOLD_60));
		time_font_small = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_GOTHAM_BOLD_40));
	} else {
		time_font_normal = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_GOTHAM_BOLD_40));
		time_font_small = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_GOTHAM_BOLD_30));
	}
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
	LOG("time_watch_format: %s", time_watch_format);

	TRACE("INIT CODE IN");

	// subscribe to the tick timer service
	if (display_seconds) tick_timer_service_subscribe(SECOND_UNIT, &handle_second_tick_cgm);

	tick_timer_service_subscribe(MINUTE_UNIT, &handle_minute_tick_cgm);
	message_tick_timer = app_timer_register(message_tick_timeout, handle_message_tick, NULL);

	// subscribe to the bluetooth connection service
	bluetooth_connection_service_subscribe(handle_bluetooth_cgm);

	//subscribe to the battery handler
	battery_state_service_subscribe(battery_handler);

#ifdef PBL_HEALTH
	//subscribe to the health service
	if(!health_service_events_subscribe(health_handler, NULL)) {
		LOG("Error subscribing to Health");
	}
#endif
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

	TRACE("INIT CODE, REGISTER APP MESSAGE ERROR HANDLERS");
	app_message_register_inbox_dropped(inbox_dropped_handler_cgm);
	app_message_register_outbox_failed(outbox_failed_handler_cgm);
	app_message_register_inbox_received(inbox_received_handler_cgm);

	TRACE("INIT CODE, ABOUT TO CALL APP MSG OPEN");
#ifdef PBL_PLATFORM_APLITE
//#ifndef PBL_COLOR
	app_message_open(512, 1024);
#else
	app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
#endif
	TRACE("INIT CODE, APP MSG OPEN DONE");

	const bool animated_cgm = true;
	window_stack_push(window_cgm, animated_cgm);

#ifdef ENABLE_COMM_FRAMEWORK
    comm_callbacks.bgl_data = NULL;
    comm_callbacks.bgl_series = NULL;
#ifdef ENABLE_TREND_RENDERER
    comm_callbacks.low_limit = trend_set_low_line;
    comm_callbacks.high_limit = trend_set_high_line;
#endif
    comm_callbacks.phonebat = set_phone_battery;
    comm_callbacks.slopeval = set_icon;
    comm_callbacks.vibe = set_vibrate;
    comm_callbacks.bgl_delta = set_bgl_delta;
    comm_callbacks.bgl_series = trend_set_series;
    comm_callbacks.bgl_data = set_bgl_data;
    comm_callbacks.bgl_timestamp = set_bgl_timestamp;
    comm_callbacks.bgl_value = set_bgl_value;
    comm_callbacks.png = set_png;
    comm_init(&comm_callbacks);
#endif

	LOG("init_cgm done.");
}	// end init_cgm

static void deinit_cgm(void)
{
	INFO("DEINIT CODE IN");
	// Make sure we are not handling a second tick.
	while (handling_second) {};

	// unsubscribe to the tick timer service
	TRACE("DEINIT, UNSUBSCRIBE TICK TIMER");
	tick_timer_service_unsubscribe();
	app_timer_cancel(message_tick_timer);

	// unsubscribe to the bluetooth connection service
	TRACE("DEINIT, UNSUBSCRIBE BLUETOOTH");
	bluetooth_connection_service_unsubscribe();

	battery_state_service_unsubscribe();
#ifdef PBL_HEALTH
	health_service_events_unsubscribe();
#endif

	// cancel timers if they exist
	TRACE("DEINIT, CANCEL APP TIMER");
	if (timer_cgm != NULL)
	{
		app_timer_cancel(timer_cgm);
		timer_cgm = NULL;
	}

	TRACE("DEINIT, CANCEL BLUETOOTH TIMER");
	if (BT_timer != NULL)
	{
		app_timer_cancel(BT_timer);
		BT_timer = NULL;
	}

	// destroy the window if it exists
	TRACE("DEINIT, CHECK WINDOW POINTER FOR DESTROY");
	TRACE("DEINIT, CHECK WINDOW POINTER FOR NULL");
	if (window_cgm != NULL)
	{
		TRACE("DEINIT, WINDOW POINTER NOT NULL, SET TO NULL");
		window_cgm = NULL;
	}
	//unload the custom time font.
	fonts_unload_custom_font(time_font_normal);
	fonts_unload_custom_font(time_font_small);


	TRACE("DEINIT CODE OUT");
} // end deinit_cgm


/**
 * Pebble SDK does not support varargs, so we result to simply writing a string
 */
int mgdl_to_mmoll_str(int mgdl, char *result, const int size, int unit) {
    const char *fmt = unit ? "%s%d.%d mmol/l" : "%s%d.%d";
    int val = MGDL_TO_MMOL(mgdl);
    int dec = MGDL_TO_MMOL_DEC(mgdl);
  
    // fix rounding up
    if (dec == 10) {
        val++;
        dec = 0;
    } else if (dec == -10) {
        val--;
        dec = 0;
    }
    return snprintf(result, size, fmt, (dec < 0 && val == 0) || val < 0 ? "-" : "", abs(val), abs(dec));
}

#ifdef ENABLE_COMM_FRAMEWORK
/*
 * Comm framework callback functions
 */
void set_icon(comm_slopeval value) {
    current_icon = value;
    load_icon();
}

void set_phone_battery(comm_phonebat value) {
    last_battlevel = value;
    load_battlevel();
}

// snprintf does not support float!
void set_bgl_delta(comm_bgl_delta value) {
    DEBUG("Delta units: undefined: %d mmol: %d display: %d value: %d", value.undefined, value.is_mmol, value.display_units, value.value);
    if (value.undefined) {
        snprintf(current_bg_delta, sizeof(current_bg_delta), "???");
    } else if (value.is_mmol && value.display_units) {
        TRACE("MMOL + Display");
        mgdl_to_mmoll_str(value.value, current_bg_delta, sizeof(current_bg_delta), 1);
    } else if (value.display_units && !value.is_mmol) {
        TRACE("MG + Display");
        snprintf(current_bg_delta, sizeof(current_bg_delta), "%hd mg/dL", value.value);
    } else if (value.is_mmol) {
        TRACE("MMOL");
        mgdl_to_mmoll_str(value.value, current_bg_delta, sizeof(current_bg_delta), 0);
    } else {
        TRACE("MG");
        int16_t delta = value.value;
        snprintf(current_bg_delta, sizeof(current_bg_delta), "%hd", delta);
    }
    dirty.delta = 1;
    load_bg_delta();
}

void set_vibrate(comm_vibe value) {
    if (!BluetoothAlert) alert_handler_cgm(value);
}

void set_bgl_timestamp(uint32_t timestamp) {
    TRACE("Set BGL Timestamp");
    current_cgm_time = timestamp;
    reset_timer_callback_cgm((timestamp - time(NULL)) + (60 * 6));
    load_cgmtime();
}

void set_bgl_value(comm_bgl_value value) {
    TRACE("Set BGL Value");
    if (value.is_mmol) {
        mgdl_to_mmoll_str(value.value, last_bg, sizeof(last_bg), 0);
    } else {
        snprintf(last_bg, sizeof(last_bg), "%d", value.value);
    }
    load_bg();
}

/**
 * update bgl values and timestamp
 */
void set_bgl_data(comm_bgl_data *value) {
    TRACE("Set BGL Data");
    if (value->timestamp != current_cgm_time) {
        set_bgl_timestamp(value->timestamp);
        set_bgl_value(value->bgl);
        if (value->timestamp - current_cgm_time > 360) {
            dirty.need_cgm = 1;
            // we likely missed a value, set minutes timer to zero and wait for global udpate
            reset_timer_callback_cgm((value->timestamp - time(NULL)) + (60));
        } else if (!use_png) trend_set_value(value);
    } else {
        WARNING("Received same bgl value twice!");
    }

}

/**
 * Since all data is in flight and copied by the Bitmap creation we 
 * do not have to copy it
 */
void set_png(comm_png_data data) {
    TRACE("Setting PNG");
    if(bg_trend_bitmap != NULL)
    {
        INFO("Destroying bg_trend_bitmap");
        gbitmap_destroy(bg_trend_bitmap);
        bg_trend_bitmap = NULL;
    }

    bg_trend_bitmap = gbitmap_create_from_png_data(data.data, data.length);
    if(bg_trend_bitmap != NULL)
    {
        LOG("bg_trend_bitmap created, setting to layer");
        bitmap_layer_set_bitmap(bg_trend_layer, bg_trend_bitmap);
    }
    else
    {
        WARNING("bg_trend_bitmap creation FAILED!");
    }
    dirty.need_cgm = 0;
}
#endif

int main(void)
{
	init_cgm();
	app_event_loop();
	deinit_cgm();

} // end main
