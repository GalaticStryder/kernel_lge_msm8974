#!/bin/sh

sdate=${1}
cdate=`date +"%m_%d_%Y"`
rdir=`pwd`

rm -rf changelog_*

clear

echo ""
echo "   \    "
echo "   /\   "
echo "  /  \  "
echo " /    \ "
echo ""
echo "Changelog generator is starting..."

# Check the date start range is set
if [ -z "$sdate" ]; then
    echo ""
    echo "Failed!"
    echo "Add a date in mm/dd/yyyy format to count from..."
    echo ""
    read sdate
fi

# Find the directories to log
echo "Starting date picking based on the input date..."
find $rdir -name .git | sed 's/\/.git//g' | sed 'N;$!P;$!D;$d' | while read line
do
cd $line
    # Test to see if the repo needs to have a changelog written
    log=$(git log --pretty="%an - %s" --no-merges --since=$sdate --date-order)
    project="Lambda Kernel"
    if [ -z "$log" ]; then
    echo "Nothing updated on $project changelog, skipping..."
    else
        # Write the changelog
        echo "Changelog is updated and written for $project..."
        echo "Project: $project" >> "$rdir"/changelog_$cdate.log
        echo "$log" | while read line
        do
echo "$line" >> "$rdir"/changelog_$cdate.log
        done
echo "" >> "$rdir"/changelog_$cdate.log
    fi
done
echo ""
echo " Changelog script for $project has finished."
echo ""
