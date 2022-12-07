#include "input.h"

namespace Input {
  unsigned char keys[400] = {};
  unsigned char mouse_buttons[10] = {};
  double cursor_x = {};
  double cursor_y = {};
  double previous_cursor_x = {};
  double previous_cursor_y = {};
  bool mouse_cursor_changed = false;
}