#include "econet.h"

#include "adlc.h"
#include "util.h"

static int BUFFSIZE = 16384;

uint config_Station = 2;
uint config_Net = 0; // TODO: Not strictly speaking config, Needs a bridge discovery to fix this later on

bool got_scout = false;

uint rx_buff[16384]; // TODO should match BUFFSIZE
uint scout_buff[8];
uint ack_buff[8];

uint cfg_scout_timeout = 100;
uint cfg_ack_timeout = 200;
uint cfg_tx_begin_timeout = 5000;

void econet_init(void) {
    adlc_init();
    adlc_irq_reset();
}

void rx_reset(void) {
    sleep_us(2); // Give the last byte a moment to drain
    adlc_write(REG_CONTROL_2, 0b00100001); // Clear RX status, prioritise status 
}

bool wait_for_idle() {
    // Wait for network to become idle, returns false if network error or if not idle in 2 seconds.
    unsigned int byte1;
    int sr1, sr2;

    unsigned long timeOut = time_ms() + cfg_tx_begin_timeout;

    sr2 = 0; // Force while loop to run first time
    while (!(sr2 & STATUS_2_INACTIVE_IDLE_RX)) {
        sleep_ms(40);
        sr1 = adlc_read(REG_STATUS_1);
        sr2 = adlc_read(REG_STATUS_2);

        if (sr2 & STATUS_2_NOT_DCD) {
            return false;
        }
        
        if (time_ms() > timeOut){
            return false;
        }

        adlc_irq_reset();
    }

    return true;
}

/*
bool checkAck(){

  uint theirStation = scoutBuff[2];
  uint theirNet = scoutBuff[3];
  
  int ptr=0;
  uint stat = 0;
  bool frame=true;
  static bool gotScout = false; // This maintains our state in the 4 way handshake between calls.

  // First byte should be address
  rx_buff[0] = readFIFO();

  if (rx_buff[0] != config_Station) {
      // If frame is not for me, then bail out
      resetIRQ();
      return false;
  }
  
  ptr++;

  while (frame){
    
    // Now check the status register
    do {
      delayMicroseconds(1); // Even at the fastest clock rate, it still takes several uS to get another byte.      
      stat=readSR2();
    } while (!(stat & 250)); // While no errors, or available data - keep polling  

    rx_buff[ptr]=readFIFO();
    if (ptr < BUFFSIZE) {ptr++;}; //keep overwriting last byte for now until I do this properly!


    if (stat & 122 ) frame=false; // If error or end of frame bits set, drop out
  } // End of while in frame - Data Available with no error bits or end set
 

   // Frame is valid
   if(ptr!=4) return(false); // frame is wrong size for an ack
   if((rx_buff[1] == theirNet) && (rx_buff[0] == theirStation)) return (true);
 
    return (false);   // Not bothering with any other error checking - if we are here the frame is wrong regardless of cause.
}
*/

bool wait_for_ack() {
    uint statReg1, statReg2;
    bool ackResult = false, inLoop = true;

    unsigned long timeOut = time_ms() + cfg_ack_timeout;

    adlc_irq_reset();

    // IRQ polling loop
    while (inLoop) {
        if (time_ms() > timeOut) { 
            inLoop = false;
        }

        if (adlc_get_irq()) {
            // There is an IRQ to service
            statReg1 = adlc_read(REG_STATUS_1);

            if (statReg1 & 2){
                statReg2 = adlc_read(REG_STATUS_2);

                if (statReg2 & STATUS_2_ADDR_PRESENT) {
                    // Address present in FIFO, so fetch it and the rest of frame

                    //TODO
                    //ackResult = checkAck();
                    ackResult = true;
                }
                if (statReg2 & STATUS_2_FRAME_VALID) {
                    // Frame complete - not expecting a frame here
                    adlc_read(REG_FIFO);
                    adlc_irq_reset();
                }
                if (statReg2 & STATUS_2_INACTIVE_IDLE_RX) {
                    // Inactive idle
                    adlc_irq_reset();
                }
                if (statReg2 & STATUS_2_ABORT_RX) {
                    // TX abort received
                    adlc_irq_reset();
                }
                if (statReg2 & STATUS_2_FCS_ERROR) {
                    // Frame error
                    adlc_irq_reset();
                }
                if (statReg2 & STATUS_2_NOT_DCD) {
                    // Carrier loss
                    // Serial.println("No clock!");
                    adlc_irq_reset();
                }
                if (statReg2 & STATUS_2_RX_OVERRUN) {
                    // Overrun
                    adlc_irq_reset();
                }
                if (statReg2 & STATUS_2_RDA) {
                    // RX data available
                    adlc_read(REG_FIFO);
                }
            } else {
                // Something else in SR1 needs servicing        
                if (statReg1 & STATUS_1_RDA) {
                    // Not expecting data, so just read and ignore it!
                    adlc_read(REG_FIFO);
                }

                // Reset IRQ as not expecting anything!
                adlc_irq_reset();
            }
        }

        if (ackResult) {
            inLoop = false;
        }
    }

    return ackResult;
}

void ack_rx() {
    // Generate an acknowledgement packet 
    uint tx_buff[4];

    // Build the ack frame from the data in rx buffer  
    tx_buff[0] = rx_buff[2];
    tx_buff[1] = rx_buff[3];
    tx_buff[2] = rx_buff[0];
    tx_buff[3] = rx_buff[1];

    if (!transmit((uint *) &tx_buff, 4, false, false, false)) {

    }

    return;
}

tEconetTxResult ack_scout(uint sender_station, uint sender_network, uint control_byte, uint port) {
    uint tx_buff[4];

    tx_buff[0] = sender_station;
    tx_buff[1] = sender_network;
    tx_buff[2] = control_byte;
    tx_buff[3] = port;

    return transmit((uint *) &tx_buff, 4, false, false, false);
}

//TODO: remove scout param
tEconetTxResult transmit(uint *buff, int bytes, bool getAck, bool scout, bool immediate) {
    int sr1, sr2;
    unsigned long timeOut;
    bool ackResult = false, done = false;

    adlc_write(REG_CONTROL_1, 0b00000000); // Disable RX interrupts

    timeOut = time_ms() + cfg_tx_begin_timeout;

    while (!(adlc_read(REG_STATUS_1) & 64)) { // If we don't have TDRA, clear status until we do!  
        adlc_write(REG_CONTROL_2, 0b11100101); // Raise RTS, clear TX and RX status, flag fill and Prioritise status

        if (time_ms() > timeOut) {
            adlc_irq_reset();
            return PICONET_TX_RESULT_ERROR_TIMEOUT;
        }
    }

    for (int buffPtr = 0; buffPtr < bytes; buffPtr += 1) {
        // While not TDRA set, loop until it is - or we get an error
        while (true) {
            sr1 = adlc_read(REG_STATUS_1);

            if (sr1 & STATUS_1_FRAME_COMPLETE) {
                // We have TDRA
                break;
            }

            if (sr1 & 192) {
                // Some other error
                //printSR1(sr1); 
                adlc_irq_reset(); 
                return PICONET_TX_RESULT_ERROR_MISC;
            }

            if (time_ms() > timeOut) {
                adlc_irq_reset();
                return PICONET_TX_RESULT_ERROR_TIMEOUT;
            }
        }
  
        // Now we are ready, write the byte.
        adlc_write(REG_FIFO, buff[buffPtr]);
    } // End of for loop to tx bytes

    adlc_write(REG_CONTROL_2, 0b00111001); // Tell the ADLC that was the last byte, and clear flag fill modes and RTS. 
    adlc_write(REG_CONTROL_1, 0b00000100); // Tx interrupt enable
  
    // Do a last check for errors
    while (!adlc_get_irq()) {}; // Wait for IRQ
 
    sr1 = adlc_read(REG_STATUS_1);
    if (!sr1 & 64) {
        // Something other than Frame complete happened
        // printSR1(sr1);
        adlc_irq_reset();
        return false;
    }

    adlc_write(REG_CONTROL_2, 0b01100001); // Clear any pending status, prioritise status
    adlc_write(REG_CONTROL_1, 0b00000010); // Suppress tx interrupts, Enable RX interrupts

    if (!getAck) {
        // If ack not expected, return now
        return PICONET_TX_RESULT_OK;
    }

    if (wait_for_ack()) {
        return PICONET_TX_RESULT_OK;
    }

    return PICONET_TX_RESULT_ERROR_NO_ACK;
}

tEconetRxResult read_frame(void) {
    tEconetRxResult result;
    result.type = PICONET_RX_RESULT_NONE;

    uint ptr = 0;
    uint stat = 0;
    bool frame = true;

    uint rxControlByte;
    uint rxPort;

    uint scout_timeout = 0;

    // First byte should be address
    rx_buff[0] = adlc_read(REG_FIFO);

    // TODO: being nosey
    /*
    if (rx_buff[0] != config_Station && rx_buff[0] != 255 ) {
        // If frame is not for me, or a broadcast, then bail out
        adlc_irq_reset();
        return;
    }
    */

    // Wait for second byte (net address) 
    do {
        stat = adlc_read(REG_STATUS_2);
    } while (!(stat & 0b11111010)); // While no errors, or available data - keep polling

    rx_buff[1] = adlc_read(REG_FIFO);

    if (rx_buff[1] != config_Net && rx_buff[1] != 0 && !(rx_buff[1] >252) ) {
        // If frame is not addressed to my network, or a broadcast, then bail out
        // Nets:
        //    255 - global short broadcast (8 bytes)
        //    254 - global large broadcast (1020/1024 maximum)
        //    253 - local large broadcast  
        adlc_irq_reset();
        return result;
    }

    ptr = 2;

    if (time_ms() > scout_timeout && got_scout) {
        got_scout = false;
    }

    while (frame) {
        // Now check the status register
        do {
            stat = adlc_read(REG_STATUS_2);
        } while (!(stat & 0b11111010)); // While no errors, or available data - keep polling  

        rx_buff[ptr] = adlc_read(REG_FIFO);
        if (ptr < BUFFSIZE) {
            ptr++;
        } //TODO: keep overwriting last byte for now until I do this properly and abort the rx!

        if (stat & 0b01111010) {
            // Error or end of frame bits set so drop out
            frame = false;
        }
    } // End of while in frame - Data Available with no error bits or end set
 

    if (stat & STATUS_2_FRAME_VALID) { 
        // Frame is valid
  
        if (rx_buff[0] == 255 || rx_buff[0] == 0) {
            // Broadcast frame - treat specially, then drop out of receive flow
            result.type = PICONET_RX_RESULT_BROADCAST;
            result.detail.buffer = (uint *) &rx_buff;
            result.detail.length = ptr;
            return result;
            // TODO: could prolly fall-through here but let's get it working first!
        } else {
            result.type = PICONET_RX_RESULT_FRAME;
            result.detail.buffer = (uint *) &rx_buff;
            result.detail.length = ptr;

            // // Still here so a frame addressed to me - flag fill while we work out what to do with it
            
            // adlc_flag_fill(); // Flag fill and reset statuses - seems to need calling twice to clear everything
            // adlc_flag_fill();
        
            // if (got_scout == false) {
            //     // Am expecting a scout here, but could be an immediate op
            //     if (ptr > 7) {
            //         // Too big for a scout so must be an immediate operation
            //         result.type = PICONET_RX_RESULT_IMMEDIATE_OP;
            //         result.detail.buffer = (uint *) &rx_buff;
            //         result.detail.length = ptr;
            //     } else {
            //         // Acknowledge the scout, and set flag for next run
                    
            //         // rx_buff[2] = Sender station
            //         // rx_buff[3] = Sender network
            //         // rx_buff[4] = ControlByte
            //         // rx_buff[5] = Port
                    
            //         // Make a note of these, as we won't get them again in the payload
            //         rxControlByte = rx_buff[4];
            //         rxPort = rx_buff[5];

            //         // Only acknowledge if we are expecting something on the port
            //         if (true) { // (portInUse[rxPort]) { // TODO
            //             ack_rx();
            //             got_scout = true;

            //             // TODO: dubious - won't this always be time_ms as scout_timeout initialised as 0??
            //             scout_timeout = time_ms() + cfg_scout_timeout;

            //             result.type = PICONET_RX_RESULT_SCOUT;
            //             result.detail.buffer = (uint *) &rx_buff;
            //             result.detail.length = ptr;
            //         }
            //     }
            // } else {
            //     // Have got a payload after the scout, acknowledge and process
            //     ack_rx();
            //     got_scout = false;

            //     result.type = PICONET_RX_RESULT_FRAME;
            //     result.detail.buffer = (uint *) &rx_buff;
            //     result.detail.length = ptr;
            // }
        }

        // TODO: remove me
        result.detail.sr1 = adlc_read(REG_STATUS_1);
        result.detail.sr2 = adlc_read(REG_STATUS_2);
    } else {
        // Frame not valid - what happened?
        uint sr1 = adlc_read(REG_STATUS_1);
        uint sr2 = adlc_read(REG_STATUS_2);

        result.type = PICONET_RX_RESULT_ERROR;
        result.error.type = PICONET_ERROR_INVALID_FRAME;
        result.error.sr1 = sr1;
        result.error.sr2 = sr2;

        got_scout = false; 
    }

    // Do a final clear of any outstanding status bits
    sleep_us(1);  
    rx_reset();

    return result;
}

tEconetRxResult receive(void) {
    int do_irq_reset = false;
    uint status_reg_1 = adlc_read(REG_STATUS_1);

    if (status_reg_1 & STATUS_1_S2_RD_REQ) {
        uint status_reg_2 = adlc_read(REG_STATUS_2);

        if (status_reg_2 & STATUS_2_ADDR_PRESENT) {
            // Address present in FIFO, so fetch it and the rest of frame
            tEconetRxResult read_frame_result = read_frame();
            // adlc_reset();
            return read_frame_result;
        }
        
        if (status_reg_2 & STATUS_2_FRAME_VALID) {
            // Frame complete - not expecting a frame here, so read and discard
            adlc_read(REG_FIFO);
            do_irq_reset = true;
        }

        if (status_reg_2 & STATUS_2_INACTIVE_IDLE_RX) {
            // Inactive idle - clear open rx
            got_scout = false;
            do_irq_reset = true;
        }

        if (status_reg_2 & STATUS_2_ABORT_RX) {
            // TX abort received - not inside a frame here, so clear it
            do_irq_reset = true;
        }

        if (status_reg_2 & STATUS_2_FCS_ERROR) {
            // Frame error - not inside a frame here, so clear it 
            do_irq_reset = true;
        }

        if (status_reg_2 & STATUS_2_NOT_DCD) {
            // No clock
            do_irq_reset = true;
        }

        if (status_reg_2 & STATUS_2_RX_OVERRUN) {
            // Overrun - not inside a frame here, so clear it
            do_irq_reset = true;
        }

        if (status_reg_2 & STATUS_2_RDA) {
            // RX data available - not inside a frame here, so read and discard it
            do_irq_reset = true;
        }
    } else {             
        // Something else in SR1 needs servicing        
        if (status_reg_1 & STATUS_1_RDA) {
            // Not expecting data, so just read and ignore it!
            adlc_read(REG_FIFO);
        }
        do_irq_reset = true;
    }

    if (do_irq_reset) {
        adlc_irq_reset();
    }

    tEconetRxResult result;
    result.type = PICONET_RX_RESULT_NONE;
    return result;
}
