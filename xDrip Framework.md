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
1. Index 1000 - A uint 32 bit set of flags that include:
	- Bit 0 - Colour - True if the watch has a colour display, false if otherwise.
	- Bit 1 - Time Series Data - If true, xDrip will send time series data to the watch for it to draw the trend. Otherwise xDrip will send a PNG of the trend composed and sized as requested.
	- Bit 2 to 3  - Time Period - For time series or trend PNG, the time period to send.  The values will be:
		1. 00 - 1 hour
		2. 01 - 2 hour
		3. 10 - 3 hour
		4. 11 - 4 hour

	- Bit 4 - High limit - True will either add the High limit line to graph, or send the High Limit value (HIGHVAL) to the watch.  Otherwise no High line is added to the graph and no High value is sent.
	- Bit 5 - Low limit - True will either add the Low limit line to graph, or sends the Low Limit value (LOWLIMIT) to the watch.  Otherwise no Low line is added to the graph and no Low value is sent.
	- Bit 6 - Small dots - True will create the image with small dots rather than the larger ones.  Only relevant and used if the Time Series Data flag is false.
 	- Bit 7 - Send IOB data - True will cause xDrip to send IOB value.  TBD
    - Bit 8 - Send Pump State - True will have xDrip send the pump status.  TBD
    - Bit 9 - Send Phone Battery (PHONEBAT) - True will send the Phone battery level.
    - Bit 10 - Send Pump Battery - TBD.  True will send the Pump battery level.  Note, while it could be more efficient to have one bit to select the phone OR the pump, some people may want both sent.
    - Bit 11 - Send Delta value (BGL)DELTA)
    - Bit 12 - Send slope arrow value - True will cause xDrip to send a value related to the slope arrows indicated on it's home screen.  (SLOPEVAL)
	- Bit 13 to 31 -  Not yet allocated.
   
2.  Index 1001 - A 16 bit integer describing the dimensions of the PNG image required by the watch.  This allows variations and more easily integrates with the Round watches.  This will only be sent IF Time Series Data is false.
The Most Signigicant Byte will hold the Trend width, while the Least Significant Byte will hold the Trend height.

3.  Index 1002 - An 8 but integer representing control flags sent from the watch.
	- Bit 1 - Snooze Active Alert.
	- Bit 2 to 8 - Not yet allocated.

## Data sent from xDrip to the watch face/app
The data sent to the watch will consist of a series of messages, depending on what the watch has requested.
### BGL data
xDrip will send the current BGL, Delta, and Timestamp of the reading.  The watch will at the very least display these values.
The dictionary will be the following:
|Key Name	| Index	| Type 		| Description|
|-----------|-------|-----------|------------|
|BGL_TIME	| 0 	| uint32 	| The current timestamp of this BGL reading.|
|BGL_VALUE	| 1 	| uint16 	| The BGL reading in mg/dl.  Values 40-400.  Note, the MSbit will indicate if this should be displayed as mmol/l, so the values will be 32808-11810880 |
|BGL_DELTA	| 2 	| int8		| The delta from this reading to the previous reading in mg/dl.  This is only sent IF the watch face/app has requested it.  If the BGL_VALUE has the MSbit set, this will display in mmol/l.  Slope arrows, if displayed, will be determined by this value in the watch face or app.|

Note, this could also be used to send any BGL reading for any time frame, but it is probably more efficient to fill the BGL trend buffer through a different means, rather than a series of these.

Additional data that can be sent as part of any message from xDrip:
|Key Name	| Index	| Type 		| Description |
|-----------|-------|-----------|-------------|
|PHONEBAT	| 3		| uint8		| Phone battery percent (0-100 value expected) |
|MESSAGE	| 4		| char[]	| Message string to display on the face/app. |
|HIGHLIMIT	| 5		| uint16	| High Alert Limit in mg/dl.  Values 40-400. Only sent if the watch indicates it wants it.|
|LOWLIMIT	| 6`	| uint16	| Low Alert Limit in mg/dl.  Values 40-400.	 Only sent if the watch indicates it wants it.|
|VIBE		| 7		| uint8		| Vibration pattern to initiate. | 
|SLOPEVAL	| 8		| uint8		| Value indicating the sope arrow(s) to display.  0= No arrow, 1 = Double Up, 2 = Up, 3 = 45 Up , 4 = Flat arrow, 5 = 45 Down, 6 = Down, 7 = Double down, 8 = Error, 9 = Out of Range/No Signal

Note, just because there is a dictionary index described, it does not mean the index has to be added to a dictionary for sending.  These are simply Key/Value pairs in the dicitonary, so if a key is missing, it is not missed by xDrip or the watch face/app.

### Trend Image (optional)
The watch, if it has requested a PNG trend image, will then receive chunks of the image to reconstitute and display.  
This will be the size requested and with the various options requested, and for the time period requested.

### Trend Time Series (optional)
The trend series will be sent to the watch only when requested, say initially to fill up the watch trend buffer.  
The watch trend buffer will be a FIFO, so every new individual reading will cause the watch to add it to the trend buffer in time order (newest to oldets) and regenerate the trend display.

## Data semt from the watch face/app to xDrip.
The watch face/app will obviously send the heartbeat message described above to xDrip.  However, there are other pieces of information that could be of use for xDrip to get from the watch face/app.
1. Alarm silence/snooze
2. Carbohydrate intake
3. Insulin dose.  (could this tell the pump to do this?  I'm not a pumper so don't know)
4. Temporary Basal adjustment?  (I'm not a pumper, so not sure)

