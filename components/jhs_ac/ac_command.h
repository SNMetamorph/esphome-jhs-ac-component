#pragma once
#include "binary_output_stream.h"
#include <stdint.h>

namespace esphome {
namespace jhs_ac {

class AirConditionerCommand
{
public:
    static constexpr uint8_t PACKET_START_MARKER = 0xA5;
    static constexpr uint8_t PACKET_END_MARKER = 0xF5;
    static constexpr uint32_t PACKET_AC_COMMAND_SIZE = 6;
    static constexpr uint32_t PACKET_AC_STATE_CHECKSUM_LENGTH = 3;

    enum class Function : uint8_t
    {
        Power = 0x11,
        Mode = 0x12,
        Sleep = 0x13,
        Temperature = 0x14,
        Oscillation = 0x15,
        FanSpeed = 0x16
    };

    uint32_t calculate_checksum(const BinaryOutputStream &packet);
    virtual void write_to_packet(BinaryOutputStream &packet) = 0;
};

static_assert(sizeof(AirConditionerCommand::Function) == sizeof(uint8_t), "Enumeration should have single byte size.");

} // namespace jhs_ac
} // namespace esphome
