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
CROSS_COMPILE=aarch64-none-linux-gnu-
SYSROOT=/home/admin/Downloads/install-lnx/gcc-arm-10.2-2020.11-x86_64-aarch64-none-linux-gnu

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

    # TODO: Add your kernel build steps here
    # from Module 2, Building the Linux Kernel, 7:05
    make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- mrproper
    make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- defconfig
    make -j4 ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- all
    make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- modules
    make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- dtbs
fi

echo "Adding the Image in outdir"
cd /tmp/aeld/linux-stable/arch/arm64/boot
cp Image ${OUTDIR}

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi

# TODO: Create necessary base directories
# from Module 2, Linux Root Filesystems, 6:52
mkdir rootfs
cd rootfs
mkdir bin dev etc home lib lib64 proc sbin sys tmp usr var
cd usr
mkdir bin lib sbin
cd ..
cd var
mkdir log
cd ..
cd home
mkdir conf
cd ..

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
    # TODO:  Configure busybox
    # from Module 2, Linux Root Filesystems, 10:23
    make distclean
    make defconfig
else
    cd busybox
fi

# TODO: Make and install busybox
# from Module 2, Linux Root Filesystems, updated slide 14
make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu-
make CONFIG_PREFIX="${OUTDIR}/rootfs" ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- install

# return to rootfs diretory
cd ${OUTDIR}/rootfs

echo "Library dependencies"
aarch64-none-linux-gnu-readelf -a bin/busybox | grep "program interpreter"
aarch64-none-linux-gnu-readelf -a bin/busybox | grep "Shared library"

# TODO: Add library dependencies to rootfs
# copy required libraries from sysroot
cd ${SYSROOT}/aarch64-none-linux-gnu/libc/lib64
cp libm.so.6 libresolv.so.2 libc.so.6 ${OUTDIR}/rootfs/lib64
cd ${SYSROOT}/aarch64-none-linux-gnu/libc/lib
cp ld-linux-aarch64.so.1 ${OUTDIR}/rootfs/lib

# TODO: Make device nodes
# from Module 2, Linux Root Filesystems, updated slide 16
cd ${OUTDIR}/rootfs
sudo mknod -m 666 dev/null c 1 3
sudo mknod -m 600 dev/console c 5 1

# TODO: Clean and build the writer utility
cd /home/admin/Documents/assignment3/assignment-3-and-later-mash9756/finder-app
make clean
make CROSS_COMPILE

# TODO: Copy the finder related scripts and executables to the /home directory
# on the target rootfs
cp aarch64-writer finder-test.sh finder.sh autorun-qemu.sh ${OUTDIR}/rootfs/home
cp conf/username.txt conf/assignment.txt ${OUTDIR}/rootfs/home/conf

# TODO: Chown the root directory
# from Module 2, Linux Root Filesystems, updated slide 16
cd ${OUTDIR}/rootfs
sudo chown -R root:root *

# TODO: Create initramfs.cpio.gz
find . | cpio -H newc -ov --owner root:root > ${OUTDIR}/initramfs.cpio
cd ..
gzip -f initramfs.cpio