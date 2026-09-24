#include <pebble.h>
#include "../xdrip.h"
#include <stdint.h>
#include "../debug.h"
#include "../api/settings.h"
#include "../api/trend.h"
#include "ui_og.h"
#ifdef ENABLE_TOUCH
#include "ui_insulin.h"
#endif


#ifdef ENABLE_COMM_FRAMEWORK
#include "../api/communication.h"
#endif
// UI timers
AppTimer *hr_draw_timer = NULL;
/**
 * Message timeout indicator, this is separate from the other timers to allow users 
 * to control the update rate.
 */
static AppTimer *message_tick_timer = NULL;

// Strings
static char last_bg[6];
static char current_bg_delta[14];
static char formatted_cgm_timeago[12];
static char time_watch_format[10] = TIME_24H_FORMAT;
static char time_watch_text[] = "00:00:00";
static char date_app_text[] = "Wed 13 Jan";
static char message_layer_text[13];
static char s_hrm_buffer[16];
static GFont time_font;
static char message_layer_text[13];
static GFont time_font_small;
static GFont time_font_normal;
static GFont bg_value_font;

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
BitmapLayer *bg_trend_layer_draw = NULL;
BitmapLayer *bg_trend_layer_png = NULL;
BitmapLayer *upper_face_layer = NULL;
BitmapLayer *lower_face_layer = NULL;

GBitmap *icon_bitmap = NULL;
GBitmap *appicon_bitmap = NULL;
GBitmap *specialvalue_bitmap = NULL;
GBitmap *bg_trend_bitmap = NULL;

#ifdef PBL_HEALTH
// --- Health: report current heart rate / step total back to the phone.
// The phone (SET_COLLECT_HEALTH) turns this on. Values go out as a standalone
// AppMessage a couple of seconds after xDrip pushes CGM data - xDrip's process
// is awake then, so its broadcast receiver actually gets the reply. This is
// deliberately not tied to send_cmd_cgm: under the framework xDrip pushes data
// when it has it and the watch's heartbeat may never run.
static HealthValue health_hr = 0;
static HealthValue health_steps = 0;
static AppTimer *health_send_timer = NULL;
#endif


/**
 * Static functions
 */
#ifdef PBL_HEALTH
/* static void health_poll(void); */
/* static void health_send_values(void *data); */
/* static void health_schedule_send(void); */
#endif

void set_message(char *message, size_t length);
void set_icon(uint8_t icon);
void set_delta(char *message, size_t length);
void set_delta_colour(GColor colour);
void set_cgmtime(char *message, size_t length);

// update functions
void update_message_timeout(uint32_t timeout);
GRect ui_og_trend_bounds(void);

// draw functions
static void load_bg(void);
static void load_icon(void);
static void load_cgmtime(void);
static void load_bg_delta(void);
static void load_battlevel_phone(void);

dirty_markers dirty = {
	.delta = 1,
	.need_cgm = 1,
	.step_count = 0,
	.hbm = 1,
    .sensor_info = 1
};

static AppState *state;

/**
 * Static helper functions
 */


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

/**************************************************************************
 * UI helper functions                                                    *
 **************************************************************************/
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

/**************************************************************************
 * Health functions                                                       *
 **************************************************************************/

// update_health_metric_displays - Updates the bottom left and right metrics displays if they are displaying health metrics
#ifndef PBL_HEALTH
#define update_health_metric_displays()
#else
void update_health_metric_displays() {
	static char step_count_text[9];
	int step_count;

	// If there are no health metrics to display, do nothing and return.
	if(state->left_text_field != METRIC_STEPS && state->right_text_field != METRIC_STEPS && state->left_text_field != METRIC_HEARTRATE && state->right_text_field != METRIC_HEARTRATE) return;

    if(state->left_text_field == METRIC_STEPS || state->right_text_field == METRIC_STEPS) {
        HealthMetric metric = HealthMetricStepCount;
        time_t start = time_start_of_today();
        time_t end = time(NULL);

        // Check the metric has data available for today
        HealthServiceAccessibilityMask mask = health_service_metric_accessible(metric, start, end);

        if(mask & HealthServiceAccessibilityMaskAvailable) {
            // Data is available!
            step_count = health_service_sum_today(metric);
            if (step_count != state->step_count)  {
                state->dirty.step_count = 1;
                state->step_count = step_count;
                LOG("Steps today: %d", step_count);
                snprintf(step_count_text,8, "%i s", step_count);
            }
        } else {
            // No data recorded yet today
            LOG("Data unavailable!");
        }
        if(state->left_text_field == METRIC_STEPS && state->dirty.step_count) {
            text_layer_set_text(bottom_left_text_layer, step_count_text);
            state->dirty.step_count = 0;
        }
        if(state->right_text_field == METRIC_STEPS && state->dirty.step_count) {
            text_layer_set_text(bottom_right_text_layer, step_count_text);
            state->dirty.step_count = 0;
        }
    }	
#if defined(PBL_PLATFORM_EMERY) || defined(PBL_PLATFORM_FLINT)
    if(state->left_text_field == METRIC_HEARTRATE || state->right_text_field == METRIC_HEARTRATE) {
        HealthServiceAccessibilityMask hr = health_service_metric_accessible(HealthMetricHeartRateBPM, time(NULL), time(NULL));
        HealthValue val = health_service_peek_current_value(HealthMetricHeartRateBPM);
        LOG("Heart Rate data is \"%lu\"", (uint32_t)val);
        if (hr & HealthServiceAccessibilityMaskAvailable || (val != 0 && val != state->hbm)) {
            // value can either be changed or new available, check if changed then update (e.g. initial condition) 
            if(val > 0 && val != state->hbm) {
                // Display HRM value
                state->hbm = val;
                state->dirty.hbm = 1;
                snprintf(s_hrm_buffer, sizeof(s_hrm_buffer), "%lu \U0001F493", (uint32_t)val);
            }
        } else if (state->hbm == 0) {
            state->dirty.hbm = 1;
            snprintf(s_hrm_buffer, sizeof(s_hrm_buffer), "Wait.. \U0001F493");
        }

        if (state->dirty.hbm && (hr_draw_timer == NULL || !app_timer_reschedule(hr_draw_timer, 1000))) {
            // TODO fix hr_draw_timer = app_timer_register(1000, hr_draw_callback, NULL);
        }
    }
#endif
}
#endif


/**
 * Sensor info drawing into the two state fields
 */
void update_sensor_info_displays(void) {
    if ((state->left_text_field == METRIC_SENSOR_EXPIRY || state->right_text_field == METRIC_SENSOR_EXPIRY) && state->dirty.sensor_info) {
        static char sensor_info_text[10] = { 0xF0, 0x9F, 0x8C, 0x99 }; // crecent moon unicode U+1F319
        // convert to days/hours/minutes
        int32_t remaining = state->sensor_end_time - time(NULL);
        // we ignore truncation warnings since the time left cannot be more than two characters
        // change sensor text length once sensors can last > 99 days
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
        if (remaining > 86400) snprintf(sensor_info_text + 4, sizeof(sensor_info_text) - 4, "%2ld d", remaining / 86400); 
        else if ((remaining % 86400) > 3600) snprintf(sensor_info_text + 4, sizeof(sensor_info_text) - 4, "%2ld h", (remaining % 86400) / 3600); 
        else if ((remaining % 60) > 0) snprintf(sensor_info_text + 4, sizeof(sensor_info_text) - 4, "%2ld m", (remaining % 3600) / 60);
        else snprintf(sensor_info_text, sizeof(sensor_info_text), "exp");
#pragma GCC diagnostic pop
        text_layer_set_text(state->left_text_field == METRIC_SENSOR_EXPIRY ? bottom_left_text_layer : bottom_right_text_layer, sensor_info_text);
    }
}


#ifdef ENABLE_TOUCH
#define TOUCH_STOP 0
#define TOUCH_START_DOWN 1
#define TOUCH_START_UP 2

#define TOUCH_REGION_TOP 1
#define TOUCH_REGION_BOTTOM 2

static int touch_state = TOUCH_STOP;
static int touch_value = 0;
static int32_t touch_time = 0;
AppTimer *touch_timer = NULL;
static int touch_region = TOUCH_REGION_TOP;

void touch_timer_handler(void *context) {
    touch_value = 0;
    touch_state = TOUCH_STOP;
    touch_timer = NULL;
}

#define TOUCH_TIMEOUT 1500
#define TOUCH_TICKS_REQUIRED 5

void touch_handler(const TouchEvent *event, void *context) {
    switch(event->type) {
        case TouchEvent_Touchdown:
            TRACE("DOWN : %dx%d", event->x, event->y);
            switch (touch_state) {
                case TOUCH_STOP:
                    touch_region = event->y > PBL_DISPLAY_HEIGHT / 2 ? TOUCH_REGION_BOTTOM : TOUCH_REGION_TOP;
                    // fall-through
                case TOUCH_START_UP:
                    touch_state = TOUCH_START_DOWN;
                    touch_time = time(NULL);
                    if (touch_timer == NULL || !app_timer_reschedule(touch_timer, TOUCH_TIMEOUT)) {
                        touch_timer = app_timer_register(TOUCH_TIMEOUT, touch_timer_handler, NULL);
                    }
                    break;
                case TOUCH_START_DOWN: // fall-through
                default:
                    break;
            }
            break;
        case TouchEvent_Liftoff:
            TRACE("UP : %dx%d", event->x, event->y);
            TRACE("DOWN : %dx%d", event->x, event->y);
            switch (touch_state) {
                case TOUCH_START_DOWN:
                    touch_state = TOUCH_START_UP;
                    if (time(NULL) - touch_time < 2) {
                        touch_value++;
                    }
                    touch_time = time(NULL);
                    if (touch_timer == NULL || !app_timer_reschedule(touch_timer, TOUCH_TIMEOUT)) {
                        touch_timer = app_timer_register(TOUCH_TIMEOUT, touch_timer_handler, NULL);
                    }
                    break;
                case TOUCH_STOP: // fall-through
                case TOUCH_START_UP: // fall-through
                default:
                    break;
            }
            break;
        case TouchEvent_PositionUpdate:
            LOG("POSITION: %dx%d", event->x, event->y);
            // don't care
            break;
        default:
            TRACE("Unknown");
            break;
    }
    LOG("Val: %d", touch_value);
    if (touch_value >= TOUCH_TICKS_REQUIRED) {
        LOG("Success! %d", touch_region);
        touch_value = 0;
        // launch insuling thing
        insulin_display_init(state, window_get_root_layer(window_cgm), touch_handler);
    }
}
#endif

/**
 * Ticks
 */
void seconds_tick(struct tm* tick_time_cgm, TimeUnits units_changed) {
	// VARIABLES
	size_t tick_return_cgm = 0;
	if (SECOND_UNIT && state->enable_seconds)
	{
		tick_return_cgm = strftime(time_watch_text, TIME_TEXTBUFF_SIZE, time_watch_format, tick_time_cgm);
		if (tick_return_cgm != 0)
		{
			text_layer_set_text(time_watch_layer, time_watch_text);
		}
		INFO("handle_second_tick_cgm: display_seconds = %i, time_watch_text = %s, time_watch_format = %s", state->enable_seconds, time_watch_text, time_watch_format);
	}
}

void minutes_tick(struct tm *tick_time_cgm, TimeUnits units_changed) {

	// VARIABLES
	size_t tick_return_cgm = 0;

	if (units_changed & MINUTE_UNIT)
	{
		LOG("handle_minute_tick_cgm: tick");
		tick_return_cgm = strftime(time_watch_text, TIME_TEXTBUFF_SIZE, time_watch_format, tick_time_cgm);
#ifdef PBL_HEALTH
		// keep health_hr / health_steps current; the send itself is driven off
		// an incoming CGM push (health_schedule_send), not this tick
		// TODO FIX health_poll();
#endif
        update_sensor_info_displays(); 
	}

	if (tick_return_cgm != 0)
	{
		text_layer_set_text(time_watch_layer, time_watch_text);
	}

	if (units_changed & DAY_UNIT)
	{
		INFO("handle_minute_tick_cgm: Day changed");
		tick_return_cgm = strftime(date_app_text, DATE_TEXTBUFF_SIZE, "%a %d %b", tick_time_cgm);
		if (tick_return_cgm != 0)
		{
			text_layer_set_text(date_app_layer, date_app_text);
		}
	}

	// We wake up every minute anyway and the resolution of all display time items
	// is 1m except for the clock
	load_cgmtime();
	load_bg_delta();

}

// message/delta tick layer
void handle_message_tick(void *data) 
{
	INFO("handle_message_tick: Handling alert tick, display_message is %i", state.show_message);
	if (state->show_message) update_message_timeout(state->message_timeout);
}
/** 
 * Callbacks
 */

void update_battery_state(void) {
	static char watch_battlevel_percent[11]; // extended for unicode support
#ifdef PBL_COLOR 
	#ifdef PBL_ROUND
	snprintf(watch_battlevel_percent, sizeof(watch_battlevel_percent), "%i%% ", state->battery_level);
	#else
#if defined(PBL_PLATFORM_EMERY) || defined(PBL_PLATFORM_GABBRO)
	snprintf(watch_battlevel_percent, sizeof(watch_battlevel_percent), "\U0001F50B %i%% ", state->battery_level);
#else
	snprintf(watch_battlevel_percent, sizeof(watch_battlevel_percent), "W:%i%% ", state->battery_level);
#endif
	#endif
#else
	snprintf(watch_battlevel_percent, sizeof(watch_battlevel_percent), "W:%i%%", state->battery_level);
#endif
	LOG(" update_battery_state: watch_battlevel_percent: %s", watch_battlevel_percent);
	LOG(" update_battery_state: BackLightOnCharge: %u", state->backlight_on_charge);
	if(state->backlight_on_charge)
	{
		if(state->battery_is_charging)
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
			
	if((state->left_text_field == METRIC_WATCHBATT || state->right_text_field == METRIC_WATCHBATT) && state->battery_is_charging)
	{
		LOG("Charging.  BacklightOnCharge:%u", state->backlight_on_charge);
		if(state->left_text_field == METRIC_WATCHBATT) {
#ifdef PBL_COLOR
			TRACE("COLOR DETECTED");
			text_layer_set_text_color(bottom_left_text_layer, state->background_colour);
			text_layer_set_background_color(bottom_left_text_layer, GColorGreen);
#else
			TRACE("BW DETECTED");
			text_layer_set_text_color(bottom_left_text_layer, state->background_colour);
			text_layer_set_background_color(bottom_left_text_layer, state->foreground_colour);
#endif
		}
		if(state->right_text_field == METRIC_WATCHBATT) {
#ifdef PBL_COLOR
			TRACE("COLOR DETECTED");
			text_layer_set_text_color(bottom_right_text_layer, state->background_colour);
			text_layer_set_background_color(bottom_right_text_layer, GColorGreen);
#else
			TRACE("BW DETECTED");
			text_layer_set_text_color(bottom_right_text_layer, state->background_colour);
			text_layer_set_background_color(bottom_right_text_layer, state->foreground_colour);
#endif
		}
	}
	else if(state->left_text_field == METRIC_WATCHBATT || state->right_text_field == METRIC_WATCHBATT)
	{
		LOG("update_battery_state: Not Charging.  BacklightOnCharge:%u", state->backlight_on_charge);
#ifdef PBL_COLOR
		TRACE("update_battery_state: COLOR DETECTED");
		if(state->battery_level > 40)
		{
			TRACE("update_battery_state: BATTERY > 40");
			if(state->left_text_field == METRIC_WATCHBATT) {
				LOG("update_battery_state: >40%% bottom_left_text_layer: GColorGreen");
				text_layer_set_text_color(bottom_left_text_layer, state->foreground_colour);
			}
			if(state->right_text_field == METRIC_WATCHBATT) {
				LOG("update_battery_state: >40%% bottom_right_text_layer: GColorGreen");
				text_layer_set_text_color(bottom_right_text_layer, state->foreground_colour);
			}
		}
		else if (state->battery_level > 20)
		{
			TRACE("update_battery_state: BATTERY > 20");
			if(state->left_text_field == METRIC_WATCHBATT) {
				LOG("update_battery_state: >20%% bottom_left_text_layer: GColorYellow");
				text_layer_set_text_color(bottom_left_text_layer, GColorYellow);
			}
			if(state->right_text_field == METRIC_WATCHBATT) {
				LOG("update_battery_state: >20%% bottom_right_text_layer: GColorYellow");
				text_layer_set_text_color(bottom_right_text_layer, GColorYellow);
			}
		}
		else
		{
			TRACE("update_battery_state: BATTERY <= 20");
			if(state->left_text_field == METRIC_WATCHBATT) {
				LOG("update_battery_state: <=20%% bottom_left_text_layer: GColorRed");
				text_layer_set_text_color(bottom_left_text_layer, GColorRed);
			}
			if(state->right_text_field == METRIC_WATCHBATT) {
				LOG("update_battery_state: <=20%% bottom_right_text_layer: GColorRed");
				text_layer_set_text_color(bottom_right_text_layer, GColorRed);
			}
		}
		if(state->left_text_field == METRIC_WATCHBATT) {
			LOG("update_battery_state: Normal, bottom_left_text_layer: GColorClear");
			text_layer_set_background_color(bottom_left_text_layer, GColorClear);
		}
		if(state->right_text_field == METRIC_WATCHBATT) {
			LOG("update_battery_state: Normal, bottom_right_text_layer: GColorClear");
			text_layer_set_background_color(bottom_right_text_layer, GColorClear);
		}
#else
		TRACE("update_battery_state: BW DETECTED");
		if(state->left_text_field == METRIC_WATCHBATT) {
			text_layer_set_text_color(bottom_left_text_layer, GColorWhite);
			text_layer_set_background_color(bottom_left_text_layer, GColorBlack);
		}
		if(state->right_text_field == METRIC_WATCHBATT) {
			text_layer_set_text_color(bottom_right_text_layer, GColorWhite);
			text_layer_set_background_color(bottom_right_text_layer, GColorBlack);
		}
#endif
	}
	if(state->left_text_field == METRIC_WATCHBATT) {
		text_layer_set_text(bottom_left_text_layer, watch_battlevel_percent);
	}
	if(state->right_text_field == METRIC_WATCHBATT) {
		text_layer_set_text(bottom_right_text_layer, watch_battlevel_percent);
	}
}

/**
 * UX Callback functions
 */
/*
 * update_colours - called when state->foreground_colour or state->background_colour is changed.
 */
void update_colours(void)
{
#ifdef PBL_COLOR
	if(state->fields_same_colour) {
		bitmap_layer_set_background_color(upper_face_layer, state->background_colour);
		text_layer_set_text_color(delta_layer, state->foreground_colour);
		text_layer_set_text_color(message_layer, state->foreground_colour);
		text_layer_set_text_color(bg_layer, state->foreground_colour);
		text_layer_set_text_color(cgmtime_layer, state->foreground_colour);
	} else {
		bitmap_layer_set_background_color(upper_face_layer, state->foreground_colour);
		text_layer_set_text_color(delta_layer, state->background_colour);
		text_layer_set_text_color(message_layer, state->background_colour);
		text_layer_set_text_color(bg_layer, state->background_colour);
		text_layer_set_text_color(cgmtime_layer, state->background_colour);
	}
    text_layer_set_text_color(bottom_left_text_layer, state->foreground_colour);
    text_layer_set_text_color(bottom_right_text_layer, state->foreground_colour);
	bitmap_layer_set_background_color(lower_face_layer, state->background_colour);
	text_layer_set_text_color(time_watch_layer, state->foreground_colour);
	text_layer_set_text_color(date_app_layer, state->foreground_colour);
	text_layer_set_background_color(bottom_right_text_layer, GColorClear);
	text_layer_set_background_color(bottom_left_text_layer, GColorClear);
	// update the watch battery colours etc.
	update_battery_state();
#else
    if(state->fields_same_colour) {
        bitmap_layer_set_background_color(upper_face_layer, state->background_colour);
    } else {
        bitmap_layer_set_background_color(upper_face_layer, state->foreground_colour);
    }
#endif

    layer_set_hidden(bitmap_layer_get_layer(bg_trend_layer_png), !state->show_trend || !state->use_png);
    layer_set_hidden(bitmap_layer_get_layer(bg_trend_layer_draw), !state->show_trend || state->use_png);
}
// end update_colours 

void update_left_field(void) {
    LOG("Left field");
    update_colours(); // in case it's not visible
    if (state->left_text_field == METRIC_NONE) {
        text_layer_set_text(bottom_left_text_layer, "");
    } else if (state->left_text_field == METRIC_PHONEBATT) {
        text_layer_set_text(bottom_left_text_layer, "Wait..");
    } else {
#ifdef PBL_HEALTH
        update_health_metric_displays();
#endif
        update_battery_state();
    }
}

void update_trend(void) {
    LOG("Update trend?");
    if (!state->use_png) {
        trend_init(bitmap_layer_get_layer(bg_trend_layer_draw));
        layer_set_hidden(bitmap_layer_get_layer(bg_trend_layer_png), true);
    } else {
        trend_deinit();
        layer_set_hidden(bitmap_layer_get_layer(bg_trend_layer_png), false);
    }
    // reset
    state->dirty.need_cgm = 1;
    state->cgm_time = 0; // force update all
    reset_timer_callback_cgm(2);
}

void update_right_field(void) {
    LOG("Right field");
    update_colours(); // in case it's not visible
    if (state->right_text_field == METRIC_NONE) {
        text_layer_set_text(bottom_right_text_layer, "");
    } else if (state->right_text_field == METRIC_PHONEBATT) {
        text_layer_set_text(bottom_right_text_layer, "Wait..");
    } else {
#ifdef PBL_HEALTH
        update_health_metric_displays();
#endif
        update_battery_state();
    }
}

void update_timeago(void) {
    if(state->bold_timeago) {
        text_layer_set_font(cgmtime_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
    }
    else
    {
        text_layer_set_font(cgmtime_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28));
    }
}

void update_seconds_timer(bool sw) {
    if(state->enable_seconds)
    {
        if (sw) {
            // register second timer
            tick_timer_service_subscribe(SECOND_UNIT, &handle_second_tick_cgm);
            // resize time and date layer iff PT2
            if (HIGH_RES()) {
                layer_set_frame((Layer *) time_watch_layer, GRect(0, 121, 200, 60));
                layer_set_frame((Layer *) date_app_layer, GRect(0, 168, 200, 39));
            }
        }
        time_font = time_font_small;
    }
    else
    {
        if (sw) {
            // unsub seconds timer to save power
            tick_timer_service_unsubscribe();
            tick_timer_service_subscribe(MINUTE_UNIT, &handle_minute_tick_cgm);
            // reset layers

            if (HIGH_RES()) {
                layer_set_frame((Layer *) time_watch_layer, GRect(0, 111, 200, 60));
                layer_set_frame((Layer *) date_app_layer, GRect(0, 176, 200, 39));
            }
        }
        time_font = time_font_normal;
    }
    text_layer_set_font(time_watch_layer, time_font);
    if(clock_is_24h_style() == true)
    {
        if(state->enable_seconds)
        {
            snprintf(time_watch_format, sizeof(time_watch_format), "%s", TIME_24HS_FORMAT);
        }
        else
        {
            snprintf(time_watch_format, sizeof(time_watch_format), "%s", TIME_24H_FORMAT);
        }
    }
    else
    {
        if(state->enable_seconds)
        {
            snprintf(time_watch_format, sizeof(time_watch_format), "%s", TIME_12HS_FORMAT);
        }
        else
        {
            snprintf(time_watch_format, sizeof(time_watch_format), "%s", TIME_12H_FORMAT);
        }
    }
    draw_date_from_app();
}

void update_message_timeout(uint32_t timeout) {
    if (message_tick_timer != NULL && !app_timer_reschedule(message_tick_timer, timeout)) {
        message_tick_timer = app_timer_register(timeout, handle_message_tick, NULL);
    }
}

void update_collect_health(void) {
#ifdef PBL_HEALTH
#if defined(PBL_PLATFORM_EMERY) || defined(PBL_PLATFORM_DIORITE)
    // sample HR on a fixed cadence while collecting so we
    // have fresh values; ~10 min trades data rate for battery
    health_service_set_heart_rate_sample_period(state->collect_health ? 600 : 0);
#endif
    if (state->collect_health) {
        // TODO fix health_poll();
    } else {
        health_hr = 0;
        health_steps = 0;
        if (health_send_timer != NULL) {
            app_timer_cancel(health_send_timer);
            health_send_timer = NULL;
        }
    }
#endif
}

void update_message(void) {
	if(state->show_message)
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
}


/*
 * Comm framework callback functions
 */
// snprintf does not support float!
void comm_set_bgl_delta(comm_bgl_delta value) {
    char text[14];
	DEBUG("Delta units: undefined: %d mmol: %d display: %d value: %d hidden: %d", 
			value.undefined, value.is_mmol, value.display_units, value.value, value.hidden);
	if (value.expired) set_delta("Expired", sizeof("Expired"));
	else if (value.undefined) set_delta("???", sizeof("???"));
	else if (value.is_mmol && value.display_units) {
		mgdl_to_mmoll_str(value.value, text, sizeof(text), 1);
        set_delta(text, strlen(text));
	} else if (value.display_units && !value.is_mmol) {
		int l = snprintf(text, sizeof(text), "%hd mg/dL", value.value);
        set_delta(text, l);
	} else if (value.is_mmol) {
		mgdl_to_mmoll_str(value.value, text, sizeof(text), 0);
        set_delta(text, strlen(text));
	} else {
		int l = snprintf(text, sizeof(text), "%hd", (int16_t) value.value);
        set_delta(text, l);
	}
	state->show_delta = value.hidden ? false : true;
}

void comm_set_icon(comm_slopeval value) {
	DEBUG("set_icon: comm_slopeval = %d, set_icon = %d, icon visibility = %d", value, state->show_slope, layer_get_hidden(bitmap_layer_get_layer(icon_layer)));
	set_icon(value);
}

void comm_set_phone_battery(comm_phonebat value) {
	state->phone_battery_level = value;
	load_battlevel_phone();
}

void comm_set_bwp(comm_bwp_value value) {
	snprintf(current_bg_delta, sizeof(current_bg_delta), BWP_SYMBOL "%lu", value);
	state->dirty.delta = 1;
	load_bg_delta();
}

void comm_set_vibrate(comm_vibe value) {
	if (!state->bluetooth_alert) state->gl_cb.alert_handler(value);
}

void comm_set_bgl_timestamp(uint32_t timestamp) {
	TRACE("Set BGL Timestamp");
	if (!state->dirty.need_cgm || state->use_png) state->cgm_time = timestamp;
	reset_timer_callback_cgm((timestamp - time(NULL)) + (60 * 6));
	load_cgmtime();
}

void comm_set_bgl_value(comm_bgl_value value) {
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
void comm_set_bgl_data(comm_bgl_data *value) {
	TRACE("Set BGL Data");
	if (value->timestamp != state->cgm_time) {
		DEBUG("%d vs %d %d", value->timestamp, state->cgm_time, value->timestamp - state->cgm_time);
		comm_set_bgl_value(value->bgl); // always show bgl value
		if (value->timestamp - state->cgm_time > 360) {
			// we likely missed a value, set minutes timer to zero and wait for global udpate
			reset_timer_callback_cgm((value->timestamp - time(NULL)) + (60));
		} else if (!state->use_png && !state->dirty.need_cgm) trend_set_value(value);
		comm_set_bgl_timestamp(value->timestamp); // can be marked dirty, so might not update
#ifdef PBL_HEALTH
		/* health_schedule_send(); // xDrip is awake now - report HR/steps shortly */
        // TODO FIX with send scheduler
#endif
	} else {
		WARNING("Received same bgl value twice!");
	}

}

/**
 * Since all data is in flight and copied by the Bitmap creation we 
 * do not have to copy it
 */
void comm_set_png(comm_png_data *data) {
	TRACE("Setting PNG");
	if (data->hidden != state->show_trend) {
		state->show_trend = !data->hidden;
		layer_set_hidden(bitmap_layer_get_layer(bg_trend_layer_png), !state->show_trend);
		persist_write_bool(SET_SHOW_TREND, state->show_trend);
	}
	if (state->show_trend) {
		if(bg_trend_bitmap != NULL)
		{
			INFO("Destroying bg_trend_bitmap");
			gbitmap_destroy(bg_trend_bitmap);
			bg_trend_bitmap = NULL;
		}

		bg_trend_bitmap = gbitmap_create_from_png_data(data->data, data->length);
		if(bg_trend_bitmap != NULL)
		{
			LOG("bg_trend_bitmap created, setting to layer");
			bitmap_layer_set_bitmap(bg_trend_layer_png, bg_trend_bitmap);
		}
		else
		{
			WARNING("bg_trend_bitmap creation FAILED!");
		}
	}
#ifdef PBL_HEALTH
// TODO Fix	health_schedule_send(); // xDrip is awake now - report HR/steps shortly
#endif
	state->dirty.need_cgm = 0;
}

void comm_set_bgl_series(comm_bgl_series *series) {
    state->dirty.need_cgm = 0;
    trend_set_series(series);
    if (series->hidden != state->show_trend) {
        state->show_trend = !series->hidden;
        layer_set_hidden(bitmap_layer_get_layer(bg_trend_layer_png), series->hidden || !state->use_png);
        trend_set_hidden(!state->show_trend && !state->use_png);
        persist_write_bool(SET_SHOW_TREND, state->show_trend);
    }
#ifdef PBL_HEALTH
	// TODO fix health_schedule_send(); // xDrip is awake now - report HR/steps shortly
#endif
}

void comm_set_message(comm_message message) {
    set_message(message.message, message.length);
}

void comm_set_sensor_info(comm_sensor_info *value) {
    // only update if need be, which it likely is since we received this message
    if (state->left_text_field == METRIC_SENSOR_EXPIRY || state->right_text_field == METRIC_SENSOR_EXPIRY) {
        // store state->sensor_end_time
        if (value->end != (uint32_t) state->sensor_end_time) {
            state->sensor_end_time = value->end;
            state->dirty.sensor_info = 1;
            update_sensor_info_displays();
        }
    }
}

/**
 * parsers
 */

void set_message(char *message, size_t length) {
	LOG("Setting message_layer visible");
	memcpy(
            message_layer_text,
            message, length > sizeof(message_layer_text) - 1 ? sizeof(message_layer_text) : length);
	text_layer_set_text(message_layer, message_layer_text);
	state->show_message = length > 0 ? true : false;
	layer_set_hidden((Layer *)message_layer, false); // show and mark dirty
#ifdef PBL_ROUND
	layer_set_hidden((Layer *)delta_layer, true);
#endif
	// hide after
    update_message_timeout(state->message_timeout);
}

void set_icon(uint8_t icon) {
    state->icon = icon;
	load_icon();
}

void set_delta(char *message, size_t length) {
    length = length < sizeof(current_bg_delta) ? length : sizeof(current_bg_delta) - 1;
    memcpy(current_bg_delta, message, length);
    current_bg_delta[length] = '\0';
	state->dirty.delta = 1;
	load_bg_delta();
}

void set_delta_colour(GColor colour) {
    text_layer_set_text_color(delta_layer, colour);
    load_bg_delta();
}

void set_cgmtime(char *message, size_t length) {
    length = length < sizeof(formatted_cgm_timeago) ? length : sizeof(formatted_cgm_timeago) - 1;
    memcpy(formatted_cgm_timeago, message, length);
    formatted_cgm_timeago[length] = '\0';
    text_layer_set_text(cgmtime_layer, formatted_cgm_timeago);
    load_cgmtime();
}

/**************************************************************************
 * Draw functions                                                         *
 **************************************************************************/
static void load_icon()
{
	TRACE("load_icon: Start");

	// check if special value set
	if (state->special_value_alert == false)
	{

		// no special value, set arrow
		// check for arrow direction, set proper arrow icon
		TRACE("load_icon: CURRENT ICON: %lu", state->icon);
		switch (state->icon) {
			case NO_ARROW:
			case NOTCOMPUTE:
			case OUTOFRANGE:
			{
				create_update_bitmap(&icon_bitmap,icon_layer, NONE_ARROW_ICON);
				state->double_up_down_alert = false;
			}
			break;

			case DOUBLEUP_ARROW:
			{
				create_update_bitmap(&icon_bitmap,icon_layer, UPUP_ICON);
				state->double_up_down_alert = false;
			}
			break;

			case SINGLEUP_ARROW:
			{
				create_update_bitmap(&icon_bitmap,icon_layer, UP_ICON);
				state->double_up_down_alert = false;
			}
			break;

			case UP45_ARROW:
			{
				create_update_bitmap(&icon_bitmap,icon_layer, UP45_ICON);
				state->double_up_down_alert = false;
			}
			break;

			case FLAT_ARROW:
			{
				create_update_bitmap(&icon_bitmap,icon_layer, FLAT_ICON);
				state->double_up_down_alert = false;
			}
			break;
			case DOWN45_ARROW:
			{
				create_update_bitmap(&icon_bitmap,icon_layer, DOWN45_ICON);
				state->double_up_down_alert = false;
			}
			break;
			case SINGLEDOWN_ARROW:
			{
				create_update_bitmap(&icon_bitmap,icon_layer, DOWN_ICON);
				state->double_up_down_alert = false;
			}
			break;
			case DOUBLEDOWN_ARROW:
			{
				create_update_bitmap(&icon_bitmap,icon_layer, DOWNDOWN_ICON);
				state->double_up_down_alert = true; // does nothing
				break;
			}
			case NO_ANTENNA:
			{
				create_update_bitmap(&icon_bitmap, icon_layer,  BROKEN_ANTENNA_ICON);
				break;
			}
			case NOT_CALIBRATED:
			{
				create_update_bitmap(&icon_bitmap, icon_layer, BLOOD_DROP_ICON);
				break;
			}
			case SENSOR_NOT_ACTIVE:
			{
				create_update_bitmap(&icon_bitmap, icon_layer, STOP_LIGHT_ICON);
				break;
			}
			case HOURGLASS:
			{
				create_update_bitmap(&icon_bitmap, icon_layer, HOURGLASS_ICON);
				break;
			}
			case QUESTIONMARK:
			{
				create_update_bitmap(&icon_bitmap, icon_layer, QUESTION_MARKS_ICON);
				break;
			}
			case SPECIAL_VALUE:
			{
				break;
			}
			default:
			{
				// check for special cases and set icon accordingly
				// check bluetooth
				state->bluetooth_is_connected = bluetooth_connection_service_peek();

				// check to see if we are in the loading screen
				if (!state->bluetooth_is_connected)
				{
					// Bluetooth is out; in the loading screen so set logo
					create_update_bitmap(&icon_bitmap,icon_layer, LOGO_ARROW_ICON);
				}
				else
				{
					// unexpected, set error icon
					create_update_bitmap(&icon_bitmap,icon_layer, ERR_ARROW_ICON);
				}
				state->double_up_down_alert = false;
			}
			break;
		}
	} // if state->special_value_alert == false
	else   // this is just for log when need it
	{
		TRACE("load_icon: DONE");
	} // else state->special_value_alert == true

	layer_set_hidden(bitmap_layer_get_layer(icon_layer), !state->show_slope);
} // end load_icon

static void load_bg()
{
	TRACE("load_bg: start");

	// CODE START

	// if special value set, erase anything in the icon field
	if (state->special_value_alert == true)
	{
		TRACE("load_bg: state->special_value_alert is true, setting icon blank.");
		create_update_bitmap(&specialvalue_bitmap,icon_layer, NONE_SPECVALUE_ICON);
	}

	// set special value alert to false no matter what
	state->special_value_alert = false;

	INFO("load_bg: last_bg: %s", last_bg);

	// check if bt is broken if > 10m no item
	if (time(NULL) - state->cgm_time > 600)
	{
		// check bluetooth
	 	state->bluetooth_is_connected = connection_service_peek_pebble_app_connection();
#ifdef TEST_MODE
		state->bluetooth_is_connected = true;
#endif
		if (!state->bluetooth_is_connected)
		{
			// Bluetooth is out; set BT message
			TRACE("load_bg: NO BT, SET NO BT MESSAGE");
			if (!TurnOff_NOBLUETOOTH_Msg)
			{
				WARNING("Bluetooth connection lost: app conn: %d, pkit: %d", connection_service_peek_pebble_app_connection(), connection_service_peek_pebblekit_connection());
				text_layer_set_text(delta_layer, "NO BLUETOOTH");
				// make sure we get the data we need
				state->dirty.need_cgm = 1;
				state->cgm_time = 0;
				reset_timer_callback_cgm(2);
			} // if turnoff nobluetooth msg
		}
	} 

	TRACE("load_bg: AFTER CREATE SPEC VALUE BITMAP");
	// always trigger since we have a new value or need to draw
	if (state->special_value_alert == false)
	{
		// we didn't find a special value, so set BG instead
		// arrow icon already set separately
		TRACE("load_bg: SET BG: %s ", last_bg);
		text_layer_set_text(bg_layer, last_bg);
	} // end bg checks (if special_value_bitmap)


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
	uint32_t cgm_timeago = 0, time_now = 0;
	int cgm_timeago_diff = 0;
	static char formatted_cgm_timeago[10];
	static char cgm_label_buffer[6];

	// CODE START
#ifdef TEST_MODE
	state->cgm_time = time(NULL);
#endif

	// initialize label buffer
	strncpy(cgm_label_buffer, "", LABEL_BUFFER_SIZE);

	if (state->cgm_time == 0)
	{
		// Init code or error code; set text layer & icon to empty value
		TRACE("load_cgmtime, CGM TIME AGO INIT OR ERROR CODE: %s", cgm_label_buffer);
		text_layer_set_text(cgmtime_layer, "");
	}
	else
	{
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
		LOG("load_cgmtime:  time_now: %lu, state->cgm_time: %lu", time_now, state->cgm_time);

		//state->cgm_timeago = abs(time_now - state->cgm_time);
		cgm_timeago = (time_now - state->cgm_time);

		TRACE("load_cgmtime: cgm_label_buffer: %s, state->cgm_timeago\"%lu\"", cgm_label_buffer);

		if (cgm_timeago < MINUTEAGO)
		{
			cgm_timeago_diff = 0;
			strncpy (formatted_cgm_timeago, "now", TIMEAGO_BUFFER_SIZE);
		}
		else if (cgm_timeago < HOURAGO)
		{
			cgm_timeago_diff = (cgm_timeago / MINUTEAGO);
			snprintf(formatted_cgm_timeago, TIMEAGO_BUFFER_SIZE, "%i", cgm_timeago_diff);
			strncpy(cgm_label_buffer, "m", LABEL_BUFFER_SIZE);
			strcat(formatted_cgm_timeago, cgm_label_buffer);
		}
		else if (cgm_timeago < DAYAGO)
		{
			cgm_timeago_diff = (cgm_timeago / HOURAGO);
			snprintf(formatted_cgm_timeago, TIMEAGO_BUFFER_SIZE, "%i", cgm_timeago_diff);
			strncpy(cgm_label_buffer, "h", LABEL_BUFFER_SIZE);
			strcat(formatted_cgm_timeago, cgm_label_buffer);
		}
		else if (cgm_timeago < WEEKAGO)
		{
			cgm_timeago_diff = (cgm_timeago / DAYAGO);
			snprintf(formatted_cgm_timeago, TIMEAGO_BUFFER_SIZE, "%i", cgm_timeago_diff);
			strncpy(cgm_label_buffer, "d", LABEL_BUFFER_SIZE);
			strcat(formatted_cgm_timeago, cgm_label_buffer);
		}
		else
		{
			strncpy (formatted_cgm_timeago, "---", TIMEAGO_BUFFER_SIZE);
		}

		text_layer_set_text(cgmtime_layer, formatted_cgm_timeago);
	} // else init code

	LOG("load_cgmtime: cgmtime_layer is \"%s\"", text_layer_get_text(cgmtime_layer));
	TRACE("load_cgmtime: cgm_label_buffer: %s", cgm_label_buffer);
} // end load_cgmtime

static void load_bg_delta()
{
	LOG("load_bg_delta: current_bg_delta is \"%s\"", current_bg_delta);
 

	// VARIABLES
	// NOTE: buffers have to be static and hardcoded
	static char formatted_bg_delta[BGDELTA_FORMATTED_SIZE];

	// CODE START

	// check bluetooth connection
	state->bluetooth_is_connected = bluetooth_connection_service_peek();

	if (!state->bluetooth_is_connected)
	{
		// Bluetooth is out; BT message already set, so return
		return;
	}

	// check for CHECK PHONE condition, if true set message
	if ((state->phone_off_alert) && (!TurnOff_CHECKPHONE_Msg))
	{
		layer_set_hidden(text_layer_get_layer(delta_layer), false);
		text_layer_set_text(delta_layer, "CHECK PHONE");
		return;
	}

	// check for special messages; if no string, set no message
	if (strcmp(current_bg_delta, "") == 0)
	{
		layer_set_hidden(text_layer_get_layer(delta_layer), false);
		strncpy(formatted_bg_delta, "", MSGLAYER_BUFFER_SIZE);
		text_layer_set_text(delta_layer, formatted_bg_delta);
		return;
	}


	// check if LOADING.., if true set message
	// put " " (space) in bg field so logo continues to show
	if (strcmp(current_bg_delta, "LOAD") == 0)
	{
		LOG("load_bg_delta: Found \"LOAD\"");

		layer_set_hidden(text_layer_get_layer(delta_layer), false);
		strncpy(formatted_bg_delta, "LOADING...", MSGLAYER_BUFFER_SIZE);
		text_layer_set_text(delta_layer, formatted_bg_delta);
		text_layer_set_text(bg_layer, " ");
		create_update_bitmap(&icon_bitmap,icon_layer, LOGO_SPECVALUE_ICON);
		state->special_value_alert = FALSE;
		return;
	}

	//check for "--" indicating an indeterminate delta.  Display it.
	if (strcmp(current_bg_delta, "???") == 0)
	{
		layer_set_hidden(text_layer_get_layer(delta_layer), false);
		strncpy(formatted_bg_delta, current_bg_delta, BGDELTA_FORMATTED_SIZE);
		text_layer_set_text(delta_layer, formatted_bg_delta);
		return;
	}

	//check for "ERR" indicating an indeterminate delta.  Display it.
	if (strcmp(current_bg_delta, "ERR") == 0)
	{
		layer_set_hidden(text_layer_get_layer(delta_layer), false);
		strncpy(formatted_bg_delta, current_bg_delta, BGDELTA_FORMATTED_SIZE);
		text_layer_set_text(delta_layer, formatted_bg_delta);
		return;
	}

	// Bluetooth is good, Phone is good, CGM connection is good, no special message
	// set delta BG message

	strncpy(formatted_bg_delta, current_bg_delta, BGDELTA_FORMATTED_SIZE);
	LOG("load_bg_delta: All good. Setting \"%s\", show_delta = %d", formatted_bg_delta, state->show_delta);

	if (layer_get_hidden(text_layer_get_layer(delta_layer)) == state->show_delta) {
		layer_set_hidden(text_layer_get_layer(delta_layer), !state->show_delta);
	}

	if (!state->dirty.delta) {

		TRACE("Delta not dirty, not changing");
		return;
	}

	text_layer_set_text(delta_layer, formatted_bg_delta);
#ifdef PBL_COLOR
	if(state->fields_same_colour) {
		text_layer_set_text_color(delta_layer,state->foreground_colour);
	} else {
		text_layer_set_text_color(delta_layer,state->background_colour);
	}

	state->dirty.delta = 0; // all next escapes are dirty
#endif
	LOG("load_bg_delta: delta_layer is \"%s\"", text_layer_get_text(delta_layer));

} // end load_bg_delta

static void load_battlevel_phone()
{
	TRACE("load_battlevel: START");

	// CONSTANTS
#if defined(PBL_PLATFORM_EMERY) || defined(PBL_PLATFORM_GABBRO)
#define PHONE_EMOJI "\U0001F4F1"
#else
    // phone emoji is not available on older versions
#define PHONE_EMOJI " B:"
#endif


	// VARIABLES
	// NOTE: buffers have to be static and hardcoded
	uint32_t current_battlevel = 0;
	static char battlevel_percent[BATTLEVEL_FORMATTED_SIZE];

	// CODE START
	//Deterime if a metric text layer is configured for phone battery
	if(state->left_text_field != METRIC_PHONEBATT && state->right_text_field == METRIC_PHONEBATT) {
		LOG("load_battlevel: No phone battery displays, exiting.");
		return;
	}
	LOG("load_battlevel: state->phone_battery_level: %lu", state->phone_battery_level);
	if (state->phone_battery_level == 255)
	{
		// Init code or no battery, can't do battery; set text layer & icon to empty value
		INFO("load_battlevel: NO BATTERY");
		if(state->left_text_field == METRIC_PHONEBATT) text_layer_set_text(bottom_left_text_layer, "");
		if(state->right_text_field == METRIC_PHONEBATT) text_layer_set_text(bottom_right_text_layer, "");
		state->battery_low_alert = false;
		return;
	}

	if (state->phone_battery_level == 0)
	{
		// Zero battery level; set here, so if we get zero later we know we have an error instead
		INFO("load_battlevel: 0 value");
		if(state->left_text_field == METRIC_PHONEBATT) text_layer_set_text(bottom_left_text_layer, PHONE_EMOJI "0%");
		if(state->right_text_field == METRIC_PHONEBATT) text_layer_set_text(bottom_right_text_layer, PHONE_EMOJI "0%");
		if (!state->battery_low_alert)
		{
			INFO("load_battlevel: 0 value, vibe");
			CALLBACK(state->gl_cb.alert_handler, LOWBATTERY_VIBE);
			state->battery_low_alert = true;
		}
		return;
	}

	if (current_battlevel == state->phone_battery_level) {
		TRACE("Battery level early exit to not mark layers dirty");
		return;
	}

	current_battlevel = state->phone_battery_level;

	INFO("load_battlevel: current_battlevel: %i", current_battlevel);

	if ((current_battlevel <= 0) || (current_battlevel > 100) || ((state->phone_battery_level > 100 && state->phone_battery_level != 255)))
	{
		// got a negative or out of bounds or error battery level
		INFO("load_battlevel: error");
		if(state->left_text_field == METRIC_PHONEBATT) text_layer_set_text(bottom_left_text_layer, PHONE_EMOJI "ERR");
		if(state->right_text_field == METRIC_PHONEBATT) text_layer_set_text(bottom_right_text_layer, PHONE_EMOJI "ERR");
		return;
	}

	// get current battery level and set battery level text with percent
#ifdef PBL_ROUND
	snprintf(battlevel_percent, BATTLEVEL_FORMATTED_SIZE, " %lu%%", current_battlevel);
#elif PBL_COLOR
	snprintf(battlevel_percent, BATTLEVEL_FORMATTED_SIZE, PHONE_EMOJI " %lu%%", current_battlevel);
#else
	snprintf(battlevel_percent, BATTLEVEL_FORMATTED_SIZE, "B:%lu%%", current_battlevel);
#endif
	LOG("load_battlevel: %s\%", battlevel_percent);
#ifndef PBL_ROUND
	if(state->left_text_field == METRIC_PHONEBATT) text_layer_set_text(bottom_left_text_layer, battlevel_percent);
	if(state->right_text_field == METRIC_PHONEBATT) text_layer_set_text(bottom_right_text_layer, battlevel_percent);
#endif
#ifdef PBL_COLOR
	// if neither bottom metric is battery indication, then return immediately and don't process the colours.
	if(state->left_text_field != METRIC_PHONEBATT && state->right_text_field != METRIC_PHONEBATT) {
		TRACE("load_battlevel: No watch battery displays, done");
		return;
	}
	if ( (current_battlevel > 0) && (current_battlevel <= 30) && (state->left_text_field == METRIC_PHONEBATT || state->right_text_field == METRIC_PHONEBATT) )
	{
		if(state->left_text_field == METRIC_PHONEBATT) {
			LOG("load_battlevel: Setting bottom_left_text_layer to GColorRed");
			text_layer_set_text_color(bottom_left_text_layer, GColorRed);
		}
		if(state->right_text_field == METRIC_PHONEBATT) {
			LOG("load_battlevel: Setting bottom_right_text_layer to GColorRed");
			text_layer_set_text_color(bottom_right_text_layer, GColorRed);
		}
		if (!state->battery_low_alert)
		{
			INFO("load_battlevel: low battery VIBRATE");
			CALLBACK(state->gl_cb.alert_handler, LOWBATTERY_VIBE);
			state->battery_low_alert = true;
		}
	}
	else if ( (current_battlevel > 30) && (current_battlevel <= 50) && (state->left_text_field == METRIC_PHONEBATT || state->right_text_field == METRIC_PHONEBATT) )
	{
		if(state->left_text_field == METRIC_PHONEBATT) {
			LOG("load_battlevel: Setting bottom_left_text_layer to GColorYellow");
			text_layer_set_text_color(bottom_left_text_layer, GColorYellow);
		}
		if(state->right_text_field == METRIC_PHONEBATT) {
			LOG("load_battlevel: Setting bottom_right_text_layer to GColorYellow");
			text_layer_set_text_color(bottom_right_text_layer, GColorYellow);
		}
	}
	else
	{
		if(state->left_text_field == METRIC_PHONEBATT) {
			LOG("load_battlevel: Setting bottom_left_text_layer to GColorGreen");
//			text_layer_set_text_color(bottom_left_text_layer, GColorGreen);
			text_layer_set_text_color(bottom_left_text_layer, state->foreground_colour);
		}
		if(state->right_text_field == METRIC_PHONEBATT) {
//			text_layer_set_text_color(bottom_right_text_layer, GColorGreen);
			text_layer_set_text_color(bottom_right_text_layer, state->foreground_colour);
			LOG("load_battlevel: Setting bottom_right_text_layer to GColorGreen");
		}
	}
#endif
	TRACE("load_battlevel: done");
} // end load_battlevel


void window_load_cgm(Window *window_cgm)
{
	TRACE("window_load_cgm: start");

	// VARIABLES
	Layer *window_layer_cgm = NULL;
//APLITE (CLASSIC)
#ifdef PBL_PLATFORM_APLITE
	LOG("window_load_cgm: Detected Aplite");
	//monochrome colours
	//static GColor state->foreground_colour;
	//static GColor state->background_colour;
	// face layer sizes
	upper_face_layer = bitmap_layer_create(GRect(0,0,144,89));
	lower_face_layer = bitmap_layer_create(GRect(0,89,144,165));
	// icon layer dimensions
	icon_layer = bitmap_layer_create(GRect(83, -7, 61, 61));
	// trend bitmap layer dimensions
	bg_trend_layer_png = bitmap_layer_create(GRect(0,24,144,64));
	bitmap_layer_set_compositing_mode(bg_trend_layer_png, GCompOpSet);
	bg_trend_layer_draw = bitmap_layer_create(GRect(0,24,144,64));
	bitmap_layer_set_compositing_mode(bg_trend_layer_draw, GCompOpSet);
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
	time_watch_layer = text_layer_create(GRect(0, 84 - 89, 143, 44));
	text_layer_set_text_alignment(time_watch_layer, GTextAlignmentCenter);
	// date layer dimenstions
	date_app_layer = text_layer_create(GRect(0, 124 - 89, 143, 29));
	text_layer_set_text_alignment(date_app_layer, GTextAlignmentCenter);
	// phone/bridge batter level layer diemnsions
	bottom_left_text_layer = text_layer_create(GRect(0, 148 - 89, 59, 18));
	text_layer_set_text_alignment(bottom_left_text_layer, GTextAlignmentLeft);
	//watch battery level layer dimensions
	bottom_right_text_layer = text_layer_create(GRect(81, 148 - 89, 59, 18));
	text_layer_set_text_alignment(bottom_right_text_layer, GTextAlignmentRight);

#endif

//BASALT (TIME, TIME STEEL)
#ifdef PBL_PLATFORM_BASALT
	LOG("window_load_cgm: Detected Basalt");
	//collour colours
	//static GColor8 state->foreground_colour;
	//static GColor8 state->background_colour;
	// upper and lower face dimensions
	upper_face_layer = bitmap_layer_create(GRect(0,0,144,84));
	lower_face_layer = bitmap_layer_create(GRect(0,84,144,165));
	// icon layer dimensions
	icon_layer = bitmap_layer_create(GRect(83, -9, 61, 61));
	bitmap_layer_set_compositing_mode(icon_layer, GCompOpSet);
	// trend bitmap layer dimensions and composition mode
	bg_trend_layer_png = bitmap_layer_create(GRect(0,0,144,84));
	bitmap_layer_set_compositing_mode(bg_trend_layer_png, GCompOpSet);
	bg_trend_layer_draw = bitmap_layer_create(GRect(0,0,144,84));
	bitmap_layer_set_compositing_mode(bg_trend_layer_draw, GCompOpSet);
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
	time_watch_layer = text_layer_create(GRect(0, 82 - 84, 143, 44));
	text_layer_set_text_alignment(time_watch_layer, GTextAlignmentCenter);
	// date layer dimenstions
	date_app_layer = text_layer_create(GRect(0, 124 - 84, 143, 29));
	text_layer_set_text_alignment(date_app_layer, GTextAlignmentCenter);
	// phone/bridge batter level layer diemnsions
	bottom_left_text_layer = text_layer_create(GRect(0, 148 - 84, 72, 20));
	layer_set_bounds((Layer *) bottom_left_text_layer, GRect(0, -1, 72, 20)); // fixes bounding box with latest sdk
	text_layer_set_text_alignment(bottom_left_text_layer, GTextAlignmentLeft);
	// watch battery level layer dimensions
	bottom_right_text_layer = text_layer_create(GRect(72, 148 - 84, 72, 20));
	layer_set_bounds((Layer *) bottom_right_text_layer, GRect(0, -1, 72, 20)); // fixes bounding box with latest sdk
	text_layer_set_text_alignment(bottom_right_text_layer, GTextAlignmentRight);

#endif

//CHALK (ROUND)
#ifdef PBL_PLATFORM_CHALK
	LOG("window_load_cgm: Detected Chalk");
	//collour colours
	//static GColor8 state->foreground_colour;
	//static GColor8 state->background_colour;
	// face layer sizes
	upper_face_layer = bitmap_layer_create(GRect(0,0,180,84));
	lower_face_layer = bitmap_layer_create(GRect(0,84,180,165));
	// icon layer size and composition mode
	icon_layer = bitmap_layer_create(GRect(119, 30, 61, 61));
	bitmap_layer_set_compositing_mode(icon_layer, GCompOpSet);
	// trend bitmap layer dimensions and composition mode
	bg_trend_layer_png = bitmap_layer_create(GRect(0,0,144,84));
	bitmap_layer_set_compositing_mode(bg_trend_layer_png, GCompOpSet);
	bg_trend_layer_draw = bitmap_layer_create(GRect(0,0,144,84));
	bitmap_layer_set_compositing_mode(bg_trend_layer_draw, GCompOpSet);
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
	time_watch_layer = text_layer_create(GRect(18, 82 - 84, 143, 44));
	text_layer_set_text_alignment(time_watch_layer, GTextAlignmentCenter);
	// date layer dimenstions
	date_app_layer = text_layer_create(GRect(18, 124 - 84, 143, 26));
	text_layer_set_text_alignment(date_app_layer, GTextAlignmentCenter);
	// phone/bridge batter level layer diemnsions
	bottom_left_text_layer = text_layer_create(GRect(48, 150 - 84, 1, 1));
	text_layer_set_text_alignment(bottom_left_text_layer, GTextAlignmentLeft);
	// watch battery level layer dimensions
	bottom_right_text_layer = text_layer_create(GRect(45, 150 - 84, 90, 18));
	text_layer_set_text_alignment(bottom_right_text_layer, GTextAlignmentCenter);

#endif

//DIORITE (PEBBLE 2)
#ifdef PBL_PLATFORM_DIORITE
	LOG("window_load_cgm: Detected Diorite");
	//monochrome colours
	//static GColor state->foreground_colour;
	//static GColor state->background_colour;
	upper_face_layer = bitmap_layer_create(GRect(0,0,144,88));
	lower_face_layer = bitmap_layer_create(GRect(0,89,144,165));
	// icon layer dimensions
	icon_layer = bitmap_layer_create(GRect(83, -7, 61, 61));
	// trend bitmap layer dimensions
	bg_trend_layer_png = bitmap_layer_create(GRect(0,24,144,64));
	bitmap_layer_set_compositing_mode(bg_trend_layer_png, GCompOpSet);
	bg_trend_layer_draw = bitmap_layer_create(GRect(0,24,144,64));
	bitmap_layer_set_compositing_mode(bg_trend_layer_draw, GCompOpSet);
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
	time_watch_layer = text_layer_create(GRect(0, 84 - 89, 143, 44));
	text_layer_set_text_alignment(time_watch_layer, GTextAlignmentCenter);
	// date layer dimenstions
	date_app_layer = text_layer_create(GRect(0, 124 - 89, 143, 29));
	text_layer_set_text_alignment(date_app_layer, GTextAlignmentCenter);
	// phone/bridge batter level layer diemnsions
	bottom_left_text_layer = text_layer_create(GRect(0, 148 - 89, 59, 18));
	text_layer_set_text_alignment(bottom_left_text_layer, GTextAlignmentLeft);
	// watch battery level layer dimensions
	bottom_right_text_layer = text_layer_create(GRect(81, 148 - 89, 59, 18));
	text_layer_set_text_alignment(bottom_right_text_layer, GTextAlignmentRight);

#endif

//EMERY (CORE TIME 2)
#ifdef PBL_PLATFORM_EMERY
	LOG("window_load_cgm: Detected Emery");
	//upper and lower face layer dimensions
	upper_face_layer = bitmap_layer_create(GRect(0,0,200,114));
	lower_face_layer = bitmap_layer_create(GRect(0,115,200,228));
	// icon layer diemnsions and composition mode.
	icon_layer = bitmap_layer_create(GRect(139, -9, 61, 61));
	bitmap_layer_set_compositing_mode(icon_layer, GCompOpSet);
	// trend bitmap layer dimensions and composition mode
	bg_trend_layer_png = bitmap_layer_create(GRect(0,0,200,114));
	bitmap_layer_set_compositing_mode(bg_trend_layer_png, GCompOpSet);
	bg_trend_layer_draw = bitmap_layer_create(GRect(0,0,200,114));
	bitmap_layer_set_compositing_mode(bg_trend_layer_draw, GCompOpSet);
	// delta layer dimensions
	delta_layer = text_layer_create(GRect(2, 78, 198, 50));
	text_layer_set_text_alignment(delta_layer, GTextAlignmentLeft);
	// message layer dimensions
	message_layer = text_layer_create(GRect(2, 49, 198, 50));
	text_layer_set_text_alignment(message_layer, GTextAlignmentCenter);
	// BG layer dimensions
	bg_layer = text_layer_create(GRect(0, -5, 144, 62));
	// cgmtime layer dimensions
	cgmtime_layer = text_layer_create(GRect(142, 78, 55, 32));
	text_layer_set_text_alignment(cgmtime_layer, GTextAlignmentRight);
	// time watch layer dimenssions
	if (state->enable_seconds) {
		time_watch_layer = text_layer_create(GRect(0, 121 - 115., 200, 60));
	} else {
		time_watch_layer = text_layer_create(GRect(0, 111 - 115, 200, 60));
	}
	text_layer_set_text_alignment(time_watch_layer, GTextAlignmentCenter);
	// date layer dimenstions
	if (state->enable_seconds) {
		date_app_layer = text_layer_create(GRect(0, 168 - 115, 200, 39));
	} else {
		date_app_layer = text_layer_create(GRect(0, 176 - 115, 200, 39));
	}
	text_layer_set_text_alignment(date_app_layer, GTextAlignmentCenter);
	// phone/bridge batter level layer diemnsions
	bottom_left_text_layer = text_layer_create(GRect(2, 203 - 115, 100, 24));
	text_layer_set_text_alignment(bottom_left_text_layer, GTextAlignmentLeft);
	// watch battery level layer dimensions
	bottom_right_text_layer = text_layer_create(GRect(98, 203 - 115, 100, 24));
	text_layer_set_text_alignment(bottom_right_text_layer, GTextAlignmentRight);

#endif

//FLINT (CORE DUO 2)
#ifdef PBL_PLATFORM_FLINT
	LOG("window_load_cgm: Detected Flint");
	//monochrome colours
	//static GColor state->foreground_colour;
	//static GColor state->background_colour;
	// upper and lower face layer dimensions
	upper_face_layer = bitmap_layer_create(GRect(0,0,144,88));
	lower_face_layer = bitmap_layer_create(GRect(0,89,144,165));
	// icon layer dimensions
	icon_layer = bitmap_layer_create(GRect(85, -7, 78, 51));
	// trend bitmap layer dimensions
	bg_trend_layer_png = bitmap_layer_create(GRect(0,24,144,64));
	bitmap_layer_set_compositing_mode(bg_trend_layer_png, GCompOpSet);
	bg_trend_layer_draw = bitmap_layer_create(GRect(0,24,144,64));
	bitmap_layer_set_compositing_mode(bg_trend_layer_draw, GCompOpSet);
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
	time_watch_layer = text_layer_create(GRect(0, 84 - 89, 143, 44));
	text_layer_set_text_alignment(time_watch_layer, GTextAlignmentCenter);
	// date layer dimenstions
	date_app_layer = text_layer_create(GRect(0, 124 - 89, 143, 29));
	text_layer_set_text_alignment(date_app_layer, GTextAlignmentCenter);
	// phone/bridge batter level layer diemnsions
	bottom_left_text_layer = text_layer_create(GRect(0, 148 - 89, 59, 18));
	text_layer_set_text_alignment(bottom_left_text_layer, GTextAlignmentLeft);
	// watch battery level layer dimensions
	bottom_right_text_layer = text_layer_create(GRect(81, 148 - 89, 59, 18));
	text_layer_set_text_alignment(bottom_right_text_layer, GTextAlignmentRight);

#endif

//GABBRO (CORE ROUND 2)
#ifdef PBL_PLATFORM_GABBRO
	// 260x260 (renumerate from original round is 180-180, everything *1.44
	//collour colours
	LOG("window_load_cgm: Detected GABBRO");
	//collour colours
	//static GColor8 state->foreground_colour;
	//static GColor8 state->background_colour;
	// face layer sizes
	upper_face_layer = bitmap_layer_create(GRect(0  ,   0, 260, 120));
	lower_face_layer = bitmap_layer_create(GRect(0   ,121, 260, 238));
	// icon layer size and composition mode
	icon_layer = bitmap_layer_create(GRect(173,  43, 112,  72));
	bitmap_layer_set_compositing_mode(icon_layer, GCompOpSet);
	// trend bitmap layer dimensions and composition mode
	bg_trend_layer_png = bitmap_layer_create(GRect(  0,   0, 260, 121));
	bitmap_layer_set_compositing_mode(bg_trend_layer_png, GCompOpSet);
	bg_trend_layer_draw = bitmap_layer_create(GRect(  0,   0, 260, 121));
	bitmap_layer_set_compositing_mode(bg_trend_layer_draw, GCompOpSet);
	// delta layer dimensions
	delta_layer = text_layer_create(GRect(  0,  52, 260,  72));
	text_layer_set_text_alignment(delta_layer, GTextAlignmentCenter);
	// message layer dimensions
	message_layer = text_layer_create(GRect(  0,  52, 260,  72));
	text_layer_set_text_alignment(message_layer, GTextAlignmentCenter);
	// BG layer dimensions
	bg_layer = text_layer_create(GRect(  0,  -7, 260,  76));
	text_layer_set_text_alignment(bg_layer, GTextAlignmentCenter);
	// cgmtime layer dimensions
	cgmtime_layer = text_layer_create(GRect(  7,  84,  58,  35));
	text_layer_set_text_alignment(cgmtime_layer, GTextAlignmentRight);
	// time watch layer dimenssions
	time_watch_layer = text_layer_create(GRect( 26, 118 - 121, 206,  64));
	text_layer_set_text_alignment(time_watch_layer, GTextAlignmentCenter);
	// date layer dimenstions
	date_app_layer = text_layer_create(GRect( 26, 178 - 121, 206,  38));
	text_layer_set_text_alignment(date_app_layer, GTextAlignmentCenter);
	// phone/bridge batter level layer diemnsions
	bottom_left_text_layer = text_layer_create(GRect( 69, 236 - 121,  130,  26));
	text_layer_set_text_alignment(bottom_left_text_layer, GTextAlignmentLeft);
	// watch battery level layer dimensions
	bottom_right_text_layer = text_layer_create(GRect( 65, 210 - 121,  130,  26));
	text_layer_set_text_alignment(bottom_right_text_layer, GTextAlignmentCenter);

#endif

	// CODE START

	window_layer_cgm = window_get_root_layer(window_cgm);
	// Platform Sepcific display objects

	if(state->fields_same_colour) {
		bitmap_layer_set_background_color(upper_face_layer, state->background_colour);
		text_layer_set_text_color(delta_layer, state->foreground_colour);
		text_layer_set_text_color(message_layer, state->foreground_colour);
		text_layer_set_text_color(bg_layer, state->foreground_colour);
		text_layer_set_text_color(cgmtime_layer, state->foreground_colour);
	} else {
		bitmap_layer_set_background_color(upper_face_layer, state->foreground_colour);
		text_layer_set_text_color(delta_layer, state->background_colour);
		text_layer_set_text_color(message_layer, state->background_colour);
		text_layer_set_text_color(bg_layer, state->background_colour);
		text_layer_set_text_color(cgmtime_layer, state->background_colour);
	}
	bitmap_layer_set_background_color(lower_face_layer, state->background_colour);
	bitmap_layer_set_alignment(icon_layer, GAlignTopLeft);
	bitmap_layer_set_background_color(icon_layer, GColorClear);
	text_layer_set_background_color(delta_layer, GColorClear);
	text_layer_set_background_color(message_layer, GColorClear);
	text_layer_set_font(message_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28));
	text_layer_set_text_alignment(message_layer, GTextAlignmentCenter);
	text_layer_set_background_color(bg_layer, GColorClear);
#if defined(PBL_PLATFORM_EMERY)
	bg_value_font=fonts_load_custom_font(resource_get_handle(RESOURCE_ID_GOTHAM_BOLD_BG_56));
#elif defined(PBL_PLATFORM_GABBRO)
	bg_value_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_GOTHAM_BOLD_BG_64));
#else
	bg_value_font = fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD);
#endif
	text_layer_set_font(bg_layer, bg_value_font);
	text_layer_set_background_color(cgmtime_layer, GColorClear);
	if(state->bold_timeago) {
		text_layer_set_font(cgmtime_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
	}
	else
	{
		text_layer_set_font(cgmtime_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28));
	}
	text_layer_set_text_color(time_watch_layer, state->foreground_colour);
	text_layer_set_background_color(time_watch_layer, GColorClear);
	text_layer_set_font(time_watch_layer,time_font);

	
	//Paint the backgrounds for upper and lower halves of the watch face.
	LOG("Creating Upper and Lower face panels");

	//create the bg_trend_layer
	INFO("Creating BG Trend Bitmap layer");


	layer_set_hidden(bitmap_layer_get_layer(bg_trend_layer_png), !state->use_png);
	layer_set_hidden(bitmap_layer_get_layer(bg_trend_layer_draw), state->use_png);

	// ARROW OR SPECIAL VALUE
	LOG("Creating Arrow Bitmap layer");

	// DELTA BG
	LOG("Creating Delta BG Text layer");
	text_layer_set_font(delta_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28));

	// MESSAGE
	LOG("Creating Message Text layer");
	snprintf(message_layer_text,sizeof(message_layer_text),"%s","");
	text_layer_set_text(message_layer, message_layer_text);
	layer_set_hidden((Layer *)message_layer, true);

	// BG
	LOG("Creating BG Text layer");

	// CGM TIME AGO READING
	LOG("Creating CGM Time Ago Bitmap layer");

	// CURRENT ACTUAL TIME FROM WATCH
	LOG("Creating Watch Time Text layer");

	// CURRENT ACTUAL DATE FROM APP
	LOG("Creating Watch Date Text layer");
	text_layer_set_text_color(date_app_layer, state->foreground_colour);
	text_layer_set_background_color(date_app_layer, GColorClear);
	text_layer_set_font(date_app_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));

	// Metric Layers
	// left metric layer
	LOG("Creating Left Metric Text layer");
	text_layer_set_text_color(bottom_left_text_layer, state->foreground_colour);
	text_layer_set_background_color(bottom_left_text_layer, GColorClear);
	text_layer_set_font(bottom_left_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));

	// right metric layer
	LOG("Creating Right Metric Text layer");
	text_layer_set_text_color(bottom_right_text_layer, state->foreground_colour);
	text_layer_set_background_color(bottom_right_text_layer, GColorClear);
	text_layer_set_font(bottom_right_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));

	// Layer definitions
	LOG("Setting Layer order");

	// bg's
	layer_add_child(window_layer_cgm, bitmap_layer_get_layer(upper_face_layer));
	layer_add_child(window_layer_cgm, bitmap_layer_get_layer(lower_face_layer));

	// trend layers
	layer_add_child(bitmap_layer_get_layer(upper_face_layer), bitmap_layer_get_layer(bg_trend_layer_png));
	layer_insert_above_sibling(bitmap_layer_get_layer(bg_trend_layer_draw), bitmap_layer_get_layer(bg_trend_layer_png));

	// top
	layer_insert_above_sibling(bitmap_layer_get_layer(icon_layer), bitmap_layer_get_layer(bg_trend_layer_draw));
	layer_insert_above_sibling(text_layer_get_layer(delta_layer), bitmap_layer_get_layer(bg_trend_layer_draw));
	layer_insert_above_sibling(text_layer_get_layer(message_layer), bitmap_layer_get_layer(bg_trend_layer_draw));
	layer_insert_above_sibling(text_layer_get_layer(bg_layer), bitmap_layer_get_layer(bg_trend_layer_draw));
	layer_insert_above_sibling(text_layer_get_layer(cgmtime_layer), bitmap_layer_get_layer(bg_trend_layer_draw));

	// bottom
	layer_add_child(bitmap_layer_get_layer(lower_face_layer), text_layer_get_layer(time_watch_layer));
	layer_insert_above_sibling(text_layer_get_layer(date_app_layer), text_layer_get_layer(time_watch_layer));
	layer_insert_above_sibling(text_layer_get_layer(bottom_left_text_layer), text_layer_get_layer(time_watch_layer));
	layer_insert_above_sibling(text_layer_get_layer(bottom_right_text_layer), text_layer_get_layer(time_watch_layer));

	if (!state->use_png) trend_init(bitmap_layer_get_layer(bg_trend_layer_draw));

	// put " " (space) in bg field so logo continues to show
	// " " (space) also shows these are init values, not bad or null values
	// Setting all default values here

	// prep for battery display, even if we don't have one.
	state->icon = 255; // no icon set and ignore
	snprintf(last_bg, BG_MSGSTR_SIZE, " ");
	state->cgm_time = 0;
	state->app_time = 0;
	snprintf(current_bg_delta, BGDELTA_MSGSTR_SIZE, "LOAD");
	state->phone_battery_level = 255;
	state->battery_level = 255;

#ifdef TEST_MODE
	snprintf(current_bg_delta, BGDELTA_MSGSTR_SIZE, "+0.08");
	state->phone_battery_level = 100;
	state->battery_level = 100;
	state->icon = 1;
	state->special_value_alert=false;

	snprintf(message_layer_text,sizeof(message_layer_text), "Test Mode");
	text_layer_set_text(message_layer, message_layer_text);
	state->show_message=true;
	layer_set_hidden((Layer *)message_layer, true);
	text_layer_set_text(delta_layer,"0.5mmol");
#endif
	LOG("Setting display values to correct state");
    update_colours();
	draw_date_from_app();
	load_cgmtime();
	load_bg();
	load_icon();
	load_bg_delta();
	load_battlevel_phone();
    update_battery_state();

	layer_mark_dirty(text_layer_get_layer(bg_layer));


#if DEBUG_LEVEL > 0
	text_layer_set_background_color(message_layer, GColorClear);
	text_layer_set_font(message_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28));
	text_layer_set_text_alignment(message_layer, GTextAlignmentCenter);
#endif
	TRACE("window_load_cgm: build done, init timer");
	// mark dirty and request data
	state->dirty.need_cgm = 1;
	reset_timer_callback_cgm(LOADING_MSGSEND_SECS);
	TRACE("window_load_cgm: timer registered");

} // end window_load_cgm


void window_unload_cgm(Window *window_cgm)
{
//	TRACE("WINDOW UNLOAD IN");


	//destroy the trend bitmap and layer
	if(bg_trend_bitmap != NULL) destroy_null_GBitmap(&bg_trend_bitmap);
	if(bg_trend_layer_draw != NULL) destroy_null_BitmapLayer(&bg_trend_layer_draw);
	if(bg_trend_layer_png != NULL) destroy_null_BitmapLayer(&bg_trend_layer_png);
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


/***
 * init WF state
 */

void ui_og_init(AppState *value)
{
    state = value; 
    state->wf_cb.set_message = set_message;
    state->wf_cb.set_icon = set_icon;
    state->wf_cb.set_delta = set_delta;
    state->wf_cb.set_delta_colour = set_delta_colour;
    state->wf_cb.set_cgmtime = set_cgmtime;
    state->wf_cb.update_battery_state = update_battery_state;
    state->wf_cb.update_seconds_timer = update_seconds_timer;
    state->wf_cb.update_timeago = update_timeago;
    state->wf_cb.update_message_timeout = update_message_timeout;
    state->wf_cb.update_timeago = update_timeago;
    state->wf_cb.update_left_field = update_left_field;
    state->wf_cb.update_right_field = update_right_field;
    state->wf_cb.update_trend = update_trend;
    state->wf_cb.update_collect_health = update_collect_health;
    state->wf_cb.update_message = update_message;
#ifdef PBL_COLOR
    state->wf_cb.update_colours = update_colours;
#endif
    state->wf_cb.trend_bounds = ui_og_trend_bounds;
	
	LOG("display_seconds: %i", state->enable_seconds);
	//initialise the Time Fonts
	if (HIGH_RES()) {
#if defined(PBL_PLATFORM_EMERY)
		// 60px clips against the date row in the 60px time box on Emery - use 54
		time_font_normal = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_GOTHAM_BOLD_54));
#else
		time_font_normal = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_GOTHAM_BOLD_60));
#endif
		time_font_small = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_GOTHAM_BOLD_40));
	} else {
		time_font_normal = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_GOTHAM_BOLD_40));
		time_font_small = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_GOTHAM_BOLD_30));
	}
	//Initialise the time format string.  No seconds here.
	if(clock_is_24h_style() == true)
	{
		if(state->enable_seconds) 
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
		if(state->enable_seconds)
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

	// create the windows
	window_cgm = window_create();
	window_set_background_color(window_cgm, GColorBlack);
	window_set_window_handlers(window_cgm, (WindowHandlers)
	{
		.load = window_load_cgm,
		.unload = window_unload_cgm
	});


#ifdef ENABLE_TREND_RENDERER
	state->comm_callbacks.low_limit = trend_set_low_line;
	state->comm_callbacks.high_limit = trend_set_high_line;
#endif
	state->comm_callbacks.message = comm_set_message;
	state->comm_callbacks.phonebat = comm_set_phone_battery;
	state->comm_callbacks.slopeval = comm_set_icon;
	state->comm_callbacks.vibe = comm_set_vibrate;
	state->comm_callbacks.bgl_delta = comm_set_bgl_delta;
	state->comm_callbacks.bgl_series = comm_set_bgl_series; 
	state->comm_callbacks.bgl_data = comm_set_bgl_data;
	state->comm_callbacks.bgl_timestamp = comm_set_bgl_timestamp;
	state->comm_callbacks.bgl_value = comm_set_bgl_value;
	state->comm_callbacks.png = comm_set_png;
    state->comm_callbacks.sensor_info = comm_set_sensor_info;
	// the watch is the health data source, so nothing to receive
	state->comm_callbacks.health = NULL;

	const bool animated_cgm = true;
	window_stack_push(window_cgm, animated_cgm);

	comm_init(&state->comm_callbacks);

	if (!state->show_trend) {
		layer_set_hidden(bitmap_layer_get_layer(bg_trend_layer_draw), true);
		layer_set_hidden(bitmap_layer_get_layer(bg_trend_layer_png), true);
	}

#ifdef ENABLE_TOUCH

    touch_service_subscribe(touch_handler, NULL);
    app_touch_navigation_enable(true);
#endif

	if (state->show_message) update_message_timeout(state->message_timeout);
}

void ui_og_deinit(void)
{
	app_timer_cancel(message_tick_timer);

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


}

GRect ui_og_trend_bounds(void) 
{
    return layer_get_bounds(bitmap_layer_get_layer(bg_trend_layer_png));
}
