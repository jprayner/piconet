#ifndef _PICONET_ADLC_H_
#define _PICONET_ADLC_H_

#include "pico/stdlib.h"

#define REG_STATUS_1              0
#define REG_STATUS_2              1
#define REG_CONTROL_1             0
#define REG_CONTROL_2             1
#define REG_FIFO                  2

#define STATUS_1_RDA              1
#define STATUS_1_S2_RD_REQ        2
#define STATUS_1_LOOP             4
#define STATUS_1_FLAG_DET         8
#define STATUS_1_NOT_CTS          16
#define STATUS_1_TX_UNDERRUN      32
#define STATUS_1_FRAME_COMPLETE   64
#define STATUS_1_IRQ              128

#define STATUS_2_ADDR_PRESENT     1
#define STATUS_2_FRAME_VALID      2
#define STATUS_2_INACTIVE_IDLE_RX 4
#define STATUS_2_ABORT_RX         8
#define STATUS_2_FCS_ERROR        16
#define STATUS_2_NOT_DCD          32
#define STATUS_2_RX_OVERRUN       64
#define STATUS_2_RDA              128

void adlc_init(void);
uint adlc_read(uint reg);
void adlc_write(uint reg, uint data_val);
void adlc_irq_reset(void);
void adlc_flag_fill(void);
bool adlc_get_irq(void);

#endif
