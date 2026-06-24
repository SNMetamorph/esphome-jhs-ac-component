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

## Usage

You need to add three sections to your ESPHome device configuration YAML file. The first section will make able to use this component in your configuration:

```yaml
external_components:
  - source: github://SNMetamorph/esphome-jhs-ac-component@master
    components: [ jhs_ac ]
    refresh: 0s
```

The second section is configuration of UART interface. It is mandatory for communicating between AC and ESPHome device:
```yaml
uart:
  # ATTENTION! Use GPIO4 (D2) and GPIO5 (D1) as the TX and RX for NodeMCU-like boards.
  # If you decided to flash ESPHome firmware to built-in Wi-Fi module, it's up to you to determine correct GPIO pinout.
  tx_pin: D2
  rx_pin: D1
  baud_rate: 9600
  data_bits: 8
  parity: NONE
  stop_bits: 1
```

The third section is definition of your air conditioning device itself. Here, you should specify the device's capabilities, model name, and protocol version. You can determine this through trial and error, or refer to the table of supported & tested air conditioners below.
```yaml
climate:
  - platform: jhs_ac
    name: "Your AC model"
    protocol_version: 1 # available options are 1 & 2, select one that works correctly with your AC
    supported_modes: # add HEAT mode, if your unit supports it
      - COOL
      - DRY
      - FAN_ONLY
    supported_fan_modes: # add MEDIUM fan mode, if your unit supports it
      - LOW
      - HIGH
    supported_swing_modes: # add VERTICAL swing mode, if your unit supports it
    water_tank_status: # this will create binary sensor indicating internal water tank occupance
      name: Water Tank Status
```

You can also check `/examples` folder for existing ESPHome configurations for specific air conditioner models.

## Tested & supported air conditioners

| *Brand*      | *Model name*      | *Protocol version* | *OEM model name*
|--------------|-------------------|--------------------|-----------------
| Timberk      | T-PAC07-P12E      | 1                  | [A029A](https://www.jhs8.com/products_detail/107.html)
| Timberk      | T-PAC07-P09E-WF   | 1                  | [A029A](https://www.jhs8.com/products_detail/107.html)
| Goldair      | GCPAC350W         | 1                  | [A018C](https://www.jhs8.com/products_detail/26.html)
| Lifetime Air | JHS-A016-09KR2/A  | 2                  | [A016A](https://www.jhs8.com/products_detail/10.html)
| Honeywell    | HJ14CESVWK        | 1                  | [A020A](https://www.jhs8.com/products_detail/43.html)

Feel free to share your experience in repository issues or submit pull requests to make this list more completed.

## Credits

Project was made possible because of [reverse-engineered JHS conditioners protocol specification](https://github.com/hutchx86/jhs-ac-protocol) made by @hutchx86.
