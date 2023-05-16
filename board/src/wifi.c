#include <stdio.h>

#include "wifi.h"
#include "pico/cyw43_arch.h"

bool connect(const char *ssid, const char *password) {
  if (cyw43_arch_init_with_country(CYW43_COUNTRY_UK)) {
    printf("failed to initialise\n");
    return false;
  }
  printf("initialised\n");
  cyw43_arch_enable_sta_mode();
  if (cyw43_arch_wifi_connect_timeout_ms(ssid, password, CYW43_AUTH_WPA2_AES_PSK, 10000)) {
    printf("failed to connect\n");
    return false;
  }
  printf("connected\n");
}
