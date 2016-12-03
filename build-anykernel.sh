#!/bin/bash
#
# Copyright - Ícaro Hoff <icarohoff@gmail.com>
#
#              \
#              /\
#             /  \
#            /    \
#
export SCRIPT_VERSION="3.0 (#ForçaChape)"

# Colorize
red='\033[01;31m'
green='\033[01;32m'
yellow='\033[01;33m'
blue='\033[01;34m'
blink_red='\033[05;31m'
blink_green='\033[05;32m'
blink_yellow='\033[05;33m'
blink_blue='\033[05;34m'
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
DORIMANX5_DIR="${TOOLCHAINS_DIR}/dorimanx-5.x"
DORIMANX6_DIR="${TOOLCHAINS_DIR}/dorimanx-6.x"
PATCH_DIR="${REPACK_DIR}/patch"
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
	if [ $USE_CCACHE == true ]; then
		CCACHE=`which ccache`
	else
		# Empty if USE_CCACHE is not set.
		CCACHE=""
	fi;
	echo -e ${yellow}"Ccache information:"${restore}
	# Print binary location as well.
	echo "binary location                     $CCACHE_BINARY"
	ccache -s
	echo ""
}

function prepare_all {
	cd $REPACK_DIR
	rm -f zImage
	rm -f dt.img
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
	make $THREAD
	cp -f arch/arm/boot/zImage $REPACK_DIR/zImage
}

function create_dtimg {
	# If there's no zImage, compilation clearly failed.
	# Avoid building the device tree image in this case.
	if [ -f $REPACK_DIR/zImage ]; then
		$REPACK_DIR/tools/dtbToolCM -v -s 2048 -o $REPACK_DIR/dt.img arch/arm/boot/ > /dev/null 2>&1 # Suppressed, 47!
	fi;
}

function changelog {
	sh changelog.sh > /dev/null 2>&1 # Suppressed, 47!
	cp changelog.txt $REPACK_DIR/ramdisk/sbin/changelog.txt
}

function make_zip {
	cd $REPACK_DIR
	if [ -f zImage ]; then
		zip -x@zipexclude -r9 "$VERSION"-"$VARIANT"-"$BUILD_DATE".zip *;
		mv "$VERSION"-"$VARIANT"-"$BUILD_DATE".zip $ZIP_MOVE;
		export COMPILATION="success"
	else
		echo ""
		echo -e ${red}"Kernel image not found, compilation failed."${restore}
		export COMPILATION="sucks"
	fi;
	cd $KERNEL_DIR;
}

function generate_md5 {
	if [ "$COMPILATION" = success ]; then
		cd $ZIP_MOVE
		md5sum "$VERSION"-"$VARIANT"-"$BUILD_DATE".zip > "$VERSION"-"$VARIANT"-"$BUILD_DATE".zip.md5
		cd $KERNEL_DIR
	fi;
}


DATE_START=$(date +"%s")
echo -e "${blue}"
echo "                   \                    "
echo "                   /\                   "
echo "                  /  \                  "
echo "                 /    \                 "
echo -e "${restore}"
echo -e "${blink_blue}" "This is the ultimate Kernel build script, $USER. " "${restore}"
echo -e "${blink_green}" "Version: $SCRIPT_VERSION " "${restore}"
echo ""
check_folders
if [ $USE_CCACHE == true ]; then
	ccache_setup
else
	echo -e ${blue}"Optional:"${restore}
	echo -e ${yellow}"Add 'export USE_CCACHE=true' to your shell configuration to enable ccache."${restore}
	echo ""
fi;

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

echo -e ${blue}"Marshmallow -> Type M"${restore}
echo -e ${green}"Nougat -> Type N"${restore}
echo ""
while read -p "Please, select the Android version (M/N)? " achoice
do
case "$achoice" in
	m|M)
		echo
		echo -e ${blue}"Building marshmallow compatible kernel..."${restore}
		ANDROID="Marshmallow"
		ANYBRANCH="marshmallow"
		echo
		break
		;;
	n|N)
		echo
		echo -e ${green}"Building nougat compatible kernel..."${restore}
		ANDROID="Nougat"
		ANYBRANCH="nougat"
		echo
		break
		;;
	* )
		echo
		echo -e ${blue}"Assuming marshmallow as Android version..."${restore}
		ANDROID="Marshmallow"
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
echo -e ${blue}"You are going to build $VERSION for the $VARIANT variant."${restore}
export LOCALVERSION=-$NAME-$RELEASE-$TAG-$VARIANT
echo -e ${blue}"Using the Linux tag: $LOCALVERSION."${restore}
echo ""

echo "Which toolchain you would like to use?"
select choice in Dorimanx-5.4 Dorimanx-6.1 #Linaro-4.9 (This is a template, add custom choices here...)
do
case "$choice" in
	"Dorimanx-5.4")
		export TOOLCHAIN="Dorimanx 5.4"
		export CROSS_COMPILE="${DORIMANX5_DIR}/bin/arm-eabi-"
		export SYSROOT="$CCACHE ${DORIMANX5_DIR}/arm-LG-linux-gnueabi/sysroot/"
		export CC="${DORIMANX5_DIR}/bin/arm-eabi-gcc --sysroot=$SYSROOT"
		export STRIP="${DORIMANX5_DIR}/bin/arm-eabi-strip"
		break;;
	"Dorimanx-6.1")
		export TOOLCHAIN="Dorimanx 6.1"
		export CROSS_COMPILE="$CCACHE ${DORIMANX6_DIR}/bin/arm-eabi-"
		export SYSROOT="${DORIMANX6_DIR}/arm-LG-linux-gnueabi/sysroot/"
		export CC="${DORIMANX6_DIR}/bin/arm-eabi-gcc --sysroot=$SYSROOT"
		export STRIP="${DORIMANX6_DIR}/bin/arm-eabi-strip"
		break;;
	#
	# Template:
	# This is a template for any other GCC compiler you'd like to use. Just put
	# the compiler in a given folder under toolchains directory and point it here,
	# the subfolder is used to keep the directory organized. The executables are
	# the only thing that matters, make sure you point them properly taking the
	# prefix 'arm-eabi-' as the normal executable naming for ARM 32-bit toolchains.
	#
	#"Linaro-4.9")
		#export TOOLCHAIN="Linaro 4.9"
		#export CROSS_COMPILE="$CCACHE ${TOOLCHAINS_DIR}/linaro/4.9/bin/arm-eabi-"
		#break;;
esac
done

echo ""
echo "You have chosen to use $TOOLCHAIN."

echo

while read -p "Are you ready to start (Y/N)? " dchoice
do
case "$dchoice" in
	y|Y)
		prepare_all
		echo
		echo "Flowing..."
		checkout_branches
		echo "Building kernel with $THREAD argument..."
		make_zImage
		create_dtimg
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
if [ "$COMPILATION" = sucks ]; then
	echo -e "${blink_red}"
	echo "                 \                  "
	echo "                 /\                 "
	echo "                /  \                "
	echo "               /    \               "
	echo -e "${restore}"
	echo -e ${blink_red}"You tried your best and you failed miserably."${restore}
	echo -e ${blink_red}"The lesson is, NEVER TRY!"${restore}
else
	echo -e "${blink_green}"
	echo "                 \                  "
	echo "                 /\                 "
	echo "                /  \                "
	echo "               /    \               "
	echo -e "${restore}"
	echo -e ${blink_green}"Completed in $(($DIFF / 60)) minute(s) and $(($DIFF % 60)) seconds."${restore}
fi;
