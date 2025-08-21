#include "ac_command.h"
#include "binary_input_stream.h"
#include "jhs_ac.h"

namespace esphome::jhs_ac {
    
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
    constexpr uint32_t protocol_version = JHS_AC_PROTOCOL_VERSION;
    packet.write<uint8_t>(PACKET_START_MARKER);
    packet.write<uint8_t>(function_code);
    packet.write<uint8_t>(protocol_version == 1 ? argument : 0x01);
    packet.write<uint8_t>(argument);
    packet.write<uint8_t>(calculate_checksum(packet));
    packet.write<uint8_t>(PACKET_END_MARKER);

    if (packet.get_length() != PACKET_AC_COMMAND_SIZE) {
        ESP_LOGW(JhsAirConditioner::TAG, "Invalid AC command packet length");
    }
}

} // namespace esphome::jhs_ac
