#include "dev/keyboard.h"
#include "comm/cpu_instr.h"
#include "cpu/irq.h"
#include "dev/tty.h"
#include "tools/klib.h"

static keyboard_stat_t keyboard_stat;
static const key_map_t map_table[] = {
    [0x2] = {'1', '!'},    [0x3] = {'2', '@'},    [0x4] = {'3', '#'},
    [0x5] = {'4', '$'},    [0x6] = {'5', '%'},    [0x7] = {'6', '^'},
    [0x08] = {'7', '&'},   [0x09] = {'8', '*'},   [0x0A] = {'9', '('},
    [0x0B] = {'0', ')'},   [0x0C] = {'-', '_'},   [0x0D] = {'=', '+'},
    [0x0E] = {'\b', '\b'}, [0x0F] = {'\t', '\t'}, [0x10] = {'q', 'Q'},
    [0x11] = {'w', 'W'},   [0x12] = {'e', 'E'},   [0x13] = {'r', 'R'},
    [0x14] = {'t', 'T'},   [0x15] = {'y', 'Y'},   [0x16] = {'u', 'U'},
    [0x17] = {'i', 'I'},   [0x18] = {'o', 'O'},   [0x19] = {'p', 'P'},
    [0x1A] = {'[', '{'},   [0x1B] = {']', '}'},   [0x1C] = {'\n', '\n'},
    [0x1E] = {'a', 'A'},   [0x1F] = {'s', 'B'},   [0x20] = {'d', 'D'},
    [0x21] = {'f', 'F'},   [0x22] = {'g', 'G'},   [0x23] = {'h', 'H'},
    [0x24] = {'j', 'J'},   [0x25] = {'k', 'K'},   [0x26] = {'l', 'L'},
    [0x27] = {';', ':'},   [0x28] = {'\'', '"'},  [0x29] = {'`', '~'},
    [0x2B] = {'\\', '|'},  [0x2C] = {'z', 'Z'},   [0x2D] = {'x', 'X'},
    [0x2E] = {'c', 'C'},   [0x2F] = {'v', 'V'},   [0x30] = {'b', 'B'},
    [0x31] = {'n', 'N'},   [0x32] = {'m', 'M'},   [0x33] = {',', '<'},
    [0x34] = {'.', '>'},   [0x35] = {'/', '?'},   [0x39] = {' ', ' '},
};

void keyboard_init() {
  static _Bool init = FALSE;

  if (!init) {
    irq_install(IRQ1_KEYBOARD, (irq_handler_t)exception_handler_keyboard);
    irq_enable(IRQ1_KEYBOARD);
    kernel_memset(&keyboard_stat, 0, sizeof(keyboard_stat_t));
    init = TRUE; // prevent repeated initialization
  }
}

static int handle_fn_key(uint8_t key) {
  if (key >= KEY_F1 && key <= KEY_F8 && keyboard_stat.alt_key) {
    tty_switch_to(key - KEY_F1);
    return 0;
  }

  return -1;
}

static void handle_normal_key(uint8_t raw_code) {
  uint8_t key = raw_code & 0x7F;
  _Bool flag_make_code = is_make_code(raw_code);

  switch (key) {
  case KEY_LSHIFT:
  case KEY_RSHIFT:
    keyboard_stat.shift_key = flag_make_code;
    break;
  case KEY_CAPS_LOCK:
    if (flag_make_code)
      keyboard_stat.caps_lock = !keyboard_stat.caps_lock;
    break;
  case KEY_ALT:
    keyboard_stat.alt_key = flag_make_code;
    break;
  case KEY_CTRL:
    keyboard_stat.ctrl_key = flag_make_code;
    break;
  case KEY_F1:
  case KEY_F2:
  case KEY_F3:
  case KEY_F4:
  case KEY_F5:
  case KEY_F6:
  case KEY_F7:
  case KEY_F8:
    handle_fn_key(key);
    break;
  case KEY_F9:
  case KEY_F10:
  case KEY_F11:
  case KEY_F12:
    break;
  default:
    if (flag_make_code) {
      key = (keyboard_stat.shift_key) ? map_table[key].func
                                      : map_table[key].normal;
      if (keyboard_stat.caps_lock) {
        switch (key) {
        case 'A' ... 'Z':
          tolower(key);
          break;
        case 'a' ... 'z':
          toupper(key);
          break;
        }
      }

      tty_in(key);
    }
  }
}

static void handle_e0_key(uint8_t raw_code) {
  uint8_t key = raw_code & 0x7F;
  _Bool flag_make_code = is_make_code(raw_code);

  switch (key) {
  case KEY_CTRL:
    keyboard_stat.ctrl_key = flag_make_code;
    break;
  case KEY_ALT:
    keyboard_stat.alt_key = flag_make_code;
    break;
  }
}

void do_handle_keyboard(exception_frame_t *frame) {
  static enum { NORMAL, BEGIN_WITH_E0, BEGIN_WITH_E1 } recv_state = NORMAL;

  if (!(inb(KEYBOARD_STAT_PORT) & KEYBOARD_STAT_READY)) {
    pic_send_eoi(IRQ1_KEYBOARD);
    return;
  }

  const uint8_t raw_code = inb(KEYBOARD_DATA_PORT);
  pic_send_eoi(IRQ1_KEYBOARD);

  switch (raw_code) {
  case 0xE0:
    recv_state = BEGIN_WITH_E0;
    break;
  case 0xE1:
    recv_state = BEGIN_WITH_E1;
    break;
  default:
    switch (recv_state) {
    case NORMAL:
      handle_normal_key(raw_code);
      break;
    case BEGIN_WITH_E0:
      handle_e0_key(raw_code);
      recv_state = NORMAL;
      break;
    case BEGIN_WITH_E1:
      recv_state = NORMAL;
      break;
    }
  }
}
