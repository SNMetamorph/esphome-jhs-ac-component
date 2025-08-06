#include "ac_command.h"
#include "binary_input_stream.h"

namespace esphome {
namespace jhs_ac {
    
uint32_t AirConditionerCommand::calculate_checksum(const BinaryOutputStream &packet)
{
    uint32_t sum = 0;
    BinaryInputStream stream(packet.get_buffer_addr(), packet.get_length());

    stream.skip_bytes(1);
    for (int32_t i = 0; i < PACKET_AC_STATE_CHECKSUM_LENGTH; i++) {
        sum += stream.read<uint8_t>().value();
    }
    return sum % 256;
}

void AirConditionerCommand::serialize_command(BinaryOutputStream &packet, uint8_t function_code, uint8_t argument)
{
    packet.write(PACKET_START_MARKER);
    packet.write(function_code);
    
    for (int32_t i = 0; i < 2; i++) {
        packet.write(argument);
    }

    packet.write<uint8_t>(calculate_checksum(packet));
    packet.write(PACKET_END_MARKER);
}

} // namespace jhs_ac
} // namespace esphome
