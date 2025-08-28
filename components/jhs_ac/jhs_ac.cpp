#include "jhs_ac.h"
#include "binary_input_stream.h"
#include "power_command.h"
#include "mode_command.h"
#include "fan_speed_command.h"
#include "sleep_command.h"
#include "temperature_command.h"
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

            mode_command.write_to_packet(packet_stream);
            add_packet_to_queue(packet_stream);
        }
    }

    if (fan_mode.has_value())
    {
        FanSpeedCommand fan_speed_command;
        BinaryOutputStream packet_stream(packet_data, sizeof(packet_data));

        if (fan_mode.value() == climate::CLIMATE_FAN_LOW) {
            fan_speed_command.set_speed(AirConditionerState::FanSpeed::Low);
        }
        else {
            fan_speed_command.set_speed(AirConditionerState::FanSpeed::High);
        }
        
        fan_speed_command.write_to_packet(packet_stream);
        add_packet_to_queue(packet_stream);
    }

    if (preset.has_value())
    {
        SleepCommand sleep_command;
        BinaryOutputStream packet_stream(packet_data, sizeof(packet_data));
        sleep_command.toggle(preset.value() == climate::CLIMATE_PRESET_SLEEP ? true : false);
        sleep_command.write_to_packet(packet_stream);
        add_packet_to_queue(packet_stream);
    }

    if (temperature.has_value())
    {
        TemperatureCommand temperature_command;
        BinaryOutputStream packet_stream(packet_data, sizeof(packet_data));
        temperature_command.set_temperature(static_cast<int32_t>(temperature.value()));
        temperature_command.write_to_packet(packet_stream);
        add_packet_to_queue(packet_stream);
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
    ESP_LOGD(TAG, "  Ambient temperature: %u", state.temperature_ambient);
    ESP_LOGD(TAG, "  Temperature setting: %u", state.temperature_setting);
    ESP_LOGD(TAG, "  Fan speed: %s", (state.fan_speed == AirConditionerState::FanSpeed::Low) ? "Low" : "High");
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

} // namespace esphome::jhs_ac
