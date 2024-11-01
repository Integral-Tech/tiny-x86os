// SPDX-FileCopyrightText: 2024 Integral <integral@member.fsf.org>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef MAIN_H
#define MAIN_H

#include "os_cfg.h"

#define CLI_INPUT_SIZE 1024
#define CMD_MAX_ARGC 10
#define STR_BUF_SIZE 1024
#define CP_BUF_SIZE 1024

#define PROMPT "[" USERNAME "@" HOSTNAME " /]#"

#define ESC_CMD_2ARG(Pn, cmd) "\033[" #Pn #cmd
#define ESC_CLEAR_SCREEN ESC_CMD_2ARG(2, J)
#define ESC_MOVE_CURSOR(row, col) "\033[" #row ";" #col "H"
#define ESC_COLOR_NORMAL ESC_CMD_2ARG(39, m)
#define ESC_COLOR_ERR ESC_CMD_2ARG(31, m)
#define ERR_STR(str) ESC_COLOR_ERR str ESC_COLOR_NORMAL "\n"

#define HELP_USAGE "- list all supported commands"
#define CLEAR_USAGE "- clear the terminal screen"
#define ECHO_USAGE "[SHORT-OPTION] [STRING] - display a line of text"
#define EXIT_USAGE "- exit from shell"
#define LS_USAGE "[OPTION] - list directory contents"
#define CAT_USAGE                                                              \
  "[OPTION] FILE - concatenate files and print on the standard output"
#define LESS_USAGE "FILE - file perusal filter for crt viewing"
#define CP_USAGE "SOURCE DEST - copy files and directories"
#define RM_USAGE "- remove files or directories"
#define POWEROFF_USAGE "- power off the machine"
#define REBOOT_USAGE "- reboot the machine"

typedef struct _cmd_t {
  const char *name, *usage;
  int (*func)(int argc, char **argv);
} cmd_t;

typedef struct _cli_t {
  char input_buf[CLI_INPUT_SIZE];
  const cmd_t *cmd_start, *cmd_end;
  const char *prompt;
} cli_t;

typedef enum { CAT, LESS } show_mode_t;

#define cli_for_each_cmd(cli, cmd)                                             \
  for (const cmd_t *cmd = (cli).cmd_start; cmd < (cli).cmd_end; (cmd)++)

#endif
