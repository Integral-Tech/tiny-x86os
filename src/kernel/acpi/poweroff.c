#include "acpi/poweroff.h"
#include "comm/cpu_instr.h"
#include "os_cfg.h"

int sys_poweroff() {
#ifdef HOST_QEMU
  outw(0x604, 0x2000);
#endif

#ifdef HOST_VIRTUALBOX
  outw(0x4004, 0x3400);
#endif
  return 0;
}
