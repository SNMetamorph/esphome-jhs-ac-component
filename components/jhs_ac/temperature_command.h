#pragma once
#include "ac_command.h"

namespace esphome {
namespace jhs_ac {
    
class TemperatureCommand : public AirConditionerCommand
{
public:
    void write_to_packet(BinaryOutputStream &packet) override
    {
        packet.write(PACKET_START_MARKER);
        packet.write(AirConditionerCommand::Function::Temperature);
        
        for (int32_t i = 0; i < 2; i++) {
            packet.write<uint8_t>(m_temperature);
        }

        packet.write<uint8_t>(calculate_checksum(packet));
        packet.write(PACKET_END_MARKER);
    }

    void set_temperature(int32_t value) { m_temperature = value; }

private:
    int32_t m_temperature;
};
    
} // namespace jhs_ac
} // namespace esphome
