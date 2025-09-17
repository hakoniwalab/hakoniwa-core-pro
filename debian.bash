#!/bin/bash

if [ $# -ne 1 ]
then
	echo "Usage: $0 <clean|build>"
	exit 1
fi

OPT=$1

if [ $OPT = "clean" ]
then
	debian/rules clean || dh clean
	exit 0
fi

debuild -us -uc -b

