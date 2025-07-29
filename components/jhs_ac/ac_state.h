#pragma once
#include <stdint.h>

namespace esphome {
namespace jhs_ac {

struct AirConditionerState
{
public:
    enum class Mode : uint8_t
    {
        Cool = 0x1,
        Dehumidifying = 0x2,
        Fan = 0x3
    };

    enum class FanSpeed : uint8_t
    {
        Low = 0x1,
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

    static const char *get_mode_name(Mode mode);

    bool power;
    bool sleep;
    uint32_t temperature_ambient;
    uint32_t temperature_setting;
    Mode mode;
    FanSpeed fan_speed;
    TemperatureUnit temperature_unit;
    WaterTankState water_tank_state;
};

static_assert(sizeof(AirConditionerState::Mode) == sizeof(uint8_t), "Enumeration should have single byte size.");
static_assert(sizeof(AirConditionerState::FanSpeed) == sizeof(uint8_t), "Enumeration should have single byte size.");
static_assert(sizeof(AirConditionerState::TemperatureUnit) == sizeof(uint8_t), "Enumeration should have single byte size.");
static_assert(sizeof(AirConditionerState::WaterTankState) == sizeof(uint8_t), "Enumeration should have single byte size.");

} // namespace jhs_ac
} // namespace esphome
