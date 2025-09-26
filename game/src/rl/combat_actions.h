#pragma once

#include "rl/battle_session.h"

#include "core/spell.h"

#include <array>
#include <optional>

namespace rl {
namespace combat {

enum class spell_target_type_t { NONE, FRIENDLY, ENEMY };

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

