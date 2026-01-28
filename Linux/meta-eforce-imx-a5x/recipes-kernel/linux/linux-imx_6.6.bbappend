FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

# List of kernel patches
SRC_URI += " \
    file://0001-uc3dts-setting.patch \
    file://0002-smp.patch \
    file://0003-irq-gic.patch \
    file://0004-irq-gic-v3.patch \
    file://0005-irq-gic-common-h.patch \
    file://0006-irq-gic-common-c.patch \
    file://0007-uio.patch \
    file://0008-uio-pdrv-genirq.patch \
    file://0009-uio-driver.patch \
    file://uio.cfg"

# Workaround for i.MX yocto Linux kernel to add configurations
do_copy_defconfig:append() {
    cat ${WORKDIR}/uio.cfg >> ${B}/.config
    cat ${WORKDIR}/uio.cfg >> ${B}/../defconfig
}
