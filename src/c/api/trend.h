#ifndef __TREND_H__
#define __TREND_H__

#include <pebble.h>
#include "communication.h"


#define TREND_LOG "TREND :: "

typedef int16_t trend_bgl_value; 

typedef enum {
    BGL_TYPE_MMOL_L = 0,
    BGL_TYPE_MG_DL = 1
} bgl_type;

typedef enum {
    TREND_STYLE_DOTS = 0,
    TREND_STYLE_LINES = 1
} trend_style;

typedef enum {
    TREND_LINE_STYLE_SOLID = 0,
    TREND_LINE_STYLE_DOTTED,
    TREND_LINE_STYLE_DOTTED_SPARSE,
    TREND_LINE_STYLE_DASHED,
    TREND_LINE_STYLE_DASHED_WIDE,
    TREND_LINE_STYLE_EDGES,
} trend_line_style;

typedef struct {
    int8_t  initialized;
    int16_t size;
    int16_t index;
    int8_t  hours;
    trend_bgl_value values[PBL_DISPLAY_WIDTH]; // maximum
} bgl_array;


typedef struct {
    bgl_type    bgl_type;           // Value style, no function as of yet 
    GColor      critical_color;     // Colour for critically high trend
    GColor      high_color;         // Colour for high trend
    GColor      average_color;      // Colour for average trend 
    GColor      good_color;         // Colour for good trend
    GColor      low_color;          // Colour for low trend
    GColor      high_line_color;    // Colour of high line
    GColor      low_line_color;     // Colour of low line
    GColor      hour_line_color;
    int8_t      hour_line_width;
    int8_t      hour_line_enabled;  
    int8_t      line_width;         // High/low line width
    int8_t      trend_width;        // Trend line width
    int16_t     bgl_low;            // Low value
    int16_t     bgl_average;        // Average value
    int16_t     bgl_high;           // High value
    int16_t     bgl_critical;       // Critical value
    int16_t     bgl_high_line;      // High line value
    int16_t     bgl_low_line;       // Low line value
    bool        show_high_line;     // Draw the high threshold line
    bool        show_low_line;      // Draw the low threshold line
    int16_t     bgl_high_limit;     // Gigh graph limit
    int16_t     bgl_low_limit;      // Low graph limit 
    Layer       *layer;             // Layer to draw into
    bgl_array   bgl;                // Storage for BGL values
    trend_style style;              // Trend line style
    trend_line_style hl_line_style;    // High/low line style
    trend_line_style hour_line_style;
    int8_t      redraw;             // Redraw value (not really working right now)
} trend_config;

/**
 * Called by main function to initialize values and load config from persistent storage
 *
 * @param Layer to draw in to
 */
void trend_init(Layer *layer);
void trend_deinit(void);

/**
 * Forse a redraw
 */
void trend_draw(void);

/**
 * Function to handle config items passed as tuples by the pebble app
 */
void trend_process_config(Tuple *data);

/**
 * Callback function to set the bgl series to use for drawing the trend line
 */
void trend_set_series(comm_bgl_series *values); 
/**
 * Callback function to add a single value to the bgl trend line
 */
void trend_set_value(comm_bgl_data *value);
void trend_set_high_line(comm_high_limit value);
void trend_set_low_line(comm_low_limit value);

int trend_isinitialized(void);

/**
 * convert bgl to y, respecting limits and bounds
 */
#define BGL_TO_Y(bgl, config, bounds) (\
            bounds.size.h - \
            ((int32_t) (bounds.size.h * (bgl - config.bgl_low_limit))) /\
            (config.bgl_high_limit - config.bgl_low_limit)\
        )

#endif
