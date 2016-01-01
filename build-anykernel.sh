#!/bin/bash

#
#  Build Script for Lambda Kernel
#  Based off AK's and Render's build script - Danke!
#

# Bash Color
green='\033[01;32m'
red='\033[01;31m'
blink_red='\033[05;31m'
restore='\033[0m'

# Clean
clear

# Resources
THREAD="-j$(grep -c ^processor /proc/cpuinfo)"
KERNEL="zImage"
DTBIMAGE="dtb"

# Variables
export ARCH=arm
export SUBARCH=arm
export KBUILD_BUILD_USER=galatic
export CCACHE=ccache

# Paths
KERNEL_DIR=`pwd`
REPACK_DIR="${HOME}/Desenvolvimento/kernel/anykernel"
PATCH_DIR="${HOME}/Desenvolvimento/kernel/anykernel/patch"
MODULES_DIR="${HOME}/Desenvolvimento/kernel/anykernel/modules"
ZIP_MOVE="${HOME}/Desenvolvimento/kernel/source/store"
ZIMAGE_DIR="${HOME}/Desenvolvimento/kernel/source/arch/arm/boot"

# Functions
function checkout_branches {
		cd $REPACK_DIR
		git checkout lambda
		cd $KERNEL_DIR
}

function clean_all {
		rm -f $KERNEL_DIR/arch/arm/boot/*.dtb
		rm -f $KERNEL_DIR/arch/arm/boot/*.cmd
		rm -f $KERNEL_DIR/arch/arm/boot/zImage
		rm -f $KERNEL_DIR/arch/arm/boot/Image
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
		zip -r9 "$VERSION"-"$VARIANT"-"$BUILD_DATE".zip *
		mv "$VERSION"-"$VARIANT"-"$BUILD_DATE".zip $ZIP_MOVE
		cd $KERNEL_DIR
}

function generate_md5 {
		cd $ZIP_MOVE
		md5sum "$VERSION"-"$VARIANT"-"$BUILD_DATE".zip > "$VERSION"-"$VARIANT"-"$BUILD_DATE".zip.md5
		cd $KERNEL_DIR
}


DATE_START=$(date +"%s")

echo -e "${red}"
echo ""
echo "   \    "
echo "   /\   "
echo "  /  \  "
echo " /    \ "
echo ""
echo -e "${restore}"

echo "Build state..."
select choice in Stable Beta Incremental
do
case "$choice" in
	"Stable")
		export STATE="stable"
		break;;
	"Beta")
		export STATE="beta"
		break;;
	"Incremental")
		export STATE="incremental"
		break;;
esac
done

echo ""
echo "You have chosen $STATE!"
echo ""

# Versioning
NAME="LambdaKernel"
RELEASE="NewYear"
BUILD_DATE=$(date -u +%m%d%Y)
if [ "$STATE" = stable ]; then
	TAG="Stable"
	export VERSION=$NAME-$RELEASE-$TAG
fi
if [ "$STATE" = beta ]; then
	TAG="Beta"
	export VERSION=$NAME-$RELEASE-$TAG
fi
if [ "$STATE" = incremental ]; then
	TAG="Incremental"
	echo "Could you assign an incremental number?"
	read -e tag_number
	TAG_NUMBER="$tag_number"
	echo ""
	export VERSION=$NAME-$RELEASE-$TAG-N$TAG_NUMBER
fi
export LOCALVERSION=-`echo $VERSION`

echo "Pick an LG G2 variant..."
select choice in d800 d801 d802 d803 ls980 vs980
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
esac
done

echo ""
echo "You are building $VERSION for $VARIANT..."
echo ""

echo "Pick Toolchain..."
select choice in Linaro-4.8
do
case "$choice" in
	"Linaro-4.8")
		export TOOLCHAIN="Linaro 4.8"
		export CROSS_COMPILE=${HOME}/Desenvolvimento/kernel/toolchains/linaro-4.8/bin/arm-eabi-
		break;;
	"Linaro-4.9")
		export TOOLCHAIN="Linaro 4.9"
		export CROSS_COMPILE=${HOME}/Desenvolvimento/kernel/toolchains/linaro-4.9/bin/arm-eabi-
		break;;
	"Linaro-5.3")
		export TOOLCHAIN="Linaro 5.3"
		export CROSS_COMPILE=${HOME}/Desenvolvimento/kernel/toolchains/linaro-5.3/bin/arm-eabi-
		break;;
	"Linaro-6.0")
		export TOOLCHAIN="Linaro 5.3"
		export CROSS_COMPILE=${HOME}/Desenvolvimento/kernel/toolchains/linaro-5.3/bin/arm-eabi-
		break;;
esac
done

echo ""
echo "You have chosen $TOOLCHAIN!"
echo ""

while read -p "Do you want to clean stuff (y/n)? " cchoice
do
case "$cchoice" in
	y|Y )
		clean_all
		echo
		echo "All cleaned."
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
		generate_md5
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
