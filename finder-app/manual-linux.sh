#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -e
set -u

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.1.10
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
GNU_PATH=/home/marshall/Programs/install-lnx/gcc-arm-10.2-2020.11-x86_64-aarch64-none-linux-gnu/bin
LIB_PATH=/home/marshall/Programs/install-lnx/gcc-arm-10.2-2020.11-x86_64-aarch64-none-linux-gnu
CROSS_COMPILE=${GNU_PATH}/aarch64-none-linux-gnu-

if [ -d $GNU_PATH ]
then
    : # NOP
else
    echo "GNU_PATH Does NOT Exist"
    exit 1
fi

if [ $# -lt 1 ]
then
	echo "Using default directory ${OUTDIR} for output"
else
	OUTDIR=$1
	echo "Using passed directory ${OUTDIR} for output"
fi

mkdir -p ${OUTDIR}

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/linux-stable" ]; then
    #Clone only if the repository does not exist.
	echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
	git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
fi
if [ ! -e ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ]; then
    cd linux-stable
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}

    echo "Apply Ubuntu V22.04 patch"
    wget https://github.com/torvalds/linux/commit/e33a814e772cdc36436c8c188d8c42d019fda639.diff
    git apply e33a814e772cdc36436c8c188d8c42d019fda639.diff

    echo "Kernel build steps:"
    #echo "Installing dependencies if not already installed..."
    #/home/marshall/git/assignment-1-VamboozerCU/finder-app/dependencies.sh
    echo "Deep clean kernal build tree"
    make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- mrproper # Deep clean kernal build tree
    echo "Configure to default config => virtual arm"
    make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- defconfig # Configure to default config = virtual arm
    echo "Build kernal image"
    make -j4 ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- all # Build kernal image
    #echo "Build any kernal modules"
    #make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- modules # Build any kernal modules
    #echo "Build the device tree"
    #make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- dtbs # Build the device tree
fi

echo "Adding the Image in outdir"

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi

echo "Create necessary base directories"
mkdir -p ${OUTDIR}/rootfs
cd ${OUTDIR}/rootfs
mkdir -p bin dev etc home lib proc sbin sys tmp usr var
mkdir -p usr/bin usr/lib usr/sbin
mkdir -p var/log

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
    echo "Configure busybox"
    make distclean
    make defconfig
    #sudo make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- defconfig
else
    cd busybox
fi

echo "Make and install busybox"
#sudo make ARCH=arm CROSS_COMPILE=aarch64-none-linux-gnu- install
#sudo make ARCH=arm CROSS_COMPILE=arm-unknown-linux-gnueabi- install
#sudo make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu-
#sudo make -j4 install
sudo make -j4 CONFIG_PREFIX=${OUTDIR}/rootfs ARCH=arm64 CROSS_COMPILE=${GNU_PATH}/aarch64-none-linux-gnu- install

echo "Library dependencies"
cd ${OUTDIR}/rootfs
echo "======Required for program interpreter======="
${CROSS_COMPILE}readelf -a bin/busybox | grep "program interpreter"
echo "======Required for Shared library======="
${CROSS_COMPILE}readelf -a bin/busybox | grep "Shared library"
echo "======Required for sysroot======="
${CROSS_COMPILE}gcc -print-sysroot 
#ls -l lib/ld-linux-armhf.so.3

echo "Add library dependencies to rootfs"
cp -a ${LIB_PATH}/aarch64-none-linux-gnu/libc/lib/ld-linux-aarch64.so.1 ${OUTDIR}/rootfs/lib/ld-linux-aarch64.so.1
cp -a ${LIB_PATH}/aarch64-none-linux-gnu/libc/lib64/libm.so.6 ${OUTDIR}/rootfs/lib/libm.so.6
cp -a ${LIB_PATH}/aarch64-none-linux-gnu/libc/lib64/libresolv.so.2 ${OUTDIR}/rootfs/lib/libresolv.so.2
cp -a ${LIB_PATH}/aarch64-none-linux-gnu/libc/lib64/libc.so.6 ${OUTDIR}/rootfs/lib/libc.so.6

cp -a ${LIB_PATH}/aarch64-none-linux-gnu/libc/lib64/ld-2.31.so ${OUTDIR}/rootfs/lib/ld-2.31.so
cp -a ${LIB_PATH}/aarch64-none-linux-gnu/libc/lib64/libm-2.31.so ${OUTDIR}/rootfs/lib/libm-2.31.so
cp -a ${LIB_PATH}/aarch64-none-linux-gnu/libc/lib64/libresolv-2.31.so ${OUTDIR}/rootfs/lib/libresolv-2.31.so
cp -a ${LIB_PATH}/aarch64-none-linux-gnu/libc/lib64/libc-2.31.so ${OUTDIR}/rootfs/lib/libc-2.31.so

echo "Make device nodes"
sudo mknod -m 666 dev/null c 1 3
sudo mknod -m 666 dev/console c 5 1

echo "TODO: Clean and build the writer utility"

echo "TODO: Copy the finder related scripts and executables to the /home directory on the target rootfs"

exit 1
echo "TODO: Chown the root directory"
${OUTDIR}/rootfs
sudo chown -R root:root *

echo "TODO: Create initramfs.cpio.gz"
