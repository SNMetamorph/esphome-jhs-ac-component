#pragma once
#include <optional>
#include <stdint.h>

template<class T, uint32_t N>
class FixedVector
{
public:
    FixedVector() : m_size(0) {}

    void clear() { m_size = 0; }
    uint32_t size() const { return m_size; }
    uint32_t capacity() const { return N; }
    uint32_t element_size() const { return sizeof(T); }
    
    T& front() { return m_buffer[0]; }
    T& back() { return m_buffer[(m_size == 0) ? 0 : m_size - 1]; }
    T& operator[](const uint32_t index) { return m_buffer[index]; }

    bool push_back(const T& value) 
    {
        const uint32_t capacity = N;
        if (m_size < capacity) 
        {
            m_buffer[m_size] = value;
            m_size++;
            return true;
        }
        return false;
    }

    std::optional<T> pop_back() 
    {
        T result = m_buffer[m_size];
        if (m_size > 0) 
        {
            m_size--;
            return result;
        }
        return std::nullopt;
    }

private:
    uint32_t m_size;
    T m_buffer[N];
};
