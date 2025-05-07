#!/bin/bash
ZIPNAME=`echo "${1}.zip"`
UPLOAD=`echo "file=@${ZIPNAME}"`
FAILED_UPLOADS=~/Hauken/failed

sleep 30 # For EBK-AI

zip -j -m $ZIPNAME -xi $1
curl -H 'application/x-www-form-url-encoded' -F "brukernavn=$2" -F "passord=$3" --cookie-jar ~/.kake "http://stoygolvet.npta.no/logon.php"
curl --cookie ~/.kake -X POST -F $UPLOAD http://stoygolvet.npta.no/Casper/lastopp_fil.php

res=$?
echo $res
if test "$res" != "0"; then
	echo "Opplasting feilet, kopierer for seinere opplasting"
	cp $ZIPNAME $FAILED_UPLOADS
	echo $2 > $FAILED_UPLOADS/.br
	echo $3 > $FAILED_UPLOADS/.pw
fi
