#!/bin/bash
#
# Copyright - Ícaro Hoff <icarohoff@gmail.com>
#
#              \
#              /\
#             /  \
#            /    \
#
export SCRIPT_VERSION="2.6 (Kashimira ist Marracache)"

# Bash color
green='\033[01;32m'
red='\033[01;31m'
blink_red='\033[05;31m'
restore='\033[0m'

# Clean
clear

# Resources
THREAD="-j$(grep -c ^processor /proc/cpuinfo)"

# Variables
export ARCH=arm
export SUBARCH=arm

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
function check_folders {
	if [ ! -d $REPACK_DIR ]; then
		echo "Could not find anykernel folder. Aborting...";
		echo "Read the readme.md for instructions.";
		echo "";
		exit;
	fi;
	if [ ! -d $TOOLCHAINS_DIR ]; then
		echo "Could not find toolchains folder. Aborting...";
		echo "Read the readme.md for instructions.";
		echo "";
		exit;
	fi;
	if [ ! -d $CCACHE_DIR ]; then
		echo "Could not find the ccache directory. Creating..."
		mkdir -p $CCACHE_DIR;
		echo ""
	fi;
	if [ ! -d $ZIP_MOVE ]; then
		echo "Could not find store folder. Creating..."
		mkdir -p $ZIP_MOVE;
		echo ""
	fi;
}

function checkout_branches {
	cd $REPACK_DIR
	git checkout $ANYBRANCH
	cd $KERNEL_DIR
}

function ccache_setup {
	export USE_CCACHE="1" # Force the use of cache.
	echo "Cache information:"
	ccache -s
	echo
}

function clean_ccache {
	echo
	echo -e ${red}"WARNING: If you are compiling between variants, clean it!"${restore}
	while read -t 15 -p "Would you like to clean ccache (Y/N)? " cchoice
	do
	case "$cchoice" in
		y|Y)
			echo
			echo "Cleaning ccache and stats..."
			ccache -C -z
			echo
			break
			;;
		n|N)
			echo
			echo "Using stored ccache nodes..."
			break
			;;
		* )
			echo
			echo "Please, type Y or N."
			echo
			;;
	esac
	done
}

function prepare_all {
	cd $REPACK_DIR
	rm -f zImage
	rm -f dt.img
	if [ ! -d $MODULES_DIR ]; then
		mkdir -p $MODULES_DIR;
	fi;
	for i in $(find "$MODULES_DIR"/ -name "*.ko"); do
		rm -f "$i";
	done;
	cd $KERNEL_DIR
	rm -f arch/arm/boot/*.dtb
	rm -f arch/arm/boot/*.cmd
	rm -f arch/arm/boot/zImage
	rm -f arch/arm/boot/Image
	make clean && make mrproper
	echo
	echo "Everything is ready to start..."
}

function make_zImage {
	cd $KERNEL_DIR # Just in case!
	echo
	make $DEFCONFIG
	make zImage-dtb $THREAD
	make modules $THREAD
	cp -f arch/arm/boot/zImage $REPACK_DIR/zImage
}

function create_dtimg {
	$REPACK_DIR/tools/dtbToolCM -v -s 2048 -o $REPACK_DIR/dt.img arch/arm/boot/
}

function copy_modules {
	for i in $(find "$KERNEL_DIR" -name '*.ko'); do
		cp -av "$i" $MODULES_DIR/;
	done;
	chmod 755 $MODULES_DIR/*
	$STRIP --strip-unneeded $MODULES_DIR/* 2>/dev/null
	$STRIP --strip-debug $MODULES_DIR/* 2>/dev/null
}

function changelog {
	sh changelog.sh > /dev/null 2>&1 # Suppressed, 47!
	cp changelog.txt $REPACK_DIR/ramdisk/sbin/changelog.txt
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
echo -e " Welcome to Lambda Kernel build script  " "${restore}"
echo -e "${green}" "Version: $SCRIPT_VERSION "
echo -e "${restore}"
check_folders
ccache_setup

echo "Which is the build tag?"
select choice in Stable Beta Experimental
do
case "$choice" in
	"Stable")
		export STATE="stable"
		break;;
	"Beta")
		export STATE="beta"
		break;;
	"Experimental")
		export STATE="experimental"
		break;;
esac
done

echo ""
echo "You have chosen the tag: $STATE!"

echo

echo "Marshmallow -> Type M"
echo "Nougat -> Type N"
echo ""
while read -p "Please, select the Android version (M/N)? " achoice
do
case "$achoice" in
	m|M)
		echo
		echo "Building marshmallow compatible kernel..."
		ANDROID="Marhsmallow"
		ANYBRANCH="marshmallow"
		echo
		break
		;;
	n|N)
		echo
		echo "Building nougat compatible kernel..."
		ANDROID="Nougat"
		ANYBRANCH="nougat"
		echo
		break
		;;
	* )
		echo
		echo "Assuming marshmallow as Android version..."
		ANDROID="Marhsmallow"
		ANYBRANCH="marshmallow"
		echo
		break
		;;
esac
done

# Versioning
NAME="Lambda"
RELEASE="Infinito"
BUILD_DATE=$(date -u +%m%d%Y)
if [ "$STATE" = stable ]; then
	TAG="Stable"
	export VERSION=$NAME-$RELEASE-$ANDROID-$TAG
fi
if [ "$STATE" = beta ]; then
	TAG="Beta"
	echo "Could you assign a beta number?"
	read -e tag_number
	TAG_NUMBER="$tag_number"
	echo ""
	export VERSION=$NAME-$RELEASE-$ANDROID-$TAG-N$TAG_NUMBER
fi
if [ "$STATE" = experimental ]; then
	TAG="Experimental"
	echo "Could you assign an experimental number?"
	read -e tag_number
	TAG_NUMBER="$tag_number"
	echo ""
	echo "What is the experimental comment?"
	read -e tag_comment
	TAG_COMMENT="$tag_comment"
	echo ""
	export VERSION=$NAME-$RELEASE-$ANDROID-$TAG-N$TAG_NUMBER-$TAG_COMMENT
fi

echo "Would you mind picking an LG G2 variant?"
select choice in d800 d801 d802 d803 f320 l01f ls980 vs980
do
case "$choice" in
	"d800")
		VARIANT="D800"
		if [ "$ANDROID" = Nougat ]; then
			DEFCONFIG="nougat_d800_defconfig"
		else
			DEFCONFIG="marshmallow_d800_defconfig"
		fi
		break;;
	"d801")
		VARIANT="D801"
		if [ "$ANDROID" = Nougat ]; then
			DEFCONFIG="nougat_d801_defconfig"
		else
			DEFCONFIG="marshmallow_d801_defconfig"
		fi
		break;;
	"d802")
		VARIANT="D802"
		if [ "$ANDROID" = Nougat ]; then
			DEFCONFIG="nougat_d802_defconfig"
		else
			DEFCONFIG="marshmallow_d802_defconfig"
		fi
		break;;
	"d803")
		VARIANT="D803"
		if [ "$ANDROID" = Nougat ]; then
			DEFCONFIG="nougat_d803_defconfig"
		else
			DEFCONFIG="marshmallow_d803_defconfig"
		fi
		break;;
	"f320")
		VARIANT="F320"
		if [ "$ANDROID" = Nougat ]; then
			DEFCONFIG="nougat_f320_defconfig"
		else
			DEFCONFIG="marshmallow_f320_defconfig"
		fi
		break;;
	"l01f")
		VARIANT="L01F"
		if [ "$ANDROID" = Nougat ]; then
			DEFCONFIG="nougat_l01f_defconfig"
		else
			DEFCONFIG="marshmallow_l01f_defconfig"
		fi
		break;;
	"ls980")
		VARIANT="LS980"
		if [ "$ANDROID" = Nougat ]; then
			DEFCONFIG="nougat_ls980_defconfig"
		else
			DEFCONFIG="marshmallow_ls980_defconfig"
		fi
		break;;
	"vs980")
		VARIANT="VS980"
		if [ "$ANDROID" = Nougat ]; then
			DEFCONFIG="nougat_vs980_defconfig"
		else
			DEFCONFIG="marshmallow_vs980_defconfig"
		fi
		break;;
esac
done

echo ""
echo "You are going to build $VERSION for the $VARIANT variant."
export LOCALVERSION=-$NAME-$RELEASE-$TAG-$VARIANT
echo "Using the Linux tag: $LOCALVERSION."
echo ""

echo "Which toolchain you would like to use?"
echo -e ${red}"WARNING: Linaro 4.9 is not supported anymore, use it as a template only!"${restore}
select choice in Linaro-4.9 Dorimanx-5.4
do
case "$choice" in
	"Linaro-4.9")
		export TOOLCHAIN="Linaro 4.9"
		export CROSS_COMPILE="${LINARO_DIR}/4.9/bin/arm-eabi-"
		break;;
	"Dorimanx-5.4")
		export TOOLCHAIN="Dorimanx 5.4"
		export CROSS_COMPILE="ccache ${DORIMANX_DIR}/bin/arm-eabi-"
		export SYSROOT="${DORIMANX_DIR}/arm-LG-linux-gnueabi/sysroot/"
		export CC="${DORIMANX_DIR}/bin/arm-eabi-gcc --sysroot=$SYSROOT"
		export STRIP="${DORIMANX_DIR}/bin/arm-eabi-strip"
		break;;
esac
done

echo ""
echo "You have chosen to use $TOOLCHAIN."

echo

while read -p "Are you ready to start (Y/N)? " dchoice
do
case "$dchoice" in
	y|Y)
		clean_ccache
		prepare_all
		echo
		echo "Flowing..."
		checkout_branches
		echo "Building kernel with $THREAD argument..."
		make_zImage
		create_dtimg
		copy_modules
		changelog
		make_zip
		generate_md5
		break
		;;
	n|N)
		echo
		echo "This can't be happening... Tell me you're OK,"
		echo "Snake! Snaaaake!"
		echo
		exit
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
