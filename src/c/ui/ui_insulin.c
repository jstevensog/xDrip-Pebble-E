#ifdef PBL_TOUCH
#include <pebble.h>
#include <stdarg.h>
#include "../debug.h"
#include "ui_insulin.h"

static AppState *state = NULL;

static Layer *root = NULL;
static Layer *bg = NULL;

static TextLayer *bolus_up = NULL;
static TextLayer *bolus_text = NULL;
static TextLayer *bolus_down = NULL;

static TextLayer *basal_up = NULL;
static TextLayer *basal_text = NULL;
static TextLayer *basal_down = NULL;

static TextLayer *back = NULL;
static TextLayer *enter = NULL;

static int bolus_value = 16;
static int basal_value = 16;

#define BOLUS_UP_X 10
#define BOLUS_UP_Y 10
#define BOLUS_UP_WIDTH (PBL_DISPLAY_WIDTH / 2) - 20
#define BOLUS_UP_HEIGHT ((PBL_DISPLAY_HEIGHT / 3) * 2) / 4
#define BOLUS_TEXT_X BOLUS_UP_X
#define BOLUS_TEXT_Y BOLUS_UP_Y + ((PBL_DISPLAY_HEIGHT / 3) * 2) / 4
#define BOLUS_TEXT_WIDTH BOLUS_UP_WIDTH
#define BOLUS_TEXT_HEIGHT ((PBL_DISPLAY_HEIGHT / 3) * 2) / 2
#define BOLUS_DOWN_X BOLUS_UP_X
#define BOLUS_DOWN_Y BOLUS_UP_Y + ((PBL_DISPLAY_HEIGHT / 3) * 2) -  BOLUS_UP_HEIGHT
#define BOLUS_DOWN_WIDTH BOLUS_UP_WIDTH
#define BOLUS_DOWN_HEIGHT BOLUS_UP_HEIGHT

#define BASAL_UP_X (PBL_DISPLAY_WIDTH / 2) + 10
#define BASAL_UP_Y 10
#define BASAL_UP_WIDTH (PBL_DISPLAY_WIDTH / 2) - 20
#define BASAL_UP_HEIGHT ((PBL_DISPLAY_HEIGHT / 3) * 2) / 4
#define BASAL_TEXT_X BASAL_UP_X
#define BASAL_TEXT_Y BASAL_UP_Y + ((PBL_DISPLAY_HEIGHT / 3) * 2) / 4
#define BASAL_TEXT_WIDTH BASAL_UP_WIDTH
#define BASAL_TEXT_HEIGHT ((PBL_DISPLAY_HEIGHT / 3) * 2) / 2
#define BASAL_DOWN_X BASAL_UP_X
#define BASAL_DOWN_Y BASAL_UP_Y + ((PBL_DISPLAY_HEIGHT / 3) * 2) -  BASAL_UP_HEIGHT
#define BASAL_DOWN_WIDTH BASAL_UP_WIDTH
#define BASAL_DOWN_HEIGHT BASAL_UP_HEIGHT

#define BACK_X BOLUS_UP_X
#define BACK_Y 10 + BOLUS_DOWN_Y + BOLUS_DOWN_HEIGHT
#define BACK_WIDTH BOLUS_UP_WIDTH
#define BACK_HEIGHT ((PBL_DISPLAY_HEIGHT / 3) - 30)

#define ENTER_X BASAL_UP_X 
#define ENTER_Y 10 + BOLUS_DOWN_Y + BOLUS_DOWN_HEIGHT
#define ENTER_WIDTH BOLUS_UP_WIDTH
#define ENTER_HEIGHT ((PBL_DISPLAY_HEIGHT / 3) - 30)

#define BOLUS_UP 1
#define BOLUS_DOWN 2
#define BASAL_UP 3
#define BASAL_DOWN 4
#define BACK 5
#define ENTER 6
#define BASAL_TEXT 7
#define BOLUS_TEXT 8

static char bolus_tx[12];
static char basal_tx[12];
static int touch_region = 0;
TouchServiceHandler cb;

static bool bolus_enabled = false;
static bool basal_enabled = false;

void update_text(void) {
    snprintf(basal_tx, sizeof(basal_tx), "%d", basal_value);
    text_layer_set_text(basal_text, basal_tx);
    snprintf(bolus_tx, sizeof(bolus_tx), "%d", bolus_value);
    text_layer_set_text(bolus_text, bolus_tx);
}

void update_bb(void) {
    if (basal_enabled) text_layer_set_background_color(basal_text, GColorGreen);
    else text_layer_set_background_color(basal_text, GColorWhite);

    if (bolus_enabled) text_layer_set_background_color(bolus_text, GColorGreen);
    else text_layer_set_background_color(bolus_text, GColorWhite);
}

bool inbounds(GRect bounds, int x, int y) {
    TRACE("%3d %3d %3d -- %3d %3d %3d", bounds.origin.x, x, bounds.origin.x+bounds.size.w, bounds.origin.y, y, bounds.origin.y + bounds.size.h);
    return (x > bounds.origin.x && x < bounds.origin.x + bounds.size.w && y > bounds.origin.y && y < bounds.origin.y + bounds.size.h);
}
void insulin_touch_handler(const TouchEvent *event, void *context) {

    switch(event->type) {
        case TouchEvent_Touchdown:
#define IN(var) (inbounds(layer_get_frame(text_layer_get_layer(var)), event->x, event->y))
            if IN(bolus_up) {
                touch_region = BOLUS_UP;
            } else if IN(bolus_down) {
                touch_region = BOLUS_DOWN;
            } else if IN(basal_up) {
                touch_region = BASAL_UP;
            } else if IN(basal_down) {
                touch_region = BASAL_DOWN;
            } else if IN(back) {
                touch_region = BACK;
            } else if IN(enter) {
                touch_region = ENTER;
            } else if IN(basal_text) {
                touch_region = BASAL_TEXT;
            } else if IN(bolus_text) {
                touch_region = BOLUS_TEXT;
            }
            break;
        case TouchEvent_Liftoff:
            ERROR("%d %d", event->x, event->y);
            if IN(bolus_up) {
                bolus_value++;
            } else if IN(bolus_down) {
                bolus_value--;
                if (bolus_value < 1) bolus_value = 1;
            } else if IN(basal_up) {
                basal_value++;
            } else if IN(basal_down) {
                basal_value--;
                if (basal_value < 1) basal_value = 1;
            } else if IN(back) {
                insulin_display_deinit();
            } else if IN(enter) {
                insulin_display_deinit();
                comm_send_basal_bolus(basal_enabled ? basal_value : 0, bolus_enabled ? bolus_value : 0);
            } else if IN(basal_text) {
                basal_enabled = !basal_enabled;
            } else if IN(bolus_text) {
                bolus_enabled = !bolus_enabled;
            }
            update_text();
            update_bb();
            break;
        default:
            break;
    }
}

static char *bolus_up_text = "+";
static char *bolus_down_text = "-";
static char *back_text = "<";
static char *enter_text = "Ok";

void insulin_display_init(AppState *val, Layer *root, TouchServiceHandler handoff) {
    state = val;
    cb = handoff;
    // bg
    bg = layer_create((GRect) { {0, 0}, {PBL_DISPLAY_WIDTH, PBL_DISPLAY_HEIGHT }});
    layer_add_child(root, bg);

    bolus_up = text_layer_create((GRect) { 
            { BOLUS_UP_X,  BOLUS_UP_Y}, 
            { BOLUS_UP_WIDTH, BOLUS_UP_HEIGHT }
    });
    bolus_text = text_layer_create((GRect) { 
            { BOLUS_TEXT_X,  BOLUS_TEXT_Y}, 
            { BOLUS_TEXT_WIDTH, BOLUS_TEXT_HEIGHT }
    });
    bolus_down = text_layer_create((GRect) { 
            { BOLUS_DOWN_X,  BOLUS_DOWN_Y}, 
            { BOLUS_DOWN_WIDTH, BOLUS_DOWN_HEIGHT }
    });

    text_layer_set_background_color(bolus_up, GColorLightGray);
    text_layer_set_background_color(bolus_down, GColorLightGray);
    text_layer_set_background_color(bolus_text, GColorWhite);
    
    FontInfo *font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_GOTHAM_BOLD_40));
    text_layer_set_font(bolus_up, font);
    text_layer_set_font(bolus_down, font);
    text_layer_set_font(bolus_text, font);
    
    text_layer_set_text(bolus_up, bolus_up_text);
    text_layer_set_text(bolus_down, bolus_down_text);

    text_layer_set_text_alignment(bolus_up, GTextAlignmentCenter);
    text_layer_set_text_alignment(bolus_text, GTextAlignmentCenter);
    text_layer_set_text_alignment(bolus_down, GTextAlignmentCenter);

    layer_add_child(bg, text_layer_get_layer(bolus_up));
    layer_add_child(bg, text_layer_get_layer(bolus_text));
    layer_add_child(bg, text_layer_get_layer(bolus_down));

    basal_up = text_layer_create((GRect) { 
            { BASAL_UP_X,  BASAL_UP_Y}, 
            { BASAL_UP_WIDTH, BASAL_UP_HEIGHT }
    });
    basal_text = text_layer_create((GRect) { 
            { BASAL_TEXT_X,  BASAL_TEXT_Y}, 
            { BASAL_TEXT_WIDTH, BASAL_TEXT_HEIGHT }
    });
    basal_down = text_layer_create((GRect) { 
            { BASAL_DOWN_X,  BASAL_DOWN_Y}, 
            { BASAL_DOWN_WIDTH, BASAL_DOWN_HEIGHT }
    });

    text_layer_set_background_color(basal_up, GColorLightGray);
    text_layer_set_background_color(basal_down, GColorLightGray);
    text_layer_set_background_color(basal_text, GColorWhite);
    
    text_layer_set_font(basal_up, font);
    text_layer_set_font(basal_down, font);
    text_layer_set_font(basal_text, font);
    
    text_layer_set_text(basal_up, bolus_up_text);
    text_layer_set_text(basal_down, bolus_down_text);
    
    text_layer_set_text_alignment(basal_up, GTextAlignmentCenter);
    text_layer_set_text_alignment(basal_text, GTextAlignmentCenter);
    text_layer_set_text_alignment(basal_down, GTextAlignmentCenter);

    layer_add_child(bg, text_layer_get_layer(basal_up));
    layer_add_child(bg, text_layer_get_layer(basal_text));
    layer_add_child(bg, text_layer_get_layer(basal_down));

    back = text_layer_create((GRect) {
            { BACK_X, BACK_Y },
            { BACK_WIDTH, BACK_HEIGHT}
    });
    text_layer_set_background_color(back, GColorRoseVale);
    text_layer_set_text(back, back_text);

    enter = text_layer_create((GRect) {
            { ENTER_X, ENTER_Y },
            { ENTER_WIDTH, ENTER_HEIGHT}
    });
    text_layer_set_background_color(enter, GColorGreen);
    text_layer_set_text(enter, enter_text);

    text_layer_set_text_alignment(back, GTextAlignmentCenter);
    text_layer_set_text_alignment(enter, GTextAlignmentCenter);
    text_layer_set_font(back, font);
    text_layer_set_font(enter, font);

    layer_add_child(bg, text_layer_get_layer(back));
    layer_add_child(bg, text_layer_get_layer(enter));

    update_text();

    // fetch touch events
    touch_service_subscribe(insulin_touch_handler, NULL);
}

void insulin_display_deinit(void) {

    text_layer_destroy(bolus_up);
    text_layer_destroy(bolus_text);
    text_layer_destroy(bolus_down);

    text_layer_destroy(basal_up);
    text_layer_destroy(basal_text);
    text_layer_destroy(basal_down);

    text_layer_destroy(back);
    text_layer_destroy(enter);

    layer_destroy(bg);

    touch_service_subscribe(cb, NULL);
}
#endif
