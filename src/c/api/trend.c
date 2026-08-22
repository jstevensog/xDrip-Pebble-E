/**
 *
 * Use as is. No guarantees.
 */
#include <pebble.h>

#include "../xdrip.h"
#include "../debug.h"
#include "communication.h"

#include "trend.h"



// trend config
trend_config config = {
    .bgl_type = BGL_TYPE_MG_DL
};

/**
 * File contains functions for drawing the trend image from a list of BGL values vs time
 */

static void trend_layer_callback(Layer *layer, GContext *ctx);

void trend_init(Layer *layer) {
    config.layer = layer;

    /*
     * trend settings
     */
    config.critical_color = persist_exists(SET_BGL_CRITICAL_COLOUR) ? GColorFromHEX(persist_read_int(SET_BGL_CRITICAL_COLOUR)) : COLOR_FALLBACK(GColorRed, GColorWhite);
    config.high_color = persist_exists(SET_BGL_HIGH_COLOUR) ? GColorFromHEX(persist_read_int(SET_BGL_HIGH_COLOUR)) : COLOR_FALLBACK(GColorOrange, GColorWhite);
    config.average_color = persist_exists(SET_BGL_AVERAGE_COLOUR) ? GColorFromHEX(persist_read_int(SET_BGL_AVERAGE_COLOUR)) : COLOR_FALLBACK(GColorYellow, GColorWhite);
    config.good_color = persist_exists(SET_BGL_GOOD_COLOUR) ? GColorFromHEX(persist_read_int(SET_BGL_GOOD_COLOUR)) : COLOR_FALLBACK(GColorGreen, GColorWhite);
    config.low_color = persist_exists(SET_BGL_LOW_COLOUR) ? GColorFromHEX(persist_read_int(SET_BGL_LOW_COLOUR)) : COLOR_FALLBACK(GColorBlue, GColorWhite);
    config.bgl_low = persist_exists(SET_BGL_LOW) ? persist_read_int(SET_BGL_LOW) : 72;
    config.bgl_average = persist_exists(SET_BGL_AVERAGE) ? persist_read_int(SET_BGL_AVERAGE) : 144;
    config.bgl_high = persist_exists(SET_BGL_HIGH) ? persist_read_int(SET_BGL_HIGH) : 190;
    config.bgl_critical = persist_exists(SET_BGL_CRITICAL) ? persist_read_int(SET_BGL_CRITICAL) : 210;
    config.high_line_color = persist_exists(SET_HIGH_LINE_COLOUR) ? GColorFromHEX(persist_read_int(SET_HIGH_LINE_COLOUR)) : COLOR_FALLBACK(GColorRed, GColorWhite);
    config.low_line_color = persist_exists(SET_LOW_LINE_COLOUR) ? GColorFromHEX(persist_read_int(SET_LOW_LINE_COLOUR)) : COLOR_FALLBACK(GColorBlue, GColorWhite);
    config.trend_width = persist_exists(SET_TREND_WIDTH) ? persist_read_int(SET_TREND_WIDTH) : 4;
    config.style = persist_exists(SET_TREND_STYLE) ? persist_read_int(SET_TREND_STYLE) : TREND_STYLE_DOTS;
    config.line_style = persist_exists(SET_LINE_STYLE) ? persist_read_int(SET_LINE_STYLE) : TREND_LINE_STYLE_SOLID;
    config.line_width = persist_exists(SET_LINE_WIDTH) ? persist_read_int(SET_LINE_WIDTH) : 4;

    /* Values for high/low lines */
    config.bgl_high_line = persist_exists(SET_HIGH_LINE_VALUE) ? persist_read_int(SET_HIGH_LINE_VALUE) : 180;
    config.bgl_low_line = persist_exists(SET_LOW_LINE_VALUE) ? persist_read_int(SET_LOW_LINE_VALUE) : 72;
    config.bgl_high_limit = persist_exists(SET_HIGH_LIMIT) ? persist_read_int(SET_HIGH_LIMIT) : 250;
    config.bgl_low_limit = persist_exists(SET_LOW_LIMIT) ? persist_read_int(SET_LOW_LIMIT) : 40;

    config.bgl.initialized = 0;

    TRACE(TREND_LOG "Setting callback");
    layer_set_update_proc((Layer *) config.layer, trend_layer_callback);
}

void trend_deinit(void) {
    if (config.layer != NULL) layer_set_update_proc(config.layer, NULL); // discard
    config.layer = NULL;
    config.bgl.initialized = 0;
}


static inline void draw_bgl_point(trend_bgl_value value, int16_t x, GRect bounds, GContext *ctx) {
    /**
     * Since the trend image is just a graph, we do not need to know the 
     * actual type of data
     */

    if (value < config.bgl_low_limit || value > config.bgl_high_limit) return;

    GColor color = config.good_color;

    if (value > config.bgl_high) color = config.critical_color;
    else if (value > config.bgl_high) color = config.high_color;
    else if (value > config.bgl_average) color = config.average_color;
    else if (value < config.bgl_low) color = config.low_color;

    graphics_context_set_stroke_color(ctx, color);
    
    GPoint point = {x, BGL_TO_Y(value, config, bounds)};

    graphics_draw_circle(ctx, point, 0);

}

static inline void draw_bgl_line(trend_bgl_value value, trend_bgl_value value2, int16_t x1, int16_t x2, GRect bounds, GContext *ctx) {
    /**
     * Since the trend image is just a graph, we do not need to know the 
     * actual type of data
     */

    // ignore zero/extreme values, single high value will stay to allow line into the graph
    if (value < config.bgl_low_limit || value2 < config.bgl_low_limit) return;
    if (value2 > config.bgl_high_limit && value2 > config.bgl_high_limit) return;

    GColor color = config.good_color;

    if (value > config.bgl_high) color = config.high_color;
    else if (value > config.bgl_average) color = config.average_color;
    else if (value < config.bgl_low) color = config.low_color;

    color = COLOR_FALLBACK(color, GColorWhite); // b/w compatability

    graphics_context_set_stroke_color(ctx, color);
    
    GPoint point = {x1, BGL_TO_Y(value, config, bounds)};
    GPoint point2 = {x2, BGL_TO_Y(value2, config, bounds)};

    graphics_draw_line(ctx, point, point2);

}

/**
 * t is the fractional distance between x0 and x1
 */
static inline int16_t lerp(int32_t y0, int32_t y1, uint32_t t) {
    if (t == 0) return y0;
    if (t == 1 << 16) return y1;
    return ((y0 << 16) + (t * (y1 - y0))) >> 16;
}

static bool draw_trend(Layer *layer, GContext *ctx) {
    graphics_context_set_stroke_width(ctx, config.trend_width); // constant

    GRect bounds = layer_get_bounds(layer);

    bool interp = config.style == TREND_STYLE_DOTS ? (bounds.size.w > config.bgl.size) : false; // do not interp on lines 
    int32_t t = 0;
    int32_t interval = ((config.bgl.size - 1) << 16) / bounds.size.w;
    int index = 0;
    TRACE(TREND_LOG "Layer Width: %d array %d ", bounds.size.w, config.bgl.size);
    TRACE(TREND_LOG "Interp settings: [%d] :: %d", interp, interval);
    TRACE(TREND_LOG "Line width: %d", config.line_width);

    if (config.style == TREND_STYLE_DOTS) {
        TRACE(TREND_LOG "Style dotted");
        // simplified
        for (int i = 0; i < bounds.size.w; i++) {
            if (interp) {
                int16_t y0 = lerp(
                        config.bgl.values[(config.bgl.index + index) % (config.bgl.size)], 
                        config.bgl.values[(config.bgl.index + index + 1) % (config.bgl.size)], 
                        t);
                t += interval;
                if (t >= (1 << 16)) index++;
                draw_bgl_point(y0, i, bounds, ctx);
                t %= 1 << 16;
            } else {
                draw_bgl_point(config.bgl.values[(config.bgl.index + i) % (config.bgl.size)], i, bounds, ctx); 
            }
        }
    } else if (config.style == TREND_STYLE_LINES) {
        TRACE(TREND_LOG "Style lines");
        if (interp) {
            for (int i = 0; i < bounds.size.w; i++) {
                if (interp) {
                    int16_t y0 = lerp(
                            config.bgl.values[(config.bgl.index + index) % (config.bgl.size)], 
                            config.bgl.values[(config.bgl.index + index + 1) % (config.bgl.size)], 
                            t);
                    int16_t y1 = lerp(
                            config.bgl.values[(config.bgl.index + index) % (config.bgl.size)], 
                            config.bgl.values[(config.bgl.index + index + 1) % (config.bgl.size)], 
                            t += interval);
                    /* TRACE(TREND_LOG "Line %d %d %d %d %d %d %d", */
                    /*         config.bgl.size, config.bgl.index, index, */
                    /*         y0, y1,  */
                    /*         config.bgl.values[(config.bgl.index + index) % (config.bgl.size)],  */
                    /*         config.bgl.values[(config.bgl.index + index + 1) % (config.bgl.size)]); */
                    draw_bgl_line(y0, y1, i, i+1, bounds, ctx);
                    if (t >= (1 << 16)) {
                        index++;
                        t %= 1 << 16;
                        /* TRACE(TREND_LOG "Index: %hu of %hu, %hu", index, config.bgl.size, t); */
                    }
                }
            }
        } else {
            // currently forced path
            uint32_t t = (bounds.size.w << 16) / (config.bgl.size - 1); 
            for (uint32_t i = 0, j = 0; i < ((uint32_t) bounds.size.w << 16); i += t, j++) {
                draw_bgl_line(
                        config.bgl.values[(config.bgl.index + j) % (config.bgl.size)],
                        config.bgl.values[(config.bgl.index + j + 1) % (config.bgl.size)],
                        i >> 16, (i+t) >> 16, bounds, ctx); 
            }
        }
    }
    return true;
}

static bool draw_trend_lines(Layer *layer, GContext *ctx) {
    // top is 0,0
    // excape if no line needs to be drawn
    GRect bounds = layer_get_bounds(layer);
    const int16_t h = BGL_TO_Y(config.bgl_high_line, config, bounds); 
    const int16_t l = BGL_TO_Y(config.bgl_low_line, config, bounds); 
    TRACE(TREND_LOG "Draw lines, high: %d, low %d", h, l);

    // drawing
    graphics_context_set_stroke_width(ctx, config.line_width);
    int w = 0, s = 0;
    switch (config.line_style) {
        default:
        case TREND_LINE_STYLE_SOLID:
            TRACE(TREND_LOG "Lines -> Solid");
            if (config.bgl_high_line) {
                graphics_context_set_stroke_color(ctx, COLOR_FALLBACK(config.high_line_color, GColorWhite));
                graphics_draw_line(ctx, (GPoint) { 0, h }, (GPoint) { bounds.size.w, h});
            }
            if (config.bgl_low_line) {
                graphics_context_set_stroke_color(ctx, COLOR_FALLBACK(config.low_line_color, GColorWhite));
                graphics_draw_line(ctx, (GPoint) { 0, l }, (GPoint) { bounds.size.w, l});
            }
            break;
        case TREND_LINE_STYLE_DOTTED_SPARSE:
            TRACE(TREND_LOG "Lines -> Dotted with extra space");
            s = 1;
            // fall through
        case TREND_LINE_STYLE_DOTTED:
            TRACE(TREND_LOG "Lines -> Dotted");
            s += 2;
            if (config.bgl_high_line) {
                graphics_context_set_stroke_color(ctx, COLOR_FALLBACK(config.high_line_color, GColorWhite));
                for (int x = 0; x < bounds.size.w; x+=s) {
                    graphics_draw_pixel(ctx, (GPoint) { x, h });
                }
            }
            if (config.bgl_low_line) {
                graphics_context_set_stroke_color(ctx, COLOR_FALLBACK(config.low_line_color, GColorWhite));
                for (int x = 0; x < bounds.size.w; x+=s) {
                    graphics_draw_pixel(ctx, (GPoint) { x, l });
                }
            }
            break;
        case TREND_LINE_STYLE_DASHED_WIDE:
            TRACE(TREND_LOG "Lines -> Dashed - wide");
            s = 5;
            w = 2;
            // fall through
        case TREND_LINE_STYLE_DASHED:
            TRACE(TREND_LOG "Lines -> Dashed");
            s += 5;
            w += 2;
            if (config.bgl_high_line) {
                graphics_context_set_stroke_color(ctx, COLOR_FALLBACK(config.high_line_color, GColorWhite));
                for (int x = 0; x < bounds.size.w; x+=s) {
                    graphics_draw_line(ctx, (GPoint) { x, h }, (GPoint) { x+w, h});
                }
            }
            if (config.bgl_low_line) {
                graphics_context_set_stroke_color(ctx, COLOR_FALLBACK(config.low_line_color, GColorWhite));
                for (int x = 0; x < bounds.size.w; x+=s) {
                    graphics_draw_line(ctx, (GPoint) { x, l }, (GPoint) { x+w, l});
                }
            }
            break;
    }
    return true;
}

void trend_layer_callback(Layer *layer, GContext *ctx) {
    if (config.bgl.initialized) {
        TRACE(TREND_LOG "Drawing trend line");
        draw_trend(layer, ctx);
    }
    TRACE(TREND_LOG "Drawing high/low lines");
    draw_trend_lines(layer, ctx);
    config.redraw = 0; 
}

void trend_draw(void) {
    DEBUG(TREND_LOG "Marking trend layer dirty");
    config.redraw = 1;
    if (config.layer != NULL) layer_mark_dirty(config.layer);
}


/*
 * Update functions
 */

void trend_set_series(comm_bgl_series *values) {
    TRACE(TREND_LOG "Trend set series"); 
    if (!config.bgl.initialized) {
        DEBUG("Initializing");
        config.bgl.size = values->length > PBL_DISPLAY_WIDTH ? PBL_DISPLAY_WIDTH : values->length;
        config.bgl.index = 0;
        config.bgl.initialized = 1;
        memset(config.bgl.values, 0x00, sizeof(config.bgl.values));
        config.bgl_type = values->bgl_values[0].is_mmol ? BGL_TYPE_MMOL_L : BGL_TYPE_MG_DL;
    }
    TRACE("Parsing data: %d", values->length);
    for (int i = 0; i < values->length && i < PBL_DISPLAY_WIDTH; i++) {
        TRACE("TREND DATA: %d %d", i, values->bgl_values[i].value);
        config.bgl.values[config.bgl.index % (config.bgl.size)] = values->bgl_values[i].value;
        config.bgl.index++;
        config.bgl.index = config.bgl.index % (config.bgl.size);
    }
    trend_draw();
}

void trend_set_value(comm_bgl_data *value) {
    TRACE(TREND_LOG "Trend set value");
    config.bgl.values[config.bgl.index % (config.bgl.size - 1)] = value->bgl.value;
    config.bgl.index++;
    config.bgl.index = config.bgl.index % (config.bgl.size - 1);
    trend_draw();

}

void trend_process_config(Tuple *data) {
    switch (data->key) {
        case SET_BGL_CRITICAL_COLOUR:
            persist_write_int(SET_BGL_CRITICAL_COLOUR, data->value->int32);
            config.critical_color = GColorFromHEX(data->value->int32);
            trend_draw();
            break;
        case SET_BGL_HIGH_COLOUR:
            persist_write_int(SET_BGL_HIGH_COLOUR, data->value->int32);
            config.high_color = GColorFromHEX(data->value->int32);
            trend_draw();
            break;
        case SET_BGL_AVERAGE_COLOUR:
            persist_write_int(SET_BGL_AVERAGE_COLOUR, data->value->int32);
            config.average_color = GColorFromHEX(data->value->int32);
            trend_draw();
            break;
        case SET_BGL_GOOD_COLOUR:
            persist_write_int(SET_BGL_GOOD_COLOUR, data->value->int32);
            config.good_color = GColorFromHEX(data->value->int32);
            trend_draw();
            break;
        case SET_BGL_LOW_COLOUR:
            persist_write_int(SET_BGL_LOW_COLOUR, data->value->int32);
            config.low_color = GColorFromHEX(data->value->int32);
            trend_draw();
            break;
        case SET_LOW_LINE_COLOUR:
            persist_write_int(SET_LOW_LINE_COLOUR, data->value->int32);
            config.low_line_color = GColorFromHEX(data->value->int32);
            trend_draw();
            break;
        case SET_HIGH_LINE_COLOUR:
            persist_write_int(SET_HIGH_LINE_COLOUR, data->value->int32);
            config.high_line_color = GColorFromHEX(data->value->int32);
            trend_draw();
            break;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wzero-length-bounds"
        case SET_LINE_STYLE:
            persist_write_int(SET_LINE_STYLE, data->value->data[0] - 0x30);
            config.line_style = data->value->data[0] - 0x30;
            trend_draw();
            break;
        case SET_TREND_STYLE:
            persist_write_int(SET_TREND_STYLE, data->value->data[0] - 0x30);
            config.style = data->value->data[0] - 0x30;
            trend_draw();
            break;
#pragma GCC diagnostic pop
        case SET_LINE_WIDTH:
            persist_write_int(SET_LINE_WIDTH, data->value->int32);
            config.line_width = data->value->int32;
            trend_draw();
            break;
        case SET_TREND_WIDTH:
            persist_write_int(SET_TREND_WIDTH, data->value->int32);
            config.trend_width = data->value->int32;
            trend_draw();
            break;
        case SET_BGL_LOW:
            persist_write_int(SET_BGL_LOW, data->value->int32);
            config.bgl_low = data->value->int32;
            trend_draw();
            break;
        case SET_BGL_AVERAGE:
            persist_write_int(SET_BGL_AVERAGE, data->value->int32);
            config.bgl_average = data->value->int32;
            trend_draw();
            break;
        case SET_BGL_HIGH:
            persist_write_int(SET_BGL_HIGH, data->value->int32);
            config.bgl_high = data->value->int32;
            trend_draw();
            break;
        case SET_BGL_CRITICAL:
            persist_write_int(SET_BGL_CRITICAL, data->value->int32);
            config.bgl_critical = data->value->int32;
            trend_draw();
            break;
        default:
            DEBUG("Not a trend key: %d", data->key);
            break;
    }
}


void trend_set_high_line(comm_high_limit value) {
    DEBUG("High line limit: %hd %hd", value.high_line, value.high_limit);
    config.bgl_high_line = value.high_line;
    config.bgl_high_limit = value.high_limit;
    persist_write_int(SET_HIGH_LINE_VALUE, config.bgl_high_line);
    persist_write_int(SET_HIGH_LIMIT, config.bgl_high_limit);
    trend_draw();
}

void trend_set_low_line(comm_low_limit value) {
    DEBUG("Low line limit: %hd %hd", value.low_line, value.low_limit);
    config.bgl_low_line = value.low_line;
    config.bgl_low_limit = value.low_limit;
    persist_write_int(SET_LOW_LINE_VALUE, config.bgl_low_line);
    persist_write_int(SET_LOW_LIMIT, config.bgl_low_limit);
    trend_draw();
}

int trend_isinitialized(void) { return config.bgl.initialized; }
