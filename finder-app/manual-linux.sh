#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -e
set -u
export PATH=$PATH:/home/marshall/Programs/install-lnx/gcc-arm-10.2-2020.11-x86_64-aarch64-none-linux-gnu/bin

OUTDIR=/tmp/aeld
FINDER_APP_DIR=$(realpath $(dirname $0))
#ARCHIVE=~/Documents/ECEA5305_LinuxSystemProgrammingIntroToBuildroot/assign3-2
ARCHIVE=${FINDER_APP_DIR}/../../../Documents/ECEA5305_LinuxSystemProgrammingIntroToBuildroot/assign3-2
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.1.10
BUSYBOX_VERSION=1_33_1
ARCH=arm64
GNU_PATH=${FINDER_APP_DIR}/../../../Programs/install-lnx/gcc-arm-10.2-2020.11-x86_64-aarch64-none-linux-gnu/bin
LIB_PATH=${FINDER_APP_DIR}/../../../Programs/install-lnx/gcc-arm-10.2-2020.11-x86_64-aarch64-none-linux-gnu
#CROSS_COMPILE=${GNU_PATH}/aarch64-none-linux-gnu-
CROSS_COMPILE=aarch64-none-linux-gnu-

if [ -d $FINDER_APP_DIR ]
then
    : # NOP
else
    echo "FINDER_APP_DIR Does NOT Exist ${FINDER_APP_DIR}"
    exit 1
fi

if [ -d $ARCHIVE ]
then
    : # NOP
else
    echo "ARCHIVE Does NOT Exist ${ARCHIVE}"
    exit 1
fi

if [ -d $GNU_PATH ]
then
    : # NOP
else
    echo "GNU_PATH Does NOT Exist ${GNU_PATH}"
    exit 1
fi

if [ -d $LIB_PATH ]
then
    : # NOP
else
    echo "LIB_PATH Does NOT Exist ${LIB_PATH}"
    exit 1
fi

if [ $# -lt 1 ]
then
	echo "Using default directory ${OUTDIR} for output"
else
	OUTDIR=$1
	echo "Using passed directory ${OUTDIR} for output"
fi

echo "Creating the staging directory for the root filesystem"
if [ -d ${OUTDIR} ]
then
	echo "Deleting output directory at ${OUTDIR} and starting over"
    sudo rm -rf ${OUTDIR}
fi
mkdir -p ${OUTDIR}

cd ${ARCHIVE}
if [ ! -d "${ARCHIVE}/linux-stable" ]; then
    #Clone only if the repository does not exist.
	echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${ARCHIVE}"
	git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
fi
if [ ! -e ${ARCHIVE}/linux-stable/arch/${ARCH}/boot/Image ]; then
    cd linux-stable
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}

    #echo "Apply Ubuntu V22.04 patch"
    #wget https://github.com/torvalds/linux/commit/e33a814e772cdc36436c8c188d8c42d019fda639.diff
    #git apply e33a814e772cdc36436c8c188d8c42d019fda639.diff

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
    make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- dtbs # Build the device tree
fi

echo "Adding the Image in outdir"
cp -a ${ARCHIVE}/linux-stable/arch/${ARCH}/boot/Image ${OUTDIR}/Image
#rm -f ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image.gz

echo "Creating the staging directory for the root filesystem"
#cd "$OUTDIR"
#if [ -d "${OUTDIR}/rootfs" ]
#then
#	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
#    sudo rm -rf ${OUTDIR}/rootfs
#fi

echo "Create necessary base directories"
mkdir -p ${OUTDIR}/rootfs
cd ${OUTDIR}/rootfs
mkdir -p bin dev etc home lib lib64 proc sbin sys tmp usr var
mkdir -p usr/bin usr/lib usr/sbin
mkdir -p var/log

cd ${ARCHIVE}
if [ ! -d "${ARCHIVE}/busybox" ]
then
    git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
    echo "Configure busybox"
    make distclean
    #make defconfig
    #sudo make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- defconfig
    make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- defconfig
else
    cd busybox
fi

echo "Make and install busybox"
sudo make -j4 CONFIG_PREFIX=${OUTDIR}/rootfs ARCH=arm64 CROSS_COMPILE=${GNU_PATH}/aarch64-none-linux-gnu- install

echo "Add library dependencies to rootfs"
# arm-unknown-linux-gnueabi ??
echo "======Required for sysroot======="
#${CROSS_COMPILE}gcc -print-sysroot 
#~/Programs/install-lnx/gcc-arm-10.2-2020.11-x86_64-aarch64-none-linux-gnu/bin/../aarch64-none-linux-gnu/libc
#ls -l lib/ld-linux-aarch64.so.1
export SYSROOT=$(${CROSS_COMPILE}gcc -print-sysroot)

cd ${OUTDIR}/rootfs
echo "======Required for program interpreter======="
${CROSS_COMPILE}readelf -a bin/busybox | grep "program interpreter"
# [Requesting program interpreter: /lib/ld-linux-aarch64.so.1]
cp -aL ${LIB_PATH}/aarch64-none-linux-gnu/libc/lib/ld-linux-aarch64.so.1 ${OUTDIR}/rootfs/lib/ld-linux-aarch64.so.1
#ls -l ${LIB_PATH}/aarch64-none-linux-gnu/libc/lib/ld-linux-aarch64.so.1
# ld-linux-aarch64.so.1 -> ../lib64/ld-2.31.so
cp -aL ${LIB_PATH}/aarch64-none-linux-gnu/libc/lib64/ld-2.31.so ${OUTDIR}/rootfs/lib/ld-2.31.so

echo "======Required for Shared library======="
${CROSS_COMPILE}readelf -a bin/busybox | grep "Shared library"
# 0x0000000000000001 (NEEDED)             Shared library: [libm.so.6]
# 0x0000000000000001 (NEEDED)             Shared library: [libresolv.so.2]
# 0x0000000000000001 (NEEDED)             Shared library: [libc.so.6]
cp -aL ${LIB_PATH}/aarch64-none-linux-gnu/libc/lib64/libm.so.6 ${OUTDIR}/rootfs/lib64/libm.so.6
cp -aL ${LIB_PATH}/aarch64-none-linux-gnu/libc/lib64/libresolv.so.2 ${OUTDIR}/rootfs/lib64/libresolv.so.2
cp -aL ${LIB_PATH}/aarch64-none-linux-gnu/libc/lib64/libc.so.6 ${OUTDIR}/rootfs/lib64/libc.so.6
#ls -l ${LIB_PATH}/aarch64-none-linux-gnu/libc/lib64/libm.so.6
#ls -l ${LIB_PATH}/aarch64-none-linux-gnu/libc/lib64/libresolv.so.2
#ls -l ${LIB_PATH}/aarch64-none-linux-gnu/libc/lib64/libc.so.6
#~/Programs/install-lnx/gcc-arm-10.2-2020.11-x86_64-aarch64-none-linux-gnu/aarch64-none-linux-gnu/libc/lib64/libm.so.6 -> libm-2.31.so
#~/Programs/install-lnx/gcc-arm-10.2-2020.11-x86_64-aarch64-none-linux-gnu/aarch64-none-linux-gnu/libc/lib64/libresolv.so.2 -> libresolv-2.31.so
#~/Programs/install-lnx/gcc-arm-10.2-2020.11-x86_64-aarch64-none-linux-gnu/aarch64-none-linux-gnu/libc/lib64/libc.so.6 -> libc-2.31.so
cp -aL ${LIB_PATH}/aarch64-none-linux-gnu/libc/lib64/libm-2.31.so ${OUTDIR}/rootfs/lib64/libm-2.31.so
cp -aL ${LIB_PATH}/aarch64-none-linux-gnu/libc/lib64/libresolv-2.31.so ${OUTDIR}/rootfs/lib64/libresolv-2.31.so
cp -aL ${LIB_PATH}/aarch64-none-linux-gnu/libc/lib64/libc-2.31.so ${OUTDIR}/rootfs/lib64/libc-2.31.so

echo "Make device nodes"
sudo mknod -m 666 dev/null c 1 3
sudo mknod -m 666 dev/console c 5 1

#echo "Installing modules"
#cd ${ARCHIVE}/linux-stable
#make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} INSTALL_MOD_PATH=${OUTDIR}/rootfs modules_install

echo "Clean and build the writer utility"
cd ${FINDER_APP_DIR}
make clean
make all

echo "Copy the finder related scripts and executables to the /home directory on the target rootfs"
cp ${FINDER_APP_DIR}/writer ${OUTDIR}/rootfs/home
cp ${FINDER_APP_DIR}/finder.sh ${OUTDIR}/rootfs/home
cp ${FINDER_APP_DIR}/finder-test.sh ${OUTDIR}/rootfs/home
cp ${FINDER_APP_DIR}/autorun-qemu.sh ${OUTDIR}/rootfs/home
mkdir -p ${OUTDIR}/rootfs/home/conf
cp ${FINDER_APP_DIR}/conf/assignment.txt ${OUTDIR}/rootfs/home/conf/assignment.txt
cp ${FINDER_APP_DIR}/conf/username.txt ${OUTDIR}/rootfs/home/conf/username.txt

echo "Chown the root directory"
cd ${OUTDIR}/rootfs
sudo chown -R root:root *
# "I would chroot in and rebuild the initramfs, 
#  while tailing the system log for io error messages."

echo "Create initramfs.cpio"
#cd ${OUTDIR}
rm -f initramfs.cpio.gz
#find . -depth | cpio -ov > initramfs.cpio
#find . -name Image | cpio -oA -F initramfs.cpio # add -v for debugging
#find rootfs -depth | cpio -o --owner root:root > initramfs.cpio # add -v for debugging
cd ${OUTDIR}/rootfs
find . | cpio -H newc -ov --owner root:root > ../initramfs.cpio
cd ..

echo "Create initramfs.cpio.gz"
#gzip -k initramfs.cpio # -k to keep initramfs.cpio
gzip initramfs.cpio
#tar -zcvf initramfs.tar.gz rootfs

if [ -e ${OUTDIR}/initramfs.cpio.gz ]
#if [ -e ${OUTDIR}/initramfs.tar.gz ]
then
    echo "Make SUCCESS!"
    exit 0
else
    echo "FAILED to create initramfs.cpio.gz"
    exit 1
fi