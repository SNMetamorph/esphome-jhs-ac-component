#pragma once
#include <stdint.h>
#include <string.h>
#include <optional>

namespace esphome::jhs_ac {

class BinaryOutputStream
{
public:
    BinaryOutputStream(uint8_t *buffer, uint32_t length) : 
        m_size(length),
        m_offset(0),
        m_data(buffer) {}

    template<class T> bool write(const T& object)
    {
        if (sizeof(T) <= remaining_size())
        {
            memcpy(m_data + m_offset, &object, sizeof(T));
            m_offset += sizeof(T);
            return true;
        }
        return false;
    }

    bool write_bytes(const uint8_t *data, uint32_t count)
    {
        if (count <= remaining_size())
        {
            memcpy(m_data + m_offset, data, count);
            m_offset += count;
            return true;
        }
        return false;
    }

    void reset() { m_offset = 0; }
    uint32_t get_capacity() const { return m_size; }
    uint32_t get_length() const { return m_offset; }
    uint32_t remaining_size() const { return m_size - m_offset; }
    uint8_t *get_buffer_addr() { return m_data; }
    const uint8_t *get_buffer_addr() const { return m_data; }

private:
    uint32_t m_size;
    uint32_t m_offset;
    uint8_t *m_data;
};

} // namespace esphome::jhs_ac
