#include "packet_parser.h"
#include <cstring>

void PacketParser::process_byte(uint8_t data)
{
    if (m_current_state == State::Pending) 
    {
        if (data == PACKET_START_MARKER) 
        {
            m_current_state = State::Parsing;
            m_buffer.push_back(data);
        }
    }
    else if (m_current_state == State::Parsing) 
    {
        if (m_buffer.size() >= PACKET_AC_STATE_SIZE) 
        {
            m_buffer.clear();
            m_current_state = State::Pending;
        }
        else 
        {
            if (data == PACKET_END_MARKER)
            {
                if (m_buffer.size() == PACKET_AC_STATE_SIZE - 1) {
                    m_current_state = State::Finished;
                }
            }
            m_buffer.push_back(data);
        }
    }
}

bool PacketParser::packet_ready() const
{
    return m_current_state == State::Finished;
}

uint32_t PacketParser::read_packet(uint8_t *buffer, uint32_t buffer_size)
{
    const uint32_t data_size = m_buffer.size() * m_buffer.element_size();
    if (m_current_state == State::Finished && data_size <= buffer_size)
    {
        std::memcpy(buffer, &m_buffer[0], data_size);
        m_current_state = State::Pending;
        m_buffer.clear();
        return data_size;
    }
    return 0;
}
