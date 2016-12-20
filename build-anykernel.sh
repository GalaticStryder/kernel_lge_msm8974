#!/bin/bash
#
# Copyright - Ícaro Hoff <icarohoff@gmail.com>
#
#              \
#              /\
#             /  \
#            /    \
#
export SCRIPT_VERSION="3.2.1 (#ForçaChape)"

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

# Arguments
first=${1};

# Resources
THREAD="-j$(grep -c ^processor /proc/cpuinfo)"

# Variables
export ARCH=arm
export SUBARCH=arm

# Branches
ANYBRANCH="lambda"

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

function variant_assign {
	spec=${1};
	DEFCONFIG="lambda_'$spec'_defconfig"
	if [ "$spec" == d800 ]; then
		VARIANT="D800"
	elif [ "$spec" == d801 ]; then
		VARIANT="D801"
	elif [ "$spec" == d802 ]; then
		VARIANT="D802"
	elif [ "$spec" == d803 ]; then
		VARIANT="D803"
	elif [ "$spec" == f320 ]; then
		VARIANT="F320"
	elif [ "$spec" == l01f ]; then
		VARIANT="L01F"
	elif [ "$spec" == ls980 ]; then
		VARIANT="LS980"
	elif [ "$spec" == vs980 ]; then
		VARIANT="VS980"
	fi
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
	if [ ! "$first" == "--serialized" ]; then
		if [ -f zImage ]; then
			zip -x@zipexclude -r9 "$VERSION"-"$VARIANT"-"$BUILD_DATE".zip *;
			mv "$VERSION"-"$VARIANT"-"$BUILD_DATE".zip $ZIP_MOVE;
			export COMPILATION="success"
		else
			echo ""
			echo -e ${red}"Kernel image not found, compilation failed."${restore}
			export COMPILATION="sucks"
		fi;
	else if [ "$first" == "--serialized" ]; then
		spec=${1};
		if [ -f zImage ]; then
			zip -x@zipexclude -r9 "$VERSION"-"$VARIANT"-"$BUILD_DATE".zip *;
			mv "$VERSION"-"$VARIANT"-"$BUILD_DATE".zip $ZIP_MOVE;
			echo ""
			echo -e ${green}"Successfully built Lambda Kernel for $i"${restore}
			export COMPILATION="success"
		else
			echo ""
			echo -e ${red}"Kernel image not found, compilation failed for $i."${restore}
			export COMPILATION="sucks"
		fi;
	fi;
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

# Versioning
NAME="Lambda"
RELEASE="Infinito"
if [ "$STATE" = stable ]; then
	TAG="Stable"
	VERSION="$NAME-$RELEASE-$TAG"
fi
if [ "$STATE" = beta ]; then
	TAG="Beta"
	echo "Could you assign a beta number?"
	read -e tag_number
	TAG_NUMBER="$tag_number"
	echo ""
	VERSION="$NAME-$RELEASE-$TAG-N$TAG_NUMBER"
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
	VERSION="$NAME-$RELEASE-$TAG-N$TAG_NUMBER-$TAG_COMMENT"
fi
export LOCALVERSION=-$VERSION
BUILD_DATE=$(date -u +%m%d%Y)

if [ ! "$first" == "--serialized" ]; then
	echo "Would you mind picking an LG G2 variant?"
	select choice in d800 d801 d802 d803 f320 l01f ls980 vs980
	do
	case "$choice" in
		"d800")
			VARIANT="D800"
			DEFCONFIG="lambda_d800_defconfig"
			break;;
		"d801")
			VARIANT="D801"
			DEFCONFIG="lambda_d801_defconfig"
			break;;
		"d802")
			VARIANT="D802"
			DEFCONFIG="lambda_d802_defconfig"
			break;;
		"d803")
			VARIANT="D803"
			DEFCONFIG="lambda_d803_defconfig"
			break;;
		"f320")
			VARIANT="F320"
			DEFCONFIG="lambda_f320_defconfig"
			break;;
		"l01f")
			VARIANT="L01F"
			DEFCONFIG="lambda_l01f_defconfig"
			break;;
		"ls980")
			VARIANT="LS980"
			DEFCONFIG="lambda_ls980_defconfig"
			break;;
		"vs980")
			VARIANT="VS980"
			DEFCONFIG="lambda_vs980_defconfig"
			break;;
	esac
	done

	echo ""
	echo -e ${blue}"You are going to build $VERSION for the $VARIANT variant."${restore}
	echo -e ${blue}"Using the Linux tag: $LOCALVERSION."${restore}
	echo ""
fi

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

if [ ! "$first" == "--serialized" ]; then
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
else if [ "$first" == "--serialized" ]; then
	function build {
		spec=("$@")
		for i in "${spec[@]}";
			do
				prepare_all
				echo
				echo "Flowing..."
				checkout_branches
				echo ""
				echo "Building kernel with $THREAD argument..."
				variant_assign "$i"
				echo ""
				echo -e ${blue}"Building the following variant: $i"${restore}
				export LOCALVERSION=-$NAME-$RELEASE-$TAG
				echo -e ${blue}"Using the Linux tag: $LOCALVERSION."${restore}
				make_zImage
				create_dtimg
				changelog
				make_zip "$i"
				echo ""
				generate_md5
			done
	}
	# We want all of them right now!
	variant=( d800 d801 d802 d803 f320 l01f ls980 vs980 )
	build "${variant[@]}"
fi
fi

DATE_END=$(date +"%s")
DIFF=$(($DATE_END - $DATE_START))
if [ "$first" == "--serialized" ]; then
	if [ "$COMPILATION" = sucks ]; then
		echo -e ${blink_red}"                 \                  "
		echo "                 /\                 "
		echo "                /  \                "
		echo "               /    \               "
		echo -e "${restore}"
		echo -e ${blink_red}"Serialized mode finished, one or more variants failed to compile."${restore}
		echo -e ${blink_red}"The lesson is, GIVE UP!"${restore}
	else
		echo -e ${blink_yellow}"                 \                  "
		echo "                 /\                 "
		echo "                /  \                "
		echo "               /    \               "
		echo -e "${restore}"
		echo -e ${blink_yellow}"Serialized mode finished, all variants compiled."${restore}
		echo -e ${blink_yellow}"Completed in $(($DIFF / 60)) minute(s) and $(($DIFF % 60)) seconds."${restore}
	fi;
else
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
fi;
