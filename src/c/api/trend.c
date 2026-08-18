/**
 *
 * Use as is. No guarantees.
 */
#include <pebble.h>

#include "../xdrip.h"
#include "../debug.h"
#include "communication.h"

#include "trend.h"

/**
 * File contains functions for drawing the trend image from a list of BGL values vs time
 *
 * Options/constraints:
 * - Draw only lates <n>, e.g. if you have 500 items draw only <pixel width of draw box>
 * - Draw all and aliase
 * - Color/gray(scale)
 * - Enable/disable high/low line
 * - Set colors
 * - Set line colors
 */

static void trend_layer_callback(Layer *layer, GContext *ctx);

trend_config *config = NULL;

void trend_set_config(trend_config *cfg) {
    config = cfg;
    if (cfg == NULL) {
        LOG("NULL configuration! Graph is disabled");
        return;
    }
    TRACE(TREND_LOG "Setting callback");
    layer_set_update_proc((Layer *) config->layer, trend_layer_callback);
}


static inline void draw_bgl_point(trend_bgl_value value, int16_t x, trend_config *config, GRect bounds, GContext *ctx) {
    /**
     * Since the trend image is just a graph, we do not need to know the 
     * actual type of data
     */

    if (value < config->bgl_low_limit || value > config->bgl_high_limit) return;

    GColor color = config->good_color;

    if (value > config->bgl_high) color = config->high_color;
    else if (value > config->bgl_average) color = config->average_color;
    else if (value < config->bgl_low) color = config->low_color;

    graphics_context_set_stroke_color(ctx, color);
    
    GPoint point = {x, BGL_TO_Y(value, config, bounds)};

    graphics_draw_circle(ctx, point, 0);

}

static inline void draw_bgl_line(trend_bgl_value value, trend_bgl_value value2, int16_t x1, int16_t x2, trend_config *config, GRect bounds, GContext *ctx) {
    /**
     * Since the trend image is just a graph, we do not need to know the 
     * actual type of data
     */

    // ignore zero/extreme values, single high value will stay to allow line into the graph
    if (value < config->bgl_low_limit || value2 < config->bgl_low_limit) return;
    if (value2 > config->bgl_high_limit && value2 > config->bgl_high_limit) return;

    GColor color = config->good_color;

    if (value > config->bgl_high) color = config->high_color;
    else if (value > config->bgl_average) color = config->average_color;
    else if (value < config->bgl_low) color = config->low_color;

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

static bool draw_trend(trend_config *config, Layer *layer, GContext *ctx) {
    graphics_context_set_stroke_width(ctx, config->trend_width); // constant

    GRect bounds = layer_get_bounds(layer);

    bool interp = config->style == TREND_STYLE_DOTS ? (bounds.size.w > config->bgl.size) : false; // do not interp on lines 
    int32_t t = 0;
    int32_t interval = ((config->bgl.size - 1) << 16) / bounds.size.w;
    int index = 0;
    TRACE(TREND_LOG "Size: %d array %d ", bounds.size.w, config->bgl.size);
    TRACE(TREND_LOG "Interp settings: [%d] :: %d", interp, interval);

    if (config->style == TREND_STYLE_DOTS) {
        // simplified
        TRACE(TREND_LOG "Doing interpolation"); 
        for (int i = 0; i < bounds.size.w; i++) {
            if (interp) {
                int16_t y0 = lerp(
                        config->bgl.values[(config->bgl.index + index) % (config->bgl.size)], 
                        config->bgl.values[(config->bgl.index + index + 1) % (config->bgl.size)], 
                        t);
                t += interval;
                if (t >= (1 << 16)) index++;
                draw_bgl_point(y0, i, config, bounds, ctx);
                t %= 1 << 16;
            } else {
                draw_bgl_point(config->bgl.values[(config->bgl.index + i) % (config->bgl.size)], i, config, bounds, ctx); 
            }
        }
    } else if (config->style == TREND_STYLE_LINES) {
        if (interp) {
            for (int i = 0; i < bounds.size.w; i++) {
                if (interp) {
                    int16_t y0 = lerp(
                            config->bgl.values[(config->bgl.index + index) % (config->bgl.size)], 
                            config->bgl.values[(config->bgl.index + index + 1) % (config->bgl.size)], 
                            t);
                    int16_t y1 = lerp(
                            config->bgl.values[(config->bgl.index + index) % (config->bgl.size)], 
                            config->bgl.values[(config->bgl.index + index + 1) % (config->bgl.size)], 
                            t += interval);
                    /* TRACE(TREND_LOG "Line %d %d %d %d %d %d %d", */
                    /*         config->bgl.size, config->bgl.index, index, */
                    /*         y0, y1,  */
                    /*         config->bgl.values[(config->bgl.index + index) % (config->bgl.size)],  */
                    /*         config->bgl.values[(config->bgl.index + index + 1) % (config->bgl.size)]); */
                    draw_bgl_line(y0, y1, i, i+1, config, bounds, ctx);
                    if (t >= (1 << 16)) {
                        index++;
                        t %= 1 << 16;
                        /* TRACE(TREND_LOG "Index: %hu of %hu, %hu", index, config->bgl.size, t); */
                    }
                }
            }
        } else {
            // currently forced path
            uint32_t t = (bounds.size.w << 16) / (config->bgl.size - 1); 
            for (uint32_t i = 0, j = 0; i < ((uint32_t) bounds.size.w << 16); i += t, j++) {
                draw_bgl_line(
                        config->bgl.values[(config->bgl.index + j) % (config->bgl.size)],
                        config->bgl.values[(config->bgl.index + j + 1) % (config->bgl.size)],
                        i >> 16, (i+t) >> 16, config, bounds, ctx); 
            }
        }
    }
    return true;
}

static bool draw_trend_lines(trend_config  *config, Layer *layer, GContext *ctx) {
    // top is 0,0
    GRect bounds = layer_get_bounds(layer);
    const int16_t h = BGL_TO_Y(config->bgl_high_line, config, bounds); 
    const int16_t l = BGL_TO_Y(config->bgl_low_line, config, bounds); 
    TRACE(TREND_LOG "Draw lines, high: %d, low %d", h, l);

    // drawing
    graphics_context_set_stroke_width(ctx, config->line_width);
    int w = 0, s = 0;
    switch (config->line_style) {
        default:
        case TREND_LINE_STYLE_SOLID:
            graphics_context_set_stroke_color(ctx, COLOR_FALLBACK(config->high_line_color, GColorWhite));
            graphics_draw_line(ctx, (GPoint) { 0, h }, (GPoint) { bounds.size.w, h});
            graphics_context_set_stroke_color(ctx, COLOR_FALLBACK(config->low_line_color, GColorWhite));
            graphics_draw_line(ctx, (GPoint) { 0, l }, (GPoint) { bounds.size.w, l});
            break;
        case TREND_LINE_STYLE_DOTTED_SPARSE:
            s = 1;
            // fall through
        case TREND_LINE_STYLE_DOTTED:
            s += 2;
            graphics_context_set_stroke_color(ctx, COLOR_FALLBACK(config->high_line_color, GColorWhite));
            for (int x = 0; x < bounds.size.w; x+=s) {
                graphics_draw_pixel(ctx, (GPoint) { x, h });
            }
            graphics_context_set_stroke_color(ctx, COLOR_FALLBACK(config->low_line_color, GColorWhite));
            for (int x = 0; x < bounds.size.w; x+=s) {
                graphics_draw_pixel(ctx, (GPoint) { x, l });
            }
            break;
        case TREND_LINE_STYLE_DASHED_WIDE:
            s = 5;
            w = 2;
            // fall through
        case TREND_LINE_STYLE_DASHED:
            s += 5;
            w += 2;
            graphics_context_set_stroke_color(ctx, COLOR_FALLBACK(config->high_line_color, GColorWhite));
            for (int x = 0; x < bounds.size.w; x+=s) {
                graphics_draw_line(ctx, (GPoint) { x, h }, (GPoint) { x+w, h});
            }
            graphics_context_set_stroke_color(ctx, COLOR_FALLBACK(config->low_line_color, GColorWhite));
            for (int x = 0; x < bounds.size.w; x+=s) {
                graphics_draw_line(ctx, (GPoint) { x, l }, (GPoint) { x+w, l});
            }
            break;
    }
    return true;
}

void trend_layer_callback(Layer *layer, GContext *ctx) {
    if (config == NULL) {
        LOG("No trend configuration set, do nothing");
        return; 
    }

    TRACE(TREND_LOG "Drawing trend line");
    draw_trend(config, layer, ctx);
    TRACE(TREND_LOG "Drawing high/low lines");
    draw_trend_lines(config, layer, ctx);
    config->redraw = 0; 
}

void trend_draw(void) {
    if (config == NULL) {
        INFO(TREND_LOG "No trend configuration set, do nothing");
        return;
    }
    DEBUG(TREND_LOG "Marking trend layer dirty");
    config->redraw = 1;
    layer_mark_dirty(config->layer);
}


/*
 * Update functions
 */

void trend_set_series(comm_bgl_series *values) {
    TRACE(TREND_LOG "Trend set series"); 
    if (config == NULL) {
        INFO("No config set");
        return;
    }
    if (!config->bgl.initialized) {
        DEBUG("Initializing");
        config->bgl.size = values->length > PBL_DISPLAY_WIDTH ? PBL_DISPLAY_WIDTH : values->length;
        config->bgl.index = 0;
        config->bgl.initialized = 1;
        memset(config->bgl.values, 0x00, sizeof(config->bgl.values));
        config->bgl_type = values->bgl_values[0].is_mmol ? BGL_TYPE_MMOL_L : BGL_TYPE_MG_DL;
    }
    TRACE("Parsing data: %d", values->length);
    for (int i = 0; i < values->length && i < PBL_DISPLAY_WIDTH; i++) {
        TRACE("TREND DATA: %d %d", i, values->bgl_values[i].value);
        config->bgl.values[config->bgl.index % (config->bgl.size)] = values->bgl_values[i].value;
        config->bgl.index++;
        config->bgl.index = config->bgl.index % (config->bgl.size);
    }
    trend_draw();
}

void trend_set_value(comm_bgl_data *value) {
    TRACE(TREND_LOG "Trend set value");
    if (config == NULL) return;
    config->bgl.values[config->bgl.index % (config->bgl.size - 1)] = value->bgl.value;
    config->bgl.index++;
    config->bgl.index = config->bgl.index % (config->bgl.size - 1);
    trend_draw();

}
