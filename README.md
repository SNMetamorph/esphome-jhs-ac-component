# ESPHome component for JHS manufactured AC units

External ESPHome component for air conditioners made by [JHS (Dongguan Jinhongsheng Electric Co., Ltd.)](https://www.jhs8.com/). These air conditioners known under brands Timberk, Hyundai, Scoole, Lifetime Air.

Component allows you to control the AC from Home Assistant. It works by using UART interface between Wi-Fi module and AC main board, and in this scheme ESP8266 or ESP32 are used as replacement for Wi-Fi module (even if AC unit does not have module out of the box).

## Wiring scheme

This is image of air conditioner PCB, your specific board may look slightly different, but the only thing we need is WIFI header located in bottom right corner of PCB.

![PCB Image](/img/pcb.jpg)

The connection is quite simple. At first, measure the voltage between the RX and GND contacts on the air conditioner board: if there are 3.3 volts, then you can skip the logic level shifter and just directly connect the ESP pins to the contacts on the board. Otherwise, if there are 5 volts, you should use a 5V -> 3.3V logic level shifter to avoid damaging the ESP microcontroller.

Next, use this scheme to connect all the necessary pins:

- UART RX pin (ESP) -> TX pin (AC PCB)
- UART TX pin (ESP) -> RX pin (AC PCB)
- GND (ESP) -> GND (PCB)
- +5V (PCB) -> VCC/Vin (assuming that you're using ESP board like NodeMCU or Wemos which has on-board voltage regulator, otherwise you should manually use voltage regulator to get +3.3 volts for ESP)

## Tested & supported air conditioners

- Timberk T-PAC07-P12E
- Timberk T-PAC07-P09E-WF
- Goldair GCPAC350W

Feel free to share your experience in repository issues or submit pull requests to make this list more completed.

## Credits

Project was made possible because of [reverse-engineered JHS conditioners protocol specification](https://github.com/hutchx86/jhs-ac-protocol) made by @hutchx86.
