#!/bin/bash
#
# An automated sync script written in bash.
# It avoids the usage of the repo utility.
#

KERNEL_DIR=`pwd`
echo "You are in directory $KERNEL_DIR."
echo "Going back one folder."
cd ..
echo "Cleaning..."
rm -rf anykernel
rm -rf linaro
echo "Downloading Anykernel utility..."
git clone https://github.com/GalaticStryder/AnyKernel2 anykernel -b lambda
echo "Downloading Linaro toolchains..."
git clone https://github.com/Christopher83/arm-cortex-linux-gnueabi-linaro_5.2 linaro/5.2 -b master
git clone https://github.com/Christopher83/arm-cortex_a15-linux-gnueabihf-linaro_4.9 linaro/4.9 -b master
cd $KERNEL_DIR
echo "Everything in place, ready to build!"
