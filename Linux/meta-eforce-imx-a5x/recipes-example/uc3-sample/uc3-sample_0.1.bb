#
# uC3/Standard RPMsg Sample for Linux
#

SUMMARY = "Sample uc3 rpmsg application"
SECTION = "examples"
LICENSE = "CLOSED"
DEPENDS = "libmetal open-amp"
#INHIBIT_PACKAGE_DEBUG_SPLIT = "1"
#INHIBIT_PACKAGE_STRIP = "1"

SRC_URI = " \
    file://ampext_shmem.c \
    file://ampext_shmem.h \
    file://ampext_shmem_cfg.h \
    file://ampext_shmem_spinlock.c \
    file://ampext_shmem_spinlock.h \
    file://platform_info.c \
    file://platform_info.h \
    file://platform_info_cfg.c \
    file://OpenAMP_RPMsg_cfg.h \
    file://helper.c \
    file://rsc_table.h \
    file://main.c \
    file://imx8_a5x_rproc.c \
    file://Makefile \
    file://Makeconf"

S = "${WORKDIR}"

do_install() {
    install -d ${D}${bindir}
    install -m 0755 uc3_rpmsg_sample_client ${D}${bindir}
}

