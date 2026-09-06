#ifndef __DEBUG_H__
#define __DEBUG_H__
/*
 * Debug helper macros
 */

/**
 * Defines for testing modes
 */

/* The line below, if defined, will only indicate test values on the display.
this is for testing purposes only until I can get the PebbleKit.JS code operating with the emulator.
Make sure you udefine this before building a release.
*/
/* #define TEST_MODE */


/*
 * Set debug text to show and compile DEBUG_APP_[NONE,INFO,DEBUG,TRACE]
 * Note: Aplite will not go above INFO logging.
 */
#define DEBUG_APP_TRACE 3
#define DEBUG_APP_DEBUG 2
#define DEBUG_APP_INFO 1
#define DEBUG_APP_NONE 0

/* #define DEBUG_LEVEL DEBUG_APP_TRACE */

/*  
 *  The line below will set the debug message level.
 *  Make sure you set this to 0 or DEBUG_APP_NONE before building a release. 
 *  Aplite will not build due to limited .text size with DEBUG_APP_TRACE, you can only enable info logging. 
 *
 *  The pebble logging system uses enums and a variable to define logging making it always compile all debug
 *  data into the app, which is problematic on the versions with smaller app footprint size (24k for aplite,
 *  64k for others)
*/

// guard againast debug in release, -DPBL_DEBUG is added to debug builds
// disabled due to running with --debug crashes PNG decoding
/* #if !defined(PBL_DEBUG) */
/* #undef DEBUG_LEVEL */
/* #endif */

/**
 * prevent aplite from not building in trace logging
 */
#if defined(DEBUG_LEVEL) && defined(PBL_PLATFORM_APLITE) && DEBUG_LEVEL >= DEBUG_APP_DEBUG
#pragma message "Lowering debug level to suit aplite"
#undef DEBUG_LEVEL
#endif

#if DEBUG_LEVEL >= 3
#define TRACE(...)  APP_LOG(APP_LOG_LEVEL_DEBUG_VERBOSE, __VA_ARGS__)
#else
#define TRACE(...)
#endif

#if DEBUG_LEVEL >= 2
#define DEBUG(...) APP_LOG(APP_LOG_LEVEL_DEBUG, __VA_ARGS__)
#else
#define DEBUG(...)
#endif

#if DEBUG_LEVEL >= 1
#define INFO(...) APP_LOG(APP_LOG_LEVEL_INFO, __VA_ARGS__)
#else
#define INFO(...)
#endif

#if defined(DEBUG_LEVEL)
#define LOG(...) APP_LOG(APP_LOG_LEVEL_INFO, __VA_ARGS__)
#else
#define LOG(...)
#endif

#define WARNING(...) APP_LOG(APP_LOG_LEVEL_WARNING, __VA_ARGS__)
#define ERROR(...) APP_LOG(APP_LOG_LEVEL_WARNING, __VA_ARGS__)

#endif // __DEBUG_H__ 
