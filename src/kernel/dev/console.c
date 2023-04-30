#include "dev/console.h"
#include "comm/cpu_instr.h"
#include "ipc/mutex.h"
#include "tools/klib.h"

static console_t console_buf[CONSOLE_NUM];
static mutex_t console_mutex;
static _Bool console_mutex_inited = FALSE;
static int curr_console_id = 0;

static uint16_t read_cursor_pos() {
  uint16_t position;
  mutex_lock(&console_mutex);

  outb(0x3D4, CURSOR_LOC_LOW_REG);
  position = inb(0x3D5);
  outb(0x3D4, CURSOR_LOC_HIGH_REG);
  position |= inb(0x3D5) << 8;

  mutex_unlock(&console_mutex);
  return position;
}

static uint16_t update_cursor_pos(const console_t *console) {
  const uint16_t position =
      (console - console_buf) * console->row * console->col +
      (console->cursor.row) * console->col + console->cursor.col;
  mutex_lock(&console_mutex);

  outb(0x3D4, CURSOR_LOC_LOW_REG);
  outb(0x3D5, (uint8_t)(position & 0xFF));
  outb(0x3D4, CURSOR_LOC_HIGH_REG);
  outb(0x3D5, (uint8_t)(position >> 8));

  mutex_unlock(&console_mutex);
  return position;
}

static void clear_rows(const console_t *console, int start, int end) {
  vga_char_t *ptr_start = console->base + console->col * start;
  const vga_char_t *ptr_end = console->base + console->col * (end + 1);
  while (ptr_start < ptr_end) {
    ptr_start->ch = ' ';
    ptr_start->attr = console->attr;
    ptr_start++;
  }
}

static void clear_display(const console_t *console) {
  int size = console->col * console->row;
  vga_char_t *dest = console->base;

  while (size--) {
    dest->ch = ' ';
    dest->attr = console->attr;
    dest++;
  }
}

static void scroll_up(console_t *console, int lines) {
  vga_char_t *dest = console->base;
  const vga_char_t *src = console->base + lines * console->col;
  const uint32_t size =
      (console->row - lines) * console->col * sizeof(vga_char_t);
  kernel_memcpy(dest, src, size);

  clear_rows(console, console->row - lines, console->row - 1);
  console->cursor.row -= lines;
}

static void move_forward(console_t *console, int step) {
  if (console->cursor.col + step >= console->col) {
    console->cursor.row += (console->cursor.col + step) / (console->col);
    console->cursor.col = (console->cursor.col + step) % (console->col);
  } else
    console->cursor.col += step;

  if (console->cursor.row >= console->row)
    scroll_up(console, 1);
}

static int move_backward(console_t *console, int step) {
  if (console->cursor.col - step < 0) {
    console->cursor.row += (console->cursor.col - step) / (console->col) - 1;
    console->cursor.col =
        (console->cursor.col - step) % (console->col) + console->col;
  } else
    console->cursor.col -= step;

  if (console->cursor.row < 0 || console->cursor.col < 0)
    return -1;

  return 0;
}

static void erase_forward(console_t *console, int step) {
  vga_char_t *dest = console->base +
                     (console->cursor.row * console->col + console->cursor.col);

  for (int i = 0; i < step; i++, dest++) {
    dest->ch = ' ';
    dest->attr = console->attr;
  }

  move_forward(console, step);
}

static void erase_backward(console_t *console, int step) {
  vga_char_t *dest = console->base +
                     (console->cursor.row * console->col + console->cursor.col);

  for (int i = 0; i < step && dest-- >= console->base; i++) {
    dest->ch = ' ';
    dest->attr = console->attr;
  }

  move_backward(console, step);
}

static void show_char(console_t *console, char ch) {
  vga_char_t *dest = console->base +
                     (console->cursor.row * console->col + console->cursor.col);
  dest->ch = ch;
  dest->attr = console->attr;
  move_forward(console, 1);
}

int console_init(int console_id) {
  console_t *console = console_buf + console_id;

  console->row = CONSOLE_ROW_MAX;
  console->col = CONSOLE_COL_MAX;
  console->old_cursor.row = console->cursor.row;
  console->old_cursor.col = console->cursor.col;

  console->write_state = CONSOLE_WRITE_NORMAL;

  console->foreground = COLOR_WHITE;
  console->background = COLOR_BLACK;
  console->base = (vga_char_t *)CONSOLE_VGA_ADDR +
                  console_id * CONSOLE_ROW_MAX * CONSOLE_COL_MAX;

  if (!console_mutex_inited) {
    mutex_init(&console_mutex);
    console_mutex_inited = TRUE;
  }

  if (!console_id) {
    const uint16_t cursor_pos = read_cursor_pos();
    console->cursor.row = cursor_pos / console->col;
    console->cursor.col = cursor_pos % console->col;
  } // reserve the original content on tty0
  else {
    console->cursor.row = console->cursor.col = 0;
    clear_display(console);
  }
  return 0;
}

static void write_normal_char(console_t *console, char ch) {
  switch (ch) {
  case ASCII_ESC:
    console->write_state = CONSOLE_WRITE_ESC;
    break;
  case ASCII_DEL:
  case '\b':
    erase_backward(console, 1);
    break;
  case '\r':
    console->cursor.col = 0;
    break;
  case '\n':
    if (++console->cursor.row >= console->row)
      scroll_up(console, 1);
    break;
  case '\t':
    erase_forward(console, 4);
    break;
  default:
    show_char(console, ch);
  }
}

static void set_font_style(console_t *console) {
  static const color_t color_table[] = {
      [0] = COLOR_BLACK,  [1] = COLOR_RED,  [2] = COLOR_GREEN,
      [3] = COLOR_YELLOW, [4] = COLOR_BLUE, [5] = COLOR_MAGENTA,
      [6] = COLOR_CYAN,   [7] = COLOR_WHITE};

  for (int i = 0; i <= console->curr_arg_index; i++) {
    const int arg = console->esc_arg[i];
    switch (arg) {
    case FG_COLOR_START ... FG_COLOR_END:
      console->foreground = color_table[arg - FG_COLOR_OFFSET];
      break;
    case BG_COLOR_START ... BG_COLOR_END:
      console->background = color_table[arg - BG_COLOR_OFFSET];
      break;
    case FG_COLOR_DEFAULT:
      console->foreground = COLOR_WHITE;
      break;
    case BG_COLOR_DEFAULT:
      console->background = COLOR_BLACK;
      break;
    }
  }
}

static void write_esc_char(console_t *console, char ch) {
  switch (ch) {
  case '7': // Save the position of cursor
    console->old_cursor.row = console->cursor.row;
    console->old_cursor.col = console->cursor.col;
    console->write_state = CONSOLE_WRITE_NORMAL;
    break;
  case '8': // Restore the position of cursor
    console->cursor.row = console->old_cursor.row;
    console->cursor.col = console->old_cursor.col;
    console->write_state = CONSOLE_WRITE_NORMAL;
    break;
  case '[': // The start of argeter list
    kernel_memset(console->esc_arg, 0, sizeof(console->esc_arg));
    console->curr_arg_index = 0;
    console->write_state = CONSOLE_WRITE_BRACKET;
    break;
  default:
    console->write_state = CONSOLE_WRITE_NORMAL;
  }
}

static void erase_in_display(console_t *console) {
  if (console->curr_arg_index < 0)
    return;

  switch (console->esc_arg[0]) {
  case 0:
    erase_forward(console, console->col - console->cursor.col +
                               (console->row - console->cursor.row - 1) *
                                   (console->col));
    break;
  case 1:
    erase_backward(console, console->cursor.row * console->col +
                                console->cursor.col + 1);
    break;
  case 2 ... 3:
    clear_display(console);
    console->cursor.col = console->cursor.row = 0;
    break;
  }
}

static void write_esc_bracket(console_t *console, char ch) {
  switch (ch) {
  case '0' ... '9': {
    int *arg = &console->esc_arg[console->curr_arg_index];
    *arg = *arg * 10 + ch - '0';
    break;
  }
  case ';':
    if (console->curr_arg_index < ESC_ARGC_MAX)
      console->curr_arg_index++;
    break;
  default:
    switch (ch) {
    case 'm':
      set_font_style(console);
      break;
    case 'D': // move the cursor left
      console->cursor.col -= console->esc_arg[0];
      if (console->cursor.col < 0)
        console->cursor.col = 0;
      break;
    case 'C': // move the cursor right
      console->cursor.col += console->esc_arg[0];
      if (console->cursor.col >= console->col)
        console->cursor.col = console->col - 1;
      break;
    case 'H':
    case 'f': // move the cursor to a specific position
      console->cursor.row = console->esc_arg[0];
      console->cursor.col = console->esc_arg[1];
      break;
    case 'J':
      erase_in_display(console);
      break;
    }
    console->write_state = CONSOLE_WRITE_NORMAL;
  }
}

int console_write(tty_t *tty) {
  console_t *console = console_buf + tty->console_id;
  int len = 0;

  mutex_lock(&console_mutex);
  while (1) {
    char ch;
    if (tty_dequeue(&tty->output_queue, &ch) < 0)
      break;
    sem_notify(&tty->output_sem);

    switch (console->write_state) {
    case CONSOLE_WRITE_NORMAL:
      write_normal_char(console, ch);
      break;
    case CONSOLE_WRITE_ESC:
      write_esc_char(console, ch);
      break;
    case CONSOLE_WRITE_BRACKET:
      write_esc_bracket(console, ch);
      break;
    }
    len++;
  }

  mutex_unlock(&console_mutex);
  if (tty->console_id == curr_console_id)
    update_cursor_pos(console);

  return len;
}

void console_close(int console_id) {}

void console_switch_to(int console_id) {
  console_t *console = console_buf + console_id;
  if (!console->base)
    console_init(console_id);

  uint16_t offset = console_id * console->row * console->col;
  outb(0x3D4, START_ADDR_HIGH_REG);
  outb(0x3D5, (uint8_t)(offset >> 8));
  outb(0x3D4, START_ADDR_LOW_REG);
  outb(0x3D5, (uint8_t)(offset & 0xFF));

  curr_console_id = console_id;
  update_cursor_pos(console);
}
