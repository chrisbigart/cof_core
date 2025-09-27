#pragma once

#include "core/game_config.h"

#include <cstddef>

namespace rl {
namespace combat {

constexpr std::size_t MAX_ARMY_TROOPS = 16;
constexpr std::size_t BATTLEFIELD_WIDTH = static_cast<std::size_t>(game_config::BATTLEFIELD_WIDTH);
constexpr std::size_t BATTLEFIELD_HEIGHT = static_cast<std::size_t>(game_config::BATTLEFIELD_HEIGHT);
constexpr std::size_t BATTLEFIELD_HEX_COUNT = BATTLEFIELD_WIDTH * BATTLEFIELD_HEIGHT;

} // namespace combat
} // namespace rl

