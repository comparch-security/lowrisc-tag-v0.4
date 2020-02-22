BUSYBOX=$TOP/riscv-tools/busybox-1.21.1
BUSYBOX_CFG=$TOP/riscv-tools/busybox_config.spike
ROOT_INITTAB=$TOP/riscv-tools/inittab
LINUX=$TOP/riscv-tools/linux-4.1.25
LINUX_CFG=$TOP/riscv-tools/vmlinux_config.spike

echo "build busybox..."
cp  $BUSYBOX_CFG "$BUSYBOX"/.config
make -j$(nproc) -C "$BUSYBOX" 2>&1 1>/dev/null
if [ -d ramfs ]; then rm -fr ramfs; fi
mkdir ramfs && cd ramfs
mkdir -p bin etc dev lib proc sbin sys tmp usr usr/bin usr/lib usr/sbin
cp "$BUSYBOX"/busybox bin/
ln -s bin/busybox ./init
cp $ROOT_INITTAB etc/inittab

# here, copy your program to here

echo "\
  mknod dev/null c 1 3 && \
  mknod dev/tty c 5 0 && \
  mknod dev/zero c 1 5 && \
  mknod dev/console c 5 1 && \
  find . | cpio -H newc -o > "$LINUX"/initramfs.cpio\
  " | fakeroot
if [ $? -ne 0 ]; then echo "build busybox failed!"; fi
   
echo "build linux..."
cp $LINUX_CFG "$LINUX"/.config
make -j$(nproc) -C "$LINUX" ARCH=riscv vmlinux 2>&1 1>/dev/null
if [ $? -ne 0 ]; then echo "build linux failed!"; fi
   
echo "build bbl..."
if [ ! -d $TOP/riscv-tools/riscv-pk/build ]; then
    mkdir -p $TOP/riscv-tools/riscv-pk/build
fi
cd $TOP/riscv-tools/riscv-pk/build
../configure \
    --host=riscv64-unknown-elf \
    --with-payload="$LINUX"/vmlinux \
    2>&1 1>/dev/null
make -j$(nproc) bbl 2>&1 1>/dev/null
if [ $? -ne 0 ]; then echo "build linux failed!"; fi
