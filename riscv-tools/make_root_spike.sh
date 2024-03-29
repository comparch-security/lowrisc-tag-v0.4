#!/bin/bash
#
# help:
# BUSYBOX=path/to/busybox LINUX=path/to/linux LOWRISC=path/to/lowrisc make_root.sh

if [ -z "$BUSYBOX" ]; then BUSYBOX=$TOP/riscv-tools/busybox-1.21.1; fi
BUSYBOX_CFG=$TOP/riscv-tools/busybox_config.spike

ROOT_INITTAB=$TOP/riscv-tools/inittab

if [ -z "$LINUX" ]; then LINUX=$TOP/riscv-tools/linux-4.6.3; fi
LINUX_CFG=$TOP/riscv-tools/vmlinux_config.spike


CDIR=$PWD

if [ -d "$BUSYBOX" ] && [ -d "$LINUX" ]; then
    echo "build busybox..."
    cp  $BUSYBOX_CFG "$BUSYBOX"/.config &&
    make -j$(nproc) -C "$BUSYBOX" 2>&1 1>/dev/null &&
    if [ -d ramfs ]; then rm -fr ramfs; fi &&
    mkdir ramfs && cd ramfs &&
    mkdir -p bin etc dev lib mnt proc sbin sys tmp usr usr/bin usr/lib usr/sbin &&
    cp "$BUSYBOX"/busybox bin/ &&
    ln -s bin/busybox ./init &&
    cp $ROOT_INITTAB etc/inittab &&
    printf "#!/bin/sh\necho 0 > /proc/sys/kernel/randomize_va_space\n" > disable_aslr.sh &&
    chmod +x disable_aslr.sh &&
    riscv64-unknown-linux-gnu-gcc -O1 -static -I$TOP/riscv-tools/riscv-isa-sim/riscv ../hello_tagctrl.c -o hello &&
    riscv64-unknown-linux-gnu-gcc -O2 -static -I$TOP/riscv-tools/riscv-isa-sim/riscv ../stack_vuln.c -o stack_vuln &&
    riscv64-unknown-linux-gnu-gcc -O2 -static -I$TOP/riscv-tools/riscv-isa-sim/riscv ../stack_tagged.c -o stack_tagged &&
    riscv64-unknown-linux-gnu-gcc -O2 -static -I$TOP/riscv-tools/riscv-isa-sim/riscv ../secret_vuln.c -o secret_vuln &&
    riscv64-unknown-linux-gnu-gcc -O2 -static -I$TOP/riscv-tools/riscv-isa-sim/riscv ../secret_tagged.c -o secret_tagged &&
    echo "\
        mknod dev/null c 1 3 && \
        mknod dev/tty c 5 0 && \
        mknod dev/zero c 1 5 && \
        mknod dev/console c 5 1 && \
        find . | cpio -H newc -o > "$LINUX"/initramfs.cpio\
        " | fakeroot &&
    if [ $? -ne 0 ]; then echo "build busybox failed!"; fi &&
    \
    echo "build linux..." &&
    cp $LINUX_CFG "$LINUX"/.config &&
    make -j$(nproc) -C "$LINUX" ARCH=riscv vmlinux 2>&1 1>/dev/null &&
    if [ $? -ne 0 ]; then echo "build linux failed!"; fi &&
    \
    echo "build bbl..." &&
    if [ ! -d $TOP/riscv-tools/riscv-pk/build ]; then
        mkdir -p $TOP/riscv-tools/riscv-pk/build
    fi   &&
    cd $TOP/riscv-tools/riscv-pk/build &&
    ../configure \
        --host=riscv64-unknown-elf \
        --with-payload="$LINUX"/vmlinux \
        2>&1 1>/dev/null &&
    make -j$(nproc) bbl 2>&1 1>/dev/null &&
    if [ $? -ne 0 ]; then echo "build bbl failed!"; fi &&
    \
    cd "$CDIR"
    cp $TOP/riscv-tools/riscv-pk/build/bbl ./boot.bin
else
    echo "make sure you have both linux and busybox downloaded."
    echo "usage:  [BUSYBOX=path/to/busybox] [LINUX=path/to/linux] make_root.sh"
fi
