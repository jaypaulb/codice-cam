#!/bin/bash
pkill -f CodiceCam
pkill -f opencv
# Kill any remaining OpenCV windows
pkill -f "Camera Preview"
pkill -f "Marker Detection Debug"
pkill -f "Processed Frame"
echo "Processes killed"
