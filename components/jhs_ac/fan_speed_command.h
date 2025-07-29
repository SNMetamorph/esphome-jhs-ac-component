#pragma once
#include "ac_command.h"
#include "ac_state.h"

namespace esphome {
namespace jhs_ac {
    
class FanSpeedCommand : public AirConditionerCommand
{
public:
    void write_to_packet(BinaryOutputStream &packet) override
    {
        packet.write(PACKET_START_MARKER);
        packet.write(AirConditionerCommand::Function::FanSpeed);
        
        for (int32_t i = 0; i < 2; i++) {
            packet.write<AirConditionerState::FanSpeed>(m_speed);
        }

        packet.write<uint8_t>(calculate_checksum(packet));
        packet.write(PACKET_END_MARKER);
    }

    void set_speed(AirConditionerState::FanSpeed value) { m_speed = value; }

private:
    AirConditionerState::FanSpeed m_speed;
};
    
} // namespace jhs_ac
} // namespace esphome
