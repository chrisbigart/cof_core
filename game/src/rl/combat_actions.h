#pragma once

#include "rl/combat_constants.h"

#include "core/spell.h"

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace rl {
namespace combat {

enum class combat_action_type_t : uint8_t {
        WAIT = 0,
        DEFEND,
        AUTO_RESOLVE,
        CAST_BLESS,
        CAST_CURSE,
        CAST_HASTE,
        CAST_SLOW,
        CAST_SHIELD,
        CAST_LIGHTNING_BOLT,
        MOVE_TO_HEX,
        ATTACK_STACK,
        COUNT,
};

enum class spell_target_type_t { NONE, FRIENDLY, ENEMY };

struct combat_action_spec_t {
        combat_action_type_t type = combat_action_type_t::WAIT;
        int target_x = -1;
        int target_y = -1;
        int target_stack_index = -1;
};

struct spell_action_definition_t {
        combat_action_type_t action;
        spell_e spell;
        spell_target_type_t target;
};

constexpr std::array<spell_action_definition_t, 6> kSpellActions = {{
        {combat_action_type_t::CAST_BLESS, SPELL_BLESS, spell_target_type_t::FRIENDLY},
        {combat_action_type_t::CAST_CURSE, SPELL_CURSE, spell_target_type_t::ENEMY},
        {combat_action_type_t::CAST_HASTE, SPELL_HASTE, spell_target_type_t::FRIENDLY},
        {combat_action_type_t::CAST_SLOW, SPELL_SLOW, spell_target_type_t::ENEMY},
        {combat_action_type_t::CAST_SHIELD, SPELL_SHIELD, spell_target_type_t::FRIENDLY},
        {combat_action_type_t::CAST_LIGHTNING_BOLT, SPELL_LIGHTNING_BOLT, spell_target_type_t::ENEMY},
}};

constexpr std::size_t SPELL_ACTION_COUNT = kSpellActions.size();
constexpr std::size_t BASE_ACTION_COUNT = static_cast<std::size_t>(combat_action_type_t::MOVE_TO_HEX);
constexpr std::size_t HEX_ACTION_COUNT = BATTLEFIELD_HEX_COUNT;
constexpr std::size_t ATTACK_ACTION_COUNT = MAX_ARMY_TROOPS;
constexpr std::size_t MOVEMENT_ACTION_OFFSET = BASE_ACTION_COUNT;
constexpr std::size_t ATTACK_ACTION_OFFSET = MOVEMENT_ACTION_OFFSET + HEX_ACTION_COUNT;
constexpr std::size_t ACTION_COUNT = BASE_ACTION_COUNT + HEX_ACTION_COUNT + ATTACK_ACTION_COUNT;

using action_mask_t = std::vector<uint8_t>;

inline combat_action_spec_t decode_action_index(std::size_t action_index) {
        combat_action_spec_t spec;
        if(action_index < BASE_ACTION_COUNT) {
                spec.type = static_cast<combat_action_type_t>(action_index);
                return spec;
        }

        action_index -= BASE_ACTION_COUNT;
        if(action_index < HEX_ACTION_COUNT) {
                spec.type = combat_action_type_t::MOVE_TO_HEX;
                spec.target_x = static_cast<int>(action_index % BATTLEFIELD_WIDTH);
                spec.target_y = static_cast<int>(action_index / BATTLEFIELD_WIDTH);
                return spec;
        }

        action_index -= HEX_ACTION_COUNT;
        if(action_index < ATTACK_ACTION_COUNT) {
                spec.type = combat_action_type_t::ATTACK_STACK;
                spec.target_stack_index = static_cast<int>(action_index);
                return spec;
        }

        spec.type = combat_action_type_t::AUTO_RESOLVE;
        return spec;
}

inline bool is_spell_action(combat_action_type_t type) {
        for(const auto& action : kSpellActions) {
                if(action.action == type)
                        return true;
        }
        return false;
}

inline std::optional<spell_action_definition_t> lookup_spell_action(combat_action_type_t type) {
        for(const auto& action : kSpellActions) {
                if(action.action == type)
                        return action;
        }
        return std::nullopt;
}

} // namespace combat
} // namespace rl

