#pragma once

#include "rl/battle_session.h"
#include "rl/combat_actions.h"

#include <array>
#include <cstddef>

#ifdef slots
#undef slots
#endif
#ifdef signals
#undef signals
#endif

#include <torch/torch.h>

namespace rl {
namespace combat {

constexpr std::size_t MAX_ARMY_TROOPS = 16;
constexpr std::size_t STACK_FEATURES = 15;
constexpr std::size_t GLOBAL_FEATURES = 18 + (2 * SPELL_ACTION_COUNT);

struct stack_observation_t {
        uint16_t unit_type = 0;
        uint16_t stack_size = 0;
        uint16_t unit_health = 0;
        uint16_t original_stack_size = 0;
        int8_t retaliations_remaining = 0;
        int8_t x = -1;
        int8_t y = -1;
        bool is_attacker = false;
        bool is_alive = false;
        bool has_waited = false;
        bool has_moved = false;
        bool has_defended = false;
        bool has_cast_spell = false;
        bool has_moraled = false;
        bool is_disabled = false;
};

struct combat_observation_t {
        uint16_t round = 0;
        bool attacker_moved_last = false;
        bool is_siege = false;
        bool is_quick_combat = false;
        int8_t active_unit_id = -1;
        bool active_unit_is_attacker = false;
        int8_t active_unit_x = -1;
        int8_t active_unit_y = -1;
        uint16_t attacker_mana = 0;
        uint16_t defender_mana = 0;
        int16_t attacker_morale = 0;
        int16_t defender_morale = 0;
        int16_t attacker_luck = 0;
        int16_t defender_luck = 0;
        uint8_t attacker_stack_count = 0;
        uint8_t defender_stack_count = 0;
        bool attacker_can_cast_spell = false;
        bool defender_can_cast_spell = false;
        std::array<uint8_t, SPELL_ACTION_COUNT> attacker_spellbook{};
        std::array<uint8_t, SPELL_ACTION_COUNT> defender_spellbook{};
        std::array<stack_observation_t, MAX_ARMY_TROOPS> attacker_stacks{};
        std::array<stack_observation_t, MAX_ARMY_TROOPS> defender_stacks{};
};

combat_observation_t capture_observation(const combat_session_t& session);

std::size_t observation_feature_count();

torch::Tensor observation_to_tensor(const combat_observation_t& observation, torch::Device device = torch::kCPU);

torch::Tensor stack_to_tensor(const stack_observation_t& stack, torch::Device device = torch::kCPU);

} // namespace combat
} // namespace rl

