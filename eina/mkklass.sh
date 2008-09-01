#!/bin/bash

UPPER="$(echo $1 | tr '[:lower:]' '[:upper:]')"
LOWER="$(echo $1 | tr '[:upper:]' '[:lower:]')"
CAMEL="$(echo -n ${LOWER:0:1} |  tr '[:lower:]' '[:upper:]'; echo ${LOWER:1})"

# echo "$UPPER"
# echo "$LOWER"
# echo "$CAMEL"

cat ether.c | \
	sed -e "s/Ether/$CAMEL/g" | \
	sed -e "s/ether/$LOWER/g" | \
	sed -e "s/ETHER/$UPPER/g" > $LOWER.c

cat ether.h | \
	sed -e "s/Ether/$CAMEL/g" | \
	sed -e "s/ether/$LOWER/g" | \
	sed -e "s/ETHER/$UPPER/g" > $LOWER.h

