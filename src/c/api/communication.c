#include "communication.h"
#include "../xdrip.h"
#include "../debug.h"
#include "../api/settings.h"
#include <pebble.h>

#define CM "COMM FW: "

extern AppState state;
CommunicationCallbacks *cb = NULL;

void comm_init(void *value)
{
    /* state = value; */
    cb = &state.comm_callbacks;

	TRACE("INIT CODE, REGISTER APP MESSAGE ERROR HANDLERS");
	app_message_register_inbox_dropped(inbox_dropped_handler_cgm);
	app_message_register_outbox_failed(outbox_failed_handler_cgm);
	app_message_register_inbox_received(inbox_received_handler_cgm);
}

void comm_handle(Tuple *data)
{
    uint32_t key = data->key;
    INFO(CM "Key: %d", key);

    if (cb == NULL) {
        WARNING(CM "Cannot perform anu actions, no callback registered");
        return;
    }

    switch (key) {
        case FRAMEWORK_HIGHLIMIT:
            TRACE(CM "High limit");
            if (cb->high_limit) cb->high_limit((comm_high_limit) { .raw = data->value->uint32 });
            break;
        case FRAMEWORK_LOWLIMIT:
            TRACE(CM "Low  limit");
            if (cb->low_limit) cb->low_limit((comm_low_limit) { .raw = data->value->uint32 });
            break;
        case FRAMEWORK_SLOPEVAL:
            TRACE(CM "Slope icon value");
            if (cb->slopeval != NULL) cb->slopeval(data->value->uint8);
            break;
        case FRAMEWORK_PHONEBAT:
            TRACE(CM "Phone battery level");
            if (cb->phonebat != NULL) cb->phonebat(data->value->uint8);
            break;
        case FRAMEWORK_VIBE:
            TRACE(CM "Vibrate");
            if (cb->vibe != NULL) cb->vibe(data->value->uint8);
            break;
        case FRAMEWORK_BGL_DELTA:
            TRACE(CM "Delta value");
            if (cb->bgl_delta != NULL) cb->bgl_delta((comm_bgl_delta) { .raw = data->value->uint16 });
            break;
        case FRAMEWORK_BGL_VALUE:
            TRACE(CM "Update value");
            comm_bgl_data *value = (comm_bgl_data *) data->value->data;
            if (cb->bgl_data != NULL) cb->bgl_data(value);
            if (cb->bgl_timestamp != NULL) cb->bgl_timestamp(value->timestamp);
            if (cb->bgl_value != NULL) {
                cb->bgl_value(value->bgl);
#ifdef PBL_HEALTH
                CALLBACK(state.gl_cb.health_schedule_send);
#endif
            }
            break;
        case FRAMEWORK_BGL_SERIES:
            TRACE(CM "BGL Data stream");
            comm_bgl_series *series = (comm_bgl_series *) data->value->data;
            TRACE(CM "Since %d", series->timestamp);
            if (cb->bgl_series != NULL) cb->bgl_series(series);
            if (cb->bgl_timestamp != NULL) cb->bgl_timestamp(series->timestamp);
            if (cb->bgl_value != NULL) {
                cb->bgl_value(series->bgl_values[series->length - 1]);
#ifdef PBL_HEALTH
                CALLBACK(state.gl_cb.health_schedule_send);
#endif
            }
            break;
        case FRAMEWORK_PNG_IMAGE:
            TRACE(CM "PNG image data");
            if (cb->png != NULL) cb->png((comm_png_data *) data->value->data);
            break;
        case FRAMEWORK_SENSOR_INFO:
            TRACE(CM "Sensor info");
            if (cb->sensor_info != NULL) cb->sensor_info((comm_sensor_info *) data->value->data);
            break;
        case FRAMEWORK_BWP_VALUE:
            TRACE(CM "Bolus wizard previes value");
            if (cb->bwp_value != NULL) cb->bwp_value(data->value->uint32);
            break;
        case FRAMEWORK_MESSAGE:
            TRACE(CM "Message received");
            // avoid malformed cstrings crashing the watch via OOB
            if (cb->message != NULL) cb->message((comm_message){ .length = data->length, .message = data->value->cstring});
            break;
#ifdef PBL_HEALTH
        case FRAMEWORK_HEALTH_HR:
            TRACE(CM "Health heart rate");
            if (cb->health != NULL) cb->health((comm_health){ .heart_rate = data->value->uint32, .steps = 0 });
            break;
        case FRAMEWORK_HEALTH_STEPS:
            TRACE(CM "Health step count");
            if (cb->health != NULL) cb->health((comm_health){ .heart_rate = 0, .steps = data->value->uint32 });
            break;
#endif
        default:
            DEBUG(CM "id %ld not handled by communications framework", key);
            break;
    }
}


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

	state.bluetooth_is_connected = bluetooth_connection_service_peek();

	if (!state.bluetooth_is_connected)
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

	DEBUG("APP SYNC RESEND ERR CODE: %i RES: %s", appsync_err_openerr, translate_app_error(appsync_err_openerr));
	DEBUG("state.app_sync_error_alert:	%i", state.app_sync_error_alert);

	state.bluetooth_is_connected = bluetooth_connection_service_peek();

	if (!state.bluetooth_is_connected || appsync_err_openerr == APP_MSG_BUSY)
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
		CALLBACK(state.gl_cb.alert_handler, APPSYNC_ERR_VIBE);
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
	
	state.bluetooth_is_connected = bluetooth_connection_service_peek();

	if (!state.bluetooth_is_connected)
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

	state.bluetooth_is_connected = bluetooth_connection_service_peek();

	if (!state.bluetooth_is_connected)
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
		CALLBACK(state.gl_cb.alert_handler, APPMSG_INDROP_VIBE);
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

	state.bluetooth_is_connected = bluetooth_connection_service_peek();

	if (!state.bluetooth_is_connected)
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

	state.bluetooth_is_connected = bluetooth_connection_service_peek();

	if (!state.bluetooth_is_connected || appmsg_outfail_senderr != APP_MSG_SEND_REJECTED)
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
        CALLBACK(state.gl_cb.alert_handler, APPMSG_OUTFAIL_VIBE);
		state.app_msg_out_fail_alert = true;
	}

} // end outbox_failed_handler_cgm

void comm_request_init(void)
{
}

void comm_request_end(void)
{
}

void comm_request_png(DictionaryIterator *iter, GRect bounds)
{
    TRACE(CM "Sending PNG Request");
    comm_trend_size ts;

    ts.width = bounds.size.w;
    ts.height = bounds.size.h;

#ifdef PBL_PLATFORM_GABBRO
    ts.rgb8 = 1;
#endif
    dict_write_uint32(iter, FRAMEWORK_PNG_IMAGE, ts.raw);
}

void comm_request_heartbeat(
        DictionaryIterator *iter,
        bool use_png, GRect png_bounds,
        bool update_lines,
        bool update_cgm, uint32_t current_cgm_time, 
        bool update_battery,
        bool update_sensor
)
{
	comm_heartbeat hb = {0}; // force zero init

    // send if we are a colour pebble or not 
#ifdef PBL_COLOR
	hb.colour = 1;
#else 
	hb.colour = 0;
#endif

	hb.time_series = use_png ? 0 : 1;

    // currently not used
#ifdef PBL_PLATFORM_GABBRO
	hb.time_period = 1;
#else
	hb.time_period = 3;
#endif

	// trend line and limit values
	if (update_lines) { 
		hb.high_limit = 1;
		hb.low_limit = 1;
	}

	// pump values
    // these are currently not implemented in xdrip
	/* hb.send_iob = 1; */
	/* hb.send_pump_state = 1; */
	/* hb.send_pump_battery = 1; */
   

    // update cgm triggers quite a bit of data, it at least requires the delta and slope
    // xdrip decides what to send with respect to the CGM_TIME and if use_png is set or not
    // Due to xdrip possibly not knowing what the state of the screen is the png size is also
    // send to xdrip
	if (update_cgm) {
		hb.send_slope_arrow = 1;
		hb.send_delta_value = 1;
		dict_write_uint32(iter, FRAMEWORK_BGL_VALUE, current_cgm_time); // request update
		if (use_png) {
			comm_request_png(iter, png_bounds);
		}
	}

    if (update_sensor) {
        hb.send_sensor_info = 1;
    }

	if (update_battery) hb.send_phone_battery = 1;

	dict_write_uint32(iter, FRAMEWORK_HEARTBEAT, hb.raw);
}

#ifdef PBL_HEALTH
void comm_send_health(DictionaryIterator *iter, comm_health data)
{
    TRACE(CM "Sending health hr=%d steps=%d", data.heart_rate, (int) data.steps);
    if (iter == NULL) return;
    if (data.heart_rate > 0) dict_write_uint32(iter, FRAMEWORK_HEALTH_HR, data.heart_rate);
    if (data.steps > 0)      dict_write_uint32(iter, FRAMEWORK_HEALTH_STEPS, data.steps);
}
#endif

// health_send_values - push the current heart rate + step total to the phone as
// a standalone AppMessage via the comm framework. Scheduled ~2s after an
// incoming CGM push (health_schedule_send), when xDrip's process is awake and
// its broadcast receiver will actually get the reply.
// // TODO fix health
void health_send_values(void *data) {
	if (!state.collect_health || state.bluetooth_alert) return;

	CALLBACK(state.gl_cb.health_poll);
	if (state.hbm == 0 && state.step_count == 0) return;

	DictionaryIterator *iter = NULL;
	if (app_message_outbox_begin(&iter) != APP_MSG_OK) {
		LOG("health_send_values: outbox busy");
		return;
	}
	comm_send_health(iter, (comm_health){
		.heart_rate = (uint16_t) state.hbm,
		.steps = (uint32_t) state.step_count,
	});

	dict_write_end(iter);
	if (app_message_outbox_send() == APP_MSG_OK) {
		LOG("health_send_values: sent hr=%ld steps=%ld", state.step_count, state.step_count);
	}
}

#ifdef ENABLE_TOUCH
void comm_send_basal_bolus(int32_t basal, int32_t bolus)
{
    DictionaryIterator *iter = NULL;
	if (app_message_outbox_begin(&iter) != APP_MSG_OK) {
		LOG("health_send_values: outbox busy");
		return;
	}
    comm_basal_bolus values = {
        .basal = basal,
        .bolus = bolus
    };
    dict_write_data(iter, FRAMEWORK_BASAL_BOLUS, (uint8_t *) &values, sizeof(values));
	dict_write_end(iter);
	if (app_message_outbox_send() == APP_MSG_OK) {
        LOG("Send basal and bolus values: %d %d", basal, bolus);
	}
}
#endif
