#!/bin/bash
# Unset any snap-related environment variables
unset SNAP
unset SNAP_NAME
unset SNAP_REVISION
unset SNAP_VERSION
unset SNAP_ARCH
unset SNAP_LIBRARY_PATH
unset SNAP_DATA
unset SNAP_COMMON
unset SNAP_INSTANCE_NAME
unset SNAP_INSTANCE_KEY
unset SNAP_USER_DATA
unset SNAP_USER_COMMON
unset LD_LIBRARY_PATH
# Export clean library path
#export LD_LIBRARY_PATH=/usr/lib/x86_64-linux-gnu:/lib/x86_64-linux-gnu

# Run the application
./build/SegmentationClient "$@"