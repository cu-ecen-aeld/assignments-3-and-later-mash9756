#!/bin/bash

filesdir=$1;
searchstr=$2;

if [ ! $filesdir ] || [ ! -d $filesdir ]
then
    echo "Invalid Search Directory. Exiting..."
    exit 1;
fi

if [ ! $searchstr ]
then
    echo "Invalid Search String. Exiting..."
    exit 1;
fi

echo -e "\nSearch Directory: $filesdir"
echo "Search String:    $searchstr"

filecnt=$(find $filesdir -type f | wc -l)
searchcnt=$(grep -R $searchstr $filesdir | wc -l)

echo -e "\nFile Count: $filecnt"
echo "Searched String Count: $searchcnt"

echo -e "\nThe number of files are $filecnt and the number of matching lines are $searchcnt"

#echo -e "\nSuccess!"
exit 0;