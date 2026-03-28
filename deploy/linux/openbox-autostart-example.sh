#!/usr/bin/env bash
# Example snippet for ~/.config/openbox/autostart

# set wallpaper
feh --bg-fill /home/pveclient/Pictures/wallpaper.jpg &

# disable blank/suspend
xset -dpms
xset s off
xset s noblank

# start supervisor in background
/home/pveclient/bin/pveclient-supervisor.sh &
