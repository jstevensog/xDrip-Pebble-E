#include <pebble.h>
#include <stdarg.h>
#include "xdrip.h" // set DEBUG_LEVEL in here or on the pebble build command line
#include "constant.h"
#include "debug.h" // must be included after xdrip.h
#include "api/communication.h"
#ifdef ENABLE_TREND_RENDERER
#include "api/trend.h"
#endif
#include "ui/ui_og.h"
#include "api/settings.h"

/**
 * Variables
 */
AppState state = {0};
bool global_lock = false;

// Boolean as mutex to prevent use after free 
static bool handling_second = false;
// variables for AppSync
AppSync sync_cgm;

// variables for timers and time
AppTimer *timer_cgm = NULL;
AppTimer *BT_timer = NULL;
AppTimer *health_send_timer = NULL;
time_t time_now = 0;

// global variable for bluetooth connection
bool bluetooth_connected_cgm = true;


/**
 * predefines
 */
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
		case APP_MSG_INVALID_STATE:
			return "APP_MSG_INVALID_STATE";
		default:
			return "APP UNKNOWN ERROR";
	}
}

#if defined(DEBUG_LEVEL) && DEBUG_LEVEL >= DEBUG_LEVEL_INFO
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


#ifdef PBL_HEALTH





// health_poll - peek the current heart rate and step total into health_hr /
// health_steps. PebbleOS does not reliably emit HealthEventHeartRateUpdate at
// rest, so we read on demand from health_handler and the minute tick rather
// than keying off a specific event.
void health_poll(void) {
	if (!state.collect_health) return;

	time_t now = time(NULL);
#if defined(PBL_PLATFORM_EMERY) || defined(PBL_PLATFORM_DEORITE)
	if (health_service_metric_accessible(HealthMetricHeartRateBPM, now, now)
		& HealthServiceAccessibilityMaskAvailable) {
		HealthValue bpm = health_service_peek_current_value(HealthMetricHeartRateBPM);
		if (bpm > 0) state.hbm = bpm;
	}
#endif

	time_t day_start = time_start_of_today();
	if (health_service_metric_accessible(HealthMetricStepCount, day_start, now)
		& HealthServiceAccessibilityMaskAvailable) {
		state.step_count = health_service_sum_today(HealthMetricStepCount);
	}
}


// health_handler - handler to deal with health events
void health_handler(HealthEventType event, void *context) {
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
	health_poll();
	update_health_metric_displays();
} //end health_handler

// health_schedule_send - arm the one-shot send timer. Called from the CGM
// receive path so the reply goes out while the phone is awake.
void health_schedule_send(void) {
	if (!state.collect_health) return;
    // TODO fix health
	/* if (health_send_timer == NULL || !app_timer_reschedule(health_send_timer, 2000)) { */
	/* 	health_send_timer = app_timer_register(2000, health_send_values, NULL); */
	/* } */
}
#endif


// battery_handler - updates the pebble battery percentage.
static void battery_handler()
{
    BatteryChargeState charge_state = battery_state_service_peek();
	// If there are no battery level metric display elements, exit
	if(state.left_text_field != METRIC_WATCHBATT && state.right_text_field != METRIC_WATCHBATT) {
		return;
	}
    state.battery_is_charging = charge_state.is_charging;
    state.battery_level = charge_state.charge_percent;

    CALLBACK(state.wf_cb.update_battery_state);
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

	if (state.vibrate_off)
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
			if (state.vibrate_strong_off)
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
			if (state.vibrate_strong_off)
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
			if (state.vibrate_strong_off)
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

		// Check state.bluetooth_alert for extended Bluetooth outage; if so, do nothing
		if (state.bluetooth_alert)
		{
			//Already vibrated and set message; out
			return;
		}

		// Check to see if the BT_timer needs to be set; if BT_timer is not null we're still waiting
		if (BT_timer == NULL)
		{
			// check to see if timer has popped
			if (!state.bluetooth_timer_pop)
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
		// Vibrate; state.bluetooth_alert takes over until Bluetooth connection comes back on
		LOG("BT HANDLER: TIMER POP, NO BLUETOOTH");
		alert_handler_cgm(BTOUT_VIBE);
		state.bluetooth_alert = true;

		// Reset timer pop
		if(state.vibrate_repeat) 
		{
			state.bluetooth_timer_pop = false;
		}

		TRACE("NO BLUETOOTH");
		if (!state.bluetooth_message_off)
		{
#ifdef PBL_COLOR
            CALLBACK(state.wf_cb.set_delta_colour, GColorRed);
#endif
            CALLBACK(state.wf_cb.set_delta, "NO BLUETOOTH", 12);
			// make sure we get the data we need
			//dirty.need_cgm = 1;
			state.cgm_time = 0;
			reset_timer_callback_cgm(2);
		}

		// erase cgm and app ago times
        CALLBACK(state.wf_cb.set_cgmtime, "", 0);
	}

	else
	{
		// Bluetooth is on, reset state.bluetooth_alert
		TRACE("HANDLE BT: BLUETOOTH ON");
		state.bluetooth_alert = false;
		if (BT_timer == NULL)
		{
			// no timer is set, so need to reset timer pop
			state.bluetooth_timer_pop = false;
		}
#ifdef PBL_COLOR
        CALLBACK(state.wf_cb.update_colours);
#endif

	}

	TRACE("state.bluetooth_alert: %i", state.bluetooth_alert);
} // end handle_bluetooth_cgm


void BT_timer_callback(void *data)
{
	TRACE("BT TIMER CALLBACK: ENTER CODE");

	// reset timer pop and timer
	state.bluetooth_timer_pop = TRUE;
	if (BT_timer != NULL)
	{
		BT_timer = NULL;
	}

	// check bluetooth and call handler
	bluetooth_connected_cgm = bluetooth_connection_service_peek();
	handle_bluetooth_cgm(bluetooth_connected_cgm);

} // end BT_timer_callback

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
		// reset state.app_sync_error_alert to flag for vibrate
		state.app_sync_error_alert = false;

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
	DEBUG("state.app_sync_error_alert:	%i", state.app_sync_error_alert);

	bluetooth_connected_cgm = bluetooth_connection_service_peek();

	if (!bluetooth_connected_cgm || appsync_err_openerr == APP_MSG_BUSY)
	{
		// bluetooth is out, BT message already set; return out
		return;
	}

	// set message to RESTART WATCH -> PHONE
#if DEBUG
	text_layer_set_text(delta_layer, translate_app_error(appsync_err_openerr));
    CALLBACK(state.wf_cb.set_delta, translate_app_error(appsync_err_openerr), strlen(translate_app_error(appsync_err_openerr))); 
#else
    CALLBACK(state.wf_cb.set_delta, "RSTRT WCH/PHN", 14); 
#endif

	// erase cgm and app ago times
    CALLBACK(state.wf_cb.set_cgmtime, "", 0);

	// check if need to vibrate
	if (!state.app_sync_error_alert)
	{
		LOG("APPSYNC ERROR: VIBRATE");
		alert_handler_cgm(APPSYNC_ERR_VIBE);
		state.app_sync_error_alert = true;
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
		// reset state.app_msg_in_drop_alert to flag for vibrate
		state.app_msg_in_drop_alert = false;

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
	DEBUG("state.app_msg_in_drop_alert:	%i", state.app_msg_in_drop_alert);

	bluetooth_connected_cgm = bluetooth_connection_service_peek();

	if (!bluetooth_connected_cgm)
	{
		// bluetooth is out, BT message already set; return out
		return;
	}

	// set message to RESTART WATCH -> PHONE
    CALLBACK(state.wf_cb.set_delta, "RSTRT WCH/PHN", 14); 

	// erase cgm and app ago times
    CALLBACK(state.wf_cb.set_cgmtime, "", 0);

	// check if need to vibrate
	if (!state.app_msg_in_drop_alert)
	{
	LOG("inbox_drepped_handler_cgm: VIBRATE");
		alert_handler_cgm(APPMSG_INDROP_VIBE);
		state.app_msg_in_drop_alert = true;
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
	ERROR("outbox_failed_handler_cgm: ERR CODE: %i RES: %s", appmsg_outfail_error, translate_app_error(appmsg_outfail_error));

	bluetooth_connected_cgm = bluetooth_connection_service_peek();

	if (!bluetooth_connected_cgm)
	{
		// bluetooth is out, BT message already set; return out
		return;
	}

	appmsg_outfail_openerr = app_message_outbox_begin(&iter);

	if (appmsg_outfail_openerr == APP_MSG_OK)
	{
		// reset state.app_msg_out_fail_alert to flag for vibrate
		state.app_msg_out_fail_alert = false;

		// send message
		AppMessageResult appsync_err_senderr = app_message_outbox_send();
		TRACE("APP SYNC SEND ERR CODE: %i RES: %s", appsync_err_senderr, translate_app_error(appsync_err_senderr));
		if (appsync_err_senderr != APP_MSG_OK  && appsync_err_senderr != APP_MSG_BUSY && appsync_err_senderr != APP_MSG_SEND_REJECTED)
		{
			INFO("APP SYNC SEND ERROR");
			DEBUG("APP SYNC SEND ERR CODE: %i RES: %s", appsync_err_senderr, translate_app_error(appsync_err_senderr));
		}
		return;
	}

//	INFO("APPMSG OUT FAIL RESEND ERROR");
	DEBUG("outbox_failed_handler_cgm RESEND ERR CODE: %i RES: %s", appmsg_outfail_openerr, translate_app_error(appmsg_outfail_openerr));
	DEBUG("state.app_msg_out_fail_alert: %i", state.app_msg_out_fail_alert);

	bluetooth_connected_cgm = bluetooth_connection_service_peek();

	if (!bluetooth_connected_cgm || appmsg_outfail_senderr != APP_MSG_SEND_REJECTED)
	{
		// bluetooth is out, BT message already set; return out
		return;
	}

	// set message to RESTART WATCH -> PHONE
#if DEBUG
    CALLBACK(state.wf_cb.set_delta, translate_app_error(appmsg_outfail_openerr), strlen(translate_app_error(appmsg_outfail_openerr));
	text_layer_set_text(delta_layer, translate_app_error(appmsg_outfail_openerr));
#else
    CALLBACK(state.wf_cb.set_delta, "RSTRT WCH/PHN", 14);
#endif

	// erase cgm and app ago times
    CALLBACK(state.wf_cb.set_cgmtime, "", 0);
	//text_layer_set_text(time_app_layer, "");

	// check if need to vibrate
	if (!state.app_msg_out_fail_alert)
	{
		LOG("outbox_failed_handler_cgm: VIBRATE");
		alert_handler_cgm(APPMSG_OUTFAIL_VIBE);
		state.app_msg_out_fail_alert = true;
	}

} // end outbox_failed_handler_cgm



// send_cmd_cgm - Function to send dat to xDrip to cause a refresh/update of data.
// Needs to include configuration values that xDrip can read and respond to.
static void send_cmd_cgm(void)
{
	AppMessageResult sendcmd_openerr = APP_MSG_OK;
	AppMessageResult sendcmd_senderr = APP_MSG_OK;
	DictionaryIterator *iter = NULL;

	if(state.bluetooth_alert)
	{
		//BT is down rignt now, so don't do anything.
		//Note, we cannot log this, as BT must be up in order to log it.
		return;
	}

	// if bt escapes early (above) outbox_begin MAY NOT be triggered as it will leave the outbox in
	// an unrecoverable state, the entire function must be performed if app_message_outbox_begin succeeds.
	sendcmd_openerr = app_message_outbox_begin(&iter);

	if (sendcmd_openerr != APP_MSG_OK)
	{
		ERROR("send_cmd_cgm: ERR CODE: %i RES: %s", sendcmd_openerr, translate_app_error(sendcmd_openerr));
		// proceed to send since it's the only way to recover
		goto send_appmsg;
	}

    comm_request_heartbeat(
            iter,
            state.use_png, state.wf_cb.trend_bounds(),
            !trend_isinitialized() && !state.use_png,
            state.dirty.need_cgm, state.cgm_time,
            state.right_text_field == METRIC_PHONEBATT || state.left_text_field == METRIC_PHONEBATT,
            state.right_text_field == METRIC_SENSOR_EXPIRY || state.left_text_field == METRIC_SENSOR_EXPIRY
    );

	dict_write_end(iter);

send_appmsg:
	TRACE("send_cmd_cgm: Opening outbox");
	sendcmd_senderr = app_message_outbox_send();
	if (sendcmd_senderr != APP_MSG_OK && sendcmd_senderr != APP_MSG_BUSY && sendcmd_senderr != APP_MSG_SEND_REJECTED)
	{
		ERROR("send_cmd_cgm: ERR CODE: %i RES: %s", sendcmd_senderr, translate_app_error(sendcmd_senderr));
	}
	TRACE("send_cmd_cgm: done");
} // end send_cmd_cgm


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
        if (data->key >= 100 && data->key < 200) settings_handle(data);
#ifdef ENABLE_TREND_RENDERER
        if (data->key >= 300 && data->key < 400) trend_process_config(data);
#endif
#ifdef ENABLE_COMM_FRAMEWORK
        if (data->key >= 2000 && data->key < 3000) comm_handle(data);
#endif
		data = dict_read_next(iterator);
	}
} // end sync_tuple_changed_callback_cgm()

void reset_timer_callback_cgm(int32_t seconds) {
	int32_t retimer = (seconds) * 1000;
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
	DEBUG("timer %d %d", state.cgm_time, time(NULL));
	// if we have not received anything for over 6 minutes, keep checking
	if ((long) (state.cgm_time + 360) < time(NULL)) {
		// mark cgm data as dirty, send heartbeat
		state.dirty.need_cgm = 1;
		// we do not reset time as likely we only missed a few messages
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


void handle_second_tick_cgm(struct tm* tick_time_cgm, TimeUnits units_changed_cgm)
{
	TRACE("handle_second_tick_cgm:");
	// CODE START
	handling_second = true;
    CALLBACK(state.wf_cb.seconds_tick, tick_time_cgm, units_changed_cgm);
	handling_second = false;

} // end handle_second_tick_cgm

/**
 * Check minute and day data
 */
void handle_minute_tick_cgm(struct tm* tick_time_cgm, TimeUnits units_changed_cgm)
{
	TRACE("handle_minute_tick_cgm:");

	// CODE START
	if (units_changed_cgm & MINUTE_UNIT)
	{
		LOG("handle_minute_tick_cgm: tick");
#ifdef PBL_HEALTH
		// keep health_hr / health_steps current; the send itself is driven off
		// an incoming CGM push (health_schedule_send), not this tick
		health_poll();
#endif
        CALLBACK(state.wf_cb.minutes_tick, tick_time_cgm, units_changed_cgm);
	}

	// detect some error in rescheduling
	if (time(NULL) - state.cgm_time > (10 * 60)) {
		reset_timer_callback_cgm(2);
	}

} // end handle_minute_tick_cgm

static void init_cgm(void)
{
	LOG("init_cgm");
    settings_init(&state); // register appmg updates and callbacks
    ui_og_init(&state); // register main ui and callbacks

	TRACE("INIT CODE IN");

	// subscribe to the tick timer service
	if (state.enable_seconds) tick_timer_service_subscribe(SECOND_UNIT, &handle_second_tick_cgm);

	tick_timer_service_subscribe(MINUTE_UNIT, &handle_minute_tick_cgm);

	// subscribe to the bluetooth connection service
	bluetooth_connection_service_subscribe(handle_bluetooth_cgm);

	//subscribe to the battery handler
	battery_state_service_subscribe(battery_handler);
    battery_handler();
#ifdef PBL_HEALTH
	//subscribe to the health service
	if(!health_service_events_subscribe(health_handler, NULL)) {
		LOG("Error subscribing to Health");
	}
	// restore whether the phone last asked us to report health data
	state.collect_health = persist_exists(state.collect_health) ? persist_read_bool(state.collect_health) : false;
	LOG("init_cgm: state.collect_health \"%u\".", state.collect_health);
	if (state.collect_health) {
#if defined(PBL_PLATFORM_EMERY) || defined(PBL_PLATFORM_DIORITE) || defined(PBL_PLATFORM_FLINT) || defined(PBL_PLATFORM_GABBRO)
		health_service_set_heart_rate_sample_period(600);
#endif
		health_poll();
	}
#endif
	TRACE("INIT CODE, REGISTER APP MESSAGE ERROR HANDLERS");
	app_message_register_inbox_dropped(inbox_dropped_handler_cgm);
	app_message_register_outbox_failed(outbox_failed_handler_cgm);
	app_message_register_inbox_received(inbox_received_handler_cgm);

	TRACE("INIT CODE, ABOUT TO CALL APP MSG OPEN");
#ifdef PBL_PLATFORM_APLITE
	app_message_open(512, 256);
#else
	app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
#endif
	TRACE("INIT CODE, APP MSG OPEN DONE");

	LOG("init_cgm done.");
}	// end init_cgm

static void deinit_cgm(void)
{
	INFO("DEINIT CODE IN");
	// Make sure we are not handling a second tick.
	while (handling_second) {};

    ui_og_deinit();
    //settings_deinit();

	TRACE("window_unload_cgm: deinitialise app_sync");
	app_sync_deinit(&sync_cgm);

	// unsubscribe to the tick timer service
	TRACE("DEINIT, UNSUBSCRIBE TICK TIMER");
	tick_timer_service_unsubscribe();

	// unsubscribe to the bluetooth connection service
	TRACE("DEINIT, UNSUBSCRIBE BLUETOOTH");
	bluetooth_connection_service_unsubscribe();

	battery_state_service_unsubscribe();
#ifdef PBL_HEALTH
	health_service_events_unsubscribe();
	if (health_send_timer != NULL) {
		app_timer_cancel(health_send_timer);
		health_send_timer = NULL;
	}
#if defined(PBL_PLATFORM_EMERY) || defined(PBL_PLATFORM_DIORITE) || defined(PBL_PLATFORM_FLINT) || defined(PBL_PLATFORM_GABBRO)
	if (state.collect_health) health_service_set_heart_rate_sample_period(0);
#endif
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

int main(void)
{
	init_cgm();
	app_event_loop();
	deinit_cgm();
} // end mai
