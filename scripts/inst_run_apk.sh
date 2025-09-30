#!/bin/bash
adb install -r ${1:-./build/apk/cuno.apk}
adb shell monkey -p ${2:-com.sceneak.cuno} -c android.intent.category.LAUNCHER 1
