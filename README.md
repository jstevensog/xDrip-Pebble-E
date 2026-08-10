# xDrip-Pebble
Offline Pebble watchface for xDrip+, based on the Nightscout community version
Note: You require xDrip+ version later than January 2025 and Pebble application version 1.0.10.8 or later.  
And until further notice, or this face is added to xDrip+, please install the "Pebble Trend Clay Version (Test)" watch face before installing this on Colour Pebbles, or "Pebble Trend Classic" for monochrome Pebbles.
Otherwise you may be stuck at the LOADING... message as that code is required in xDrip to support this face.

As such, it is based on the cgm-pebble version 6.0 code.
There ARE NO EASTER EGGS, unless they are provided by xDrip.

This watch face is meant for patients to use with xDrip, not for parents/carers.
Ensure you enable notifications from xDrip in the Pebble App otherwise these will NOT come through on the Pebble.
Note:  The following settings are available from xDrip:
 * Display Trend - Turning on will display the BGL trend, sent as a Pebble format PNG.
 * Display Low Line - Display the low limit line on the trend.
 * Display High Line - Display the high limit line on the trend.
 * Trend Period - The period to display the trend (1 hour, 2 hour, 3 hour, 4 hour.  Beyond 4 hour is meaningless).
 * Display Delta - Display the delta (change) value.
 * Display Delta Units - Display mmol/l or mg/dl (as set in xDrip) units in the Delta.
 * Display Slope Arrows - Display the flat, 45 up/down, up/down, double up/down arrows like the Dexcom reader.
 * Special Value - A BGL value to display a message on the watch face (default 5.5 mmol/l or 99 mg/dl, but fully configurable so the people that prefer the 100 value can set that).
 * Text to Display - The text to display if the above special value is reached.  (default BAZINGA!)  Needs to be short.

Using Pebble Clay, the watch face now has settings available in the Pebble app.  Eventually, most of the settings above will move into the Pebble app settings, and the watch face will ask for the things it wants, rather than them being sent regardless.
As of this version, the following settings are available.
 * Display Seconds - Appends the Seconds to the watch time.  ie 00:00:00, instead of 00:00. Default off.
 * Re Raise NO BLUETOOTH vibration alert - Causes a periodic vibration until NO BLUETOOTH is cleared.  Default on.
 * Watch will not make any vigrations - effectively silences the watch face vibrations.  Default off.
 * Light on charge - Illuminates the watch face when charging.  (note, you have to dismiss the charging display and get back to the watch face)  Allows you to see the watch in the dark when charghing.  Default off.
 * Same background top and bottom - Sets the same background colour top and bottom.
 * Foreground Colour - Colour to use for "light" sections of the display.  Default white.  Only useful for Basalt, Chalk, and Emery
 * Background Colour - Colour to use for "Dark" sections of the display.  Default Duke Blue.  Only useful for Basalt, Chalk, and Emery
 * Message timer - Seconds between each check for displaying the message / delta. This is done to avoid having to use the seconds timer for everything increasing the system/battery load. Default is 15 seconds (5-60s)
 * Lower Metrics (left and right, or centre on Round Pebbles) are selectable, based on the capabilities of the Pebble watch in use.  You can select:
 * * Phone Battery Level - All
 * * Watch Battery Level - All
 * * Step Count - Basalt and above
 * * Heart Rate - Emery and Gabbro

The above settings are stored in the watch, and persist between watch face transitions.

## Build Environment:
* Pebble Tool: latest 
* SDK: latest  
* Clay: @rebble/clay latest v1.0.10 or later

## Contributing:
Please do not fork and fragment this code to create your own face to put into xDrip+.  This happened with my initial work on this face causing a number of issues, such as:
* Every watch face added to xDrip+ base repository uses the same UUID, which is against how the Pebble development is meant to work.
* Every change that was made and sent back to me created a lot of rework because people used different editors/IDEs that did not honour the hard tabs and hard line feekds of the original code.  I lost many hours having to reformat the code so that diffs were understandable and merges of PRs made sense.
* When the xDrip+ developers added different model watch faces, each with it's own PBW as a BIN file, it caused a lot of confusion for users, and I was inundated with questions and requests for changes that I could neither answer or make as I had no way of tracing back to the forks.

That said, I appreciate all contributions, especially as the xDrip+ Pebble Protocol Framework takes shape and matures.  I am not aware of how everyone uses xDrip+ and the various other apps it integrates with, so the proposed framework will only accommodate what you want if you contribute to it's development.  

Feel free to use the Discussions to get more information, or reach out to me directly via e-mail at jstevensog@gmail.com.
If you fork this to develop and improve the code, please raise PRs against this repository and explain what you have improved.
The master branch is not being updated frequently, most work is being done in the "new" branch.  That means it is also not as stable.  Please choose wisely wich branch you wish to develop with to improve.

# Contributors:
* jstevensog
* AdrianLXM
* Consp

## Plan for Multiple Apps.
 * Apps will be identified in xDrip settings as their name/version.  This will set a string that matches the PBL_APP_VER key that is sent when the Pebble requests an update or responds to xDrip.  The xDrip Pebble watch face settings will also set the watch face/app UUID in settings.  No two watch faces can have the same UUID.
 * The Pebble code in xDrip will try to activate the selected watch face, and if it fails to do so, will install the face.

## Build Environment:
* Pebble Tool: v5.0.27
* SDK: v4.9.127
* Clay: @rebble/clay latest v1.0 or later

## Change Log:
20260806 - Fixes and allows watch face to specify the size and depth of Trend image creation from xDrip+
* Added PBL_TREND_SIZE uint32_t to send the Trend dimensions and colour depth to xDrip+ to generate the preferred size and depth.  Gabbro only supports PNG8.
* Added "Wait.." to the HR readout as initial value.  Prevents a blank HR display when firmware is updated.
* Fixed (hopefully) the Message Timeout issue.

20260702 - Built with SDK 4.17.  Added features.
* Added selectable bottom left/right metrics to Clay and in code.  Tested on Basalt so far, and working.  Users can select which metrics they wish to display, including None, Phone Battery, Watch Battery, Step Count (for Health enabled platforms), or Heart Rate (for platforms that support it).  Untested on Flint and Gabbro as yet, as I have no access to those watches.
* For Round watches, only the bottom metric that appears mid screen is selectable.
* NOTE TO DEVELOPERS!!!  When Logging is enabled, remove aplite from package.json, as it will not build.  Too many log strings to occupy the string space on the platform.  I will try and figure out how to fix this soon.

20260620 - Built with latest SDK.

20260609 - Refactored code, fixed outstanding issues and made it nicer to watch on a PT2 display.
* Fixed issues with wrong x/y width/height values on PT2, PD2, Gabbro
* Fixed Gabbro face to look like the one for the PR
* Moved seconds timer and message display timer into their own functions to avoid waking every second when seconds is off (this should reduce battery load)
* Removed most ifdefs for debug messages and replaced them with vararg macros 
* Added 60pt font
* Dynamic 40pt/60pt font for PT2 without/with seconds enabled. This also moves the date to accomodate a bit more screenspace niceness
* Increased size of the CGM Time value to make it readable for those without microscopic vision
* Moved all text 1 or 2 pixels from the border
* Fixed bounding box on PT, for some reason on the latest SDK it would otherwise render some text patially off screen
* Switched green to brightgreen to increase contrast

20260227 - This is a refactor of the oringinal code to build with SDK v4.9.127 and add initial support for Gabbro (Core Round 2).
Notes:
* Gabbro will look strange as none of the bitmap or text layers have been resized from Clay as yet.  Also, it is not tested. This will be fixed in later releases.
* The SDK has imposed a lot more "errors" and will not build if there are unused defines or variables.  These have been removed by commenting, not yet removed fully from the source.
