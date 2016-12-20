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
kdir=`pwd`
anydir="${kdir}/../anykernel"

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
    echo "Example: ./changelog.sh 05/05/2005"
    sdate=`date --date="2 weeks ago" +"%m/%d/%Y"`
fi

# Log the Kernel source
cd $kdir # Make sure were at Kernel source code.
project="Kernel"
find $kdir -name .git | sed 's/\/.git//g' | sed 'N;$!P;$!D;$d' | while read line
do
cd $line
    # Test to see if the repo needs to have a changelog written
    log=$(git log --pretty="%an - %s" --no-merges --since=$sdate --date-order)
    if [ -z "$log" ]; then
    echo "Nothing updated on $project changelog, skipping..."
    else
        # Write the changelog
        echo "Changelog was updated and written for $project..."
        echo "Project: $project" >> "$kdir"/changelog.txt
        echo "$log" | while read line
        do
echo "$line" >> "$kdir"/changelog.txt
        done
echo "" >> "$kdir"/changelog.txt
    fi
done

# Log the AnyKernel source
cd $anydir # Make sure were at Kernel source code.
aproject="AnyKernel"
find $anydir -name .git | sed 's/\/.git//g' | sed 'N;$!P;$!D;$d' | while read aline
do
cd $aline
    # Test to see if the repo needs to have a changelog written
    log=$(git log --pretty="%an - %s" --no-merges --since=$sdate --date-order)
    if [ -z "$log" ]; then
    echo "Nothing updated on $project changelog, skipping..."
    else
        # Write the changelog
        echo "Changelog was updated and written for $aproject..."
        echo "Project: $aproject" >> "$kdir"/changelog.txt
        echo "$log" | while read aline
        do
echo "$aline" >> "$kdir"/changelog.txt
        done
echo "" >> "$kdir"/changelog.txt
    fi
done

echo ""
echo -e ${green}"Changelog for $project has been written."${restore}
