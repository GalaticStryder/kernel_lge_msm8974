Lambda Kernel for the LG G2
==========================

> There’s only one horizon, yet it can be found.

A general purpose custom Kernel for the **LG G2** that embraces freedom and speed on a **CAF** oriented shape.

Information
-------------------------

- Linux version: **3.4.113**
- Dorimanx's toolchain: **GCC 5.4** - **GCC 6.1**
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

Also, if the host kernel was not compressed with lz4, you might not have that package installed, install it if you don't.

	sudo pacman -S lz4

###### Unconventional

Distributions of **GNU/Linux** that I call _unconventional_: **Funtoo**, **Gentoo**, **Sabayon** and **Slackware**. They can also be called as source based distros.
Since you need to **compile** the Kernel yourself when _chrooting_ in the core of the system, the dependencies were already pulled and compiled during the installation.

Here's a brief exchange of the tools you'll need on **Gentoo**:

	su
	emerge --sync
	emerge lz4 # Located in app-arch/lz4.
	emerge dtc # Located in sys-apps/dtc.

You can also punch everything into one single command, compat or declare the ebuild path at will. It's up to you.

Compilation
-------------------------

To make the **guide** easier, I've created some headers for: **obligatory**, **optional** and **example** steps. Make sure you're reading everything carefully and following those headers.

###### Obligatory

Create a development folder if you don't have one yet.

	mkdir Development && cd Development # Name it as you'd like, this is not hardcoded.

Create the kernel folder inside the development folder.

	mkdir -p kernel/lambda # To avoid conflicts with any other kernel you might already have.
	cd kernel/lambda

We have an automated downloader script to get everything needed per **Android** version to be checked out after sync. Currently, anykernel is the only repo that benefits of this branch selector since the kernel is **common** for both **Android** versions.

	curl https://gist.githubusercontent.com/GalaticStryder/d4f189e6dac50f755f2c5e1e7dcdad92/raw/886ff1cc57a0f19b7af71a22e3d14861ad394be0/sync-lambda.sh | sh

###### Optional

This script uses **HTTPS** protocol. It's also possible that you want to use **git**. You'll need to **wget** the script locally and edit to use that protocol.

	wget https://gist.githubusercontent.com/GalaticStryder/d4f189e6dac50f755f2c5e1e7dcdad92/raw/886ff1cc57a0f19b7af71a22e3d14861ad394be0/sync-lambda.sh
	chmod sync-lambda.sh
	$EDITOR sync-lambda.sh

Change the "https://github.com/" to "git@github.com:" in both cloning processes.

###### Obligatory

And then finally, download the **GCC compiler**. Also known as **toolchain**.

###### Example

A fair advise, the current state of **Lambda Kernel** on this particular device **will not** allow the usage of old toolchains. Though you can use this **Linaro 4.9** __example__ to download your own **custom** toolchain. If I were you I'd just skip to the next instruction block.

	mkdir -p toolchains/linaro/4.9
	git clone https://github.com/Christopher83/arm-cortex_a15-linux-gnueabihf-linaro_4.9 toolchains/linaro/4.9 # Don't follow this command at all!

Modify the **build-anykernel** script to point to your custom toolchain following the Linaro 4.9 **example** as well.

###### Obligatory

The **"right"** toolchain we use for this particular device comes from **@dorimanx**, you **must** use it as of now. The current version is **GCC 6.1**.

	# OBS: Dorimanx's toolchain is hosted in his own LG G2 kernel.
	cd lge_msm8974
	mkdir -p ../toolchains
	git remote add dorimanx https://github.com/dorimanx/DORIMANX_LG_STOCK_LP_KERNEL
	git fetch dorimanx master
	git checkout dorimanx/master # This will get into dorimanx kernel tree.
	cp -R android-toolchain/ ../toolchains/
	mv ../toolchains/android-toolchain ../toolchains/dorimanx-6.x # This is just a renaming method.
	git checkout lambda # This will get into lambda kernel tree back again.

If you want to use an **older** version of his toolchain, you can checkout his tree at any point in git history that contains that specific version.

###### Example

Go to dorimanx kernel tree as you did **above**.

	git checkout dorimanx/master # This will get into dorimanx kernel tree again.

The master branch uses always the latest **GCC** version. To get the older one in place, take a look at the commit history and copy the commit **SHA** and check it out.

	git checkout 993282bdb9eb4156a724c07782bf058f42d6470e # This is GCC 5.4 by the way.
	cp -R android-toolchain/ ../toolchains/
	mv ../toolchains/android-toolchain ../toolchains/dorimanx-5.x # This is just a renaming method.
	git checkout lambda # This will get into lambda kernel tree back again.

###### Obligatory

Finally, everything will be settled down and ready to compile, just **run**:

	./build-anykernel.sh

Follow the on-screen guide to compile your variant for a given **Android** version compatibility.

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
