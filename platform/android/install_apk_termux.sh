#!/bin/bash
filepath="$1"
filename=$(basename "$filepath")

targetdir=${2:-~/storage/downloads/termux-apk}
mkdir -p $targetdir

targetpath=$(realpath "$targetdir/$filename")

cp $filepath $targetpath
chmod 664 $targetpath
termux-media-scan $targetpath

echo "Target directory: $targetdir"
echo "Target path: $targetpath"
am start -a android.intent.action.VIEW \
	 -d "file://$targetpath" \
         -t application/vnd.android.package-archive
