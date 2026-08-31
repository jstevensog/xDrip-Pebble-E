#ifndef __COMMUNICATION_H__
#define __COMMUNICATION_H__

#include <stdint.h>
#include <pebble.h>

/**
 * Message codes for framework communication
 */
#define FRAMEWORK_HEARTBEAT         2000
#define FRAMEWORK_BGL_DELTA         2001
#define FRAMEWORK_BGL_VALUE         2002
#define FRAMEWORK_PHONEBAT          2003
#define FRAMEWORK_MESSAGE           2004
#define FRAMEWORK_HIGHLIMIT         2005
#define FRAMEWORK_LOWLIMIT          2006
#define FRAMEWORK_VIBE              2007
#define FRAMEWORK_SLOPEVAL          2008 
#define FRAMEWORK_BGL_SERIES        2009
#define FRAMEWORK_PNG_IMAGE         2010
#define FRAMEWORK_BWP_VALUE         2011
#define FRAMEWORK_SENSOR_TIME_LEFT  2012
// watch->phone: current health metrics, sent as plain uint32 values so the
// phone side can decode them with PebbleKit without a struct layout. A zero
// value means "not available" and is not written. See comm_send_health().
#define FRAMEWORK_HEALTH_HR         2013
#define FRAMEWORK_HEALTH_STEPS      2014

/**
 * Struct definitions for communication
 */
#pragma pack(1)
/**
 * "old" trend size, note: endianess is BE, so some
 * conversion has to be done
 */
typedef union comm_trend_size_t {
    struct {
        uint32_t width : 14;
        uint32_t height : 14;
        uint32_t : 3;
        uint32_t rgb8 : 1;              // Send RGB8 png
    };
    uint32_t raw;                       // Data Blob
} comm_trend_size;

typedef struct comm_trend_request_t {
    uint32_t from;
} comm_trend_request;

/**
 * Heartbeat data request indicator
 * Note: transport endianess is BE, LE on chip
 */
typedef union comm_heartbeat_t {
    struct {
        // B2-3
        uint32_t : 16;                  // RFU
        // B1
        uint32_t : 3;
        uint32_t send_slope_arrow : 1;  // Send slope arrow value
        uint32_t send_delta_value : 1;  // Send delta value
        uint32_t send_pump_battery : 1; // Send battery state
        uint32_t send_phone_battery : 1; // 
        uint32_t send_pump_state : 1;   // Send pump state
        // B0
        uint32_t send_iob : 1;          // Send IOB data
        uint32_t small_dots : 1;        // Use smal dots (1) or larger (0), PNG only
        uint32_t low_limit : 1;         // Add low limit line or send low limit line value
        uint32_t high_limit : 1;      // Add height line or send height line value
        uint32_t time_period : 2;       // Time period (0b00 = 1h, 0b01 = 2h, 0b10 = 3h, 0b11 = 4h)
        uint32_t time_series : 1;       // Send time series (1) or PNG (0)
        uint32_t colour : 1;            // Is pebble colour capable
    };
    uint32_t    raw;
} comm_heartbeat;

/**
 * measurement range is 40-400, 9 bits is enough
 */
typedef struct comm_bgl_value_t {
    uint16_t        value : 15;     // bgl in mg/dl
    uint16_t        is_mmol : 1;    // display value as mmol/l
} comm_bgl_value;

typedef struct comm_bgl_data_t {
    uint32_t        timestamp;      // timestamp of bgl value
    comm_bgl_value  bgl;
} comm_bgl_data;

typedef union  comm_bgl_delta_t {
    struct {
        int8_t          value;
        uint8_t : 3;
        uint8_t         expired : 1;
        uint8_t         hidden : 1;
        uint8_t         undefined : 1; 
        uint8_t         display_units : 1;
        uint8_t         is_mmol : 1;
    };
    uint16_t raw;
} comm_bgl_delta;

typedef struct comm_bgl_series_t {
    uint32_t        timestamp;      // current timestamp of last reading
    uint16_t        length;         // values to receive
    comm_bgl_value  bgl_values[];   // open ended array of values
} comm_bgl_series;

typedef uint8_t comm_phonebat;

typedef struct comm_message_t {
    uint32_t length;
    char    *message;
} comm_message;

typedef union  {
    struct {
        uint16_t high_line;
        uint16_t high_limit;
    };
    uint32_t raw;
} comm_high_limit;
typedef union {
    struct {
        uint16_t low_line;
        uint16_t low_limit;
    };
    uint32_t raw;
} comm_low_limit;
typedef uint8_t comm_vibe;
typedef uint8_t comm_slopeval;
typedef struct {
    uint16_t length;
    uint8_t  *data;
} comm_png_data;


typedef uint32_t comm_sensor_time_left;
typedef uint32_t comm_bwp_value;

typedef struct comm_health_t {
    uint16_t heart_rate;   // current heart rate in BPM, 0 = not available
    uint32_t steps;        // step count so far today, 0 = not available
} comm_health;

#pragma pack()

/*
 * Callback struct
 */
typedef struct comm_callback_t {
    void (*phonebat)(comm_phonebat value);
    void (*low_limit)(comm_low_limit value);
    void (*high_limit)(comm_high_limit value);
    void (*slopeval)(comm_slopeval value);
    void (*vibe)(comm_vibe value);
    void (*message)(comm_message message);
    void (*bgl_data)(comm_bgl_data *value);
    void (*bgl_series)(comm_bgl_series *value);
    void (*bgl_delta)(comm_bgl_delta value);
    void (*bgl_value)(comm_bgl_value value);
    void (*bgl_timestamp)(uint32_t timestamp);
    void (*bwp_value)(comm_bwp_value value);
    void (*sensor_time_left)(comm_sensor_time_left value);
    void (*png)(comm_png_data data);
    void (*health)(comm_health value);
} comm_callback;


void comm_init(comm_callback *cb);
void comm_handle(Tuple *data);
void comm_request_png(DictionaryIterator *iter, GRect bounds);
// Write the current heart rate / step total into an already-open outbox
// dictionary. Fields that are 0 are skipped. Kept here so any face can report
// health data without duplicating the key layout. Independent of send_cmd_cgm,
// which is not guaranteed to run under the framework.
void comm_send_health(DictionaryIterator *iter, comm_health data);
#endif // __COMMUNICATION_H__
