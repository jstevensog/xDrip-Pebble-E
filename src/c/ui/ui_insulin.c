#ifdef PBL_TOUCH
#include <pebble.h>
#include <stdarg.h>
#include "../debug.h"
#include "ui_insulin.h"

void reset_stale_timer(void);

extern AppState state;

Layer *root = NULL;
Layer *bg = NULL;

TextLayer *bolus_up = NULL;
TextLayer *bolus_text = NULL;
TextLayer *bolus_down = NULL;

TextLayer *basal_up = NULL;
TextLayer *basal_text = NULL;
TextLayer *basal_down = NULL;

TextLayer *carbs_up = NULL;
TextLayer *carbs_text = NULL;
TextLayer *carbs_down = NULL;

TextLayer *back = NULL;
TextLayer *enter = NULL;

int bolus_value = 0;
int basal_value = 0;
int carbs_value = 0;

#define BUTTON_WIDTH 34
#define TEXT_HEIGHT 48

/**
 * [-][bolus][+]
 * [-][basal][+]
 * [-][carbs][+]
 */

#define FRAME(down, textfield, up, X, Y, BUTTON_WIDTH, HEIGHT) \
{\
    down = text_layer_create((GRect) { \
            { X,  Y}, \
            { BUTTON_WIDTH, HEIGHT }\
    });\
    textfield = text_layer_create((GRect) {\
            { X + BUTTON_WIDTH,  Y}, \
            { PBL_DISPLAY_WIDTH - ((X + BUTTON_WIDTH)*2), HEIGHT }\
    });\
    up = text_layer_create((GRect) { \
            { PBL_DISPLAY_WIDTH - X - BUTTON_WIDTH ,  Y}, \
            { BUTTON_WIDTH, HEIGHT }\
    });\
}

#define BOLUS_X 10
#define BOLUS_Y 5 

#define BASAL_X 10 
#define BASAL_Y TEXT_HEIGHT + BOLUS_Y + 5 

#define CARBS_X 10
#define CARBS_Y TEXT_HEIGHT + BASAL_Y + 5

/* #define BACK_X BOLUS_UP_X */
#define BACK_X BASAL_X
#define BACK_Y CARBS_Y + TEXT_HEIGHT + 10
#define BACK_WIDTH ((PBL_DISPLAY_WIDTH / 2) - 20) 
#define BACK_HEIGHT ((PBL_DISPLAY_HEIGHT / 3) - 30)

/* #define ENTER_X BASAL_UP_X  */
#define ENTER_X (PBL_DISPLAY_WIDTH / 2) + 10
#define ENTER_Y CARBS_Y + TEXT_HEIGHT + 10
#define ENTER_WIDTH BACK_WIDTH 
#define ENTER_HEIGHT ((PBL_DISPLAY_HEIGHT / 3) - 30)

#define BOLUS_UP 1
#define BOLUS_DOWN 2
#define BASAL_UP 3
#define BASAL_DOWN 4
#define BACK 5
#define ENTER 6
#define BASAL_TEXT 7
#define BOLUS_TEXT 8
#define CARBS_UP 9
#define CARBS_TEXT 10
#define CARBS_DOWN 11


#define CARBS_PREFIX "\U0001F34C"
#define BOLUS_PREFIX ""
#define BASAL_PREFIX

static char bolus_tx[12];
static char basal_tx[12];
static char carbs_tx[12];
static int touch_region = 0;
static TouchServiceHandler cb;

static bool bolus_enabled = false;
static bool basal_enabled = false;
static bool carbs_enabled = false;

void update_text(void) {
    snprintf(basal_tx, sizeof(basal_tx), "%d", basal_value);
    text_layer_set_text(basal_text, basal_tx);
    snprintf(bolus_tx, sizeof(bolus_tx), "%d", bolus_value);
    text_layer_set_text(bolus_text, bolus_tx);
    snprintf(carbs_tx, sizeof(carbs_tx), "%d", carbs_value);
    text_layer_set_text(carbs_text, carbs_tx);
}

void update_bb(void) {
    if (basal_enabled) text_layer_set_background_color(basal_text, GColorGreen);
    else text_layer_set_background_color(basal_text, GColorWhite);

    if (bolus_enabled) text_layer_set_background_color(bolus_text, GColorGreen);
    else text_layer_set_background_color(bolus_text, GColorWhite);

    if (carbs_enabled) text_layer_set_background_color(carbs_text, GColorGreen);
    else text_layer_set_background_color(carbs_text, GColorWhite);
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
            DEBUG("%d %d", event->x, event->y);
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
            } else if IN(carbs_up) {
                carbs_value++;
            } else if IN(carbs_down) {
                carbs_value--;
                if (carbs_value < 1) carbs_value = 1;
            } else if IN(back) {
                insulin_display_deinit();
                return; // do not trigger stale reset
            } else if IN(enter) {
                insulin_display_deinit();
                // NOTE values are 4 bit decimals, so you can enter 0.0675 values (e.g. 0.5 is 8)
                comm_send_treatment(basal_enabled ? basal_value * 16 : 0, bolus_enabled ? bolus_value * 16 : 0, carbs_enabled ? carbs_value : 0);
                return; // do not trigger stale reset
            } else if IN(basal_text) {
                basal_enabled = !basal_enabled;
            } else if IN(bolus_text) {
                bolus_enabled = !bolus_enabled;
            } else if IN(carbs_text) {
                carbs_enabled = !carbs_enabled;
            }
            update_text();
            update_bb();
            break;
        default:
            break;
    }
    reset_stale_timer();
}

#define IN_IDLE_TIME 10000

AppTimer *ui_in_stale_timer = NULL;

void ui_in_handle_stale(void *data) {
    INFO("Timer TRIGGERED");
    // if triggered we are idle for too long
    ui_in_stale_timer = NULL;
    insulin_display_deinit();
}

void reset_stale_timer(void) {
    DEBUG("RESET TIMEOUT");
    if (NULL == ui_in_stale_timer || !app_timer_reschedule(ui_in_stale_timer, IN_IDLE_TIME)) {
        ui_in_stale_timer = app_timer_register(IN_IDLE_TIME, ui_in_handle_stale, NULL);
    }
}
char *bolus_up_text = "+";
char *bolus_down_text = "-";
char *back_text = "<";
char *enter_text = "Ok";

void insulin_display_init(Layer *root, TouchServiceHandler handoff) {
    cb = handoff;
    // bg
    bg = layer_create((GRect) { {0, 0}, {PBL_DISPLAY_WIDTH, PBL_DISPLAY_HEIGHT }});
    layer_add_child(root, bg);

    FRAME(bolus_down, bolus_text, bolus_up, BOLUS_X, BOLUS_Y, BUTTON_WIDTH, TEXT_HEIGHT);
    /* bolus_up = text_layer_create((GRect) {  */
    /*         { BOLUS_UP_X,  BOLUS_UP_Y},  */
    /*         { BOLUS_UP_WIDTH, BOLUS_UP_HEIGHT } */
    /* }); */
    /* bolus_text = text_layer_create((GRect) {  */
    /*         { BOLUS_TEXT_X,  BOLUS_TEXT_Y},  */
    /*         { BOLUS_TEXT_WIDTH, BOLUS_TEXT_HEIGHT } */
    /* }); */
    /* bolus_down = text_layer_create((GRect) {  */
    /*         { BOLUS_DOWN_X,  BOLUS_DOWN_Y},  */
    /*         { BOLUS_DOWN_WIDTH, BOLUS_DOWN_HEIGHT } */
    /* }); */

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

    FRAME(basal_down, basal_text, basal_up, BASAL_X, BASAL_Y, BUTTON_WIDTH, TEXT_HEIGHT);
    /* basal_up = text_layer_create((GRect) {  */
    /*         { BASAL_UP_X,  BASAL_UP_Y},  */
    /*         { BASAL_UP_WIDTH, BASAL_UP_HEIGHT } */
    /* }); */
    /* basal_text = text_layer_create((GRect) {  */
    /*         { BASAL_TEXT_X,  BASAL_TEXT_Y},  */
    /*         { BASAL_TEXT_WIDTH, BASAL_TEXT_HEIGHT } */
    /* }); */
    /* basal_down = text_layer_create((GRect) {  */
    /*         { BASAL_DOWN_X,  BASAL_DOWN_Y},  */
    /*         { BASAL_DOWN_WIDTH, BASAL_DOWN_HEIGHT } */
    /* }); */

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

    FRAME(carbs_down, carbs_text, carbs_up, CARBS_X, CARBS_Y, BUTTON_WIDTH, TEXT_HEIGHT);
    /* carbs_up = text_layer_create((GRect) {  */
    /*         { CARBS_UP_X,  CARBS_UP_Y},  */
    /*         { CARBS_UP_WIDTH, CARBS_UP_HEIGHT } */
    /* }); */
    /* carbs_text = text_layer_create((GRect) {  */
    /*         { CARBS_TEXT_X,  CARBS_TEXT_Y},  */
    /*         { CARBS_TEXT_WIDTH, CARBS_TEXT_HEIGHT } */
    /* }); */
    /* carbs_down = text_layer_create((GRect) {  */
    /*         { CARBS_DOWN_X,  CARBS_DOWN_Y},  */
    /*         { CARBS_DOWN_WIDTH, CARBS_DOWN_HEIGHT } */
    /* }); */

    text_layer_set_background_color(carbs_up, GColorLightGray);
    text_layer_set_background_color(carbs_down, GColorLightGray);
    text_layer_set_background_color(carbs_text, GColorWhite);
    
    text_layer_set_font(carbs_up, font);
    text_layer_set_font(carbs_down, font);
    text_layer_set_font(carbs_text, font);
    
    text_layer_set_text(carbs_up, bolus_up_text);
    text_layer_set_text(carbs_down, bolus_down_text);
    
    text_layer_set_text_alignment(carbs_up, GTextAlignmentCenter);
    text_layer_set_text_alignment(carbs_text, GTextAlignmentCenter);
    text_layer_set_text_alignment(carbs_down, GTextAlignmentCenter);

    layer_add_child(bg, text_layer_get_layer(carbs_up));
    layer_add_child(bg, text_layer_get_layer(carbs_text));
    layer_add_child(bg, text_layer_get_layer(carbs_down));

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

    if (bolus_value == 0) bolus_value = state.default_bolus;
    if (basal_value == 0) basal_value = state.default_basal;
    if (carbs_value == 0) carbs_value = state.default_carbs;

    update_text();
    update_bb();

    // fetch touch events
    touch_service_subscribe(insulin_touch_handler, NULL);

    reset_stale_timer();
}

void insulin_display_deinit(void) {

    INFO("EXIT ui_insulin");
    app_timer_cancel(ui_in_stale_timer);
    ui_in_stale_timer = NULL;

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
