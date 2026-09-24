#include <pebble.h>
#include "debug.h"

char *translate_app_error(AppMessageResult result)
{
	switch (result)
	{
		case APP_MSG_OK:
			return "APP_MSG_OK";
		case APP_MSG_SEND_TIMEOUT:
			return "APP_MSG_SEND_TIMEOUT";
		case APP_MSG_SEND_REJECTED:
			return "APP_MSG_SEND_REJECTED";
		case APP_MSG_NOT_CONNECTED:
			return "APP_MSG_NOT_CONNECTED";
		case APP_MSG_APP_NOT_RUNNING:
			return "APP_MSG_APP_NOT_RUNNING";
		case APP_MSG_INVALID_ARGS:
			return "APP_MSG_INVALID_ARGS";
		case APP_MSG_BUSY:
			return "APP_MSG_BUSY";
		case APP_MSG_BUFFER_OVERFLOW:
			return "APP_MSG_BUFFER_OVERFLOW";
		case APP_MSG_ALREADY_RELEASED:
			return "APP_MSG_ALREADY_RELEASED";
		case APP_MSG_CALLBACK_ALREADY_REGISTERED:
			return "APP_MSG_CALLBACK_ALREADY_REGISTERED";
		case APP_MSG_CALLBACK_NOT_REGISTERED:
			return "APP_MSG_CALLBACK_NOT_REGISTERED";
		case APP_MSG_OUT_OF_MEMORY:
			return "APP_MSG_OUT_OF_MEMORY";
		case APP_MSG_CLOSED:
			return "APP_MSG_CLOSED";
		case APP_MSG_INTERNAL_ERROR:
			return "APP_MSG_INTERNAL_ERROR";
		case APP_MSG_INVALID_STATE:
			return "APP_MSG_INVALID_STATE";
		default:
			return "APP UNKNOWN ERROR";
	}
}

#if defined(DEBUG_LEVEL) && DEBUG_LEVEL >= DEBUG_LEVEL_INFO
char *translate_dict_error(DictionaryResult result)
{
	switch (result)
	{
		case DICT_OK:
			return "DICT_OK";
		case DICT_NOT_ENOUGH_STORAGE:
			return "DICT_NOT_ENOUGH_STORAGE";
		case DICT_INVALID_ARGS:
			return "DICT_INVALID_ARGS";
		case DICT_INTERNAL_INCONSISTENCY:
			return "DICT_INTERNAL_INCONSISTENCY";
		case DICT_MALLOC_FAILED:
			return "DICT_MALLOC_FAILED";
		default:
			return "DICT UNKNOWN ERROR";
	}
}
#endif
