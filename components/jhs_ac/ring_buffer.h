#pragma once
#include <optional>
#include <stdint.h>

namespace esphome::jhs_ac {

template<class T, uint32_t N>
class RingBuffer 
{
public:
    RingBuffer() : m_buffer{}, m_head(0), m_tail(0), m_count(0) {}

    bool is_empty() const { return m_count == 0; }
    bool is_full() const { return m_count == N; }
    uint32_t size() const { return m_count; }
    uint32_t capacity() const { return N; }
    
    T& front() { return m_buffer[m_tail]; }
    const T& front() const { return m_buffer[m_tail]; }
    T& back() { return m_buffer[(m_head == 0) ? N - 1 : m_head - 1]; }
    const T& back() const { return m_buffer[(m_head == 0) ? N - 1 : m_head - 1]; }
    
    bool push(const T& value)
    {
        if (is_full()) {
            return false;
        }
        m_buffer[m_head] = value;
        m_head = (m_head + 1) % N;
        m_count++;
        return true;
    }

    std::optional<T> pop() 
    {
        if (is_empty()) {
            return std::nullopt;
        }
        T result = m_buffer[m_tail];
        m_tail = (m_tail + 1) % N;
        m_count--;
        return result;
    }

    void clear() 
    {
        m_head = 0;
        m_tail = 0;
        m_count = 0;
    }

private:
    T m_buffer[N];
    uint32_t m_head;
    uint32_t m_tail;
    uint32_t m_count;
};

} // namespace esphome::jhs_ac
