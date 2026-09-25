#ifndef __UI_OG_H__
#define __UI_OG_H__

#include <pebble.h>
#include "../xdrip.h"
#include <stdint.h>

#include "../api/communication.h"
#include "../api/callbacks.h"
#include "../api/settings.h"


#if defined(PBL_PLATFORM_EMERY) || defined(PBL_PLATFORM_GABBRO)
#define PHONE_ICON "\U0001F4F1"
#define STEPS_ICON "\U0001F9B6"
#define HRM_ICON "\U0001F493" 
#define WATCH_BATTERY_ICON "\U0000231A"
#else
// emojis are not available on older versions
#define PHONE_ICON " B:"
#define STEPS_ICON "s"
#define HRM_ICON "h" 
#define WATCH_BATTERY_ICON " W:"
#endif

#define STATUS_TEXT_SIZE 12
void ui_og_init(AppState *value);
void ui_og_deinit(void);
#endif
