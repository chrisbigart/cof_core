#pragma once

#include "rl/battle_sim.h"
#include "rl/combat_actions.h"

#include "core/hero.h"
#include "core/troop.h"

#include <array>
#include <string>
#include <vector>

namespace rl {
namespace combat {

struct troop_stack_spec_t {
        unit_type_e unit_type = UNIT_UNKNOWN;
        uint16_t stack_size = 0;
};

struct hero_loadout_spec_t {
        player_e player = PLAYER_1;
        hero_class_e hero_class = HERO_KNIGHT;
        uint16_t hero_id = 0;
        std::string name;
        uint8_t level = 1;
        uint64_t experience = 0;
        uint8_t attack = 0;
        uint8_t defense = 0;
        uint8_t power = 1;
        uint8_t knowledge = 1;
        uint16_t mana = 10;
        uint16_t dark_energy = 0;
        bool has_ballista = false;
        bool human_controlled = true;
        std::array<troop_stack_spec_t, game_config::HERO_TROOP_SLOTS> troops = {};
        std::vector<talent_e> talents;
        std::vector<spell_e> spellbook;
        std::vector<skill_e> enabled_skills;
};

struct combat_scenario_spec_t {
        hero_loadout_spec_t attacker;
        hero_loadout_spec_t defender;
        bool is_deathmatch = false;
        battlefield_environment_e environment = BATTLEFIELD_ENVIRONMENT_GRASS;
        bool quick_combat = false;
};

class combat_session_t {
public:
        explicit combat_session_t(game_t& game_instance);

        void configure(const combat_scenario_spec_t& spec);
        battle_result_e reset();
        battle_result_e reset(const combat_scenario_spec_t& spec);

        battle_result_e step();
        bool apply_action(const combat_action_spec_t& action);

        action_mask_t legal_actions(bool controls_attacker);

        const combat_scenario_spec_t& scenario() const { return scenario_spec; }

        hero_t& attacker();
        const hero_t& attacker() const;
        hero_t& defender();
        const hero_t& defender() const;

        battlefield_t& battlefield();
        const battlefield_t& battlefield() const;

        game_t& game();
        const game_t& game() const;

private:
        void apply_loadout(const hero_loadout_spec_t& spec, hero_t& hero, bool is_attacker);
        void apply_troops(const hero_loadout_spec_t& spec, hero_t& hero);
        void apply_talents(const hero_loadout_spec_t& spec, hero_t& hero);
        void apply_spells_and_skills(const hero_loadout_spec_t& spec, hero_t& hero);
        void configure_player_control();

        battle_sim_t simulator;
        combat_scenario_spec_t scenario_spec;
        hero_t attacker_hero;
        hero_t defender_hero;
        bool scenario_loaded = false;
};

} // namespace combat
} // namespace rl

