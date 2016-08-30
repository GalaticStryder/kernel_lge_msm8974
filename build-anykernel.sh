#!/bin/bash
#
#
#  An automated build script for Lambda Kernel written in bash.
#  Based off AK's and Render's build script - Danke!
#

# Bash color
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
REPACK_DIR="${KERNEL_DIR}/../anykernel"
TOOLCHAINS_DIR="${KERNEL_DIR}/../toolchains"
LINARO_DIR="${TOOLCHAINS_DIR}/linaro"
DORIMANX_DIR="${TOOLCHAINS_DIR}/dorimanx"
PATCH_DIR="${REPACK_DIR}/patch"
MODULES_DIR="${REPACK_DIR}/ramdisk/lib/modules"
ZIP_MOVE="${KERNEL_DIR}/store"
ZIMAGE_DIR="${KERNEL_DIR}/arch/arm/boot"

# Functions
function checkout_branches {
	cd $REPACK_DIR
	git checkout infinito
	cd $KERNEL_DIR
}

function count_cpus {
	echo "Building kernel with $THREAD argument...";
}

function prepare_all {
	cd $REPACK_DIR
	rm -rf $KERNEL
	rm -rf $DTBIMAGE
	if [ ! -d $MODULES_DIR ]; then
		mkdir -p $MODULES_DIR;
	fi;
	for i in $(find "$MODULES_DIR"/ -name "*.ko"); do
		rm -f "$i";
	done;
	cd $KERNEL_DIR
	rm -f $KERNEL_DIR/arch/arm/boot/*.dtb
	rm -f $KERNEL_DIR/arch/arm/boot/*.cmd
	rm -f $KERNEL_DIR/arch/arm/boot/zImage
	rm -f $KERNEL_DIR/arch/arm/boot/Image
	make clean && make mrproper
	echo
	echo "Everything is ready to start..."
}

function make_me {
	echo
	make $DEFCONFIG
	make $THREAD
	cp -vr $ZIMAGE_DIR/$KERNEL $REPACK_DIR
	make modules $THREAD
}

function make_dtb {
	$REPACK_DIR/tools/dtbToolCM -2 -o $REPACK_DIR/$DTBIMAGE -s 2048 -p scripts/dtc/ arch/arm/boot/
}

function copy_modules {
	for i in $(find "$KERNEL_DIR" -name '*.ko'); do
		cp -av "$i" $MODULES_DIR/;
	done;
	chmod 755 $MODULES_DIR/*
	$STRIP --strip-unneeded $MODULES_DIR/* 2>/dev/null
	$STRIP --strip-debug $MODULES_DIR/* 2>/dev/null
}

function make_zip {
	cd $REPACK_DIR
	zip -x@zipexclude -r9 "$VERSION"-"$VARIANT"-"$BUILD_DATE".zip *
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
echo "                   \                    "
echo "                   /\                   "
echo "                  /  \                  "
echo "                 /    \                 "
echo ''
echo " Welcome to Lambda Kernel build script! "
echo -e "${restore}"

echo "Which is the build tag?"
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
echo "You have chosen the tag: $STATE!"
echo ""

# Versioning
NAME="Lambda"
RELEASE="Infinito"
BUILD_DATE=$(date -u +%m%d%Y)
if [ "$STATE" = stable ]; then
	TAG="Stable"
	export VERSION=$NAME-$RELEASE-$TAG
fi
if [ "$STATE" = beta ]; then
	TAG="Beta"
	echo "Could you assign a beta number?"
	read -e tag_number
	TAG_NUMBER="$tag_number"
	echo ""
	export VERSION=$NAME-$RELEASE-$TAG-N$TAG_NUMBER
fi
if [ "$STATE" = incremental ]; then
	TAG="Incremental"
	echo "Could you assign an incremental number?"
	read -e tag_number
	TAG_NUMBER="$tag_number"
	echo ""
	echo "What is the incremental comment?"
	read -e tag_comment
	TAG_COMMENT="$tag_comment"
	echo ""
	export VERSION=$NAME-$RELEASE-$TAG-N$TAG_NUMBER-$TAG_COMMENT
fi
export LOCALVERSION=-`echo $VERSION`

echo "Would you mind picking an LG G2 variant?"
select choice in d800 d801 d802 d803 f320 l01f ls980 vs980
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
	"f320")
		VARIANT="f320"
		DEFCONFIG="f320_defconfig"
		break;;
	"l01f")
		VARIANT="l01f"
		DEFCONFIG="l01f_defconfig"
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
echo "You are going to build $VERSION for the $VARIANT variant."
echo ""

echo "Which toolchain you would like to use?"
select choice in Linaro-4.9 Dorimanx-5.3
do
case "$choice" in
	"Linaro-4.9")
		export TOOLCHAIN="Linaro 4.9"
		export CROSS_COMPILE="${LINARO_DIR}/4.9/bin/arm-eabi-"
		break;;
	"Dorimanx-5.3")
		export TOOLCHAIN="Dorimanx 5.3"
		export CROSS_COMPILE="${DORIMANX_DIR}/bin/arm-eabi-"
		export SYSROOT="${DORIMANX_DIR}/arm-LG-linux-gnueabi/sysroot/"
		export CC="${DORIMANX_DIR}/bin/arm-eabi-gcc --sysroot=$SYSROOT"
		export STRIP="${DORIMANX_DIR}/bin/arm-eabi-strip"
		break;;
esac
done

echo ""
echo "You have chosen to use $TOOLCHAIN."
echo ""

echo

while read -p "Are you ready to start (y/n)? " dchoice
do
case "$dchoice" in
	y|Y)
		prepare_all
		echo
		checkout_branches
		count_cpus
		make_me
		make_dtb
		copy_modules
		make_zip
		generate_md5
		break
		;;
	n|N)
		break
		;;
	* )
		echo
		echo "Stop peeing yourself, coward!"
		echo
		;;
esac
done

DATE_END=$(date +"%s")
DIFF=$(($DATE_END - $DATE_START))
echo -e "${red}"
echo "                 \                  "
echo "                 /\                 "
echo "                /  \                "
echo "               /    \               "
echo ''
echo "Completed in $(($DIFF / 60)) minute(s) and $(($DIFF % 60)) seconds."
echo -e "${restore}"
