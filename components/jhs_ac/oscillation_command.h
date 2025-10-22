#pragma once
#include "ac_command.h"

namespace esphome::jhs_ac {
    
class OscillationCommand : public AirConditionerCommand
{
public:
    void write_to_packet(BinaryOutputStream &packet) override
    {
        serialize_command(packet, 
            static_cast<uint8_t>(Function::Oscillation), 
            static_cast<uint8_t>(m_status ? 0x01 : 0x00));
    }

    void set_status(bool status) { m_status = status; }

private:
    bool m_status;
};

} // namespace esphome::jhs_ac
