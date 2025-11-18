#pragma once
#include "fixed_vector.h"
#include <stdint.h>

namespace esphome::jhs_ac {

class PacketParser
{
public:
    PacketParser() : m_current_state(State::Pending) {};

    void process_byte(uint8_t data);
    bool packet_ready() const;
    uint32_t read_packet(uint8_t *buffer, uint32_t buffer_size);

private:
    static constexpr uint8_t PACKET_START_MARKER = 0xA5;
    static constexpr uint8_t PACKET_END_MARKER = 0xF5;
    static constexpr uint8_t PACKET_AC_STATE_SIZE = 18;

    enum class State
    {
        Pending,
        Parsing,
        Finished
    };

    State m_current_state;
    FixedVector<uint8_t, 32> m_buffer;
};

} // namespace esphome::jhs_ac
