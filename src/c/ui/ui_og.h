#ifndef __UI_OG_H__
#define __UI_OG_H__

#include <pebble.h>
#include "../xdrip.h"
#include <stdint.h>

#include "../api/communication.h"
#include "../api/callbacks.h"
#include "../api/settings.h"

void ui_og_init(AppState *value);
void ui_og_deinit(void);
#endif
