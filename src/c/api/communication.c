#include "communication.h"
#include "../xdrip.h"
#include "../debug.h"
#include <pebble.h>

#define CM "COMM FW: "

comm_callback *cb = NULL;

void comm_init(comm_callback *callbacks) {
    cb = callbacks;
}

void comm_handle(Tuple *data) {
    uint32_t key = data->key;
    DEBUG(CM "Key: %d", key);

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
            TRACE(CM "SLope icon value");
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
        default:
            WARNING(CM "id %ld not handled by communications framework", key);
            break;
    }
}

            /* case CGM_TREND_BEGIN_NEW_KEY: */
            /*     TRACE("New Trend data begin"); */
			/* 	expected_trend_buffer_length = data->value->uint16; */
			/* 	LOG("TREND_BEGIN; About to receive Trend data of %i size.", expected_trend_buffer_length); */
            /*     t_config.bgl.size = expected_trend_buffer_length; */
            /*     break; */
            /* case CGM_TREND_DATA_NEW_KEY: */
            /*     TRACE("New Trend data blob of size %d", data->length >> 1); */
            /*     data16 = (int16_t *) data->value->data; */
            /*     for (int i = 0; i < data->length >> 1; i++) { */
            /*         #<{(| TRACE("TREND DATA: %d", *data16); |)}># */
            /*         t_config.bgl.values[t_config.bgl.index % (t_config.bgl.size)] = *data16++; */
            /*         t_config.bgl.index++; */
            /*         t_config.bgl.index = t_config.bgl.index % (t_config.bgl.size); */
            /*     } */
            /*     #<{(| if (data->length < 100 && t_config.bgl.initialized == 0) t_config.bgl.initialized = 1; |)}># */
            /*     break; */
            /* case CGM_TREND_END_NEW_KEY: */
            /*     TRACE("New Trend data end"); */
            /*     t_config.bgl.initialized = 1; */
            /*     // draw trend */
            /*     trend_draw(); */
            /*     break; */
            /* case CGM_TREND_UPDATE_NEW_KEY: */
            /*     TRACE("New Trend data update: %hd", data->value->int16); */
            /*     data16 = (int16_t *) data->value->data; */
            /*     t_config.bgl.values[t_config.bgl.index % (t_config.bgl.size - 1)] = *data16; */
            /*     t_config.bgl.index++; */
            /*     t_config.bgl.index = t_config.bgl.index % (t_config.bgl.size - 1); */
            /*     trend_draw(); */
            /*     break; */
            /*  */
