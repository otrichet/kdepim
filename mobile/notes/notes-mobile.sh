#!/bin/sh

QT_DIR=/opt/qt4-maemo5/lib

echo $LD_LIBRARY_PATH | grep "$QT_DIR" > /dev/null 2>&1

if [ "$?" -ne "0"  ]; then
	export LD_LIBRARY_PATH=$QT_DIR:$LD_LIBRARY_PATH
fi

/usr/bin/notes-mobile --graphicssystem raster
