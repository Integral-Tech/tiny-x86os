#ifndef OS_CFG_H
#define OS_CFG_H

#define GDT_TABLE_SIZE 256
#define IDT_TABLE_NUM 128

#define KERNEL_SELECTOR_CS (1 << 3)
#define KERNEL_SELECTOR_DS (2 << 3)
#define SYSCALL_SELECTOR (3 << 3)
#define KERNEL_STACK_SIZE 8192

#define OS_TICKS_MS 10
#define OS_NAME "Tiny x86 OS"
#define OS_VERSION "0.01_Alpha"
#define SYS_ARCH "i386"

#define HOST_QEMU
// #define HOST_VIRTUALBOX

#define USERNAME "root"
#define HOSTNAME "qemu_pc"

#define PASSWD_MD5SUM "c8ef550b0c1522c99097c8a8abe1279a"
#define PASSWD_MAX_TRY_TIMES 3

#define TASK_NUM 128

#define ROOT_DEV DEV_DISK, 0xB1 // The first partition of the second disk

#define IDLE_TASK_SIZE 1024
#endif
