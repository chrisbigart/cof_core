#include "rl/battle_python_bindings.h"

#include "rl/battle_session.h"

#include "core/battlefield.h"
#include "core/game_config.h"

#include <algorithm>
#include <cstring>
#include <limits>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

namespace rl {
namespace combat {

rl_combat_stack_observation_c encode_stack_observation(const battlefield_unit_t& unit) {
        rl_combat_stack_observation_c output{};
        output.unit_type = static_cast<uint16_t>(unit.unit_type);
        output.stack_size = unit.stack_size;
        output.unit_health = unit.unit_health;
        output.original_stack_size = unit.original_stack_size;
        output.retaliations_remaining = unit.retaliations_remaining;
        output.x = unit.x;
        output.y = unit.y;
        output.is_attacker = unit.is_attacker ? 1 : 0;
        const bool alive = unit.unit_type != UNIT_UNKNOWN && unit.stack_size > 0 && unit.unit_health > 0;
        output.is_alive = alive ? 1 : 0;
        output.has_waited = unit.has_waited ? 1 : 0;
        output.has_moved = unit.has_moved ? 1 : 0;
        output.has_defended = unit.has_defended ? 1 : 0;
        output.has_cast_spell = unit.has_cast_spell ? 1 : 0;
        output.has_moraled = unit.has_moraled ? 1 : 0;
        output.is_disabled = unit.is_disabled() ? 1 : 0;
        return output;
}

hero_loadout_spec_t convert_loadout(const rl_combat_hero_loadout_c& input) {
        hero_loadout_spec_t output;
        output.player = static_cast<player_e>(input.player);
        output.hero_class = static_cast<hero_class_e>(input.hero_class);
        output.hero_id = input.hero_id;
        if(input.name)
                output.name = input.name;
        output.level = input.level;
        output.experience = input.experience;
        output.attack = input.attack;
        output.defense = input.defense;
        output.power = input.power;
        output.knowledge = input.knowledge;
        output.mana = input.mana;
        output.dark_energy = input.dark_energy;
        output.has_ballista = input.has_ballista != 0;
        output.human_controlled = input.human_controlled != 0;

        for(auto& slot : output.troops)
                slot = troop_stack_spec_t();

        const size_t troop_limit = std::min(input.troop_count, output.troops.size());
        if(input.troops) {
                for(size_t i = 0; i < troop_limit; ++i) {
                        const auto& troop = input.troops[i];
                        output.troops[i].unit_type = static_cast<unit_type_e>(troop.unit_type);
                        output.troops[i].stack_size = troop.stack_size;
                }
        }

        output.talents.clear();
        if(input.talents && input.talent_count > 0) {
                const size_t capped = std::min(input.talent_count, static_cast<size_t>(game_config::HERO_TALENT_POINTS));
                output.talents.reserve(capped);
                for(size_t i = 0; i < capped; ++i)
                        output.talents.push_back(static_cast<talent_e>(input.talents[i]));
        }

        output.spellbook.clear();
        if(input.spells && input.spell_count > 0) {
                output.spellbook.reserve(input.spell_count);
                for(size_t i = 0; i < input.spell_count; ++i)
                        output.spellbook.push_back(static_cast<spell_e>(input.spells[i]));
        }

        output.enabled_skills.clear();
        if(input.skills && input.skill_count > 0) {
                output.enabled_skills.reserve(input.skill_count);
                for(size_t i = 0; i < input.skill_count; ++i)
                        output.enabled_skills.push_back(static_cast<skill_e>(input.skills[i]));
        }

        return output;
}

battlefield_environment_e clamp_environment(uint8_t environment) {
        switch(environment) {
        case BATTLEFIELD_ENVIRONMENT_RANDOM:
        case BATTLEFIELD_ENVIRONMENT_WATER:
        case BATTLEFIELD_ENVIRONMENT_GRASS:
        case BATTLEFIELD_ENVIRONMENT_DIRT:
        case BATTLEFIELD_ENVIRONMENT_WASTELAND:
        case BATTLEFIELD_ENVIRONMENT_LAVE:
        case BATTLEFIELD_ENVIRONMENT_DESERT:
        case BATTLEFIELD_ENVIRONMENT_BEACH:
        case BATTLEFIELD_ENVIRONMENT_SNOW:
        case BATTLEFIELD_ENVIRONMENT_SWAMP:
        case BATTLEFIELD_ENVIRONMENT_JUNGLE:
        case BATTLEFIELD_ENVIRONMENT_GRAVEYARD:
        case BATTLEFIELD_ENVIRONMENT_PYRAMID:
                return static_cast<battlefield_environment_e>(environment);
        default:
                return BATTLEFIELD_ENVIRONMENT_GRASS;
        }
}

combat_action_spec_t convert_action(const rl_combat_action_c& input) {
        combat_action_spec_t output;
        switch(static_cast<rl_combat_action_type_e>(input.type)) {
        case RL_COMBAT_ACTION_WAIT:
                output.type = combat_action_type_t::WAIT;
                break;
        case RL_COMBAT_ACTION_DEFEND:
                output.type = combat_action_type_t::DEFEND;
                break;
        case RL_COMBAT_ACTION_AUTO_RESOLVE:
        default:
                output.type = combat_action_type_t::AUTO_RESOLVE;
                break;
        }

        return output;
}

} // namespace combat
} // namespace rl

namespace {

struct game_handle_t {
        std::unique_ptr<game_t> game_instance;
};

struct session_handle_t {
        std::unique_ptr<rl::combat::combat_session_t> session;
};

} // namespace

extern "C" {

rl_combat_game_handle* rl_combat_create_game() {
        static std::once_flag config_once;
        std::call_once(config_once, []() {
                game_config::load_game_data();
        });

        auto handle = std::make_unique<game_handle_t>();
        handle->game_instance = std::make_unique<game_t>();
        handle->game_instance->game_configuration.player_count = game_config::MAX_NUMBER_OF_PLAYERS;
        for(size_t i = 0; i < game_config::MAX_NUMBER_OF_PLAYERS; ++i) {
                auto player_id = static_cast<player_e>(i + 1);
                auto& config_entry = handle->game_instance->game_configuration.player_configs[i];
                config_entry.player_number = player_id;
                config_entry.is_human = false;
        }

        return reinterpret_cast<rl_combat_game_handle*>(handle.release());
}

void rl_combat_destroy_game(rl_combat_game_handle* handle) {
        if(!handle)
                return;

        auto owned = std::unique_ptr<game_handle_t>(reinterpret_cast<game_handle_t*>(handle));
        owned.reset();
}

rl_combat_session_handle* rl_combat_create_session(rl_combat_game_handle* game_handle) {
        if(!game_handle)
                return nullptr;

        auto* internal_game = reinterpret_cast<game_handle_t*>(game_handle);
        if(!internal_game->game_instance)
                return nullptr;

        auto handle = std::make_unique<session_handle_t>();
        handle->session = std::make_unique<rl::combat::combat_session_t>(*internal_game->game_instance);
        return reinterpret_cast<rl_combat_session_handle*>(handle.release());
}

void rl_combat_destroy_session(rl_combat_session_handle* session_handle) {
        if(!session_handle)
                return;

        auto owned = std::unique_ptr<session_handle_t>(reinterpret_cast<session_handle_t*>(session_handle));
        owned.reset();
}

int rl_combat_session_configure(rl_combat_session_handle* session_handle, const rl_combat_scenario_c* scenario) {
        if(!session_handle || !scenario)
                return -1;

        auto* internal = reinterpret_cast<session_handle_t*>(session_handle);
        if(!internal->session)
                return -2;

        rl::combat::combat_scenario_spec_t spec;
        spec.attacker = rl::combat::convert_loadout(scenario->attacker);
        spec.defender = rl::combat::convert_loadout(scenario->defender);
        spec.is_deathmatch = scenario->is_deathmatch != 0;
        spec.environment = rl::combat::clamp_environment(scenario->environment);
        spec.quick_combat = scenario->quick_combat != 0;

        internal->session->configure(spec);
        return 0;
}

int rl_combat_session_reset(rl_combat_session_handle* session_handle, int* out_result) {
        if(!session_handle)
                return -1;

        auto* internal = reinterpret_cast<session_handle_t*>(session_handle);
        if(!internal->session)
                return -2;

        auto result = internal->session->reset();
        if(out_result)
                *out_result = static_cast<int>(result);
        return 0;
}

int rl_combat_session_step(rl_combat_session_handle* session_handle, int* out_result) {
        if(!session_handle)
                return -1;

        auto* internal = reinterpret_cast<session_handle_t*>(session_handle);
        if(!internal->session)
                return -2;

        auto result = internal->session->step();
        if(out_result)
                *out_result = static_cast<int>(result);
        return 0;
}

int rl_combat_session_apply_action(rl_combat_session_handle* session_handle, const rl_combat_action_c* action) {
        if(!session_handle || !action)
                return -1;

        auto* internal = reinterpret_cast<session_handle_t*>(session_handle);
        if(!internal->session)
                return -2;

        auto spec = rl::combat::convert_action(*action);
        if(!internal->session->apply_action(spec))
                return -3;

        return 0;
}

int rl_combat_session_observation(const rl_combat_session_handle* session_handle, rl_combat_observation_c* out_observation) {
        if(!session_handle || !out_observation)
                return -1;

        auto* internal = reinterpret_cast<const session_handle_t*>(session_handle);
        if(!internal->session)
                return -2;

        std::memset(out_observation, 0, sizeof(*out_observation));
        out_observation->active_unit_id = -1;
        out_observation->active_unit_x = -1;
        out_observation->active_unit_y = -1;

        const auto& session = *internal->session;
        const auto& battlefield = session.battlefield();

        out_observation->round = static_cast<uint16_t>(std::min(
                battlefield.round,
                static_cast<uint>(std::numeric_limits<uint16_t>::max())));
        out_observation->attacker_moved_last = battlefield.attacker_moved_last ? 1 : 0;
        out_observation->is_siege = battlefield.is_siege() ? 1 : 0;
        out_observation->is_quick_combat = battlefield.is_quick_combat ? 1 : 0;

        if(const auto* active_unit = battlefield.get_active_unit()) {
                out_observation->active_unit_id = active_unit->troop_id;
                out_observation->active_unit_is_attacker = active_unit->is_attacker ? 1 : 0;
                out_observation->active_unit_x = active_unit->x;
                out_observation->active_unit_y = active_unit->y;
        }

        const auto& attacker = session.attacker();
        const auto& defender = session.defender();
        out_observation->attacker_mana = attacker.mana;
        out_observation->defender_mana = defender.mana;
        out_observation->attacker_morale = static_cast<int16_t>(attacker.get_morale());
        out_observation->defender_morale = static_cast<int16_t>(defender.get_morale());
        out_observation->attacker_luck = static_cast<int16_t>(attacker.get_luck());
        out_observation->defender_luck = static_cast<int16_t>(defender.get_luck());

        size_t attacker_count = 0;
        for(const auto& unit : battlefield.attacking_army.troops) {
                if(unit.unit_type == UNIT_UNKNOWN || unit.stack_size == 0)
                        continue;
                if(attacker_count >= RL_COMBAT_MAX_ARMY_TROOPS)
                        break;
                out_observation->attacker_stacks[attacker_count++] = rl::combat::encode_stack_observation(unit);
        }
        out_observation->attacker_stack_count = static_cast<uint8_t>(attacker_count);

        size_t defender_count = 0;
        for(const auto& unit : battlefield.defending_army.troops) {
                if(unit.unit_type == UNIT_UNKNOWN || unit.stack_size == 0)
                        continue;
                if(defender_count >= RL_COMBAT_MAX_ARMY_TROOPS)
                        break;
                out_observation->defender_stacks[defender_count++] = rl::combat::encode_stack_observation(unit);
        }
        out_observation->defender_stack_count = static_cast<uint8_t>(defender_count);

        return 0;
}

const battlefield_t* rl_combat_session_battlefield(const rl_combat_session_handle* session_handle) {
        if(!session_handle)
                return nullptr;

        auto* internal = reinterpret_cast<const session_handle_t*>(session_handle);
        if(!internal->session)
                return nullptr;

        return &internal->session->battlefield();
}

const hero_t* rl_combat_session_attacker(const rl_combat_session_handle* session_handle) {
        if(!session_handle)
                return nullptr;

        auto* internal = reinterpret_cast<const session_handle_t*>(session_handle);
        if(!internal->session)
                return nullptr;

        return &internal->session->attacker();
}

const hero_t* rl_combat_session_defender(const rl_combat_session_handle* session_handle) {
        if(!session_handle)
                return nullptr;

        auto* internal = reinterpret_cast<const session_handle_t*>(session_handle);
        if(!internal->session)
                return nullptr;

        return &internal->session->defender();
}

const game_t* rl_combat_session_game(const rl_combat_session_handle* session_handle) {
        if(!session_handle)
                return nullptr;

        auto* internal = reinterpret_cast<const session_handle_t*>(session_handle);
        if(!internal->session)
                return nullptr;

        return &internal->session->game();
}

} // extern "C"
