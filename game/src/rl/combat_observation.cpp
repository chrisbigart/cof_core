#include "combat_observation.h"

#include "core/battlefield.h"
#include "core/game_config.h"

#include <algorithm>
#include <limits>
#include <vector>

stack_observation_t encode_stack(const battlefield_unit_t& unit) {
        stack_observation_t output;
        output.unit_type = static_cast<uint16_t>(unit.unit_type);
        output.stack_size = unit.stack_size;
        output.unit_health = unit.unit_health;
        output.original_stack_size = unit.original_stack_size;
        output.retaliations_remaining = unit.retaliations_remaining;
        output.x = unit.x;
        output.y = unit.y;
        output.is_attacker = unit.is_attacker;
        const bool alive = unit.unit_type != UNIT_UNKNOWN && unit.stack_size > 0 && unit.unit_health > 0;
        output.is_alive = alive;
        output.has_waited = unit.has_waited;
        output.has_moved = unit.has_moved;
        output.has_defended = unit.has_defended;
        output.has_cast_spell = unit.has_cast_spell;
        output.has_moraled = unit.has_moraled;
        output.is_disabled = unit.is_disabled();
        return output;
}

void append_stack_features(std::vector<float>& buffer, const stack_observation_t& stack) {
        buffer.push_back(static_cast<float>(stack.unit_type));
        buffer.push_back(static_cast<float>(stack.stack_size));
        buffer.push_back(static_cast<float>(stack.unit_health));
        buffer.push_back(static_cast<float>(stack.original_stack_size));
        buffer.push_back(static_cast<float>(stack.retaliations_remaining));
        buffer.push_back(static_cast<float>(stack.x));
        buffer.push_back(static_cast<float>(stack.y));
        buffer.push_back(stack.is_attacker ? 1.0F : 0.0F);
        buffer.push_back(stack.is_alive ? 1.0F : 0.0F);
        buffer.push_back(stack.has_waited ? 1.0F : 0.0F);
        buffer.push_back(stack.has_moved ? 1.0F : 0.0F);
        buffer.push_back(stack.has_defended ? 1.0F : 0.0F);
        buffer.push_back(stack.has_cast_spell ? 1.0F : 0.0F);
        buffer.push_back(stack.has_moraled ? 1.0F : 0.0F);
        buffer.push_back(stack.is_disabled ? 1.0F : 0.0F);
}

combat_observation_t capture_observation(const combat_session_t& session) {
        combat_observation_t output;
        battlefield_t& battlefield = (battlefield_t&)(session.simulator.battlefield());

        output.round = static_cast<uint16_t>(std::min(
                battlefield.round,
                static_cast<uint>(std::numeric_limits<uint16_t>::max())));
        output.attacker_moved_last = battlefield.attacker_moved_last;
        output.is_siege = battlefield.is_siege();
        output.is_quick_combat = battlefield.is_quick_combat;

        if(auto* active_unit = battlefield.get_active_unit()) {
                output.active_unit_id = active_unit->troop_id;
                output.active_unit_is_attacker = active_unit->is_attacker;
                output.active_unit_x = active_unit->x;
                output.active_unit_y = active_unit->y;
        } else {
                output.active_unit_id = -1;
                output.active_unit_is_attacker = false;
                output.active_unit_x = -1;
                output.active_unit_y = -1;
        }

        const auto& attacker = session.attacker_hero;
        const auto& defender = session.defender_hero;
        output.attacker_mana = attacker.mana;
        output.defender_mana = defender.mana;
        output.attacker_morale = static_cast<int16_t>(attacker.get_morale());
        output.defender_morale = static_cast<int16_t>(defender.get_morale());
        output.attacker_luck = static_cast<int16_t>(attacker.get_luck());
        output.defender_luck = static_cast<int16_t>(defender.get_luck());

        output.attacker_stacks.fill(stack_observation_t());
        output.defender_stacks.fill(stack_observation_t());

        std::size_t attacker_count = 0;
        for(const auto& unit : battlefield.attacking_army.troops) {
                if(unit.unit_type == UNIT_UNKNOWN || unit.stack_size == 0)
                        continue;
                if(attacker_count >= MAX_ARMY_TROOPS)
                        break;
                output.attacker_stacks[attacker_count++] = encode_stack(unit);
        }
        output.attacker_stack_count = static_cast<uint8_t>(attacker_count);

        std::size_t defender_count = 0;
        for(const auto& unit : battlefield.defending_army.troops) {
                if(unit.unit_type == UNIT_UNKNOWN || unit.stack_size == 0)
                        continue;
                if(defender_count >= MAX_ARMY_TROOPS)
                        break;
                output.defender_stacks[defender_count++] = encode_stack(unit);
        }
        output.defender_stack_count = static_cast<uint8_t>(defender_count);

        return output;
}

std::size_t observation_feature_count() {
        return GLOBAL_FEATURES + (2 * MAX_ARMY_TROOPS * STACK_FEATURES);
}

torch::Tensor stack_to_tensor(const stack_observation_t& stack, torch::Device device) {
        std::vector<float> features;
        features.reserve(STACK_FEATURES);
        append_stack_features(features, stack);
        return torch::tensor(features, torch::TensorOptions().dtype(torch::kFloat32).device(device));
}

torch::Tensor observation_to_tensor(const combat_observation_t& observation, torch::Device device) {
        std::vector<float> features;
        features.reserve(observation_feature_count());

        features.push_back(static_cast<float>(observation.round));
        features.push_back(observation.attacker_moved_last ? 1.0F : 0.0F);
        features.push_back(observation.is_siege ? 1.0F : 0.0F);
        features.push_back(observation.is_quick_combat ? 1.0F : 0.0F);
        features.push_back(static_cast<float>(observation.active_unit_id));
        features.push_back(observation.active_unit_is_attacker ? 1.0F : 0.0F);
        features.push_back(static_cast<float>(observation.active_unit_x));
        features.push_back(static_cast<float>(observation.active_unit_y));
        features.push_back(static_cast<float>(observation.attacker_mana));
        features.push_back(static_cast<float>(observation.defender_mana));
        features.push_back(static_cast<float>(observation.attacker_morale));
        features.push_back(static_cast<float>(observation.defender_morale));
        features.push_back(static_cast<float>(observation.attacker_luck));
        features.push_back(static_cast<float>(observation.defender_luck));
        features.push_back(static_cast<float>(observation.attacker_stack_count));
        features.push_back(static_cast<float>(observation.defender_stack_count));

        for(const auto& stack : observation.attacker_stacks)
                append_stack_features(features, stack);
        for(const auto& stack : observation.defender_stacks)
                append_stack_features(features, stack);

        return torch::tensor(features, torch::TensorOptions().dtype(torch::kFloat32).device(device));
}

