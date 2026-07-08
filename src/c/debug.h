#ifdef __DEBUG_H__
#define __DEBUG_H_
/*
 * Debug helper macros
 */

// Scope debug to cleanup debug ifdefs

#define DEBUG_APP_TRACE 3
#define DEBUG_APP_DEBUG 2
#define DEBUG_APP_INFO 1
#define DEBUG_APP_NONE 0

/*  
 *  The line below will set the debug message level.
 *  Make sure you set this to 0 or DEBUG_APP_NONE before building a release. 
 *  Aplite will not build due to limited .text size with DEBUG_APP_TRACE, you can only enable info logging. 
 *
 *  The pebble logging system uses enums and a variable to define logging making it always compile all debug
 *  data into the app, which is problematic on the versions with smaller app footprint size (24k for aplite,
 *  64k for others)
*/


/**
 * prevent aplite from not building in trace logging
 */
#if defined(DEBUG_LEVEL) && defined(PBL_PLATFORM_APLITE) && DEBUG_LEVEL >= DEBUG_APP_DEBUG
#pragma message "Lowering debug level to suit aplite"
#undef DEBUG_LEVEL
#define DEBUG_LEVEL DEBUG_APP_INFO
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

#define WARNING(...) APP_LOG(APP_LOT_LEVEL_WARNING, __VA_ARGS__)
#define ERROR(...) APP_LOG(APP_LOT_LEVEL_WARNING, __VA_ARGS__)

#endif // __DEBUG_H__ 
