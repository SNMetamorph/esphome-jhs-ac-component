#include "ac_state.h"

namespace esphome {
namespace jhs_ac {

const char *AirConditionerState::get_mode_name(Mode mode)
{
    switch (mode)
    {
        case Mode::Cool: return "Cool";
        case Mode::Dehumidifying: return "Dehumidifying";
        case Mode::Fan: return "Fan";
        default: return "Unknown";
    }
}

} // namespace jhs_ac
} // namespace esphome
