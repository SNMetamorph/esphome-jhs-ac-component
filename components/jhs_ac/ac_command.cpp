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
    const uint32_t protocol_version = JHS_AC_PROTOCOL_VERSION;
    packet.write<uint8_t>(PACKET_START_MARKER);
    packet.write<uint8_t>(function_code);
    
    if (protocol_version == 1) 
    {
        for (int32_t i = 0; i < 2; i++) {
            packet.write<uint8_t>(argument);
        }
    }
    else if (protocol_version == 2)
    {
        packet.write<uint8_t>(0x01);
        packet.write<uint8_t>(argument);
    }

    packet.write<uint8_t>(calculate_checksum(packet));
    packet.write<uint8_t>(PACKET_END_MARKER);

    if (packet.get_length() != PACKET_AC_COMMAND_SIZE) {
        ESP_LOGW(JhsAirConditioner::TAG, "Invalid AC command packet length");
    }
}

} // namespace esphome::jhs_ac
