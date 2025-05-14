env -i   LD_LIBRARY_PATH=/usr/lib/x86_64-linux-gnu:/lib/x86_64-linux-gnu   PATH=/usr/bin:/bin   QT_QPA_PLATFORM=offscreen   QT_LOGGING_RULES="qt.qpa.*=false"   XDG_RUNTIME_DIR=/tmp/runtime-$USER   ./build/SegmentationClient http://10.10.31.99:8080/video
Starting segmentation pipeline with camera: http://10.10.31.99:8080/video
