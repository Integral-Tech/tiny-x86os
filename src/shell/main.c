#include "main.h"
#include "dev/tty.h"
#include "lib_syscall.h"
#include "md5.h"
#include "os_cfg.h"
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>

static cli_t cli;
static const char *prompt = "[" USERNAME "@" HOSTNAME " /]#";

static const char echo_esc_map[] = {
    ['\\'] = '\\', ['b'] = '\b', ['r'] = '\r', ['n'] = '\n', ['t'] = '\t'};

static int cmd_help(int argc, char **argv) {
  for (const cmd_t *start = cli.cmd_start; start < cli.cmd_end; start++)
    printf("%s %s\n", start->name, start->usage);

  return 0;
}

static int cmd_clear(int argc, char **argv) {
  printf(ESC_CLEAR_SCREEN);
  printf(ESC_MOVE_CURSOR(0, 0));
  return 0;
}

static void print_echo_help() {
  printf("echo %s\n", ECHO_USAGE);
  puts("-e      enable interpretation of backslash escapes");
  puts("-E      disable interpretation of backslash escapes (default)");
  puts("--help  display this help and exit");
}

static int cmd_echo(int argc, char **argv) {
  switch (argc) {
  case 1:
    putchar('\n');
    break;
  case 2:
    if (!strcmp(argv[1], "--help"))
      print_echo_help();
    else
      printf("%s\n", argv[1]);
    break;
  case 3:
    if (!strcmp(argv[1], "-e")) {
      char str_buf[STR_BUF_SIZE];
      memset(str_buf, 0, STR_BUF_SIZE);
      char *ptr_str = argv[2], *ptr_str_buf = str_buf;
      while (*ptr_str != '\0') {
        if (*ptr_str != '\\')
          *ptr_str_buf++ = *ptr_str++;
        else if (!strncmp(ptr_str + 1, "033", 3) ||
                 !strncmp(ptr_str + 1, "x1b", 3) ||
                 !strncmp(ptr_str + 1, "x1B", 3)) {
          *ptr_str_buf++ = '\033';
          ptr_str += 4; // ANSI escape sequence
        } else {
          *ptr_str_buf++ = echo_esc_map[(int)*++ptr_str];
          ptr_str++;
        } // backslash escape sequence
      }

      puts(str_buf);
    } else if (!strcmp(argv[1], "-E"))
      puts(argv[2]);
    else {
      printf("echo: Unknown option '%s'\n", argv[1]);
      print_echo_help();
      return -1;
    }

    break;
  default:
    puts("Invalid options!");
    print_echo_help();
    return -1;
  }

  return 0;
}

static void print_ls_help() {
  printf("ls %s\n", LS_USAGE);
  puts("-l        use a long listing format");
  puts("--help    display this help and exit");
}

static int cmd_ls(int argc, char **argv) {
  DIR *dir = opendir("temp");
  if (!dir) {
    puts("Failed to open directory!");
    return -1;
  }

  struct dirent *dirent;
  switch (argc) {
  case 1:
    while ((dirent = readdir(dir))) {
      strlwr(dirent->name); // Convert filenames to lowercase ones
      printf("%s  ", dirent->name);
      fflush(stdout);
    }
    putchar('\n');
    break;
  case 2:
    if (!strcmp(argv[1], "-l")) {
      while ((dirent = readdir(dir))) {
        strlwr(dirent->name); // Convert filenames to lowercase ones
        printf("%c %s %u\n", dirent->type == DIR_FILE ? 'd' : '-', dirent->name,
               dirent->size);
      }
    } else if (!strcmp(argv[1], "--help"))
      print_ls_help();
    else {
      printf("ls: Unknown option '%s'\n", argv[1]);
      print_ls_help();
      return -1;
    }
    break;
  default:
    puts("Invalid options!");
    print_ls_help();
    return -1;
  }

  closedir(dir);
  return 0;
}

static void print_cat_help() {
  printf("cat %s\n", CAT_USAGE);
  puts("-n, --number        number all output lines");
  puts("--help              display this help and exit");
}

static int print_file(const char *filename, _Bool line_no, show_mode_t mode) {
  FILE *fp = fopen(filename, "r");
  if (!fp) {
    printf("%s: %s: No such file or directory\n", mode == CAT ? "cat" : "less",
           filename);
    return -1;
  }

  char *buf = (char *)malloc(STR_BUF_SIZE * sizeof(char));
  if (mode == CAT) {
    if (line_no) {
      size_t count = 1;
      while (fgets(buf, STR_BUF_SIZE, fp)) {
        printf("%6d  ", count++);
        fputs(buf, stdout);
      }
    } else {
      while (fgets(buf, STR_BUF_SIZE, fp))
        fputs(buf, stdout);
    }

  } else {
    setvbuf(stdin, NULL, _IONBF, 0);
    ioctl(0, TTY_CMD_ECHO, NULL, NULL);
    while (1) {
      if (!fgets(buf, STR_BUF_SIZE, fp))
        break;

      fputs(buf, stdout);

      char key;
      while ((key = fgetc(stdin)) != 'n') {
        if (key == 'q')
          goto exit_less;
      }
    }

  exit_less:
    setvbuf(stdin, NULL, _IOLBF, BUFSIZ);
    ioctl(0, TTY_CMD_ECHO, (void *)1, NULL);
  }

  free(buf);
  fclose(fp);
  return 0;
}

static int cmd_cat(int argc, char **argv) {
  switch (argc) {
  case 2:
    if (!strcmp(argv[1], "--help"))
      print_cat_help();
    else
      print_file(argv[1], FALSE, CAT);
    break;
  case 3:
    if (!strcmp(argv[1], "-n") || !strcmp(argv[1], "--number"))
      print_file(argv[2], TRUE, CAT);
    else {
      printf("echo: Unknown option '%s'\n", argv[1]);
      print_echo_help();
      return -1;
    }

    break;
  default:
    puts("Invalid options!");
    print_cat_help();
    return -1;
  }

  return 0;
}

static void print_less_help() {
  printf("less %s\n", LESS_USAGE);
  puts("--help      display this help and exit");
  puts("Press 'n' to get the next line, press 'q' to quit.");
}

static int cmd_less(int argc, char **argv) {
  switch (argc) {
  case 0: // The execution of less finishes
    return 0;
  case 1:
    puts("Missing filename!");
    print_less_help();
    return -1;
  case 2:
    if (!strcmp(argv[1], "--help"))
      print_less_help();
    else
      print_file(argv[1], FALSE, LESS);
    break;
  default:
    puts("Invalid options!");
    print_less_help();
    return -1;
  }

  return 0;
}

static void print_cp_help() {
  printf("cp %s\n", CP_USAGE);
  puts("--help   display this help and exit");
}

static int cmd_cp(int argc, char **argv) {
  int err = 0;
  switch (argc) {
  case 3: {
    FILE *src = fopen(argv[1], "rb"), *dest = fopen(argv[2], "wb");
    if (!src || !dest) {
      puts("Failed to open file!");
      err = -1;
      goto cp_failed;
    }

    void *buf = malloc(CP_BUF_SIZE);
    size_t size;
    while ((size = fread(buf, 1, CP_BUF_SIZE, src)))
      fwrite(buf, 1, size, dest);

    free(buf);
  cp_failed:
    if (src)
      fclose(src);

    if (dest)
      fclose(dest);
  } break;
  default:
    puts("Invalid options!");
    print_cp_help();
    err = -1;
  }

  return err;
}

static int cmd_poweroff(int argc, char **argv) {
  while (1) {
    printf("Are you sure to power off the machine? [Y/N]");
    fflush(stdout);
    char ch = fgetc(stdin);
    getchar(); // Ignore '\r'
    getchar(); // Ignore '\n'
    if (ch == 'Y' || ch == 'y')
      poweroff();
    else if (ch == 'N' || ch == 'n')
      break;
    else
      puts("Invalid option!");
  }

  return 0;
}

static int cmd_rm(int argc, char **argv) {
  int err;
  switch (argc) {
  case 2: {
    err = unlink(argv[1]);
    if (err < 0)
      printf("Failed to remove file %s!", argv[1]);
  } break;
  default:
    puts("Invalid options!");
    print_cp_help();
    err = -1;
  }

  return err;
}

static int cmd_reboot(int argc, char **argv) {
  while (1) {
    printf("Are you sure to reboot the machine? [Y/N]");
    fflush(stdout);
    char ch = fgetc(stdin);
    getchar(); // Ignore '\r'
    getchar(); // Ignore '\n'
    if (ch == 'Y' || ch == 'y')
      reboot();
    else if (ch == 'N' || ch == 'n')
      break;
    else
      puts("Invalid option!");
  }

  return 0;
}

static int cmd_exit(int argc, char **argv) {
  exit(0);
  return 0;
}

static const cmd_t cmd_list[] = {
    {.name = "help", .usage = HELP_USAGE, .func = cmd_help},
    {.name = "clear", .usage = CLEAR_USAGE, .func = cmd_clear},
    {.name = "echo", .usage = ECHO_USAGE, .func = cmd_echo},
    {.name = "ls", .usage = LS_USAGE, .func = cmd_ls},
    {.name = "cat", .usage = CAT_USAGE, .func = cmd_cat},
    {.name = "less", .usage = LESS_USAGE, .func = cmd_less},
    {.name = "cp", .usage = CP_USAGE, .func = cmd_cp},
    {.name = "rm", .usage = RM_USAGE, .func = cmd_rm},
    {.name = "poweroff", .usage = POWEROFF_USAGE, .func = cmd_poweroff},
    {.name = "reboot", .usage = REBOOT_USAGE, .func = cmd_reboot},
    {.name = "exit", .usage = EXIT_USAGE, .func = cmd_exit}};

static const cmd_t *find_builtin_cmd(const char *name) {
  for (const cmd_t *cmd = cli.cmd_start; cmd < cli.cmd_end; cmd++)
    if (!strcmp(cmd->name, name))
      return cmd;

  return NULL;
}

static void run_builtin_cmd(const cmd_t *cmd, int argc, char **argv) {
  const int ret = cmd->func(argc, argv);
  if (ret < 0)
    fprintf(stderr, ERR_STR("Error occurs during the execution: %d"), ret);
}

static const char *find_exec_file(const char *path) {
  int fd;
  if ((fd = open(path, 0)) < 0)
    return NULL;

  close(fd);
  return path;
}

static void run_exec_file(const char *path, int argc, char **argv) {
  const int pid = fork();
  if (pid < 0)
    fprintf(stderr, "Fork failed! File path = %s", path);
  else if (pid == 0) { // child process
    const int err = execve(path, argv, NULL);
    if (err < 0)
      fprintf(stderr, "Failed to execute file %s, error code = %d\n", path,
              err);

    exit(err);
  } else { // parent process
    int status;
    const int pid = wait(&status);
    printf("File path = %s, result = %d, pid = %d\n", path, status, pid);
  }
}

static void cli_init(const char *prompt, const cmd_t *cmd_list, size_t size) {
  cli.prompt = prompt;
  memset(cli.input_buf, 0, CLI_INPUT_SIZE);
  cli.cmd_start = cmd_list;
  cli.cmd_end = cmd_list + size;
}

static void show_prompt() {
  printf("%s", cli.prompt);
  fflush(stdout);
}

int main(int argc, char **argv) {
  open(*argv, O_RDWR); // stdin(0)
  dup(0);              // stdout(1)
  dup(0);              // stderr(2)

  cli_init(prompt, cmd_list, sizeof(cmd_list) / sizeof(*cmd_list));
  printf("%s %s (%s)\n", OS_NAME, OS_VERSION, *argv);

  ioctl(0, TTY_CMD_ECHO, NULL, NULL); // Disable input echo

  for (int i = 0; i < PASSWD_MAX_TRY_TIMES; i++) {
    printf("qemu_pc login: ");
    fflush(stdout);
    char *passwd = fgets(cli.input_buf, CLI_INPUT_SIZE, stdin);
    *(strchr(passwd, '\r')) = '\0';

    uint8_t passwd_md5buf[MD5SUM_LEN + 1];
    md5_string(passwd, passwd_md5buf);

    char passwd_md5[MD5STR_LEN + 1];
    for (int i = 0; i < MD5SUM_LEN; i++)
      sprintf(passwd_md5 + 2 * i, "%02x", passwd_md5buf[i]);

    if (!strcmp(passwd_md5, PASSWD_MD5SUM))
      goto passwd_correct;
    else
      puts("\nPassword incorrect!");
  }

  puts("You have reached the maximum login attempts! Powering off...");
  poweroff();

passwd_correct:
  ioctl(0, TTY_CMD_ECHO, (void *)1, NULL); // Enable input echo
  putchar('\n');

  while (1) {
    show_prompt();
    char *str = fgets(cli.input_buf, CLI_INPUT_SIZE, stdin);
    if (!str)
      continue;

    *(strchr(str, '\r')) = '\0';

    int argc = 0;
    char *argv[CMD_MAX_ARGC];
    memset(argv, 0, sizeof(argv));

    char *token = strtok(str, " ");
    while (token && argc < CMD_MAX_ARGC) {
      argv[argc++] = token;
      token = strtok(NULL, " "); // find next token
    }

    if (!argc)
      continue;

    const cmd_t *cmd = find_builtin_cmd(*argv);
    if (cmd) {
      run_builtin_cmd(cmd, argc, argv);
      continue;
    }

    const char *path = find_exec_file(argv[0]);
    if (path) {
      run_exec_file(path, argc, argv);
      continue;
    } // Execute applications from disk

    fprintf(stderr, ERR_STR("Command not found: %s"), str);
  }
}
