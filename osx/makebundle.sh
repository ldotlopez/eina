#!/bin/bash

if [ -d eina.app ] && [ "$1x" = "-fx" ]; then
	rm -rf eina.app
fi

if [ ! -d eina.app ]; then
    gtk-mac-bundler eina.bundle
else
	echo "Note eina.app bundle already exists, only stripping it..."
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
for i in $(find eina.app/Contents/Resources/lib/python2.7 -type f -regex '.*\.py[oc]'); do
    rm -f "$i"
done

echo "Strip debug symbols from bundle binaries"

# Strip debug symbols
for i in $(find -E eina.app/Contents/Resources -type f -regex '.*\.(so|dylib)$'); do
    do_strip "$i"
done

for i in $(find eina.app/Contents/Resources/bin -type f); do
    if [ -x "$i" ]; then
        do_strip "$i"
    fi
done

do_strip eina.app/Contents/MacOS/eina-bin
