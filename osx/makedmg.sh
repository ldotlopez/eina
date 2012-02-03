#!/bin/bash

# Copied and modified from Banshee

pushd $(dirname $0) &>/dev/null

echo "Generating bundle..."
./makebundle.sh

VOLUME_NAME=eina

if [ "$1x" = "x" ]; then
	DMG_FILE=$VOLUME_NAME.dmg
else
	DMG_FILE=$1.dmg
fi

DMG_APP=eina.app
MOUNT_POINT=$VOLUME_NAME.mounted

rm -f $DMG_FILE
rm -f $DMG_FILE.master

# Compute an approximated image size in MB, and bloat by 15 MB
image_size=$(du -ck $DMG_APP dmg-data | tail -n1 | cut -f1)
image_size=$((($image_size + 15000) / 1000))

echo "Creating disk image (${image_size}MB)..."
#hdiutil create $DMG_FILE -megabytes $image_size -volname $VOLUME_NAME -fs HFS+ -quiet || exit $?
cp template.dmg tmp.dmg
hdiutil resize -size ${image_size}m tmp.dmg

echo "Attaching to disk image..."
hdiutil attach tmp.dmg -readwrite -noautoopen -mountpoint $MOUNT_POINT -quiet

echo "Populating image..."
cp -r $DMG_APP $MOUNT_POINT

echo "Detaching from disk image..."
hdiutil detach $MOUNT_POINT -quiet

rm -rf $DMG_APP

#mv $DMG_FILE $DMG_FILE.master

#echo "Creating distributable image..."
#hdiutil convert -quiet -format UDBZ -o $DMG_FILE $DMG_FILE.master

#echo "Installing end user license agreement..."
#hdiutil flatten -quiet $DMG_FILE
#/Developer/Tools/Rez /Developer/Headers/FlatCarbon/*.r dmg-data/license.r -a -o $DMG_FILE
#hdiutil unflatten -quiet $DMG_FILE

echo "Converting to final image..."
hdiutil convert -quiet -format UDBZ -o $DMG_FILE tmp.dmg
rm tmp.dmg

echo "Done."

#if [ ! "x$1" = "x-m" ]; then
#	rm $DMG_FILE.master
#fi

popd &>/dev/null
