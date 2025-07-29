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
        packet.write(PACKET_START_MARKER);
        packet.write(AirConditionerCommand::Function::Mode);
        
        for (int32_t i = 0; i < 2; i++) {
            packet.write<AirConditionerState::Mode>(m_mode);
        }

        packet.write<uint8_t>(calculate_checksum(packet));
        packet.write(PACKET_END_MARKER);
    }

    void select(AirConditionerState::Mode value) { m_mode = value; }

private:
    AirConditionerState::Mode m_mode;
};
    
} // namespace jhs_ac
} // namespace esphome
