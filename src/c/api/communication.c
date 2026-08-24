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
            /* if (cb->bgl_timestamp != NULL) cb->bgl_timestamp(value->timestamp); */
            /* if (cb->bgl_value != NULL) cb->bgl_value(value->bgl); */
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
            comm_png_data pngdata = {
                .length = data->length,
                .data = data->value->data
            };
            if (cb->png != NULL) cb->png(pngdata);
            break;
        case FRAMEWORK_SENSOR_TIME_LEFT:
            TRACE(CM "Sensor expiry time left");
            if (cb->sensor_time_left != NULL) cb->sensor_time_left(data->value->uint32);
            break;
        case FRAMEWORK_BWP_VALUE:
            TRACE(CM "Bolus wizard previes value");
            if (cb->bwp_value != NULL) cb->bwp_value(data->value->uint32);
            break;
        case FRAMEWORK_MESSAGE:
            TRACE(CM "Message received");
            // avoid malformed cstrings crashing the watch via OOB
            if (data->length - 1 != data->value->data[0]) {
                ERROR("Invalid message received");
            } else {
                if (cb->message != NULL) cb->message((comm_message *) data->value->cstring);
            }
            break;
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
