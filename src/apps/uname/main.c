#include "main.h"
#include "os_cfg.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv) {
  switch (argc) {
  case 1:
    puts(OS_NAME);
    break;
  case 2:
    if (!strcmp(argv[1], "-a") || !strcmp(argv[1], "--all")) {
      printf("%s %s %s %s\n", OS_NAME, HOSTNAME, OS_VERSION, SYS_ARCH);
      puts("A UNIX-like operating system, referring to Linux 0.11.");
    } else if (!strcmp(argv[1], "-s") || !strcmp(argv[1], "--kernel-name"))
      puts(OS_NAME);
    else if (!strcmp(argv[1], "-r") || !strcmp(argv[1], "--kernel-release"))
      puts(OS_VERSION);
    else if (!strcmp(argv[1], "-m") || !strcmp(argv[1], "--machine"))
      puts(SYS_ARCH);
    else if (!strcmp(argv[1], "--help"))
      print_uname_help();
    else {
      printf("uname: Unknown option '%s'\n", argv[1]);
      print_uname_help();
      return -1;
    }

    break;
  default:
    printf("Invalid options!\n");
    print_uname_help();
    return -1;
  }

  return 0;
}

void print_uname_help() {
  printf("uname %s\n", UNAME_USAGE);
  puts("-a, --all               print all information");
  puts("-s, --kernel-name       print the kernel name");
  puts("-r, --kernel-release    print the kernel release");
  puts("-m, --machine           print the machine hardware name");
  puts("--help                  display this help and exit");
}
