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

#define CR1_ADDR_CONTROL          1
#define CR1_RIE                   2
#define CR1_TIE                   4
#define CR1_RDSR_MODE             8
#define CR1_TDSR_MODE             16
#define CR1_RX_FRAME_DISCONTINUE  32
#define CR1_RX_RESET              64
#define CR1_TX_RESET              128

#define CR2_PRIO_STATUS_ENABLE    1
#define CR2_2_BYTE_TRANSFER       2
#define CR2_FLAG_IDLE             4
#define CR2_FRAME_COMPLETE        8
#define CR2_TX_LAST_DATA          16
#define CR2_CLEAR_RX_STATUS       32
#define CR2_CLEAR_TX_STATUS       64
#define CR2_RTS_CONTROL           128

#define CR3_LCF_SELECT            1
#define CR3_ECF_SELECT            2
#define CR3_ADDR_EXT_MODE         4
#define CR3_IDLE_TYPE             8
#define CR3_FLAG_DETECT_ENABLE    16
#define CR3_LOOP_MODE             32
#define CR3_ACTIVE_ON_POLL        64
#define CR3_LOOP_ONLINE_CTRL_DTR  128

#define CR4_INTERFRAME_CTRL       1
#define CR4_TX_WORD_LEN_1         2
#define CR4_TX_WORD_LEN_2         4
#define CR4_RX_WORD_LEN_1         8
#define CR4_RX_WORD_LEN_2         16
#define CR4_TX_ABORT              32
#define CR4_ABORT_EXTEND          64
#define CR4_NRZI_NRZ              128

void adlc_init(void);
void adlc_reset(void);
uint adlc_read(uint reg);
void adlc_write(uint reg, uint data_val);
void adlc_write_cr1(uint data_val);
void adlc_write_cr2(uint data_val);
void adlc_write_cr3(uint data_val);
void adlc_write_cr4(uint data_val);
void adlc_write_fifo(uint data_val);
void adlc_irq_reset(void);
void adlc_flag_fill(void);
void adlc_update_data_led(bool new_activity);

#endif
