# pico-icsp-programmer
ICSP programmer for PIC18FxxQxx family devices on Raspberry Pi Pico

Not supported by Microchip. Use at your own risk.

## Usage
Build with CMake (`mkdir build; cd build; cmake ..; cmake --build .`), then drop the UF2 file onto a BOOTSEL-booted Pi Pico.

Connect the following pins to the PIC chip or ZIF/ICSP breakout:
| PIC/ICSP Pin | Pico Pin  |
|-------------:|:----------|
| Vdd          | 3V3       |
| GND/Vss      | GND       |
| Vpp/MCLR     | GP20      |
| ICSPDAT      | GP19/GP16 |
| ICSPCLK      | GP18      |

Then plug in the Pico, and send the HEX file over the serial connection. Feedback is printed over serial.

## Supported Devices
This should be compatible with most PIC18FxxQxx family devices. Some PIC18FxxKxx devices may work, but this is untested. PIC16F180xx chips may also work with some timing adjustments.