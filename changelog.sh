#!/bin/bash
#
# Copyright - Ícaro Hoff <icarohoff@gmail.com>
#
#              \
#              /\
#             /  \
#            /    \
#
sdate=${1};
rdir=`pwd`

rm changelog.txt

clear

# Bash Color
green='\033[01;32m'
red='\033[01;31m'
blink_red='\033[05;31m'
restore='\033[0m'

echo -e "${red}"
echo "                     \                      "
echo "                     /\                     "
echo "                    /  \                    "
echo "                   /    \                   "
echo ''
echo " Welcome to Lambda Kernel changelog script! "
echo -e "${restore}"

if [ -z "$sdate" ]; then
    echo "Counting from 2 weeks ago as per release schedule."
    echo "You can specify a date in this format: mm/dd/yyyy."
    echo "Example: ./changelog 05/05/2005"
    sdate=`date --date="2 weeks ago" +"%m/%d/%Y"`
fi

# Find the directories to log
project="Lambda Kernel"
find $rdir -name .git | sed 's/\/.git//g' | sed 'N;$!P;$!D;$d' | while read line
do
cd $line
    # Test to see if the repo needs to have a changelog written
    log=$(git log --pretty="%an - %s" --no-merges --since=$sdate --date-order)
    if [ -z "$log" ]; then
    echo "Nothing updated on Lambda Kernel changelog, skipping..."
    else
        # Write the changelog
        echo "Changelog was updated and written for $project..."
        echo "Project: $project" >> "$rdir"/changelog.txt
        echo "$log" | while read line
        do
echo "$line" >> "$rdir"/changelog.txt
        done
echo "" >> "$rdir"/changelog.txt
    fi
done
echo ""
echo -e ${green}"Changelog for $project has been written."${restore}
