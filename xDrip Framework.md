# Pebble Framework for xDrip
## Introduction

This document describes a framework for Pebble App/Watch face communication with xDrip.  Why?
* Allows the App/Watch face to identify itself to xDrip so xDrip knows what to send it.
* Allows watch faces to be sourced through the Pebble store, allowing faster and easier releases and bug fixes without reliance on getting them into the main xDrip code base and releases.
* Removes the reliance of code changes/additions to xDrip in order for end users to use the app/watch face.
* Allows expansion of the framework to add things like insulin doses, carb intake, pump controls, etc from the Pebble.

# Basic Overview
The idea for this framework was to allow the reliance on complex changes to xDrip code base to implement a new Pebble app or watch face.  Currently, all xDrip watch faces use the same UUID, which is less than ideal.  The idea would be to move the UUID to an xDrip setting so the user can advise xDrip which watch face or app is in use, OR simply include the watch face or app UUID in a settings selection (maximum effort for a Pebble developer to get their integration with xDrip done is to add their UUID to selection list).  All communications to outbound to the watch would then be unified.  The watch will request the components it wants, and how it wants them, over and above the basic BGL, Delta, and Timestamp.

## Communications Flow
* xDrip will not send, or attempt to send, any data to a Pebble watch until it first receives an initial communication from the watch. Note, the same packet will be sent from the watch any time it requires/requests an update.  This essentially becomes a heartbeat from the watch.
* The initial communication will identify the watch capabilities and expected format of any additional data.
* Once the initial communications from the watch are received, xDrip will send the initial data set to the watch, and then regularly after every new BGL reading, or when the watch requests and update.
* The watch may send other information for processing by xDrip, such as insulin doses, carb intake, pump controls, etc.

## Heartbeat Description
The heartbeat will occur when the Pebble face/app starts, and then at regular intervals (ideally at 6 minute intervals) unless data is received from xDrip.  This allows the watch face/app "prompt" xDrip for an update IF nothing is sent within the usual expected 5 minutes per reading from the CGM.
The content of the heartbeat will be:
1. Index 0 - A uint 16 value representing the Framework protocol version.  While xDrip will always support the latest version, some watch faces/apps may not keep up.  So, telling xDrip which version the watch can handle may help the back end code.  Most of the protocol will be defined by the the flags in the next dictionary entry.  This will only increment when the dictionary in the framework changes and the watch face/app supports the new dictionary.  It is not expected to change all that often, only when new data or capabilties are added to the framework.
2. Index 1 - A uint 16 bit set of flags that include:
	1. Bit 0 - Colour - True if the watch has a colour display, false if otherwise.
	2. Bit 1 - Time Series Data - If true, xDrip will send time series data to the watch for it to draw the trend. Otherwise xDrip will send a PNG of the trend composed and sized as requested.
	3. Bit 2 to 4  - Time Period - For time series or trend PNG, the time period to send.  This will comprise 3 bits.  the values will be:
		1. 0 - 1 hour
		2. 1 - 3 hour
		3. 2 - 6 hour
		4. 3 - 12 hour
		5. 4 - 24 hour
	6. Bit 5 - Graph High/Low limit lines - True adds the limit lines to graph, otherwise no lines added.  Only relevant and used if the Time Series Data flag is false.
	7. Bit 6 - Small dots - True will create the image with small dots rather than the larger ones.  Only relevant and used if the Time Series Data flag is false.
 	8. Bit 7 - Send IOB data - True will cause xDrip to send IOB value.
  	9. Bit 8 - Send Pump State - True will have xDrip send the pump status.
   10. Bit 9 - Send Phone Battery - True will send the Phone battery level.
   11. Bit 10 - Send Pump Battery - True will send the Pump battery level.  Note, while it could be more efficient to have one bit to select the phone OR the pump, some people may want both sent.
   12. Bit 11 - Not yet allocated.
   13. Bit 12 - Not yet allocated.
   14. Bit 13 - Not yet allocated.
   15. Bit 14 - Not yet allocated.
   16. Bit 15 - Not yet allocated.
8. 32 bit integer describing the dimensions of the PNG image required by the watch.  This allows variations and more easily integrates with the Round watches.  This will only be sent IF Time Series Data is false.

## Data sent from xDrip to the watch face/app
The data sent to the watch will consist of a series of messages, depending on what the watch has requested.
### Basic Data
The first message sent to the watch will always be the current BGL, Delta, and Timestamp of the reading.  The watch will at the very least display these values.
The dictionary will be the following:
|Key Name| Index | Description|
|---|---|---|
|CGM_ICON_KEY |0	| TUPLE_CSTRING, MAX 2 BYTES.  Displays an icon showing states from xDrip, such as Sensor Stopped or other error icons/states. (Note, this may change as it is a hangover from the legacy Nightscout work.  Still useful, but may not be relevant these days)|
|CGM_BG_KEY |1	| TUPLE_CSTRING, MAX 4 BYTES.  The current BGL value formatted in mg/dl or mmol/l depending on the xDrip settings.|
|CGM_TCGM_KEY |2 | TUPLE_INT, 4 BYTES (CGM TIME).  Indicates ?|
|CGM_TAPP_KEY |3 | TUPLE_INT, 4 BYTES (APP / PHONE TIME).  Current time from the phone from xDrip.  Not sure this is required anymore.|
|CGM_DLTA_KEY |4 | TUPLE_CSTRING, MAX 5 BYTES (BG DELTA, -100 or -10.0). The current BG Delta from xDrip formatted in either mg/dl or mmol/l depending on the settings in xDrip.|
|CGM_UBAT_KEY |5 | TUPLE_CSTRING, MAX 3 BYTES (UPLOADER BATTERY, 100). A hangover from Nightscout that send the phone battery to the watch.  A text value from 0-100.|
|CGM_NAME_KEY |6 | TUPLE_CSTRING, MAX 9 BYTES (Christine).  A hangover from Nightscout, not used in xDrip and should probably be removed or repurposed.|
|CGM_TREND_BEGIN_KEY |7	| TUPLE_INT, 4 BYTES (length of CGM_TREND_DATA_KEY).  Essenmtially the number of "chunks" of either the PNG OR the Timestamped data table being sent to the watch.|
|CGM_TREND_DATA_KEY |8 | TUPLE_BYTE[], No Maximum, based on value found in CGM_TREND_DATA_KEY.  The "chunk" number being transferred.|
|CGM_TREND_END_KEY |9 | TUPLE_INT, always 0.  This marks the end of the trend data transfer.|
|CGM_MESSAGE_KEY |10 | A small text string from xDrip to display on the watch.  This may now be redundant and could be repurposed.  Used to display the "BAZINGA!" at 5.5mmol/l or 100 mg/dl.|
|CGM_VIBE_KEY |11 | The vibration pattern to use when an alert situation exists with xDrip.  May now be redundant and could be repurposed.|
|CGM_LOW_ALERT_VALUE |12 | The value of the Low display alert.  Only sent if the watch is requesting time series data to be sent and allows the graph to display this line.|
|CGM_HI_ALERT_VALUE |13 | The value of the HIgh display alert.  Only sent if the watch is requesting time series data to be sent and allows the graph to display this line.|
|PBL_TREND_SIZE	|1003 | key pebble will use to send trend image size to xDrip to get the size it wants to display.|
|PBL_TREND_LINES |1004 |key pebble will use to send trend line options.|
|PBL_DISP_OPTS |1005 | key pebble will use to send display options (delta/arrows).  Not sure this will be required, it has been too many years since I formulated the dictionary and this could already be covered in the above.|

Note, just because there is an dictionary index described, it does not mean the index has to be added to a dictionary for sending.  These are simply Key/Value pairs in the dicitonary, so if a key is missing, it is not missed by xDrip or the watch face/app.

### Battery Data
### Trend Image (optional)
The watch, if it has requested a PNG trend image, will then receive chunks of the image to reconstitute and display.  This will be the size requested and with the various options requested, and for the time period requested.
### Trend Series (optional)
The trend series will be sent to the watch only when requested, say initially to fill up the watch trend buffer.  This watch trend buffer will be a FIFO, so every new individual reading will cause the watch to add it to the trend buffer and regenerate the trend.

## Data semt from the watch face/app to xDrip.
The watch face/app will obviously send the heartbeat message described above to xDrip.  However, there are other pieces of information that could be of use for xDrip to get from the watch face/app.
1. Carbohydrate intake
2. Insuling dose.  (could this tell the pump to do this?  I'm not a pumper so don't know)
3. Temporary Basal adjustment?  (I'm not a pumper, so not sure)

