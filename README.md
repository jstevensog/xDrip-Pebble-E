# cgm-pebble-offline
Offline pebble watchface based on the Nightscout community version
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
