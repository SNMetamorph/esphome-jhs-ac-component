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
#include <optional>
#include <set>

namespace esphome::jhs_ac {

struct CommandPacket
{
    uint32_t length;
    uint8_t data[18];
};

class JhsAirConditioner : public climate::Climate, public uart::UARTDevice, public esphome::Component
{
public:
    JhsAirConditioner() : 
        m_water_tank_sensor(nullptr), 
        m_last_command_send_time(0) {};

    static constexpr const char *TAG = "jhs-ac";
    static constexpr float MIN_VALID_TEMPERATURE = 16.0f;
    static constexpr float MAX_VALID_TEMPERATURE = 31.0f;
    static constexpr float TEMPERATURE_STEP = 1.0f;
    static constexpr uint8_t PACKET_AC_STATE_CHECKSUM_LEN = 15;
    static constexpr uint32_t TX_QUEUE_PACKETS_INTERVAL_MS = 100;

    void setup() override;
    void loop() override;
    void dump_config() override;
    void control(const climate::ClimateCall &call) override;
    float get_setup_priority() const override;
    void set_water_tank_sensor(binary_sensor::BinarySensor *sensor);
    void add_supported_mode(climate::ClimateMode mode);
    void add_supported_fan_mode(climate::ClimateFanMode fan_mode);
    void add_supported_swing_mode(climate::ClimateSwingMode swing_mode);

protected:
    climate::ClimateTraits traits() override;
    void read_uart_data();
    void parse_received_data();
    void send_queued_command();
    void add_packet_to_queue(const BinaryOutputStream &packet);
    void send_packet_to_ac(const uint8_t *data, uint32_t length);
    void dump_packet(const char *title, const uint8_t *data, uint32_t length);
    void dump_ac_state(const AirConditionerState &state);
    void update_ac_state(const AirConditionerState &state);
    bool validate_state_packet_checksum(const BinaryInputStream &stream, uint32_t checksum);

    std::optional<AirConditionerState::Mode> get_mapped_ac_mode(climate::ClimateMode climate_mode) const;
    std::optional<AirConditionerState::FanSpeed> get_mapped_fan_speed(climate::ClimateFanMode fan_mode) const;
    std::optional<climate::ClimateFanMode> get_mapped_climate_fan_mode(AirConditionerState::FanSpeed fan_speed) const;
    const char* get_fan_speed_name(AirConditionerState::FanSpeed fan_speed) const;

private:
    AirConditionerState m_state;
    climate::ClimateTraits m_traits;
    std::set<climate::ClimateMode> m_supported_modes;
    std::set<climate::ClimateFanMode> m_supported_fan_modes;
    std::set<climate::ClimateSwingMode> m_supported_swing_modes;
    binary_sensor::BinarySensor *m_water_tank_sensor;
    PacketParser m_parser;
    RingBuffer<uint8_t, 128> m_data_buffer;
    RingBuffer<CommandPacket, 8> m_tx_queue;
    uint32_t m_last_command_send_time;
};

} // namespace esphome::jhs_ac
