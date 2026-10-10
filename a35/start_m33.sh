#!/bin/sh
# Load and start the M33 firmware via remoteproc, resolving the M33 by name
# (on the STM32MP257F-DK remoteproc0 is the M0+, the M33 is usually remoteproc1).
#   ./start_m33.sh ipc_m33_CM33.sig      # file must already be in /lib/firmware
#   ./start_m33.sh stop
set -e
FW=${1:-ipc_m33_CM33.sig}
for d in /sys/class/remoteproc/remoteproc*; do
    [ "$(cat "$d/name")" = m33 ] && M33=$d && break
done
[ -n "$M33" ] || { echo "no m33 remoteproc found" >&2; exit 1; }

if [ "$FW" = stop ]; then
    echo stop > "$M33/state"; exit 0
fi
[ "$(cat "$M33/state")" = offline ] || echo stop > "$M33/state"
echo "$FW" > "$M33/firmware"
echo start > "$M33/state"
sleep 1
cat "$M33/state"
ls -l /dev/ttyRPMSG0
