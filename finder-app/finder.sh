#!/bin/bash

filesdir=$1
searchstr=$2
#echo "Parent Directory to search: ${filesdir}";
#echo "String to search: ${searchstr}";

if [ $# -ne 2 ]; then
    echo "ERROR: $# argument(s) received. Only 2 is acceptable."
    exit 1
fi

if [ -d "$filesdir" ]; then
    : # NOP
else
    echo "ERROR: $filesdir does NOT exist."
    exit 1
fi

X=$(find $filesdir -mindepth 1 -type f | wc -l)
ERRORCODES=$?

Y=$(grep -r $filesdir -e $searchstr | wc -l)
ERRORCODES=$(($?+$ERRORCODES))

echo "The number of files are $X and the number of matching lines are $Y"

#echo "ERRORCODES = $ERRORCODES"
exit $ERRORCODES