#include "communication.h"
#include "../xdrip.h"
#include "../debug.h"
#include "../api/settings.h"
#include <pebble.h>

#define CM "COMM FW: "

static AppState *state = NULL;
static CommunicationCallbacks *cb = NULL;

void comm_init(void *value) {
    state = value;
    cb = &state->comm_callbacks;
}

void comm_handle(Tuple *data) {
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
            if (cb->bgl_value != NULL) cb->bgl_value(value->bgl);
            break;
        case FRAMEWORK_BGL_SERIES:
            TRACE(CM "BGL Data stream");
            comm_bgl_series *series = (comm_bgl_series *) data->value->data;
            TRACE(CM "Since %d", series->timestamp);
            if (cb->bgl_series != NULL) cb->bgl_series(series);
            if (cb->bgl_timestamp != NULL) cb->bgl_timestamp(series->timestamp);
            if (cb->bgl_value != NULL) cb->bgl_value(series->bgl_values[series->length - 1]);
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

void comm_request_png(DictionaryIterator *iter, GRect bounds) {
    TRACE(CM "Sending PNG Request");
    comm_trend_size ts;

    ts.width = bounds.size.w;
    ts.height = bounds.size.h;

#ifdef PBL_PLATFORM_GABBRO
    ts.rgb8 = 1;
#endif
    dict_write_uint32(iter, FRAMEWORK_PNG_IMAGE, ts.raw);
}

extern uint32_t current_cgm_time; // for now steal time, we could track it locally though

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
/* static void health_send_values(void *data) { */
/* 	health_send_timer = NULL; */
/* 	if (!state.collect_health || state.bluetooth_alert) return; */
/*  */
/* 	health_poll(); */
/* 	if (state.hbm == 0 && state.step_count == 0) return; */
/*  */
/* 	DictionaryIterator *iter = NULL; */
/* 	if (app_message_outbox_begin(&iter) != APP_MSG_OK) { */
/* 		LOG("health_send_values: outbox busy"); */
/* 		return; */
/* 	} */
/* 	comm_send_health(iter, (comm_health){ */
/* 		.heart_rate = (uint16_t) state.hbm, */
/* 		.steps = (uint32_t) state.step_count, */
/* 	}); */
/*  */
/* 	dict_write_end(iter); */
/* 	if (app_message_outbox_send() == APP_MSG_OK) { */
/* 		LOG("health_send_values: sent hr=%ld steps=%ld", state.step_count, state.step_count); */
/* 	} */
/* } */

#ifdef ENABLE_TOUCH
void comm_send_basal_bolus(int32_t basal, int32_t bolus) {
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
