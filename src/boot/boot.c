__asm__(".code16gcc");

#define LOADER_START_ADDR 0x8000

void boot_entry(void) {
  ((void (*)(void))LOADER_START_ADDR)(); // force to convert address to function pointer
}
