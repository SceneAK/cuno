#!/bin/bash
adb logcat --pid=$(adb shell pidof ${1:-com.sceneak.cuno})
