# xDrip-Pebble
Offline pebble watchface for xDrip, based on the Nightscout community version
Note:  This current version is a modifcation of Kevin Lee's original cgm-pebble-offline.  It is intended for use with xDrip, not Nightscout.

As such, it is based on the cgm-pebble version 6.0 code.
There ARE NO EASTER EGGS, unless they are provided by xDrip.
This watch face is meant for patients to use with xDrip, not for parents/carers.
Ensure you enable notifications from xDrip in the Pebble App, and enable Third Party notifications, otherwise these will NOT come through on the pebble.
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
 * Foreground Colour - Colour to use for "light" sections of the display.  Default white.
 * Background Colour - Colour to use for "Dark" sections of the display.  Default Duke Blue.
The above settings are stored in the watch, and persist between watch face transitions.

To Do:
 * I am working on a watch face framework that will allow other developers to enhance and expand the number and styles of watch faces in xDrip.
 * Move most settings in xDrip into persistent settings in the watch face.  This will allow watch faces to request the items they want from xDrip.  ie, Trend size, Trend lines, Delta, Arrows, etc.
 * Add settings to select font sizes for some display components.
 * Add automation of display components, such that if a component is turned off, or it's font size is changed, components will dynamically reposition to make the best possible use of the available screen realestate.

Plan for Multiple Apps.
 * Apps will be identified in xDrip settings as their name/version.  This will set a string that matches the PBL_APP_VER key that is sent when the pebble requests an update or responds to xDrip.  The xDrip Pebble watch face settings will also set the watch face/app UUID in PebbleTrend.  No two watch faces can have the same UUID.
 * PebbleTrend will try to activate the selected watch face, and if it fails to do so, will install the face.

