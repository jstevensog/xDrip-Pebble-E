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
The heartbeat will occur when the face/app starts, and then at regular intervals (ideally at 6 minute intervals unless data is received from xDrip).  The content of the heartbeat will be:
1. A uint 16 bit set of flags that include:
	1. Colour - True if the watch has a colour display, false if otherwise.
	2. Time Series Data - If true, xDrip will send time series data to the watch for it to draw the trend. Otherwise xDrip will send a PNG of the trend composed and sized as requested.
	3. Time Period - For time series or trend PNG, the time period to send.  This will comprise 3 bits.  the values will be:
		1. 0 - 1 hour
		2. 1 - 3 hour
		3. 3 - 6 hour
		4. 4 - 12 hour
		5. 5 - 24 hour
	6. Graph High/Low limit lines - True adds the limit lines to graph, otherwise no lines added.  Only relevant and used if the Time Series Data flag is false.
	7. Small dots - True will create the image with small dots rather than the larger ones.  Only relevant and used if the Time Series Data flag is false.
8. 32 bit integer describing the dimensions of the PNG image required by the watch.  This allows variations and more easily integrates with the Round watches.  This will only be sent IF Time Series Data is false.

## Data sent to watch
The data sent to the watch will consist of a series of messages, depending on what the watch has requested.  
### Basic Data
The first message sent to the watch will always be the current BGL, Delta, and Timestamp of the reading.  The watch will at the very least display these values.
### Trend Image (optional)
The watch, if it has requested a PNG trend image, will then receive chunks of the image to reconstitute and display.  This will be the size requested and with the various options requested, and for the time period requested.
### Trend Series (optional)
The trend series will be sent to the watch only when requested, say initially to fill up the watch trend buffer.  This watch trend buffer will be a FIFO, so every new individual reading will cause the watch to add it to the trend buffer and regenerate the trend.