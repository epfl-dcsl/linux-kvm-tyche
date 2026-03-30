export RISCV_ROOTFS_HOME="/home/guokai/ELF/riscv-rootfs"
export RISCV_LINUX_HOME="/home/neelu/flashpoint/linux-6.10.3"
export ARCH=riscv
export CROSS_COMPILE=riscv64-linux-gnu-
make -j$(nproc)
