#ifndef __UX_INSULIN_H__
#define __UX_INSULIN_H__
#include <pebble.h>
#include "../api/settings.h"
#ifndef PBL_TOUCH
#define insulin_display_init(...)
#define insulin_display_deinit(...)
#else
void insulin_display_init(AppState *val, Layer *root, TouchServiceHandler handoff);
void insulin_display_deinit(void);
#endif
#endif
