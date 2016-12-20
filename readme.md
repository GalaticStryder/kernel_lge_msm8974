Lambda Kernel for the LG G2
==========================

> There’s only one horizon, yet it can be found.

A general purpose custom **Kernel** for the **LG G2** that embraces freedom and speed on a **CAF** oriented shape.

Information
-------------------------

- Linux version: **3.4.113**
- Compiler: **Dorimanx GCC 6.1**
- Packager: **AnyKernel2**

Dependencies
-------------------------

As any other **Linux Kernel**, it depends on other software to compile it. I'll list them per GNU operating system distributions:

###### Fedora/CentOS/RHEL

You'll need to install the development tools group at first.

	sudo dnf group install 'Development Tools'

It's worth installing **ncurses** in case you want to change the configuration from a **GUI**.

	sudo dnf install ncurses-devel

You might also need some compression and device tree tools like lz4 and dtc.

	sudo dnf install lz4 lz4-devel dtc

###### Ubuntu/Debian

I don't use Ubuntu but the dependencies are generically the same with a couple differences in naming or versioning. In Ubuntu, you will need to setup source dependencies by going to **System Settings > Software and Update** and then checking the box for source code. The basic set of programs needed might be achieved with the following command.

	sudo apt-get install build-essential libncurses5 libncurses5-dev libelf-dev binutils-dev liblz4-tool device-tree-compiler open-jdk-8-jdk git
	sudo apt-get build-dep build-essential libncurses5 libncurses5-dev libelf-dev binutils-dev # This step may be needed for VM environments.

###### Arch Linux

If you're running Arch Linux you probably already have the dependencies needed, in any case you can install them running:

	sudo pacman -S base-devel kmod inetutils bc libelf dtc

If you really follow the K.I.S.S. principle you'll probably only need the **base-devel** and it's present in 99% of the installations.

Also, if the host Kernel was not compressed with lz4, you might not have that package installed, install it if you don't.

	sudo pacman -S lz4

###### Unconventional

Distributions of **GNU/Linux** that I call _unconventional_: **Funtoo**, **Gentoo**, **Sabayon** and **Slackware**. They can also be called as source based distros.
Since you need to **compile** the Kernel yourself when _chrooting_ in the core of the system, the dependencies were already pulled and compiled during the installation.

Here's a brief exchange of the tools you'll need on **Gentoo**:

	su
	emerge --sync
	emerge --ask app-arch/lz4
	emerge --ask sys-apps/dtc

You can also punch everything into one single command, compat or declare the ebuild path at will. It's up to you.

Compilation
-------------------------

To make the **guide** easier, I've created some headers for: **obligatory** and **optional** steps. Make sure you're reading everything carefully and following those headers.

###### Optional - Creating the folders

Create a development folder if you don't have one yet.

	mkdir Development && cd Development # Name it as you'd like, this is not hardcoded.

Create the Kernel folder inside the development folder.

	mkdir lambda # Create the folder as you wish, this is not hardcoded.
	cd lambda

###### Obligatory - Download the source code

We have an automated downloader script to get everything needed per **Android** version to be checked out after sync. This utility is also used when you want to get the latest version of the **Kernel** without "troubles" doing weird merges yourself. Just run it anytime you want to upgrade your local **Lambda** tree.

The script will download the following **three objects** and place them correctly as the _build-anykernel_ compilation script wants, it's all handled automatically.

| Object        | Description           |
| ------------- |:---------------------:|
| [Kernel](https://github.com/GalaticStryder/kernel_lge_msm8974) | Lambda Kernel for the LG G2 |
| [AnyKernel](https://github.com/GalaticStryder/kernel_lge_msm8974) | AnyKernel for the LG G2 |
| [Toolchain](https://github.com/dorimanx/DORIMANX_LG_STOCK_LP_KERNEL) | Dorimanx's GCC 6.1 |

	wget https://gist.githubusercontent.com/GalaticStryder/d4f189e6dac50f755f2c5e1e7dcdad92/raw/a55eba3a07e0b8f4d884ff1a5fd0582598935002/sync-lambda.sh
	chmod a+x sync-lambda.sh
	./sync-lambda.sh

###### Optional - Using SSH instead of HTTPS

When downloading the source code, the automated script uses **HTTPS** by default, although, if you have a configured **git** on your computer, you can use this protocol instead. Just pass the argument _ssh_ in **sync-lambda.sh**.

	./sync-lambda.sh ssh # To download the source code and toolchain using git protocol.

###### Optional - Getting an older version of the main toolhain

If you want to use an **older** version of the main toolchain for whatever reason, you can checkout dorimanx's tree, which was already downloaded before, at any point in **git** history that contains that specific version.

	git checkout dorimanx/master # This will get into dorimanx Kernel tree again.

Remember, the master branch uses always the latest **GCC** version. To get the older one in place, take a look at the commit history and copy the commit **SHA** to check it out.

	git checkout 993282bdb9eb4156a724c07782bf058f42d6470e # This is GCC 5.4 by the way.
	cp -R android-toolchain/ ../toolchains/
	mv ../toolchains/android-toolchain ../toolchains/dorimanx-5.x # This is just a renaming method to avoid conflicts.
	git checkout lambda # This will get into lambda Kernel tree back again.

###### Optional - Using custom a toolchain

If you want to use a completely different toolchain set, although not recommended, you're able to do so by editing the template for **Linaro 4.9** in _build-anykernel.sh_. Here's an example on how you'd do to use **Linaro 4.9** compiler for **ARM Cortex A15**.

	mkdir -p ../toolchains/linaro/4.9
	git clone https://github.com/Christopher83/arm-cortex_a15-linux-gnueabihf-linaro_4.9 ../toolchains/linaro/4.9 # Don't follow this command at all!

Uncomment the **Linaro 4.9** template related code and read the information on top of it. Modify it as you wish to point the new compiler executables and the _information/name_ for the new compiler.

	$EDITOR build-anykernel.sh

###### Obligatory - Starting the compilation

Finally, everything will be settled down and ready to compile, just **run**:

	./build-anykernel.sh

Follow the on-screen guide to compile **Lambda Kernel**. The products will be located under the **store/** folder already _zipped_ with _md5_ files, ready to be flashed on _recovery_.

###### Optional - Compiling all variants in a row

The script is also capable to detect arguments such as _'serialized'_, with two dashes and without the quotes obviously.

	./build-anykernel.sh --serialized

Follow the on-screen guide to compile **Lambda Kernel**. All products will be located under the **store/** folder already _zipped_ with _md5_ files, ready to be shared _online_.

###### Optional - Compiling with ccache tunneling

The **Kernel** build script identifies the **USE_CCACHE** variable from the environment and tell the compiler to use it. Make sure you have **ccache** installed when using it, it's usually called **ccache** in most **GNU/Linux** distributions.

- **Fedora/CentOS/RHEL**

		 sudo dnf install ccache

- **Ubuntu/Debian**

		 sudo apt-get install ccache

- **Arch Linux**

		 sudo pacman -S ccache

- **Gentoo/Funtoo**

		 emerge --ask dev-util/ccache

You'll be able to speed up your builds considerably by compling the **Kernel** with ccache enabled, which "memorizes" the common parts of a _c_ file in a folder instead of reading the whole file again. This is very useful for this device due to the number of variants it has.
In order to enable it, just open your shell configuration file _(e.g.: ~/.zshrc, ~/.bashrc...)_ and add the following lines somewhere:

	export USE_CCACHE="true"
	export CCACHE_DIR="/home/$USER/.ccache"

Last but not least, add the binary folder to your **$PATH** under the **ccache** variables. Generally located under _/usr/lib/ccache/_.

	export PATH="/usr/lib/ccache:$PATH

This will add and not replace the other **$PATH** variables already set. Exit the editor and set the **ccache** size in the terminal, usually **2G** is enough for many compilations.

	ccache -M 2G

Clear the cache and reset the stats every once in a while to avoid clutter on our system.

	ccache -C -z

###### Optional - Pushing the files with ADB

This is how I organize my **Kernel** files on the device, under the _/sdcard/Flash/_ directory which is an alphabetical recovery flashing queue.

	adb shell mkdir /sdcard/Flash;
	adb push store/Lambda-Infinito-Experimental-N348-WILD-D802-12202016.zip* /sdcard/Flash;

Contributors
-------------------------

The developers and users that helped with the **Lambda Kernel** project.

- savoca
- blastagator
- myfluxi
- dorimanx
- bamonkey
- alucard24
- neobuddy89
- sultanxda
- showp1984
- faux123

Licence
-------------------------

The Lambda project is licensed under the [GNU General Public Licence - Version 3](gpl-3.0.md).

The original Linux readme has been moved to [LINUX-README](LINUX-README) and its license can be found in [COPYING](COPYING).
