#pragma once

#include "esphome/core/component.h"
#include "esphome/components/climate/climate.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/core/log.h"
#include "binary_output_stream.h"
#include "ac_state.h"
#include "packet_parser.h"
#include "ring_buffer.h"

namespace esphome::jhs_ac {

class JhsAirConditioner : public climate::Climate, public uart::UARTDevice, public esphome::Component
{
public:
    JhsAirConditioner() : m_water_tank_sensor(nullptr) {};

    static constexpr const char *TAG = "jhs-ac";
    static constexpr float MIN_VALID_TEMPERATURE = 16.0f;
    static constexpr float MAX_VALID_TEMPERATURE = 31.0f;
    static constexpr float TEMPERATURE_STEP = 1.0f;
    static constexpr uint8_t PACKET_AC_STATE_CHECKSUM_LEN = 15;

    void setup() override;
    void loop() override;
    void dump_config() override;
    void control(const climate::ClimateCall &call) override;
    void set_water_tank_sensor(binary_sensor::BinarySensor *sensor);

protected:
    climate::ClimateTraits traits() override;
    void read_uart_data();
    void parse_received_data();
    void send_packet_to_ac(const BinaryOutputStream &packet);
    void dump_packet(const char *title, const uint8_t *data, uint32_t length);
    void dump_ac_state(const AirConditionerState &state);
    void update_ac_state(const AirConditionerState &state);
    bool validate_state_packet_checksum(const BinaryInputStream &stream, uint32_t checksum);

private:
    AirConditionerState m_state;
    climate::ClimateTraits m_traits;
    binary_sensor::BinarySensor *m_water_tank_sensor;
    PacketParser m_parser;
    RingBuffer<uint8_t, 128> m_data_buffer;
};

} // namespace esphome::jhs_ac
