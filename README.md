# ESPHome component for JHS manufactured AC units

External ESPHome component for air conditioners made by [JHS (Dongguan Jinhongsheng Electric Co., Ltd.)](https://www.jhs8.com/). These air conditioners known under brands Timberk, Hyundai, Scoole.

Component allows you to control the AC from Home Assistant. It works by using UART interface between Wi-Fi module and AC main board, and in this scheme ESP8266 or ESP32 are used as replacement for Wi-Fi module (even if AC unit does not have module out of the box).

Project was made possible because of [reverse-engineered JHS conditioners protocol specification](https://github.com/hutchx86/jhs-ac-protocol) made by @hutchx86.

## Tested & supported air conditioners

Feel free to share your experience in repository issues or submit pull requests to make this list more completed.

- Timberk T-PAC07-P12E
- Timberk T-PAC07-P09E-WF
