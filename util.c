#include "util.h"

uint time_ms(void) {
  return to_ms_since_boot(get_absolute_time());
}
