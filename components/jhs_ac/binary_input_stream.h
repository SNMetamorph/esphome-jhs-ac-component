#pragma once
#include <stdint.h>
#include <optional>
#include <cstring>

namespace esphome {
namespace jhs_ac {

class BinaryInputStream
{
public:
    BinaryInputStream(const uint8_t *data, uint32_t length) :
        m_size(length),
        m_offset(0),
        m_data(data) {}

    template<class T> std::optional<T> read() 
    {
        uint8_t buffer[sizeof(T)] = {0};
        T *object = reinterpret_cast<T*>(buffer);
        if (sizeof(T) <= remaining_space()) 
        {
            memcpy(buffer, m_data + m_offset, sizeof(T));
            m_offset += sizeof(T);
            return std::optional<T>(*object);
        }
        return std::nullopt;
    }

    bool read_bytes(uint8_t *data, uint32_t count)
    {
        if (count <= remaining_space())
        {
            memcpy(data, m_data + m_offset, count);
            m_offset += count;
            return true;
        }
        return false;
    }

    bool skip_bytes(uint32_t count)
    {
        if (count <= remaining_space())
        {
            m_offset += count;
            return true;
        }
        return false;
    }

    uint32_t get_size() const { return m_size; }
    uint32_t remaining_space() const { return m_size - m_offset; }

private: 
    uint32_t m_size;
    uint32_t m_offset;
    const uint8_t *m_data;
};

} // namespace jhs_ac
} // namespace esphome
