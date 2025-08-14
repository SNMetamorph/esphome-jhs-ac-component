#pragma once
#include "ac_command.h"
#include "ac_state.h"

namespace esphome::jhs_ac {
    
class FanSpeedCommand : public AirConditionerCommand
{
public:
    void write_to_packet(BinaryOutputStream &packet) override
    {
        serialize_command(packet, 
            static_cast<uint8_t>(Function::FanSpeed), 
            static_cast<uint8_t>(m_speed));
    }

    void set_speed(AirConditionerState::FanSpeed value) { m_speed = value; }

private:
    AirConditionerState::FanSpeed m_speed;
};
    
} // namespace esphome::jhs_ac
