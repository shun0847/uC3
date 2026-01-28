SUMMARY = "eForce sample Linux image for development."

IMAGE_INSTALL = "packagegroup-core-boot ${CORE_IMAGE_EXTRA_INSTALL}"

IMAGE_LINGUAS = " "

LICENSE = "CLOSED"

inherit core-image

IMAGE_ROOTFS_SIZE ?= "8192"
IMAGE_ROOTFS_EXTRA_SPACE:append = "${@bb.utils.contains("DISTRO_FEATURES", "systemd", " + 4096", "" ,d)}"

PREFERRED_VERSION_libmetal = "2021.10%"
PREFERRED_VERSION_open-amp = "2021.10%"

IMAGE_INSTALL:append = " cpuctl"
IMAGE_INSTALL:append = " libmetal"
IMAGE_INSTALL:append = " open-amp"
IMAGE_INSTALL:append = " uc3-sample"
