# Piconet board

This directory contains the firmware for Piconet which comprises ANSI C code for the Raspberry Pi Pico and accompanying PIO assembly language.

## Hardware schematic

The Piconet board is simply a couple of SN74LVC245 buffers doing conversion between the 3.3V CMOS levels of the Pico and the 5V TTL levels of ADF10 module. SN74HCT245 buffers work fine too. The ADF10 is powered from the 5V of the USB connection, no need for a separate power supply.

![Schematic_piconet_2023-02-15](https://user-images.githubusercontent.com/909745/219204768-f511ca68-83ad-4179-b935-38036b83f05f.png)

## Prototype

It is hoped that a prebuilt board will be available for purchase soon but currently this exists only as a big ole mess of wires on a breadboard:

![breadboard](https://user-images.githubusercontent.com/909745/229339862-0e919559-c6a4-44d5-bc3d-d3d99635fa6b.jpg)

I knocked up a separate board for the DIN connector:

![DIN5-conn-front](https://user-images.githubusercontent.com/909745/229340063-d8e43cc3-caf1-4c82-9b0a-10b5a3b97085.jpg)

![DIN5-conn-back](https://user-images.githubusercontent.com/909745/229340078-ed838d5a-56e3-4ab2-a40a-79b63b046099.jpg)

### Hardware testing

Before connecting the board to the USB of your computer, use a multimeter to make sure that you don't have a short between VCC and ground.

Before connecting the board to an Econet module, it is recommended that you validate connections carefully to avoid frustration and potential damage to your Econet module! 

Flash the Pico with latest firmware as described in the [Getting started](https://github.com/jprayner/piconet#getting-started) guide.

Connect a logic analyser to the 5V side of the buffers instead of the ADF10. Open a serial connection to the Pico and execute the `TEST` command. You should see the message:

```
TESTING BOARD (REBOOT TO STOP)
```

The board will cycle through the following:

1. For each of the register addresses a0:a1 (i.e. 4 values), perform 8 writes with a single data bit d0:d7 high at a time
2. Assert !RST
3. Wait approx. 100ms
4. Release !RST
5. Wait approx. 100ms

Here is a logic analyser trace with steps [1] and [2] visible:

![newboard2](https://user-images.githubusercontent.com/909745/229341287-9c1a31fa-7497-4a26-b3a0-b025fde5e477.png)

Once you've satisfied yourself that each of the signals is doing what it should, check you have 5V across pins 16 and 17 of the header, disconnect the logic analyser and plug in your ADF10 module.

## ADLC signals & timing

The ADLC is clocked at 2 MHz from GPIO 21. The time-sensitive business of reading/writing data to/from the bus, and of generating the `!CS`, `R!W` signals, is handled using the Pico's — amazing! — PIO (Programmable Input/Output) state machine feature. This is defined in [pinctl.pio](https://github.com/jprayner/piconet/blob/main/board/src/pinctl.pio). Initialisation of the PIO and communication with it is handled by the [adlc.c](https://github.com/jprayner/piconet/blob/main/board/src/adlc.c) module.

Here's an example of a write of 0x01 followed by a write of 0x02 to control register 1 (a0 and a1 both zero):

![newboard1](https://user-images.githubusercontent.com/909745/229342641-037f1345-7197-4bba-b61d-28f8250de281.png)

Note that `d0:d7`, `a0:a1` and `R!W` are set-up — and `!ADLC` asserted to perform an operation — in good time before the clock's rising edge. Hold times are observed before the signals are released after the clock falls. See datasheet for the MC68B54 for more information.
