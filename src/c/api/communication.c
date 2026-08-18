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
            if (cb->high_limit) cb->high_limit(data->value->uint16);
            break;
        case FRAMEWORK_LOWLIMIT:
            TRACE(CM "Low  limit");
            if (cb->low_limit) cb->low_limit(data->value->uint16);
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
        default:
            WARNING(CM "id %ld not handled by communications framework", key);
            break;
    }
}
