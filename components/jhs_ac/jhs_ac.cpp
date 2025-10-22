#include "jhs_ac.h"
#include "binary_input_stream.h"
#include "power_command.h"
#include "mode_command.h"
#include "fan_speed_command.h"
#include "sleep_command.h"
#include "temperature_command.h"
#include "oscillation_command.h"
#include "esphome/core/macros.h"
#include "esphome/core/application.h"
#include <cmath>
#include <cstring>
#include <algorithm>

#define VERBOSE_LOGGING 1

namespace esphome::jhs_ac {

void JhsAirConditioner::setup()
{
    flush();
    m_traits.set_visual_min_temperature(MIN_VALID_TEMPERATURE);
    m_traits.set_visual_max_temperature(MAX_VALID_TEMPERATURE);
    m_traits.set_visual_temperature_step(TEMPERATURE_STEP);
    m_traits.set_supports_current_temperature(true);
    m_traits.set_supports_two_point_target_temperature(false);

    if (!m_supported_swing_modes.empty()) {
        m_supported_swing_modes.insert(climate::CLIMATE_SWING_OFF);
        m_traits.set_supported_swing_modes(m_supported_swing_modes);
    }
    
    m_supported_modes.insert(climate::CLIMATE_MODE_OFF);
    m_traits.set_supported_modes(m_supported_modes);

    // Fan modes will be set via add_supported_fan_mode() calls from configuration
    m_traits.set_supported_fan_modes(m_supported_fan_modes);
    
    m_traits.set_supported_presets({climate::CLIMATE_PRESET_NONE, 
                                    climate::CLIMATE_PRESET_SLEEP});
}

void JhsAirConditioner::loop()
{
    read_uart_data();
    parse_received_data();
    send_queued_command();
}

void JhsAirConditioner::dump_config()
{
    ESP_LOGCONFIG(TAG, "JHS Air Conditioner Component:");
    ESP_LOGCONFIG(TAG, "Protocol version: %d", JHS_AC_PROTOCOL_VERSION);
    this->dump_traits_(TAG);
    this->check_uart_settings(9600, 1, uart::UART_CONFIG_PARITY_NONE, 8);
}

void JhsAirConditioner::control(const climate::ClimateCall &call)
{
    uint8_t packet_data[64];
    auto mode = call.get_mode();
    auto fan_mode = call.get_fan_mode();
    auto preset = call.get_preset();
    auto temperature = call.get_target_temperature();
    auto swing_mode = call.get_swing_mode();
    
    if (mode.has_value())
    {
        bool waking_up_ac = !m_state.power && mode.value() != climate::CLIMATE_MODE_OFF;
        bool turning_off_ac = mode.value() == climate::CLIMATE_MODE_OFF;

        // turn on AC before changing mode to something else
        if (waking_up_ac || turning_off_ac)
        {
            PowerCommand power_command;
            BinaryOutputStream packet_stream(packet_data, sizeof(packet_data));
            power_command.toggle(waking_up_ac ? true : false);
            power_command.write_to_packet(packet_stream);
            add_packet_to_queue(packet_stream);  
        }

        if (mode.value() != climate::CLIMATE_MODE_OFF)
        {
            ModeCommand mode_command;
            BinaryOutputStream packet_stream(packet_data, sizeof(packet_data));
            auto desired_mode = get_mapped_ac_mode(mode.value());

            if (desired_mode.has_value() && m_supported_modes.count(mode.value())) 
            {
                if (m_state.mode != desired_mode.value())
                {
                    mode_command.select(desired_mode.value());
                    mode_command.write_to_packet(packet_stream);
                    add_packet_to_queue(packet_stream);
                }
            }
            else {
                ESP_LOGW(TAG, "Unsupported AC mode was requested, ignoring");
            }
        }
    }

    if (fan_mode.has_value())
    {
        FanSpeedCommand fan_speed_command;
        BinaryOutputStream packet_stream(packet_data, sizeof(packet_data));
        auto desired_fan_speed = get_mapped_fan_speed(fan_mode.value());

        if (desired_fan_speed.has_value() && m_supported_fan_modes.count(fan_mode.value()))
        {
            if (m_state.fan_speed != desired_fan_speed.value())
            {
                fan_speed_command.set_speed(desired_fan_speed.value());
                fan_speed_command.write_to_packet(packet_stream);
                add_packet_to_queue(packet_stream);
            }
        }
        else {
            ESP_LOGW(TAG, "Unsupported fan speed mode was requested, ignoring");
        }
    }

    if (preset.has_value())
    {
        SleepCommand sleep_command;
        BinaryOutputStream packet_stream(packet_data, sizeof(packet_data));

        if (preset.value() == climate::CLIMATE_PRESET_SLEEP || preset.value() == climate::CLIMATE_PRESET_NONE)
        {
            const bool desired_sleep_mode = preset.value() == climate::CLIMATE_PRESET_SLEEP;
            if (m_state.sleep != desired_sleep_mode)
            {
                sleep_command.toggle(desired_sleep_mode);
                sleep_command.write_to_packet(packet_stream);
                add_packet_to_queue(packet_stream);
            }
        }
        else {
            ESP_LOGW(TAG, "Unsupported preset was requested, ignoring");
        }
    }

    if (temperature.has_value())
    {
        TemperatureCommand temperature_command;
        BinaryOutputStream packet_stream(packet_data, sizeof(packet_data));

        if (m_state.temperature_setting != temperature.value())
        {
            temperature_command.set_temperature(static_cast<int32_t>(temperature.value()));
            temperature_command.write_to_packet(packet_stream);
            add_packet_to_queue(packet_stream);
        }
    }

    if (swing_mode.has_value())
    {
            OscillationCommand oscillation_command;
            BinaryOutputStream packet_stream(packet_data, sizeof(packet_data));
            const bool desired_swing_mode = swing_mode.value() == climate::CLIMATE_SWING_VERTICAL;

        if (m_supported_swing_modes.count(swing_mode.value())) 
        {
            if (m_state.oscillation != desired_swing_mode)
            {
            oscillation_command.set_status(desired_swing_mode);
            oscillation_command.write_to_packet(packet_stream);
            add_packet_to_queue(packet_stream);
            }
        }
        else {
            ESP_LOGW(TAG, "Unsupported swing mode was requested, ignoring");
        }
    }
}

float JhsAirConditioner::get_setup_priority() const
{
    return setup_priority::AFTER_WIFI;
}

void JhsAirConditioner::set_water_tank_sensor(binary_sensor::BinarySensor *sensor)
{
    m_water_tank_sensor = sensor;
}

climate::ClimateTraits JhsAirConditioner::traits()
{
    return m_traits;
}

void JhsAirConditioner::read_uart_data()
{
    uint32_t bytes_available = static_cast<uint32_t>(available());
    if (bytes_available > 0)
    {
        const uint32_t data_size = std::min(bytes_available, m_data_buffer.capacity() - m_data_buffer.size());
        for (int32_t i = 0; i < data_size; i++) {
            m_data_buffer.push(read()); // TODO replace it with single call
        }
    }
}

void JhsAirConditioner::parse_received_data()
{
    while (!m_data_buffer.is_empty())
    {
        m_parser.process_byte(m_data_buffer.pop().value());
        if (m_parser.packet_ready())
        {
            uint32_t checksum = 0;
            uint8_t packet_buffer[64];
            const uint32_t packet_length = m_parser.read_packet(packet_buffer, sizeof(packet_buffer));
            BinaryInputStream state_packet(packet_buffer, packet_length);

            m_state.read_from_packet(state_packet, checksum);
            dump_packet("Received packet", state_packet.get_buffer_addr(), state_packet.get_size());

            if (validate_state_packet_checksum(state_packet, checksum)) 
            {
                dump_ac_state(m_state);
                update_ac_state(m_state);
            }
            else {
                ESP_LOGW(TAG, "Invalid AC state packet checksum, ignoring");
            }
        }
    }
}

void JhsAirConditioner::send_queued_command()
{
    if (!m_tx_queue.is_empty())
    {
        const uint32_t current_time = App.get_loop_component_start_time();
        if (current_time - m_last_command_send_time > TX_QUEUE_PACKETS_INTERVAL_MS)
        {
            auto command_packet = m_tx_queue.pop();
            send_packet_to_ac(command_packet->data, command_packet->length);
            m_last_command_send_time = current_time;
        }
    }
}

void JhsAirConditioner::add_packet_to_queue(const BinaryOutputStream &packet)
{
    if (m_tx_queue.is_full()) {
        ESP_LOGE(TAG, "Command TX queue overflowed, last command ignored");
    }
    else
    {
        CommandPacket command_packet;
        constexpr uint32_t max_packet_size = sizeof(command_packet.data);
        if (packet.get_length() <= max_packet_size)
        {
            command_packet.length = packet.get_length();
            std::memcpy(command_packet.data, packet.get_buffer_addr(), packet.get_length());
            m_tx_queue.push(command_packet);
        }
        else {
            ESP_LOGE(TAG, "Trying to send command packet larger than %d bytes, ignoring", max_packet_size);
        }
    }
}

void JhsAirConditioner::send_packet_to_ac(const uint8_t *data, uint32_t length)
{
    write_array(data, length);
    dump_packet("Sent packet", data, length);
}

void JhsAirConditioner::dump_packet(const char *title, const uint8_t *data, uint32_t length)
{
#if VERBOSE_LOGGING == 1
    char str[256] = {0};
    char *pstr = str;
    ESP_LOGD(TAG, "%s (%u bytes):", title, length);
    for (int32_t i = 0; i < length; i++) {
        pstr += sprintf(pstr, "%02X ", data[i]);
    }
    ESP_LOGD(TAG, "%s", str);
#endif
}

void JhsAirConditioner::dump_ac_state(const AirConditionerState &state)
{
    ESP_LOGD(TAG, "Air conditioner state:");
    ESP_LOGD(TAG, "  Power: %s", state.power ? "On" : "Off");
    ESP_LOGD(TAG, "  Mode: %s", AirConditionerState::get_mode_name(state.mode));
    ESP_LOGD(TAG, "  Sleep mode: %s", state.sleep ? "On" : "Off");
    ESP_LOGD(TAG, "  Oscillation: %s", state.oscillation ? "On" : "Off");
    ESP_LOGD(TAG, "  Ambient temperature: %u", state.temperature_ambient);
    ESP_LOGD(TAG, "  Temperature setting: %u", state.temperature_setting);
    ESP_LOGD(TAG, "  Fan speed: %s", get_fan_speed_name(state.fan_speed));
    ESP_LOGD(TAG, "  Temperature units: %s", (state.temperature_unit == AirConditionerState::TemperatureUnit::Celsius) ? "Celsius" : "Fahrenheit");
    ESP_LOGD(TAG, "  Water tank state: %s", (state.water_tank_state == AirConditionerState::WaterTankState::Empty) ? "Empty" : "Full");
#if VERBOSE_LOGGING == 1
    ESP_LOGD(TAG, "  Byte [08]: 0x%02X", state.byte_08);
    ESP_LOGD(TAG, "  Byte [0A]: 0x%02X", state.byte_0A);
    ESP_LOGD(TAG, "  Byte [0B]: 0x%02X", state.byte_0B);
    ESP_LOGD(TAG, "  Byte [0C]: 0x%02X", state.byte_0C);
    ESP_LOGD(TAG, "  Byte [0D]: 0x%02X", state.byte_0D);
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
        case AirConditionerState::Mode::Heat:
            this->mode = climate::CLIMATE_MODE_HEAT;
            break;
        default:
            ESP_LOGW(TAG, "Unknown AC mode, state update was interrupted");
            return;
        }
    }

    this->target_temperature = state.temperature_setting;
    this->current_temperature = state.temperature_ambient;
    this->preset = state.sleep ? climate::CLIMATE_PRESET_SLEEP : climate::CLIMATE_PRESET_NONE;
    this->swing_mode = state.oscillation ? climate::CLIMATE_SWING_VERTICAL : climate::CLIMATE_SWING_OFF;
    
    // Map AC fan speed to climate fan mode using supported modes
    auto mapped_fan_mode = get_mapped_climate_fan_mode(state.fan_speed);
    if (mapped_fan_mode.has_value()) {
        this->fan_mode = mapped_fan_mode.value();
    }
    
    publish_state();

    if (m_water_tank_sensor) {
        m_water_tank_sensor->publish_state(state.water_tank_state == AirConditionerState::WaterTankState::Full);
    }
}

bool JhsAirConditioner::validate_state_packet_checksum(const BinaryInputStream &stream, uint32_t checksum)
{
    uint32_t sum = 0;
    BinaryInputStream packet(stream.get_buffer_addr(), stream.get_size());
    packet.skip_bytes(1);
    for (int32_t i = 0; i < PACKET_AC_STATE_CHECKSUM_LEN; i++) {
        sum += packet.read<uint8_t>().value();
    }
    return (sum % 256) == checksum;
}

std::optional<AirConditionerState::Mode> JhsAirConditioner::get_mapped_ac_mode(climate::ClimateMode climate_mode) const
{
    switch (climate_mode)
    {
        case climate::CLIMATE_MODE_COOL: return AirConditionerState::Mode::Cool;
        case climate::CLIMATE_MODE_DRY: return AirConditionerState::Mode::Dehumidifying;
        case climate::CLIMATE_MODE_FAN_ONLY: return AirConditionerState::Mode::Fan;
        case climate::CLIMATE_MODE_HEAT: return AirConditionerState::Mode::Heat;
        default: return std::nullopt;
    }
}

void JhsAirConditioner::add_supported_mode(climate::ClimateMode mode)
{
    m_supported_modes.insert(mode);
}

void JhsAirConditioner::add_supported_fan_mode(climate::ClimateFanMode fan_mode)
{
    m_supported_fan_modes.insert(fan_mode);
    m_traits.set_supported_fan_modes(m_supported_fan_modes);
}

void JhsAirConditioner::add_supported_swing_mode(climate::ClimateSwingMode swing_mode)
{
    m_supported_swing_modes.insert(swing_mode);
}

std::optional<climate::ClimateFanMode> JhsAirConditioner::get_mapped_climate_fan_mode(AirConditionerState::FanSpeed fan_speed) const
{
    switch (fan_speed)
    {
        case AirConditionerState::FanSpeed::Low:
            if (m_supported_fan_modes.count(climate::CLIMATE_FAN_LOW))
                return climate::CLIMATE_FAN_LOW;
            break;
        case AirConditionerState::FanSpeed::Medium:
            if (m_supported_fan_modes.count(climate::CLIMATE_FAN_MEDIUM))
                return climate::CLIMATE_FAN_MEDIUM;
            break;
        case AirConditionerState::FanSpeed::High:
            if (m_supported_fan_modes.count(climate::CLIMATE_FAN_HIGH))
                return climate::CLIMATE_FAN_HIGH;
            break;
    }
    
    // Fallback to first supported mode if exact match not found
    if (!m_supported_fan_modes.empty()) {
        return *m_supported_fan_modes.begin();
    }
    
    return std::nullopt;
}

const char* JhsAirConditioner::get_fan_speed_name(AirConditionerState::FanSpeed fan_speed) const
{
    switch (fan_speed)
    {
        case AirConditionerState::FanSpeed::Low: return "Low";
        case AirConditionerState::FanSpeed::Medium: return "Medium";
        case AirConditionerState::FanSpeed::High: return "High";
        default: return "Unknown";
    }
}

std::optional<AirConditionerState::FanSpeed> JhsAirConditioner::get_mapped_fan_speed(climate::ClimateFanMode fan_mode) const
{
    switch (fan_mode)
    {
        case climate::CLIMATE_FAN_LOW: return AirConditionerState::FanSpeed::Low;
        case climate::CLIMATE_FAN_MEDIUM: return AirConditionerState::FanSpeed::Medium;
        case climate::CLIMATE_FAN_HIGH: return AirConditionerState::FanSpeed::High;
        default: return std::nullopt; 
    }
}

} // namespace esphome::jhs_ac
