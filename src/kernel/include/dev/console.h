#ifndef CONSOLE_H
#define CONSOLE_H

#include "dev/tty.h"

#define CONSOLE_VGA_ADDR 0xB8000
#define CONSOLE_VGA_END (0xB8000 + 32 * 1024)
#define CONSOLE_ROW_MAX 25
#define CONSOLE_COL_MAX 80
#define CONSOLE_NUM 8

#define CURSOR_LOC_HIGH_REG 0x0E
#define CURSOR_LOC_LOW_REG 0x0F
#define START_ADDR_HIGH_REG 0x0C
#define START_ADDR_LOW_REG 0x0D

#define ASCII_ESC 0x1B
#define ASCII_DEL 0x7F

#define ESC_ARGC_MAX 10

typedef enum _color_t {
  COLOR_BLACK,
  COLOR_BLUE,
  COLOR_GREEN,
  COLOR_CYAN,
  COLOR_RED,
  COLOR_MAGENTA,
  COLOR_BROWN,
  COLOR_GRAY,
  COLOR_DARKGRAY,
  COLOR_LIGHT_BLUE,
  COLOR_LIGHT_GREEN,
  COLOR_LIGHT_CYAN,
  COLOR_LIGHT_RED,
  COLOR_LIGHT_MAGENTA,
  COLOR_YELLOW,
  COLOR_WHITE
} color_t;

#define FG_COLOR_START 30
#define FG_COLOR_END 37
#define FG_COLOR_OFFSET 30
#define FG_COLOR_DEFAULT 39

#define BG_COLOR_START 40
#define BG_COLOR_END 47
#define BG_COLOR_OFFSET 40
#define BG_COLOR_DEFAULT 49

typedef struct _vga_char_t {
  char ch;
  union {
    uint8_t attr;
    struct {
      uint8_t foreground : 4;
      uint8_t background : 3;
      uint8_t blinking : 1;
    };
  };
} vga_char_t;

typedef struct _console_t {
  enum {
    CONSOLE_WRITE_NORMAL,
    CONSOLE_WRITE_ESC,
    CONSOLE_WRITE_BRACKET
  } write_state;

  vga_char_t *base;
  int row, col;
  struct {
    int row, col;
  } cursor;

  struct {
    int row, col;
  } old_cursor;

  union {
    uint8_t attr;
    struct {
      uint8_t foreground : 4;
      uint8_t background : 3;
      uint8_t blinking : 1;
    };
  };

  int esc_arg[ESC_ARGC_MAX];
  int curr_arg_index;
} console_t;

int console_init(int console_id);
int console_write(tty_t *tty);
void console_close(int console_id);
void console_switch_to(int console_id);

#endif
