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
} trend_line_style;

typedef struct {
    int8_t  initialized;
    int16_t size;
    int16_t index;
    trend_bgl_value values[PBL_DISPLAY_WIDTH]; // maximum
} bgl_array;


typedef struct {
    bgl_type    bgl_type;
    GColor      critical_color;
    GColor      high_color;
    GColor      average_color;
    GColor      good_color;
    GColor      low_color;
    GColor      high_line_color;
    GColor      low_line_color;
    int8_t      line_width;
    int8_t      trend_width;
    int16_t     bgl_low;
    int16_t     bgl_average;
    int16_t     bgl_high;
    int16_t     bgl_high_line;
    int16_t     bgl_low_line;
    int16_t     bgl_high_limit;
    int16_t     bgl_low_limit;
    Layer       *layer;
    bgl_array   bgl;
    trend_style style;
    trend_line_style line_style;
    int8_t      redraw;
} trend_config;

void trend_set_config(trend_config *cfg);
void trend_draw(void);
void trend_set_series(comm_bgl_series *values); 
void trend_set_value(comm_bgl_data *value);

/**
 * convert bgl to y, respecting limits and bounds
 */
#define BGL_TO_Y(bgl, config, bounds) (\
            bounds.size.h - \
            ((int32_t) (bounds.size.h * (bgl - config->bgl_low_limit))) /\
            (config->bgl_high_limit - config->bgl_low_limit)\
        )

#endif
