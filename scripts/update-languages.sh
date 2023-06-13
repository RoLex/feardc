#!/bin/bash

cd ..

SOURCE=$1
if [[ ! -d $SOURCE ]] ; then
	echo $SOURCE is not a directory && exit 1
fi

shopt -s nullglob

process() {

PREFIX=$1
DIR=$2

echo Processing $PREFIX...

# move all files to the root dir
find $SOURCE -type f -exec mv '{}' $SOURCE \;

cd $DIR/po

for a in $SOURCE/${PREFIX}-*.po ; do
	FILE=${a#$SOURCE/${PREFIX}-}
	echo $FILE

	if [ -f $FILE ] ; then
		msgmerge -U -C $FILE $a $PREFIX.pot
	else
		msgmerge -U $a $PREFIX.pot
	fi && mv $a $FILE
done

cd ../..

}

scons i18n=1 dcpp/po win32/po help/po installer/po

process libdcpp dcpp
process dcpp-win32 win32
process dcpp-help help
process dcpp-installer installer
