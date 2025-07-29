#pragma once
#include "ac_command.h"

namespace esphome {
namespace jhs_ac {
    
class PowerCommand : public AirConditionerCommand
{
public:
    void write_to_packet(BinaryOutputStream &packet) override
    {
        packet.write(PACKET_START_MARKER);
        packet.write(AirConditionerCommand::Function::Power);
        
        for (int32_t i = 0; i < 2; i++) {
            packet.write<uint8_t>(m_status ? 0x1 : 0x0);
        }

        packet.write<uint8_t>(calculate_checksum(packet));
        packet.write(PACKET_END_MARKER);
    }

    void toggle(bool state) { m_status = state; }

private:
    bool m_status;
};
    
} // namespace jhs_ac
} // namespace esphome
