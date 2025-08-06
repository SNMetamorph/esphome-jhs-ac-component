#pragma once
#include "ac_command.h"

namespace esphome {
namespace jhs_ac {
    
class PowerCommand : public AirConditionerCommand
{
public:
    void write_to_packet(BinaryOutputStream &packet) override
    {
        serialize_command(packet, 
            static_cast<uint8_t>(Function::Power), 
            static_cast<uint8_t>(m_status ? 0x1 : 0x0));
    }

    void toggle(bool state) { m_status = state; }

private:
    bool m_status;
};
    
} // namespace jhs_ac
} // namespace esphome
