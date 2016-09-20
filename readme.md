Lambda Kernel for the LG G2
==========================

> There’s only one horizon, yet it can be found.

A general purpose custom Kernel for the **LG G2** that embraces freedom and speed on a **CAF** oriented shape.

Information
-------------------------

- Linux version: **3.4.112**
- Dorimanx's toolchain: **GCC 5.4**
- Android targets: **Marhsmallow** - **Nougat**
- Packager: **AnyKernel2**

Dependencies
-------------------------

As any other **Linux Kernel**, it depends on other software to compile it. I'll list them per GNU operating system distributions:

###### Fedora/CentOS/RHEL

You'll need to install the development tools group at first.

	sudo dnf group install 'Development Tools'

It's worth installing **ncurses** in case you want to change the configuration from a **GUI**.

	sudo dnf install ncurses-devel

###### Ubuntu/Debian

I don't use Ubuntu but the dependencies are generically the same with a couple differences in naming or versioning. In Ubuntu, you will need to setup source dependencies by going to **System Settings > Software and Update** and then checking the box for source code. The basic set of programs needed might be achieved with the following command.

	sudo apt-get install build-essential libncurses5 libncurses5-dev libelf-dev binutils-dev liblz4-tool ccache device-tree-compiler open-jdk-8-jdk git
	sudo apt-get build-dep build-essential libncurses5 libncurses5-dev libelf-dev binutils-dev # This step may be needed for VM environments.
	
###### Arch Linux

If you're running Arch Linux you probably already have the dependencies needed, in any case you can install them running:

	sudo pacman -S base-devel kmod inetutils bc libelf

If you really follow the K.I.S.S. principle you'll probably only need the **base-devel** and it's present in 99% of the installations.

###### Unconventional

Distributions of **GNU/Linux** that I call _unconventional_: **Funtoo**, **Gentoo**, **Sabayon** and **Slackware**. They can also be called as source based distros.
Since you need to **compile** the Kernel yourself when _chrooting_ in the core of the system, the dependencies were already pulled and compiled during the installation.

Compilation
-------------------------

Create a development folder if you don't have one yet.

	mkdir Development && cd Development # Name it as you'd like, this is not hardcoded.

Create the kernel folder inside the development folder.

	mkdir -p kernel/lambda # To avoid conflicts with any other kernel you might already have.
	cd kernel/lambda

Clone this kernel repository.

	git clone https://github.com/GalaticStryder/kernel_lge_msm8974 lge_msm8974

Before going any further, you'll need to download the __anykernel__ packager.

	git clone https://github.com/GalaticStryder/anykernel_lge_msm8974 anykernel

And then, download the **GCC compiler**. Also known as **toolchain**.

A fair advise, the current state of **Lambda Kernel** on this particular device will not allow the usage of old toolchains. Though you can use this **Linaro 4.9** __example__ to download your own **custom** toolchain. If I were you I'd just skip to the next instruction block.

	mkdir -p toolchains/linaro/4.9
	git clone https://github.com/Christopher83/arm-cortex_a15-linux-gnueabihf-linaro_4.9 toolchains/linaro/4.9 # Don't follow this command at all!

Modify the **build-anykernel** script to point to your custom toolchain following the Linaro 4.9 **example** as well.

The **"right"** toolchain we use for this particular device comes from **@dorimanx**, you **must** use it as of now.

	# OBS: Dorimanx's toolchain is hosted in his own LG G2 kernel.
	cd lge_msm8974
	mkdir -p ../toolchains
	git remote add dorimanx https://github.com/dorimanx/DORIMANX_LG_STOCK_LP_KERNEL
	git fetch dorimanx master
	git checkout dorimanx/master # This will get into dorimanx kernel tree.
	cp -R android-toolchain/ ../toolchains/
	mv ../toolchains/android-toolchain ../toolchains/dorimanx # This is just a renaming method.
	git checkout lambda # This will get into lambda kernel tree back again.

Finally everything will be settled down and ready to compile, just **run**:

	./build-anykernel.sh

Follow the on-screen guide to compile your variant for a given Android version.

Mentions
-------------------------

- savoca
- myfluxi
- dorimanx
- bbedward
- alucard24
- neobuddy89
- sultanxda
- humberos
- showp1984
- faux123

Licence
-------------------------

The Lambda project is licensed under the [GNU General Public Licence - Version 3](gpl-3.0.md).

The original Linux readme has been moved to [LINUX-README](LINUX-README) and its license can be found in [COPYING](COPYING).
