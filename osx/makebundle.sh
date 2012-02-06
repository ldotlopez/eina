#!/bin/bash

if [ -d Eina.app ] && [ "$1x" = "-fx" ]; then
	rm -rf Eina.app
fi

if [ ! -d Eina.app ]; then
    gtk-mac-bundler Eina.bundle
else
	echo "Note Eina.app bundle already exists, only stripping it..."
fi

if [ -d eina.app ]; then
	mv eina.app Eina.app
fi

function do_strip {
    tp=$(file -b --mime-type "$1")

    if [ "$tp" != "application/octet-stream" ]; then
        return
    fi

    name=$(mktemp -t bundle)
    st=$(stat -f %p "$1")
    
    strip -o "$name" -S "$1"
    mv -f "$name" "$1"

    chmod "$st" "$1"
    chmod u+w "$1"
}

echo "Removing unneeded files from bundle"

# Remove pyc and pyo files
for i in $(find Eina.app/Contents/Resources/lib/python2.7 -type f -regex '.*\.py[oc]'); do
    rm -f "$i"
done

echo "Strip debug symbols from bundle binaries"

# Strip debug symbols
for i in $(find -E Eina.app/Contents/Resources -type f -regex '.*\.(so|dylib)$'); do
    do_strip "$i"
done

for i in $(find Eina.app/Contents/Resources/bin -type f); do
    if [ -x "$i" ]; then
        do_strip "$i"
    fi
done

do_strip Eina.app/Contents/MacOS/eina-bin
