#include "applib/lib_syscall.h"
#include "dev/tty.h"

int first_task_main() {
  for (int i = 0; i < TTY_NUM; i++) {
    const int pid = fork();
    if (pid < 0) {
      print_msg("Create shell process failed!", 0);
      break;
    } else if (pid == 0) {
      char tty_path[] = "/dev/tty?";
      tty_path[sizeof(tty_path) - 2] = i + '0';
      char *argv[] = {tty_path, NULL};
      execve("shell.elf", argv, NULL);

      while (1)
        msleep(1000);
    } // Child process
  }

  while (1) {
    int status;
    wait(&status);
  }

  return 0;
}
