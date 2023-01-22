#!/bin/bash

writefile=$1;
writestr=$2;

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

writedir=$(dirname $writefile)
echo "$writedir"
mkdir -p "$writedir"
touch $writefile;
echo "$writestr" > $writefile

echo -e "\nWrite File: $writefile"
echo "Write String:    $writestr"


#echo -e "\nSuccess!"
exit 0;