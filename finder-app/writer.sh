#   file:       writer.sh
#   author:     Mark Sherman
#   Date:       01/22/2023
#   Version:    1.1

#   AESD Spring 2023
#   Assignment 1

#!/bin/bash

#   create variables for passed inputs
writefile=$1;
writestr=$2;

#   check for invalid inputs
if [ ! $writefile ]
then
    echo "Invalid Write File. Exiting..."
    exit 1;
fi

if [ ! $writestr ]
then
    echo "Invalid Write String. Exiting..."
    exit 1;
fi

#   extract directory of given file
writedir=$(dirname $writefile)
#   create all required directories of given file
mkdir -p "$writedir"
#   create actual file
touch $writefile;
#   write to created file in desired directory
echo "$writestr" > $writefile

#   print inputs for readability
echo -e "\nWrite File: $writefile"
echo "Write String:    $writestr"

exit 0;