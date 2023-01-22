#   file:       finder.sh
#   author:     Mark Sherman
#   Date:       01/22/2023
#   Version:    1.1

#   AESD Spring 2023
#   Assignment 1

#!/bin/bash

#   create variables for passed inputs
filesdir=$1;
searchstr=$2;

#   check for valid inputs
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

#   print inputs for readability
echo -e "\nSearch Directory: $filesdir"
echo "Search String:    $searchstr"

#   find all files in the given directory, pass output to counter
filecnt=$(find $filesdir -type f | wc -l)
#   find all lines with given search string, pass output to counter
searchcnt=$(grep -R $searchstr $filesdir | wc -l)

#   print counts
echo -e "\nFile Count: $filecnt"
echo "Searched String Count: $searchcnt"

echo -e "\nThe number of files are $filecnt and the number of matching lines are $searchcnt"

exit 0;