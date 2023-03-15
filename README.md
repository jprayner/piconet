# Piconet

Piconet allows modern computers (MacOS, PC and Linux) to talk to Acorn Econet networks using a board which interfaces the ADF10 Econet module (BBC Master/Archimedes Econet board) to a Raspberry Pi Pico. The Pico can then be connected to your machine via USB.

Piconet is theoretically capable of supporting anything you can dream up which runs over Econet: file transfer utilities, file servers, D&D servers etc.

![system-view](https://user-images.githubusercontent.com/909745/225333540-71af28cc-e700-44a8-87bd-7ba4ffddd420.png)

This project provides the purple boxes in the above diagram: the hardware design of the Piconet board, the firmware which executes on the Pico, and the drivers which talk to it.

## Related projects

* [ecoclient](https://github.com/jprayner/ecoclient) — a command-line utility for performing Econet operations

## State of development

This project is still under development. Currently:

* The board currently only exists in prototype form. A circuit diagram is available here. It is hoped that a prebuilt board will be available soon.
* At the moment, a (wired) USB connection is required between the Pico on the Piconet board and the host machine. In the future, this may be extended to allow your machine to connect to a Pico W over a WiFi network.
* The only driver available today is based on Node.js. Contributions of drivers based on other languages are welcome. Please get in touch if you are interested in writing one (Python, Golang etc.)!

## Getting started

1. Acquire or build the Piconet board
2. Download the `.uf2` image from the releases page
3. Connect the Pico to your machine with a USB cable whilst holding down the button on the Pico board
4. The Pico should show as a storage device: copy the `.uf2` image to it
5. The Pico should reboot automatically when the copy is complete
6. Install an app such [ecoclient](https://github.com/jprayner/ecoclient) or try one of the examples

## Credits

Thanks to the following projects:

* Base64 encode/decode code taken from: https://github.com/mbrt/libb64
* ADF-10 hardware interfacing and Econet protocol insights: https://github.com/cr12925/PiEconetBridge
* More Econet protocol insights: https://github.com/stardot/ArduinoFilestore
* General discussion on BBC Micro and Econent topics: https://stardot.org.uk
