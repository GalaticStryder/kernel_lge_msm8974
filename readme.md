Lambda Kernel for the LG G2
==========================

> There’s only one horizon, yet it can be found.

A general purpose custom Kernel for the **LG G2** that embraces freedom and speed on a **CAF** oriented shape.

Information
-------------------------

- Linux version: **3.4.112**
- Linaro LTS toolchain: **GCC 4.9**
- Dorimanx's toolchain: **GCC 5.3**
- **AnyKernel2** packager

Dependencies
-------------------------

As any other **Linux Kernel**, it depends on other software to compile it. I'll list them per GNU operating system distributions:

###### Fedora/CentOS/RHEL

You'll need to install the development tools group at first.

	sudo dnf group install 'Development Tools'

It's worth installing **ncurses** in case you want to change the configuration from a **GUI**.

	sudo dnf install ncurses-devel

###### Ubuntu/Debian

I don't use Ubuntu but the dependencies are generically the same with a couple differences in naming or versioning. The basic set of programs needed might be achieved with the following command.

	sudo apt-get install build-essential libncurses5 libncurses5-dev libelf-dev binutils-dev build-dep

###### Arch Linux

If you're running Arch Linux you probably already have the dependencies needed, in any case you can install them running:

	sudo pacman -S base-devel kmod inetutils bc libelf

If you really follow the K.I.S.S. principle you'll probably only need the **base-devel** and it's present in 99% of the installations.

###### Unconventional

Distributions of **GNU/Linux** that I call _unconventional_: **Funtoo**, **Gentoo**, **Sabayon** and **Slackware**. They can also be called as source based distros.
Since you need to **compile** the Kernel yourself when _chrooting_ in the core of the system, the dependencies were already pulled and compiled during the installation.

Compilation
-------------------------

Create a development folder if you don't have it already.

	mkdir Development && cd Development

Create the kernel folder inside the development folder.

	mkdir kernel

Clone the kernel repository.

	git clone https://github.com/GalaticStryder/kernel_lge_msm8974 lge_msm8974

Before going any further, you'll need to download the utilities and toolchains.

	git clone https://github.com/GalaticStryder/anykernel_lge_msm8974 anykernel
	mkdir toolchains
	cd toolchains
	git clone https://github.com/Christopher83/arm-cortex_a15-linux-gnueabihf-linaro_4.9 linaro/4.9
	# OBS: Dorimanx's toolchain is hosted in his own LG G2 kernel, you'll need a little extra pain to get it, although I'll explain further.

Now, you can go back to the kernel directory and then step forward for compilation.

	cd ../lge_msm8974
	mkdir store # This is where the builds will be stored.
	./build-anykernel

Follow the on-screen guide to compile your variant.

If you want to use Dorimanx's toolchain, you'll need to:

	git remote add dorimanx https://github.com/dorimanx/DORIMANX_LG_STOCK_LP_KERNEL
	git fetch dorimanx master
	git checkout dorimanx/master
	cp -R android-toolchain/ ../toolchains/
	mv ../toolchains/android-toolchain ../toolchains/dorimanx # This is just a renaming method.

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
