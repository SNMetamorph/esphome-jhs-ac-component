#pragma once
#include "ac_command.h"
#include "ac_state.h"

namespace esphome {
namespace jhs_ac {
    
class ModeCommand : public AirConditionerCommand
{
public:
    void write_to_packet(BinaryOutputStream &packet) override
    {
        serialize_command(packet, static_cast<uint8_t>(Function::Mode), static_cast<uint8_t>(m_mode));
    }

    void select(AirConditionerState::Mode value) { m_mode = value; }

private:
    AirConditionerState::Mode m_mode;
};
    
} // namespace jhs_ac
} // namespace esphome
