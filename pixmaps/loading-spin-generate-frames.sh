#!/bin/bash

F="$1"

if [ !-f "$F" ]; then
	echo "Usage $0 [frame-no0]"
	exit 1
fi

i=0
while [ $i -lt 60 ]
do
	convert -rotate $(($i*6)) -background none -gravity Center -crop 512x512+0+0 $F $(printf frame-%02d.png $i)
	let i=$i+1
done

