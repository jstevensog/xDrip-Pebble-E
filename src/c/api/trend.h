#ifndef __TREND_H__
#define __TREND_H__

#include <pebble.h>
#include "communication.h"
/**
 * This is an example, values can be set to whatever suits your need
 * + -> value <= BGL_LOW
 * = -> BGL_LOW < value <= BGL_AVERAGE  
 * * -> BGL_AVERAGE < value <= BGL_HIGH
 * ^ -> BGL_HIGH < value <= BGL_CRITICAL
 * # -> value > BGL_CRITICAL
 *                            
 *  ----------------------------##---       -> High limit, CRICICAL_COLOUR (top of layer)
 *                            ##            -> BGL_CRITICAL, CRITICAL_COLOUR
 *  - - - - - - - - - - - - ^^- - - -       -> High line value, high line style, high line width, HIGH_COLOUR 
 *                        ^^                -> BGL_HIGH, HIGH_COLOR
 *                      **                  -> AVERAGE_COLOUR
 *                   **                     -> AVERAGE_COLOUR
 *                **                        -> AVERAGE_COLOUR
 *             **                           -> BGL_AVERAGE, AVERAGE_COLOUR
 *          ==                              -> GOOD_COLOUR
 *       ==                                 -> GOOD_COLOUR
 *     ==                                   -> BGL_LOW, GOOD_COLOUR
 *  -++ - - - - - - - - - - - - - - -       -> Low line value, low line style, low line width, LOW_COLOUR
 *  +                                       -> LOW_COLOUR
 *  ---------------------------------       -> Low limit (bottom of layer)
 */

/*
 * Settings from clay
 */
#define SET_USE_PNG                 117     // Set the use of PNG images from xDrip or local rendered
#define SET_SHOW_UNIT               118     // Show unit in delta screen    @deprecated
#define SET_SHOW_DELTA              119     // Show the delta               @deprecated
#define SET_SHOW_SLOPE              120     // Show the slope icon          @deprecated
#define SET_SHOW_TREND              121     // Show the trend               @deprecated
#define SET_BGL_CRITICAL_COLOUR     301     // Colour of the top of the trend line above SET_BGL_CRITICAL
#define SET_BGL_HIGH_COLOUR         302     // Colour of the high part of the trend line between SET_BGL_HIGH and SET_BGL_CRITICAL
#define SET_BGL_AVERAGE_COLOUR      303     // Colour of the middle part of the trend line between SET_BGL_AVERAGE and SET_BGL_HIGH
#define SET_BGL_GOOD_COLOUR         304     // Colour of the "good" part of the trend line between SET_BGL_LOW and SET_BGL_AVERAGE
#define SET_BGL_LOW_COLOUR          305     // Colour of the low part of the trend line below SET_BGL_LOW
#define SET_BGL_LOW                 306     // Value below which LOW_COLOUR is valid
#define SET_BGL_AVERAGE             307     // Value below which AVERAGE_COLOUR is valid
#define SET_BGL_HIGH                308     // Value below which HIGH_COLOUR is valid
#define SET_BGL_CRITICAL            309     // Value below which CRITICAL_COLOUR is valid
#define SET_LOW_LINE_COLOUR         310     // Colour of the low line if enabled in xDrip
#define SET_HIGH_LINE_COLOUR        311     // Colour of the high line if enabled in xDrip
#define SET_LINE_STYLE              312     // Low and high line style (see trend_line_style)
#define SET_LINE_WIDTH              313     // Width of the high and low line 
#define SET_TREND_STYLE             314     // Trend drawing style (see trend_style)
#define SET_TREND_WIDTH             315     // Width of the trend items (e.g. line width or circle radius)
#define SET_HIGH_LIMIT              316     // Highest displayable limit, set by xDrip
#define SET_HIGH_LINE_VALUE         317     // High line value, set by xDrip
#define SET_LOW_LIMIT               318     // Lowest displayable value, set by xDrip 
#define SET_LOW_LINE_VALUE          319     // Low line value, set by xDrip
#define SET_HOUR_ENABLED            320     // Enable hour line marks
#define SET_HOUR_WIDTH              321     // Set width of hour lines
#define SET_HOUR_STYLE              322     // Set hour style (see trend_line_style)
#define SET_AUTO_ADJUST_MAX         323     // Enable auto-scaling

#define TREND_LOG "TREND :: "

typedef int16_t trend_bgl_value; 

/*
 * Unused since the trend line is a dimensionless value as no axis is present
 */
typedef enum {
    BGL_TYPE_MMOL_L = 0,
    BGL_TYPE_MG_DL = 1
} bgl_type;

typedef enum {
    TREND_STYLE_DOTS = 0,       // Use dots/circles to draw the trend line 
    TREND_STYLE_LINES = 1       // Use lines to draw the trend line
} trend_style;

typedef enum {
    TREND_LINE_STYLE_SOLID = 0,             // Solid line
    TREND_LINE_STYLE_DOTTED,                // Dots with equal spaceing (2px)
    TREND_LINE_STYLE_DOTTED_SPARSE,         // Dots with larger spacing (4px)
    TREND_LINE_STYLE_DASHED,                // Dashes in repeating pattern
    TREND_LINE_STYLE_DASHED_WIDE,           // Dashes both wider and more spaced
    TREND_LINE_STYLE_EDGES,                 // Edge marks (20% of left/right or top/bottom)
} trend_line_style;

typedef struct {
    int8_t  initialized;                    // 0 if no series received, 1 afterwards 
    int16_t size;                           // Actual size of series (depends on update rate and hours) 
    int16_t index;                          // Current index of the series 
    int8_t  hours;                          // Number of hours displayed
    trend_bgl_value values[PBL_DISPLAY_WIDTH]; // Values, maximum is the display width
} bgl_array;


typedef struct {
    bgl_type    bgl_type;               // Value style, no function as of yet 
    GColor      critical_color;         // Colour for critically high trend
    GColor      high_color;             // Colour for high trend
    GColor      average_color;          // Colour for average trend 
    GColor      good_color;             // Colour for good trend
    GColor      low_color;              // Colour for low trend
    GColor      high_line_color;        // Colour of high line
    GColor      low_line_color;         // Colour of low line
    GColor      hour_line_color;        // Colour of the hour lines, currently fixed
    int8_t      hour_line_width;        // Width of the hour lines
    int8_t      hour_line_enabled : 1;  // Enable / disable hour lines
    int8_t      auto_adjust_max : 1;    // Enable / disable auto-adjust
    int8_t      : 6;
    int8_t      line_width;             // High/low line width
    int8_t      trend_width;            // Trend line width
    int16_t     bgl_low;                // Low value
    int16_t     bgl_average;            // Average value
    int16_t     bgl_high;               // High value
    int16_t     bgl_critical;           // Critical value
    int16_t     bgl_high_line;          // High line value
    int16_t     bgl_low_line;           // Low line value
    int16_t     bgl_high_limit;         // Gigh graph limit
    int16_t     bgl_low_limit;          // Low graph limit 
    Layer       *layer;                 // Layer to draw into
    bgl_array   bgl;                    // Storage for BGL values
    trend_style style;                  // Trend line style
    trend_line_style hl_line_style;     // High/low line style
    trend_line_style hour_line_style;   // Hour line style
    int8_t      redraw;                 // Redraw value (not really working right now)
} trend_config;

/**
 * Called by main function to initialize values and load config from persistent storage
 *
 * @param Layer to draw in to
 */

/**
 * Initialize the trend line, layer MUST be a drawable layer (e.g. bitmap layer)
 * @param layer     The layer to draw into
 */
void trend_init(Layer *layer);

/**
 * Deinit the trend line, note: due to how pebbleos works the update_proc
 * cannot be restored to the original value. Therefore a bitmap cannot be updated
 * with a PNG after using it as a trend line drawing.
 */
void trend_deinit(void);

/**
 * Forse a redraw of the trend line, this will mark the layer dirty
 */
void trend_draw(void);

/**
 * Function to handle config items passed as tuples by the pebble app
 * Call this function in your app message processing thread
 *
 * @param data  The tuple to process
 */
void trend_process_config(Tuple *data);

/**
 * Callback function to set the bgl series to use for drawing the trend line
 *
 * If this function is called for the first time it will set the size of the trend line.
 * @param values    The trend line values from xDrip+
 */
void trend_set_series(comm_bgl_series *values); 
/**
 * Callback function to add a single value to the bgl trend line
 *
 * @param value     The value to add
 */
void trend_set_value(comm_bgl_data *value);
/**
 * Set the high line and global high limit
 *
 * @param value     The value from xDrip+
 */
void trend_set_high_line(comm_high_limit value);

/**
 * Set the low line and global low limit
 *
 * @param value     The value from xDrip+
 */
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
