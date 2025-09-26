#include "rl/battle_session.h"

#include "rl/combat_actions.h"

#include "core/adventure_map.h"

#include <algorithm>
#include <cassert>

namespace rl {
namespace combat {

namespace {
std::vector<player_e> get_unique_human_players(const combat_scenario_spec_t& spec) {
        std::vector<player_e> humans;
        if(spec.attacker.human_controlled)
                humans.push_back(spec.attacker.player);
        if(spec.defender.human_controlled)
                humans.push_back(spec.defender.player);

        std::sort(humans.begin(), humans.end(), [] (player_e lhs, player_e rhs) {
                return static_cast<uint8_t>(lhs) < static_cast<uint8_t>(rhs);
        });
        humans.erase(std::unique(humans.begin(), humans.end()), humans.end());
        return humans;
}

uint16_t resolve_hero_id(const hero_loadout_spec_t& spec, bool is_attacker) {
        if(spec.hero_id != 0)
                return spec.hero_id;

        return static_cast<uint16_t>(is_attacker ? 1 : 2);
}

std::string resolve_hero_name(const hero_loadout_spec_t& spec, bool is_attacker) {
        if(!spec.name.empty())
                return spec.name;

        return is_attacker ? std::string("RL Attacker") : std::string("RL Defender");
}

} // namespace

combat_session_t::combat_session_t(game_t& game_instance)
        : simulator(game_instance) {
}

void combat_session_t::configure(const combat_scenario_spec_t& spec) {
        scenario_spec = spec;
        apply_loadout(scenario_spec.attacker, attacker_hero, true);
        apply_loadout(scenario_spec.defender, defender_hero, false);
        configure_player_control();
        scenario_loaded = true;
}

battle_result_e combat_session_t::reset() {
        assert(scenario_loaded && "combat_session_t::reset called before configure");

        apply_loadout(scenario_spec.attacker, attacker_hero, true);
        apply_loadout(scenario_spec.defender, defender_hero, false);
        configure_player_control();

        auto& battlefield_instance = simulator.battlefield();
        battlefield_instance.environment_type = scenario_spec.environment;
        battlefield_instance.is_quick_combat = scenario_spec.quick_combat;
        battlefield_instance.init_hero_hero_battle(&attacker_hero, &defender_hero, scenario_spec.is_deathmatch);
        battlefield_instance.start_combat();
        return BATTLE_IN_PROGRESS;
}

battle_result_e combat_session_t::reset(const combat_scenario_spec_t& spec) {
        configure(spec);
        return reset();
}

battle_result_e combat_session_t::step() {
        return simulator.step();
}

namespace {
battlefield_unit_t* select_best_stack(army_t::battlefield_unit_group_t& troops) {
        battlefield_unit_t* best = nullptr;
        for(auto& troop : troops) {
                if(troop.unit_type == UNIT_UNKNOWN || troop.stack_size == 0 || troop.unit_health == 0)
                        continue;
                if(!best || troop.stack_size > best->stack_size)
                        best = &troop;
        }
        return best;
}

bool can_cast_from_side(const battlefield_t& battlefield, bool attacker_turn) {
        if(attacker_turn)
                return battlefield.attacking_hero && !battlefield.attacking_hero_used_cast;
        return battlefield.defending_hero && !battlefield.defending_hero_used_cast;
}

bool cast_spell_action(battlefield_t& battlefield,
                       bool attacker_turn,
                       combat_action_type_t action_type) {
        auto definition = lookup_spell_action(action_type);
        if(!definition)
                return false;

        hero_t* caster = attacker_turn ? battlefield.attacking_hero : battlefield.defending_hero;
        if(!caster || !caster->knows_spell(definition->spell))
                return false;

        if(!can_cast_from_side(battlefield, attacker_turn))
                return false;

        battlefield_unit_t* target = nullptr;
        switch(definition->target) {
        case spell_target_type_t::FRIENDLY:
                target = select_best_stack(attacker_turn ? battlefield.attacking_army.troops : battlefield.defending_army.troops);
                break;
        case spell_target_type_t::ENEMY:
                target = select_best_stack(attacker_turn ? battlefield.defending_army.troops : battlefield.attacking_army.troops);
                break;
        case spell_target_type_t::NONE:
                break;
        }

        if(definition->target != spell_target_type_t::NONE && !target)
                return false;

        const int target_x = target ? target->x : -1;
        const int target_y = target ? target->y : -1;
        auto result = battlefield.cast_spell(caster, definition->spell, target_x, target_y, target);
        return result == SPELL_RESULT_OK;
}

} // namespace

bool combat_session_t::apply_action(const combat_action_spec_t& action) {
        auto& battlefield_instance = simulator.battlefield();
        auto* active_unit = battlefield_instance.get_active_unit();
        if(!active_unit)
                return false;

        switch(action.type) {
        case combat_action_type_t::WAIT:
                return battlefield_instance.wait_unit(active_unit);
        case combat_action_type_t::DEFEND:
                return battlefield_instance.defend_unit(active_unit);
        case combat_action_type_t::AUTO_RESOLVE:
                return battlefield_instance.auto_move_troop();
        case combat_action_type_t::CAST_BLESS:
        case combat_action_type_t::CAST_CURSE:
        case combat_action_type_t::CAST_HASTE:
        case combat_action_type_t::CAST_SLOW:
        case combat_action_type_t::CAST_SHIELD:
        case combat_action_type_t::CAST_LIGHTNING_BOLT:
                return cast_spell_action(battlefield_instance, active_unit->is_attacker, action.type);
        default:
                break;
        }

        return false;
}

hero_t& combat_session_t::attacker() {
        return attacker_hero;
}

const hero_t& combat_session_t::attacker() const {
        return attacker_hero;
}

hero_t& combat_session_t::defender() {
        return defender_hero;
}

const hero_t& combat_session_t::defender() const {
        return defender_hero;
}

battlefield_t& combat_session_t::battlefield() {
        return simulator.battlefield();
}

const battlefield_t& combat_session_t::battlefield() const {
        return simulator.battlefield();
}

game_t& combat_session_t::game() {
        return simulator.game();
}

const game_t& combat_session_t::game() const {
        return simulator.game();
}

void combat_session_t::apply_loadout(const hero_loadout_spec_t& spec, hero_t& hero, bool is_attacker) {
        hero = hero_t();
        hero.id = resolve_hero_id(spec, is_attacker);
        hero.player = spec.player;
        hero.hero_class = spec.hero_class;
        hero.name = resolve_hero_name(spec, is_attacker);
        hero.level = spec.level;
        hero.experience = spec.experience;
        hero.attack = spec.attack;
        hero.defense = spec.defense;
        hero.power = spec.power;
        hero.knowledge = spec.knowledge;
        hero.mana = spec.mana;
        hero.dark_energy = spec.dark_energy;
        hero.has_ballista = spec.has_ballista;
        hero.uncontrolled_by_human = !spec.human_controlled;

        apply_troops(spec, hero);
        apply_talents(spec, hero);
        apply_spells_and_skills(spec, hero);
}

void combat_session_t::apply_troops(const hero_loadout_spec_t& spec, hero_t& hero) {
        hero.troops.fill(troop_t());
        for(size_t i = 0; i < spec.troops.size(); ++i) {
                const auto& slot = spec.troops[i];
                if(slot.unit_type == UNIT_UNKNOWN || slot.stack_size == 0)
                        continue;

                hero.troops[i] = troop_t(slot.unit_type, slot.stack_size);
        }
}

void combat_session_t::apply_talents(const hero_loadout_spec_t& spec, hero_t& hero) {
        hero.talents.fill(TALENT_NONE);
        for(size_t i = 0; i < spec.talents.size() && i < hero.talents.size(); ++i)
                hero.talents[i] = spec.talents[i];
}

void combat_session_t::apply_spells_and_skills(const hero_loadout_spec_t& spec, hero_t& hero) {
        hero.spellbook = spec.spellbook;
        hero.enabled_skills = spec.enabled_skills;
}

void combat_session_t::configure_player_control() {
        auto humans = get_unique_human_players(scenario_spec);
        assign_human_players(game(), humans);
}

} // namespace combat
} // namespace rl

