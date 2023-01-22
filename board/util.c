#include "util.h"
#include "adlc.h"
#include <stdio.h>

uint32_t time_ms(void) {
  return to_ms_since_boot(get_absolute_time());
}

void hexdump(uint8_t *buffer, size_t len) {
    int i;
    for (i = 0; i < len; i++) {
        printf("%02x ", buffer[i]);
    }
    printf("\n");
}

void print_status1(uint value) {
    printf("Status register 1: 0x%2x ", value);
    printf("[ ");

    if (value & STATUS_1_RDA) {
        printf(" RDA");
    }
    if (value & STATUS_1_S2_RD_REQ) {
        printf(" RD_REQ");
    }
    if (value & STATUS_1_LOOP) {
        printf(" LOOP");
    }
    if (value & STATUS_1_FLAG_DET) {
        printf(" FLAG");
    }
    if ((value & STATUS_1_NOT_CTS) == 0) {
        printf(" CTS");
    }
    if (value & STATUS_1_TX_UNDERRUN) {
        printf(" URUN");
    }
    if (value & STATUS_1_FRAME_COMPLETE) {
        printf(" FRM_COMP");
    }
    if (value & STATUS_1_IRQ) {
        printf(" IRQ");
    }

    printf(" ]\n");
}

void print_status2(uint value) {
    printf("Status register 2: 0x%2x ", value);
    printf("[ ");

    if (value & STATUS_2_ADDR_PRESENT) {
        printf(" ADDR");
    }
    if (value & STATUS_2_FRAME_VALID) {
        printf(" VALID");
    }
    if (value & STATUS_2_INACTIVE_IDLE_RX) {
        printf(" IDLE");
    }
    if (value & STATUS_2_ABORT_RX) {
        printf(" ABRT");
    }
    if (value & STATUS_2_FCS_ERROR) {
        printf(" FCS_ERR");
    }
    if ((value & STATUS_2_NOT_DCD) == 0) {
        printf(" DCD");
    }
    if (value & STATUS_2_RX_OVERRUN) {
        printf(" OVERRUN");
    }
    if (value & STATUS_2_RDA) {
        printf(" RDA");
    }

    printf(" ]\n");
}
