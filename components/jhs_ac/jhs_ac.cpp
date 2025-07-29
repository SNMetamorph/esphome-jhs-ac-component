#include "jhs_ac.h"
#include "binary_input_stream.h"
#include "power_command.h"
#include "mode_command.h"
#include "fan_speed_command.h"
#include "sleep_command.h"
#include "temperature_command.h"
#include "esphome/core/macros.h"
#include <cmath>

#define VERBOSE_LOGGING 1

namespace esphome {
namespace jhs_ac {

void JhsAirConditioner::setup()
{
    m_traits.set_visual_min_temperature(MIN_VALID_TEMPERATURE);
    m_traits.set_visual_max_temperature(MAX_VALID_TEMPERATURE);
    m_traits.set_visual_temperature_step(TEMPERATURE_STEP);
    m_traits.set_supports_current_temperature(true);
    m_traits.set_supports_two_point_target_temperature(false);

    m_traits.set_supported_swing_modes({});
    m_traits.set_supported_modes({climate::CLIMATE_MODE_OFF,
                                climate::CLIMATE_MODE_COOL,
                                climate::CLIMATE_MODE_DRY,
                                climate::CLIMATE_MODE_FAN_ONLY});

    m_traits.set_supported_fan_modes({climate::CLIMATE_FAN_LOW,
                                    climate::CLIMATE_FAN_HIGH});

    m_traits.set_supported_presets({climate::CLIMATE_PRESET_NONE, 
                                    climate::CLIMATE_PRESET_SLEEP});
}

void JhsAirConditioner::loop()
{
    uint32_t checksum;
    uint8_t packet_data[64];
    BinaryOutputStream state_packet(packet_data, sizeof(packet_data));

    while (available() >= PACKET_AC_STATE_SIZE)
    {
        if (peek() != PACKET_START_MARKER)
        {
            read(); // skip bytes until reaching packet start marker
            continue;
        }

        for (int32_t i = 0; i < PACKET_AC_STATE_SIZE; i++) {
            state_packet.write(static_cast<uint8_t>(read()));
        }
        
        dump_packet("Received packet", state_packet);
        parse_ac_state(m_state, state_packet, checksum);

        if (validate_state_packet_checksum(state_packet, checksum)) 
        {
            dump_ac_state(m_state);
            update_ac_state(m_state);
        }
        else {
            ESP_LOGW(TAG, "Invalid packet checksum, ignoring");
        }
    }
}

void JhsAirConditioner::dump_config()
{
    ESP_LOGCONFIG(TAG, "JHS Air Conditioner Component:");
    this->dump_traits_(TAG);
    this->check_uart_settings(9600, 1, uart::UART_CONFIG_PARITY_NONE, 8);
}

void JhsAirConditioner::control(const climate::ClimateCall &call)
{
    uint8_t packet_data[64];
    BinaryOutputStream packet_stream(packet_data, sizeof(packet_data));
    auto mode = call.get_mode();
    auto fan_mode = call.get_fan_mode();
    auto preset = call.get_preset();
    auto temperature = call.get_target_temperature();
    
    if (mode.has_value())
    {
        bool waking_up_ac = !m_state.power && mode.value() != climate::CLIMATE_MODE_OFF;
        bool turning_off_ac = mode.value() == climate::CLIMATE_MODE_OFF;

        // turn on AC before changing mode to something else
        if (waking_up_ac || turning_off_ac)
        {
            PowerCommand power_command;
            packet_stream.reset();
            power_command.toggle(waking_up_ac ? true : false);
            power_command.write_to_packet(packet_stream);
            send_packet_to_ac(packet_stream);  
        }

        if (mode.value() != climate::CLIMATE_MODE_OFF)
        {
            ModeCommand mode_command;
            switch (mode.value())
            {
                case climate::CLIMATE_MODE_COOL:
                    mode_command.select(AirConditionerState::Mode::Cool);
                    break;
                case climate::CLIMATE_MODE_DRY:
                    mode_command.select(AirConditionerState::Mode::Dehumidifying);
                    break;
                case climate::CLIMATE_MODE_FAN_ONLY:
                    mode_command.select(AirConditionerState::Mode::Fan);
                    break;
                default:
                    ESP_LOGW(TAG, "Unsupported AC mode was requested, fallback to fan only mode");
                    mode_command.select(AirConditionerState::Mode::Fan);
                    break;
            }

            packet_stream.reset();
            mode_command.write_to_packet(packet_stream);
            send_packet_to_ac(packet_stream);
        }
    }

    if (fan_mode.has_value())
    {
        FanSpeedCommand fan_speed_command;
        if (fan_mode.value() == climate::CLIMATE_FAN_LOW) {
            fan_speed_command.set_speed(AirConditionerState::FanSpeed::Low);
        }
        else {
            fan_speed_command.set_speed(AirConditionerState::FanSpeed::High);
        }
        
        packet_stream.reset();
        fan_speed_command.write_to_packet(packet_stream);
        send_packet_to_ac(packet_stream);
    }

    if (preset.has_value())
    {
        SleepCommand sleep_command;
        sleep_command.toggle(preset.value() == climate::CLIMATE_PRESET_SLEEP ? true : false);
        packet_stream.reset();
        sleep_command.write_to_packet(packet_stream);
        send_packet_to_ac(packet_stream);
    }

    if (temperature.has_value())
    {
        TemperatureCommand temperature_command;
        temperature_command.set_temperature(static_cast<int32_t>(temperature.value()));
        packet_stream.reset();
        temperature_command.write_to_packet(packet_stream);
        send_packet_to_ac(packet_stream);
    }
}

climate::ClimateTraits JhsAirConditioner::traits()
{
    return m_traits;
}

void JhsAirConditioner::dump_packet(const char *title, const BinaryOutputStream &stream)
{
#if VERBOSE_LOGGING == 1
    char str[256] = {0};
    char *pstr = str;
    const uint8_t *data = stream.get_buffer_addr();

    ESP_LOGD(TAG, "%s (%u bytes):", title, stream.get_length());
    for (int32_t i = 0; i < stream.get_length(); i++) {
        pstr += sprintf(pstr, "%02X ", data[i]);
    }
    ESP_LOGD(TAG, "%s", str);
#endif
}

void JhsAirConditioner::dump_ac_state(const AirConditionerState &state)
{
#if VERBOSE_LOGGING == 1
    ESP_LOGD(TAG, "Air conditioner state:");
    ESP_LOGD(TAG, "  Power: %s", state.power ? "On" : "Off");
    ESP_LOGD(TAG, "  Mode: %s", AirConditionerState::get_mode_name(state.mode));
    ESP_LOGD(TAG, "  Sleep mode: %s", state.sleep ? "On" : "Off");
    ESP_LOGD(TAG, "  Ambient temperature: %u", state.temperature_ambient);
    ESP_LOGD(TAG, "  Temperature setting: %u", state.temperature_setting);
    ESP_LOGD(TAG, "  Fan speed: %s", (state.fan_speed == AirConditionerState::FanSpeed::Low) ? "Low" : "High");
    ESP_LOGD(TAG, "  Temperature units: %s", (state.temperature_unit == AirConditionerState::TemperatureUnit::Celsius) ? "Celsius" : "Fahrenheit");
    ESP_LOGD(TAG, "  Water tank state: %s", (state.water_tank_state == AirConditionerState::WaterTankState::Empty) ? "Empty" : "Full");
#endif
}

void JhsAirConditioner::update_ac_state(const AirConditionerState &state)
{
    if (!state.power) {
        this->mode = climate::CLIMATE_MODE_OFF;
    }
    else
    {
        switch (state.mode)
        {
        case AirConditionerState::Mode::Cool:
            this->mode = climate::CLIMATE_MODE_COOL;
            break;
        case AirConditionerState::Mode::Dehumidifying:
            this->mode = climate::CLIMATE_MODE_DRY;
            break;
        case AirConditionerState::Mode::Fan:
            this->mode = climate::CLIMATE_MODE_FAN_ONLY;
            break;
        default:
            ESP_LOGW(TAG, "Unknown AC mode, command was ignored");
            return;
        }
    }

    this->target_temperature = state.temperature_setting;
    this->current_temperature = state.temperature_ambient;
    this->preset = state.sleep ? climate::CLIMATE_PRESET_SLEEP : climate::CLIMATE_PRESET_NONE;
    this->fan_mode = (state.fan_speed == AirConditionerState::FanSpeed::Low) ? climate::CLIMATE_FAN_LOW : climate::CLIMATE_FAN_HIGH;
    publish_state();
}

bool JhsAirConditioner::validate_state_packet_checksum(const BinaryOutputStream &packet, uint32_t checksum)
{
    uint32_t sum = 0;
    BinaryInputStream stream(packet.get_buffer_addr(), packet.get_length());

    stream.skip_bytes(1);
    for (int32_t i = 0; i < PACKET_AC_STATE_CHECKSUM_LEN; i++) {
        sum += stream.read<uint8_t>().value();
    }
    return (sum % 256) == checksum;
}

void JhsAirConditioner::parse_ac_state(AirConditionerState &state, BinaryOutputStream &packet, uint32_t &checksum)
{
    BinaryInputStream stream(packet.get_buffer_addr(), packet.get_length());

    stream.skip_bytes(3); // skip packet start marker and 2 blank bytes
    state.power = stream.read<uint8_t>().value();
    state.mode = stream.read<AirConditionerState::Mode>().value();
    state.sleep = stream.read<uint8_t>().value();
    state.temperature_ambient = stream.read<uint8_t>().value();
    state.temperature_setting = stream.read<uint8_t>().value();
    stream.skip_bytes(1);
    state.fan_speed = stream.read<AirConditionerState::FanSpeed>().value();
    stream.skip_bytes(4);
    state.temperature_unit = stream.read<AirConditionerState::TemperatureUnit>().value();
    state.water_tank_state = stream.read<AirConditionerState::WaterTankState>().value();
    checksum = stream.read<uint8_t>().value();
    stream.skip_bytes(1); // skip packet end marker
}

void JhsAirConditioner::send_packet_to_ac(const BinaryOutputStream &packet)
{
    write_array(packet.get_buffer_addr(), packet.get_length());
    dump_packet("Sent packet", packet);
}

} // namespace jhs_ac
} // namespace esphome
