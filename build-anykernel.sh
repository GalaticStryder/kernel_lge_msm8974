#!/bin/bash

#
#  Build Script for Render Kernel for G2!
#  Based off AK'sbuild script - Thanks!
#

# Bash Color
green='\033[01;32m'
red='\033[01;31m'
blink_red='\033[05;31m'
restore='\033[0m'

clear

# Resources
THREAD="-j$(grep -c ^processor /proc/cpuinfo)"
KERNEL="zImage"
DTBIMAGE="dtb"

# Kernel Details
VER=Render-Kernel

# Vars
export LOCALVERSION=~`echo $VER`
export ARCH=arm
export SUBARCH=arm
export KBUILD_BUILD_USER=RenderBroken
export KBUILD_BUILD_HOST=RenderServer.net
export CCACHE=ccache

# Paths
KERNEL_DIR=`pwd`
REPACK_DIR="${HOME}/android/source/kernel/G2-AnyKernel"
PATCH_DIR="${HOME}/android/source/kernel/G2-AnyKernel/patch"
MODULES_DIR="${HOME}/android/source/kernel/G2-AnyKernel/modules"
ZIP_MOVE="${HOME}/android/source/zips/g2-caf-zips"
ZIMAGE_DIR="${HOME}/android/source/kernel/msm8974_G2-CAF_render_kernel/arch/arm/boot"

# Functions
function checkout_branches {
		cd $REPACK_DIR
		git checkout rk-anykernel
		cd $KERNEL_DIR
}

function clean_all {
		rm -rf $MODULES_DIR/*
		cd $REPACK_DIR
		rm -rf $KERNEL
		rm -rf $DTBIMAGE
		cd $KERNEL_DIR
		echo
		make clean && make mrproper
}

function make_kernel {
		echo
		make $DEFCONFIG
		make $THREAD
		cp -vr $ZIMAGE_DIR/$KERNEL $REPACK_DIR
}

function make_modules {
		rm `echo $MODULES_DIR"/*"`
		find $KERNEL_DIR -name '*.ko' -exec cp -v {} $MODULES_DIR \;
}

function make_dtb {
		$REPACK_DIR/tools/dtbToolCM -2 -o $REPACK_DIR/$DTBIMAGE -s 2048 -p scripts/dtc/ arch/arm/boot/
}

function make_zip {
		cd $REPACK_DIR
		zip -r9 RenderKernel-G2-CAF-"$VARIANT"-R.zip *
		mv RenderKernel-G2-CAF-"$VARIANT"-R.zip $ZIP_MOVE
		cd $KERNEL_DIR
}


DATE_START=$(date +"%s")

echo -e "${green}"
echo "Render Kernel Creation Script:"
echo -e "${restore}"

echo "Pick VARIANT..."
select choice in d800 d801 d802 d803 ls980 vs980 f320x l01f
do
case "$choice" in
	"d800")
		VARIANT="d800"
		DEFCONFIG="d800_defconfig"
		break;;
	"d801")
		VARIANT="d801"
		DEFCONFIG="d801_defconfig"
		break;;
	"d802")
		VARIANT="d802"
		DEFCONFIG="d802_defconfig"
		break;;
	"d803")
		VARIANT="d803"
		DEFCONFIG="d803_defconfig"
		break;;
	"ls980")
		VARIANT="ls980"
		DEFCONFIG="ls980_defconfig"
		break;;
	"vs980")
		VARIANT="vs980"
		DEFCONFIG="vs980_defconfig"
		break;;
	"f320x")
		VARIANT="f320x"
		DEFCONFIG="f320x_defconfig"
		break;;
	"l01f")
		VARIANT="l01f"
		DEFCONFIG="l01f_defconfig"
		break;;
esac
done

echo "Pick Toolchain..."
select choice in UBER-4.9-Cortex-a15 UBER-5.1
do
case "$choice" in
	"UBER-4.9-Cortex-a15")
		export CROSS_COMPILE=${HOME}/android/source/toolchains/UBER-arm-eabi-4.9-cortex-a15-062715/bin/arm-eabi-
		break;;
	"UBER-5.1")
		export CROSS_COMPILE=${HOME}/android/source/toolchains/UBER-arm-eabi-5.1-062715/bin/arm-eabi-
		break;;
esac
done

while read -p "Do you want to clean stuffs (y/n)? " cchoice
do
case "$cchoice" in
	y|Y )
		clean_all
		echo
		echo "All Cleaned now."
		break
		;;
	n|N )
		break
		;;
	* )
		echo
		echo "Invalid try again!"
		echo
		;;
esac
done

echo

while read -p "Do you want to build kernel (y/n)? " dchoice
do
case "$dchoice" in
	y|Y)
		make_kernel
		make_dtb
		make_modules
		make_zip
		break
		;;
	n|N )
		break
		;;
	* )
		echo
		echo "Invalid try again!"
		echo
		;;
esac
done

echo -e "${green}"
echo "-------------------"
echo "Build Completed in:"
echo "-------------------"
echo -e "${restore}"

DATE_END=$(date +"%s")
DIFF=$(($DATE_END - $DATE_START))
echo "Time: $(($DIFF / 60)) minute(s) and $(($DIFF % 60)) seconds."
echo
