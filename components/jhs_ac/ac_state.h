#pragma once
#include "binary_input_stream.h"
#include <stdint.h>

namespace esphome::jhs_ac {

struct AirConditionerState
{
public:
    enum class Mode : uint8_t
    {
        Cool = 0x1,
        Dehumidifying = 0x2,
        Fan = 0x3,
        Heat = 0x4
    };

    enum class FanSpeed : uint8_t
    {
        Low = 0x1,
        Medium = 0x2,
        High = 0x3
    };

    enum class TemperatureUnit : uint8_t
    {
        Celsius = 0x20,
        Fahrenheit = 0x24
    };

    enum class WaterTankState : uint8_t
    {
        Empty = 0x0,
        Full = 0x3
    };

    void read_from_packet(BinaryInputStream &stream, uint32_t &checksum);
    static const char *get_mode_name(Mode mode);

    bool power;
    bool sleep;
    bool oscillation;
    uint32_t temperature_ambient;
    uint32_t temperature_setting;
    Mode mode;
    FanSpeed fan_speed;
    TemperatureUnit temperature_unit;
    WaterTankState water_tank_state;

    // debugging stuff, may help with futher JHS protocol reverse-engineering
    uint8_t byte_0A;
    uint8_t byte_0B;
    uint8_t byte_0C;
    uint8_t byte_0D;
};

static_assert(sizeof(AirConditionerState::Mode) == sizeof(uint8_t), "Enumeration should have single byte size.");
static_assert(sizeof(AirConditionerState::FanSpeed) == sizeof(uint8_t), "Enumeration should have single byte size.");
static_assert(sizeof(AirConditionerState::TemperatureUnit) == sizeof(uint8_t), "Enumeration should have single byte size.");
static_assert(sizeof(AirConditionerState::WaterTankState) == sizeof(uint8_t), "Enumeration should have single byte size.");

} // namespace esphome::jhs_ac
