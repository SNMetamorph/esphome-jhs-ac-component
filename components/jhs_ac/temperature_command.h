#pragma once
#include "ac_command.h"

namespace esphome::jhs_ac {
    
class TemperatureCommand : public AirConditionerCommand
{
public:
    void write_to_packet(BinaryOutputStream &packet) override
    {
        serialize_command(packet, 
            static_cast<uint8_t>(Function::Temperature), 
            static_cast<uint8_t>(m_temperature));
    }

    void set_temperature(int32_t value) { m_temperature = value; }

private:
    int32_t m_temperature;
};
    
} // namespace esphome::jhs_ac
