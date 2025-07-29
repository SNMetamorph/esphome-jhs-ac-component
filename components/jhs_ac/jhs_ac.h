#pragma once

#include "esphome/core/component.h"
#include "esphome/components/climate/climate.h"
#include "esphome/components/uart/uart.h"
#include "esphome/core/log.h"
#include "binary_output_stream.h"
#include "ac_state.h"

namespace esphome {
namespace jhs_ac {

class JhsAirConditioner : public climate::Climate, public uart::UARTDevice, public esphome::Component
{
public:
    static constexpr const char *TAG = "jhs-ac";
    static constexpr float MIN_VALID_TEMPERATURE = 16.0f;
    static constexpr float MAX_VALID_TEMPERATURE = 31.0f;
    static constexpr float TEMPERATURE_STEP = 1.0f;

    static constexpr uint8_t PACKET_START_MARKER = 0xA5;
    static constexpr uint8_t PACKET_END_MARKER = 0xF5;
    static constexpr uint8_t PACKET_AC_STATE_SIZE = 18;
    static constexpr uint8_t PACKET_AC_STATE_CHECKSUM_LEN = 15;

    void setup() override;
    void loop() override;
    void dump_config() override;
    void control(const climate::ClimateCall &call) override;

protected:
    climate::ClimateTraits traits() override;

    void parse_ac_state(AirConditionerState &state, BinaryOutputStream &packet, uint32_t &checksum);
    void send_packet_to_ac(const BinaryOutputStream &packet);
    void dump_packet(const char *title, const BinaryOutputStream &packet);
    void dump_ac_state(const AirConditionerState &state);
    void update_ac_state(const AirConditionerState &state);
    bool validate_state_packet_checksum(const BinaryOutputStream &packet, uint32_t checksum);

private:
    AirConditionerState m_state;
    climate::ClimateTraits m_traits;
};

} // namespace jhs_ac
} // namespace esphome
