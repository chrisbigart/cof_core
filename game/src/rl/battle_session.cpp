#include "rl/battle_session.h"

#include "rl/combat_actions.h"

#include "core/adventure_map.h"
#include "core/battlefield_hex_grid.h"
#include "core/game_config.h"

#include <algorithm>
#include <cassert>

namespace rl {
namespace combat {

namespace {
struct attack_plan_t {
        battlefield_unit_t* target = nullptr;
        bool use_ranged = false;
        bool requires_move = false;
        int move_x = -1;
        int move_y = -1;
};

bool is_stack_alive(const battlefield_unit_t& unit) {
        return unit.unit_type != UNIT_UNKNOWN && unit.stack_size > 0 && unit.unit_health > 0;
}

std::vector<battlefield_unit_t*> collect_alive_units(army_t& army) {
        std::vector<battlefield_unit_t*> units;
        units.reserve(MAX_ARMY_TROOPS);
        for(auto& troop : army.troops) {
                if(!is_stack_alive(troop))
                        continue;
                units.push_back(&troop);
                if(units.size() >= MAX_ARMY_TROOPS)
                        break;
        }
        return units;
}

std::optional<attack_plan_t> plan_attack(battlefield_t& battlefield,
                                         battlefield_unit_t& attacker,
                                         battlefield_unit_t& target) {
        attack_plan_t plan;
        plan.target = &target;

        if(battlefield.can_troop_shoot(&attacker)
           && battlefield.unit_can_attack(&attacker, &target, target.x, target.y)) {
                plan.use_ranged = true;
                return plan;
        }

        if(battlefield.are_units_adjacent(&attacker, &target)
           && battlefield.unit_can_attack(&attacker, &target, attacker.x, attacker.y)) {
                plan.move_x = attacker.x;
                plan.move_y = attacker.y;
                return plan;
        }

        auto try_adjacent = [&](int origin_x, int origin_y) -> std::optional<attack_plan_t> {
                for(int dir = 0; dir < 6; ++dir) {
                        const auto direction = static_cast<battlefield_direction_e>(dir);
                        if(auto* hex = battlefield.hex_grid.get_adjacent_hex(origin_x, origin_y, direction)) {
                                if(!battlefield.unit_can_attack(&attacker, &target, hex->x, hex->y))
                                        continue;

                                attack_plan_t move_plan;
                                move_plan.target = &target;
                                move_plan.requires_move = true;
                                move_plan.move_x = hex->x;
                                move_plan.move_y = hex->y;
                                return move_plan;
                        }
                }
                return std::nullopt;
        };

        if(auto move_plan = try_adjacent(target.x, target.y))
                return move_plan;

        if(target.is_two_hex()) {
                const int tail_offset = target.is_attacker ? -1 : 1;
                if(auto move_plan = try_adjacent(target.x + tail_offset, target.y))
                        return move_plan;
        }

        return std::nullopt;
}

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
        case combat_action_type_t::MOVE_TO_HEX: {
                if(action.target_x < 0 || action.target_y < 0)
                        return false;
                if(action.target_x >= static_cast<int>(BATTLEFIELD_WIDTH)
                   || action.target_y >= static_cast<int>(BATTLEFIELD_HEIGHT))
                        return false;
                return battlefield_instance.move_unit(*active_unit,
                                                       action.target_x,
                                                       action.target_y);
        }
        case combat_action_type_t::ATTACK_STACK: {
                if(action.target_stack_index < 0)
                        return false;

                auto enemy_units = active_unit->is_attacker ? collect_alive_units(battlefield_instance.defending_army)
                                                             : collect_alive_units(battlefield_instance.attacking_army);
                const auto target_index = static_cast<std::size_t>(action.target_stack_index);
                if(target_index >= enemy_units.size())
                        return false;

                auto* target = enemy_units[target_index];
                if(!target)
                        return false;

                auto plan = plan_attack(battlefield_instance, *active_unit, *target);
                if(!plan)
                        return false;

                if(plan->use_ranged)
                        return battlefield_instance.attack_unit(*active_unit,
                                                                 plan->target,
                                                                 true,
                                                                 plan->target->x,
                                                                 plan->target->y);

                if(plan->requires_move)
                        return battlefield_instance.move_and_attack_unit(*active_unit,
                                                                         *plan->target,
                                                                         plan->move_x,
                                                                         plan->move_y);

                return battlefield_instance.attack_unit(*active_unit,
                                                         plan->target,
                                                         false,
                                                         active_unit->x,
                                                         active_unit->y);
        }
        default:
                break;
        }

        return false;
}

action_mask_t combat_session_t::legal_actions(bool controls_attacker) {
        action_mask_t mask(ACTION_COUNT, 0);
        mask[static_cast<std::size_t>(combat_action_type_t::AUTO_RESOLVE)] = 1;

        auto& battlefield_instance = simulator.battlefield();
        auto* active_unit = battlefield_instance.get_active_unit();
        if(!active_unit)
                return mask;

        if(active_unit->is_attacker != controls_attacker)
                return mask;

        if(!battlefield_instance.can_troop_act(*active_unit))
                return mask;

        if(!active_unit->has_waited)
                mask[static_cast<std::size_t>(combat_action_type_t::WAIT)] = 1;
        if(!active_unit->has_defended)
                mask[static_cast<std::size_t>(combat_action_type_t::DEFEND)] = 1;

        hero_t* friendly_hero = active_unit->is_attacker ? battlefield_instance.attacking_hero
                                                         : battlefield_instance.defending_hero;
        const bool hero_used_cast = active_unit->is_attacker ? battlefield_instance.attacking_hero_used_cast
                                                             : battlefield_instance.defending_hero_used_cast;
        const uint16_t hero_mana = friendly_hero ? friendly_hero->mana : 0;
        const bool can_cast = friendly_hero && !hero_used_cast && hero_mana > 0;

        auto friendly_units = active_unit->is_attacker ? collect_alive_units(battlefield_instance.attacking_army)
                                                       : collect_alive_units(battlefield_instance.defending_army);
        auto enemy_units = active_unit->is_attacker ? collect_alive_units(battlefield_instance.defending_army)
                                                    : collect_alive_units(battlefield_instance.attacking_army);

        const bool friendly_has_target = !friendly_units.empty();
        const bool enemy_has_target = !enemy_units.empty();

        if(can_cast) {
                for(const auto& definition : kSpellActions) {
                        if(!friendly_hero->knows_spell(definition.spell))
                                continue;

                        bool has_target = true;
                        switch(definition.target) {
                        case spell_target_type_t::FRIENDLY:
                                has_target = friendly_has_target;
                                break;
                        case spell_target_type_t::ENEMY:
                                has_target = enemy_has_target;
                                break;
                        case spell_target_type_t::NONE:
                        default:
                                break;
                        }

                        if(!has_target)
                                continue;

                        const auto& spell_config = game_config::get_spell(definition.spell);
                        if(hero_mana < spell_config.mana_cost)
                                continue;

                        mask[static_cast<std::size_t>(definition.action)] = 1;
                }
        }

        for(std::size_t y = 0; y < BATTLEFIELD_HEIGHT; ++y) {
                for(std::size_t x = 0; x < BATTLEFIELD_WIDTH; ++x) {
                        const std::size_t index = MOVEMENT_ACTION_OFFSET + y * BATTLEFIELD_WIDTH + x;
                        if(battlefield_instance.is_move_valid(*active_unit,
                                                              static_cast<uint>(x),
                                                              static_cast<uint>(y))) {
                                mask[index] = 1;
                        }
                }
        }

        for(std::size_t idx = 0; idx < enemy_units.size(); ++idx) {
                        if(auto plan = plan_attack(battlefield_instance, *active_unit, *enemy_units[idx]))
                                mask[ATTACK_ACTION_OFFSET + idx] = 1;
        }

        return mask;
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

