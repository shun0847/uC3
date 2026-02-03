FILESEXTRAPATHS:prepend := "${THISDIR}/2021.10:"
SRCBRANCH ?= "master"
SRCREV ?= "4ead69b8f4194ea60d4d78bc1d8b57b45b467699"
LIC_FILES_CHKSUM ?= "file://LICENSE.md;md5=fe0b8a4beea8f0813b606d15a3df3d3c"

SRC_URI += " \
  file://0001-processor-arm-yield.patch \
  "
include libmetal.inc

