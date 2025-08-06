#include "ac_state.h"
#include "binary_input_stream.h"

namespace esphome {
namespace jhs_ac {

const char *AirConditionerState::get_mode_name(Mode mode)
{
    switch (mode)
    {
        case Mode::Cool: return "Cool";
        case Mode::Dehumidifying: return "Dehumidifying";
        case Mode::Fan: return "Fan";
        default: return "Unknown";
    }
}

void AirConditionerState::read_from_packet(const BinaryOutputStream &packet, uint32_t &checksum)
{
    BinaryInputStream stream(packet.get_buffer_addr(), packet.get_length());
    stream.skip_bytes(3); // skip packet start marker and 2 blank bytes
    this->power = stream.read<uint8_t>().value();
    this->mode = stream.read<Mode>().value();
    this->sleep = stream.read<uint8_t>().value();
    this->temperature_ambient = stream.read<uint8_t>().value();
    this->temperature_setting = stream.read<uint8_t>().value();
    this->byte_08 = stream.read<uint8_t>().value();
    this->fan_speed = stream.read<FanSpeed>().value();
    this->byte_0A = stream.read<uint8_t>().value();
    this->byte_0B = stream.read<uint8_t>().value();
    this->byte_0C = stream.read<uint8_t>().value();
    this->byte_0D = stream.read<uint8_t>().value();
    this->temperature_unit = stream.read<TemperatureUnit>().value();
    this->water_tank_state = stream.read<WaterTankState>().value();
    checksum = stream.read<uint8_t>().value();
    stream.skip_bytes(1); // skip packet end marker
}

} // namespace jhs_ac
} // namespace esphome
