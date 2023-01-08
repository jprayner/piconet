#include "util.h"

uint32_t time_ms(void) {
  return to_ms_since_boot(get_absolute_time());
}
