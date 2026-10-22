#!/bin/bash
# 113 - Message Timeout/Period to display
# 122 - Stale Data delay/timeout in minutes
# 312 - Trend Line Style
# 313 - Trend line width
# 112 - Stale Data Timer (Minutes)
# 115 - Bottom left metric
# 2001 - BG Delta Struct 
# 2008 - Slope value to display arrows or other icons
pebble send-app-message --int 113=5 122=6 312=2 313=2 323=1 115=1 2001=0 2008=0
