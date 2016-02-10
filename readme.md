Lambda Kernel
==========================

Focused on supplying up-to-date patches to the **LG G2** using **CyanogenMod** branches. We use custom _linaro_ gcc toolchains and _AnyKernel2_ as build environment.

Details
-------------------------

- Targets all LG G2 variants
- Linaro LTS toolchains: GCC 4.9 and GCC 5.2
- Linux updates per commit for a better control over what is needed or not
- Custom build script with version and GCC selection support
- Tweaked AnyKernel2 packing with post boot add-on
- Modded Universal Kernel Manager (UKM/Synapse) in a unique way

Instructions
-------------------------

	git clone https://github.com/GalaticStryder/kernel_lge_msm8974 source -b rework
	cd source
	./sync
	./build-anykernel

Mentions
-------------------------

- RenderBroken
- Savoca
- MyFluxi
- AK
- Snuzzo
- Osmosis
- Christopher83

Licence
-------------------------

This project is licensed under [GPL3](gpl-3.0.md).
The original Linux readme has been moved to [LINUX-README](LINUX-README). Its license can be found in [COPYING](COPYING).
