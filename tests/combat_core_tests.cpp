#include "core/battlefield.h"
#include "core/game_config.h"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace {
int failures = 0;

void expect_true(bool condition, const std::string& message) {
        if(!condition) {
                std::cerr << "FAIL: " << message << '\n';
                ++failures;
        }
}

template <typename T, typename U>
void expect_eq(const T& actual, const U& expected, const std::string& message) {
        if(!(actual == expected)) {
                std::cerr << "FAIL: " << message << " (actual=" << actual << ", expected=" << expected << ")\n";
                ++failures;
        }
}

battlefield_unit_t make_unit(unit_type_e type, uint16_t stack_size, bool is_attacker, int8_t troop_id, int8_t x, int8_t y) {
        battlefield_unit_t unit(troop_t(type, stack_size));
        unit.is_attacker = is_attacker;
        unit.troop_id = troop_id;
        unit.x = x;
        unit.y = y;
        return unit;
}

void place_unit(battlefield_t& battlefield, battlefield_unit_t& unit) {
        battlefield.hex_grid.get_hex(unit.x, unit.y)->unit = &unit;
}

void place_two_hex_unit(battlefield_t& battlefield, battlefield_unit_t& unit) {
        place_unit(battlefield, unit);
        const int tail_direction = unit.is_attacker ? -1 : 1;
        battlefield.hex_grid.get_hex(unit.x + tail_direction, unit.y)->unit = &unit;
}

void assign_army_unit(army_t& army, std::size_t slot, battlefield_unit_t unit) {
        army.troops[slot] = unit;
}

std::vector<int8_t> queue_ids(const battlefield_t& battlefield, std::size_t count) {
        std::vector<int8_t> ids;
        for(std::size_t i = 0; i < battlefield.unit_move_queue.size() && i < count; ++i)
                ids.push_back(battlefield.unit_move_queue[i]->troop_id);
        return ids;
}

void test_buff_lifecycle_and_disabled_state() {
        battlefield_unit_t unit = make_unit(UNIT_SKELETON, 8, true, 0, 3, 3);

        expect_true(unit.add_buff(BUFF_BLINDED, 2), "adding a new buff should report success");
        expect_true(unit.has_buff(BUFF_BLINDED), "added buff should be present");
        expect_true(unit.is_disabled(), "blind should disable the unit while duration remains");
        expect_true(!unit.is_disabled(2), "round offset equal to duration should no longer be disabled");

        expect_true(unit.add_buff(BUFF_BLESSED, 3, 5), "adding bless should report success");
        expect_true(unit.add_buff(BUFF_CURSED, 3, 5), "adding curse should report success");
        expect_true(unit.has_buff(BUFF_CURSED), "curse should be present");
        expect_true(!unit.has_buff(BUFF_BLESSED), "curse should remove an existing bless buff");

        expect_true(unit.remove_buff(BUFF_CURSED), "removing an existing buff should report success");
        expect_true(!unit.remove_buff(BUFF_CURSED), "removing a missing buff should report failure");
}

void test_damage_kills_and_clears_hex() {
        battlefield_t battlefield;
        battlefield.fn_emit_combat_action = [](const battle_action_t&) {};
        battlefield_unit_t defender = make_unit(UNIT_SKELETON, 3, false, 8, 5, 5);
        place_unit(battlefield, defender);

        const uint32_t skeleton_hp = defender.get_base_max_hitpoints();
        const uint16_t kills = battlefield.deal_damage_to_stack(skeleton_hp + 1, defender);

        expect_eq(kills, static_cast<uint16_t>(1), "damage over one skeleton HP should kill one unit");
        expect_eq(defender.stack_size, static_cast<uint16_t>(2), "remaining stack size should be reduced by kills");
        expect_eq(defender.unit_health, static_cast<uint16_t>(skeleton_hp - 1), "surviving top unit should retain partial health");
        expect_true(battlefield.hex_grid.get_hex(5, 5)->unit == &defender, "partially damaged stack should remain on its hex");

        const uint16_t lethal_kills = battlefield.deal_damage_to_stack(9999, defender);
        expect_eq(lethal_kills, static_cast<uint16_t>(2), "lethal damage should kill the remaining stack");
        expect_eq(defender.stack_size, static_cast<uint16_t>(0), "lethal damage should empty the stack");
        expect_true(battlefield.hex_grid.get_hex(5, 5)->unit == nullptr, "dead stack should be removed from the hex grid");
}

void test_healing_simulate_and_resurrect() {
        battlefield_t battlefield;
        battlefield.fn_emit_combat_action = [](const battle_action_t&) {};
        battlefield_unit_t unit = make_unit(UNIT_SKELETON, 3, true, 1, 4, 4);
        unit.original_stack_size = 5;
        unit.unit_health = unit.get_base_max_hitpoints() / 2;
        place_unit(battlefield, unit);

        const auto simulated = battlefield.apply_healing_to_stack(1000, unit, true, true);
        expect_true(simulated.first > 0, "simulated healing should report a positive heal amount");
        expect_true(simulated.second > 0, "simulated healing should report resurrected units when allowed");
        expect_eq(unit.stack_size, static_cast<uint16_t>(3), "simulated healing should not mutate stack size");

        const auto actual = battlefield.apply_healing_to_stack(1000, unit, true, false);
        expect_true(actual.first > 0, "actual healing should report a positive heal amount");
        expect_eq(unit.stack_size, static_cast<uint16_t>(5), "healing with resurrection should restore up to original stack size");
        expect_eq(unit.unit_health, unit.get_base_max_hitpoints(), "fully healed stack should have full top-unit health");

        battlefield.deal_damage_to_stack(9999, unit);
        expect_eq(unit.stack_size, static_cast<uint16_t>(0), "test setup should kill the whole stack before resurrection");
        expect_true(battlefield.hex_grid.get_hex(4, 4)->unit == nullptr, "dead stack should be absent before resurrection");

        const auto resurrected = battlefield.apply_healing_to_stack(unit.get_base_max_hitpoints(), unit, true, false);
        expect_eq(resurrected.second, static_cast<uint16_t>(1), "healing a dead stack should resurrect one unit when enough healing is supplied");
        expect_eq(unit.stack_size, static_cast<uint16_t>(1), "resurrected stack should contain one unit");
        expect_true(battlefield.hex_grid.get_hex(4, 4)->unit == &unit, "resurrected stack should be restored to the hex grid");
}

void test_dead_stack_zero_healing_does_not_reoccupy_hex() {
        battlefield_t battlefield;
        battlefield.fn_emit_combat_action = [](const battle_action_t&) {};
        battlefield_unit_t unit = make_unit(UNIT_SKELETON, 1, true, 2, 4, 4);
        unit.original_stack_size = 1;
        place_unit(battlefield, unit);

        battlefield.deal_damage_to_stack(9999, unit);
        expect_eq(unit.stack_size, static_cast<uint16_t>(0), "test setup should kill the stack");
        expect_true(battlefield.hex_grid.get_hex(4, 4)->unit == nullptr, "dead stack should be removed before zero healing");

        const auto healed = battlefield.apply_healing_to_stack(0, unit, true, false);
        expect_eq(healed.first, static_cast<uint32_t>(0), "zero healing should report no healing");
        expect_eq(healed.second, static_cast<uint16_t>(0), "zero healing should report no resurrected units");
        expect_eq(unit.stack_size, static_cast<uint16_t>(0), "zero healing should leave the stack dead");
        expect_true(battlefield.hex_grid.get_hex(4, 4)->unit == nullptr, "zero healing should not put a dead stack back on the hex grid");
}

void test_healing_exact_edge_cases_from_regression_suite() {
        battlefield_t battlefield;
        battlefield.fn_emit_combat_action = [](const battle_action_t&) {};
        auto& unit = battlefield.attacking_army.troops[0];
        unit.is_attacker = true;

        unit.stack_size = 1;
        unit.original_stack_size = 1;
        unit.unit_type = UNIT_HIPPOGRIFF;
        unit.unit_health = 20;
        auto result = battlefield.apply_healing_to_stack(21, unit, true);
        expect_eq(unit.stack_size, static_cast<uint16_t>(1), "basic healing should not change stack size");
        expect_eq(unit.unit_health, static_cast<uint16_t>(40), "basic healing should cap at Hippogriff max HP");

        unit.unit_health = 30;
        result = battlefield.apply_healing_to_stack(20, unit, true);
        expect_eq(unit.stack_size, static_cast<uint16_t>(1), "overhealing should not change stack size");
        expect_eq(unit.unit_health, static_cast<uint16_t>(40), "overhealing should cap at max HP");
        expect_eq(result.first, static_cast<uint32_t>(10), "overhealing should report only effective healing");

        unit.stack_size = 1;
        unit.original_stack_size = 2;
        unit.unit_health = 20;
        result = battlefield.apply_healing_to_stack(60, unit, true);
        expect_eq(unit.stack_size, static_cast<uint16_t>(2), "resurrection healing should restore one Hippogriff");
        expect_eq(unit.unit_health, static_cast<uint16_t>(40), "resurrection healing should fill the top unit");
        expect_eq(result.first, static_cast<uint32_t>(60), "resurrection should report effective healing");
        expect_eq(result.second, static_cast<uint16_t>(1), "resurrection should report one revived unit");

        unit.stack_size = 1;
        unit.original_stack_size = 5;
        unit.unit_health = 10;
        result = battlefield.apply_healing_to_stack(200, unit, true);
        expect_eq(unit.stack_size, static_cast<uint16_t>(5), "large resurrection healing should restore multiple units up to original size");
        expect_eq(unit.unit_health, static_cast<uint16_t>(40), "large resurrection healing should leave full top-unit HP");
        expect_eq(result.first, static_cast<uint32_t>(190), "large resurrection should report capped effective healing");
        expect_eq(result.second, static_cast<uint16_t>(4), "large resurrection should report all revived units");

        unit.stack_size = 2;
        unit.original_stack_size = 3;
        unit.unit_health = 20;
        result = battlefield.apply_healing_to_stack(30, unit, false);
        expect_eq(unit.stack_size, static_cast<uint16_t>(2), "non-resurrection healing should not restore missing units");
        expect_eq(unit.unit_health, static_cast<uint16_t>(40), "non-resurrection healing should fill current top unit");
        expect_eq(result.first, static_cast<uint32_t>(20), "non-resurrection healing should report only current-stack healing");
        expect_eq(result.second, static_cast<uint16_t>(0), "non-resurrection healing should not report revived units");

        unit.stack_size = 1;
        unit.original_stack_size = 1;
        unit.unit_type = UNIT_SKELETON;
        unit.unit_health = 1;
        result = battlefield.apply_healing_to_stack(3, unit, true);
        expect_eq(unit.stack_size, static_cast<uint16_t>(1), "exact skeleton heal should keep one unit");
        expect_eq(unit.unit_health, static_cast<uint16_t>(4), "exact skeleton heal should reach max HP");
        expect_eq(result.first, static_cast<uint32_t>(3), "exact skeleton heal should report exact effective healing");
        expect_eq(result.second, static_cast<uint16_t>(0), "exact skeleton heal should not revive units");

        unit.stack_size = 1;
        unit.original_stack_size = 2;
        unit.unit_type = UNIT_TITAN;
        unit.unit_health = 125;
        result = battlefield.apply_healing_to_stack(375, unit, true);
        expect_eq(unit.stack_size, static_cast<uint16_t>(2), "exact Titan resurrection should restore one unit");
        expect_eq(unit.unit_health, static_cast<uint16_t>(250), "exact Titan resurrection should fill top-unit HP");
        expect_eq(result.first, static_cast<uint32_t>(375), "exact Titan resurrection should report exact healing");
        expect_eq(result.second, static_cast<uint16_t>(1), "exact Titan resurrection should report one revived unit");

        unit.stack_size = 3;
        unit.original_stack_size = 3;
        unit.unit_type = UNIT_ELF;
        unit.unit_health = 15;
        result = battlefield.apply_healing_to_stack(100, unit, true);
        expect_eq(unit.stack_size, static_cast<uint16_t>(3), "healing a full stack should keep stack size");
        expect_eq(unit.unit_health, static_cast<uint16_t>(15), "healing a full stack should keep top-unit HP");
        expect_eq(result.first, static_cast<uint32_t>(0), "healing a full stack should report no effective healing");
        expect_eq(result.second, static_cast<uint16_t>(0), "healing a full stack should report no revived units");
}

void test_fortitude_expiration_preserves_health_percentage() {
        battlefield_t battlefield;
        battlefield.fn_emit_combat_action = [](const battle_action_t&) {};
        auto& full_unit = battlefield.attacking_army.troops[0];
        full_unit = make_unit(UNIT_HIPPOGRIFF, 2, true, 4, 4, 4);
        full_unit.unit_health = full_unit.get_base_max_hitpoints();
        expect_true(full_unit.add_buff(BUFF_INCREASED_HEALTH, 1, 50), "test setup should fortify full unit");
        battlefield.adjust_unit_health_after_max_hp_change(full_unit, full_unit.get_base_max_hitpoints());
        expect_eq(battlefield.get_unit_adjusted_hp(full_unit), static_cast<uint>(60), "fortitude should increase Hippogriff max HP by 50%");
        expect_eq(full_unit.unit_health, static_cast<uint16_t>(60), "full fortified unit should be full at adjusted max HP");

        auto& damaged_unit = battlefield.defending_army.troops[0];
        damaged_unit = make_unit(UNIT_HIPPOGRIFF, 2, false, 5, 6, 4);
        damaged_unit.unit_health = 20;
        expect_true(damaged_unit.add_buff(BUFF_INCREASED_HEALTH, 1, 50), "test setup should fortify damaged unit");
        battlefield.adjust_unit_health_after_max_hp_change(damaged_unit, damaged_unit.get_base_max_hitpoints());
        expect_eq(damaged_unit.unit_health, static_cast<uint16_t>(30), "damaged fortified unit should keep its health percentage when max HP rises");

        battlefield.next_round();

        expect_true(!full_unit.has_buff(BUFF_INCREASED_HEALTH), "fortitude should expire after its duration reaches zero");
        expect_eq(battlefield.get_unit_adjusted_hp(full_unit), static_cast<uint>(40), "expired fortitude should restore base max HP");
        expect_eq(full_unit.unit_health, static_cast<uint16_t>(40), "full unit HP should be capped when fortitude expires");
        expect_eq(damaged_unit.unit_health, static_cast<uint16_t>(20), "damaged unit should keep its health percentage when fortitude expires");
}

void test_damage_prediction_and_adjusted_stats_are_bounded() {
        battlefield_t battlefield;
        battlefield.fn_emit_combat_action = [](const battle_action_t&) {};

        battlefield_unit_t defender = make_unit(UNIT_SKELETON, 2, false, 2, 6, 6);
        place_unit(battlefield, defender);

        expect_eq(battlefield.get_kills(9999, defender), static_cast<uint16_t>(2), "predicted kills should not exceed current stack size");

        expect_true(defender.add_buff(BUFF_INFESTED, 3, 200), "test setup should add a large defense debuff");
        expect_eq(battlefield.get_unit_adjusted_defense(defender), static_cast<uint>(0), "defense debuffs should clamp at zero instead of underflowing");

        battlefield_unit_t lich = make_unit(UNIT_LICH, 1, false, 3, 7, 6);
        expect_eq(lich.get_base_resistance(MAGIC_DAMAGE_FROST), static_cast<uint>(100), "frost immune creatures should have full frost resistance");
        expect_true(lich.get_base_resistance(MAGIC_DAMAGE_FIRE) < 100, "frost immunity should not imply fire immunity");
}

void test_turn_queue_orders_by_adjusted_initiative_speed_and_troop_bar() {
        battlefield_t battlefield;
        battlefield.fn_emit_combat_action = [](const battle_action_t&) {};

        assign_army_unit(battlefield.attacking_army, 0, make_unit(UNIT_SKELETON, 5, true, 10, 3, 7));
        assign_army_unit(battlefield.attacking_army, 1, make_unit(UNIT_SKELETON, 5, true, 11, 3, 2));
        assign_army_unit(battlefield.defending_army, 0, make_unit(UNIT_DEMON, 5, false, 20, 10, 4));

        expect_true(battlefield.recompute_unit_move_queue(), "turn queue should be populated when both armies have troops");
        const auto ids = queue_ids(battlefield, 3);
        expect_eq(ids.size(), static_cast<std::size_t>(3), "queue should expose at least three units for this scenario");
        expect_eq(ids[0], static_cast<int8_t>(11), "same initiative and speed should prefer the later troop-bar slot under the battlefield comparator");
        expect_eq(ids[1], static_cast<int8_t>(10), "same-side tie should follow troop-bar ordering before lower initiative enemies");
}

void test_wait_queue_uses_adjusted_reverse_order() {
        battlefield_t battlefield;
        battlefield.fn_emit_combat_action = [](const battle_action_t&) {};

        auto fast_waiter = make_unit(UNIT_SKELETON, 5, true, 12, 3, 3);
        expect_true(fast_waiter.add_buff(BUFF_INCREASED_INITIATIVE, 1, 20), "test setup should boost waited unit initiative");
        fast_waiter.has_waited = true;

        auto slower_waiter = make_unit(UNIT_VAMPIRE, 5, true, 13, 4, 4);
        slower_waiter.has_waited = true;

        assign_army_unit(battlefield.attacking_army, 0, fast_waiter);
        assign_army_unit(battlefield.attacking_army, 1, slower_waiter);
        assign_army_unit(battlefield.defending_army, 0, make_unit(UNIT_DEMON, 5, false, 21, 10, 4));

        expect_true(battlefield.recompute_unit_move_queue(), "turn queue should include normal and waited units");
        const auto ids = queue_ids(battlefield, 3);
        expect_eq(ids[0], static_cast<int8_t>(21), "non-waiting unit should act before waited units");
        expect_eq(ids[1], static_cast<int8_t>(13), "waited units should act in reverse adjusted initiative order");
        expect_eq(ids[2], static_cast<int8_t>(12), "higher adjusted initiative waiter should act later in the wait segment");
}

void test_wait_unit_requeues_active_unit_after_non_waiters() {
        battlefield_t battlefield;
        battlefield.fn_emit_combat_action = [](const battle_action_t&) {};

        assign_army_unit(battlefield.attacking_army, 0, make_unit(UNIT_SKELETON, 5, true, 14, 3, 3));
        assign_army_unit(battlefield.defending_army, 0, make_unit(UNIT_DEMON, 5, false, 22, 10, 4));

        battlefield.compute_next_unit_to_move();
        auto* first = battlefield.get_active_unit();
        expect_true(first && first->troop_id == 14, "higher initiative attacker should be active first before waiting");

        expect_true(battlefield.wait_unit(first), "active unit should be able to wait once");
        auto* after_wait = battlefield.get_active_unit();
        expect_true(after_wait && after_wait->troop_id == 22, "waiting should pass the turn to remaining non-waited units");

        battlefield.finish_troop_action(after_wait);
        auto* waited_turn = battlefield.get_active_unit();
        expect_true(waited_turn && waited_turn->troop_id == 14, "waited unit should return after non-waited units finish");
}

void test_movement_shooting_and_retaliation_rules() {
        battlefield_t battlefield;
        battlefield.fn_emit_combat_action = [](const battle_action_t&) {};
        battlefield_unit_t archer = make_unit(UNIT_ARCHER, 10, true, 0, 3, 3);
        battlefield_unit_t enemy = make_unit(UNIT_SKELETON, 10, false, 7, 4, 3);
        place_unit(battlefield, archer);
        place_unit(battlefield, enemy);

        expect_true(!battlefield.can_troop_shoot(&archer), "shooter adjacent to an enemy should not be able to shoot by default");
        expect_true(archer.add_buff(BUFF_RANGED_CAN_SHOOT_ADJACENT), "test setup should add adjacent shooting buff");
        expect_true(battlefield.can_troop_shoot(&archer), "adjacent shooting buff should allow shooting while engaged");

        expect_true(!battlefield.is_move_valid(archer, enemy.x, enemy.y), "moving onto an enemy-occupied hex should be invalid");
        expect_true(battlefield.is_move_valid(archer, archer.x, archer.y), "remaining on the current hex should be valid");

        expect_true(battlefield.will_defender_retaliate(archer, enemy), "healthy enabled defender with retaliation charges should retaliate");
        enemy.retaliations_remaining = 0;
        expect_true(!battlefield.will_defender_retaliate(archer, enemy), "defender with no retaliation charges should not retaliate");
        enemy.retaliations_remaining = 1;
        expect_true(archer.add_buff(BUFF_NO_ENEMY_RETALIATION), "test setup should add no-retaliation buff");
        expect_true(!battlefield.will_defender_retaliate(archer, enemy), "attacker no-retaliation buff should suppress retaliation");
}

void test_resurrection_targeting_rejects_blocked_two_hex_corpse() {
        battlefield_t battlefield;
        battlefield.fn_emit_combat_action = [](const battle_action_t&) {};
        hero_t attacker;
        hero_t defender;
        battlefield.attacking_hero = &attacker;
        battlefield.defending_hero = &defender;

        battlefield_unit_t abomination = make_unit(UNIT_ABOMINATION, 3, true, 0, 5, 5);
        abomination.original_stack_size = 3;
        place_two_hex_unit(battlefield, abomination);
        battlefield.deal_damage_to_stack(9999, abomination);
        expect_eq(abomination.stack_size, static_cast<uint16_t>(0), "test setup should create a two-hex corpse");

        battlefield_unit_t blocker = make_unit(UNIT_SKELETON, 10, false, 3, 4, 5);
        place_unit(battlefield, blocker);

        expect_true(!battlefield.is_spell_target_valid(&attacker, &abomination, SPELL_RESURRECTION),
                    "resurrection should be invalid when a two-hex corpse's tail is blocked by another unit");
}

void test_summon_spell_auto_places_near_caster_and_rejects_when_full() {
        battlefield_t battlefield;
        battlefield.fn_emit_combat_action = [](const battle_action_t&) {};
        hero_t attacker;
        attacker.mana = 100;
        attacker.spellbook.push_back(SPELL_SUMMON_EFREET);
        battlefield.attacking_hero = &attacker;

        battlefield_unit_t blocker = make_unit(UNIT_SKELETON, 10, false, 7, 3, 3);
        place_unit(battlefield, blocker);

        expect_eq(battlefield.cast_spell(&attacker, SPELL_SUMMON_EFREET), SPELL_RESULT_OK,
                  "summon should succeed without requiring a target hex");
        auto summoned = battlefield.attacking_army.troops[0];
        expect_true(summoned.was_summoned, "summon should fill the first open friendly troop slot");
        expect_true(battlefield.hex_grid.get_hex(summoned.x, summoned.y)->unit == &battlefield.attacking_army.troops[0],
                    "summon should occupy the automatically selected open hex");
        expect_true(battlefield.hex_grid.get_hex(3, 3)->unit == &blocker, "summon should not overwrite occupied hexes while auto-placing");

        battlefield_t full_battlefield;
        full_battlefield.fn_emit_combat_action = [](const battle_action_t&) {};
        hero_t full_attacker;
        full_attacker.mana = 100;
        full_attacker.spellbook.push_back(SPELL_SUMMON_EFREET);
        full_battlefield.attacking_hero = &full_attacker;
        for(uint y = 0; y < game_config::BATTLEFIELD_HEIGHT; ++y) {
                for(uint x = 0; x < game_config::BATTLEFIELD_WIDTH; ++x)
                        full_battlefield.hex_grid.get_hex(x, y)->passable = false;
        }
        full_attacker.mana = 100;
        expect_eq(full_battlefield.cast_spell(&full_attacker, SPELL_SUMMON_EFREET), SPELL_RESULT_INVALID_TARGET,
                  "summon should fail when no passable open hex is available");
}
}

int main() {
        auto config_root = std::filesystem::current_path();
        while(!std::filesystem::exists(config_root / "config" / "creatures.tsv") && config_root.has_parent_path())
                config_root = config_root.parent_path();

        const auto config_prefix = config_root.string() + "/";
        if(game_config::load_buffs(config_prefix) != 0 || game_config::load_creatures(config_prefix) != 0 || game_config::load_spells(config_prefix) != 0) {
                std::cerr << "FAIL: required combat config failed to load\n";
                return EXIT_FAILURE;
        }

        test_buff_lifecycle_and_disabled_state();
        test_damage_kills_and_clears_hex();
        test_healing_simulate_and_resurrect();
        test_dead_stack_zero_healing_does_not_reoccupy_hex();
        test_healing_exact_edge_cases_from_regression_suite();
        test_fortitude_expiration_preserves_health_percentage();
        test_damage_prediction_and_adjusted_stats_are_bounded();
        test_turn_queue_orders_by_adjusted_initiative_speed_and_troop_bar();
        test_wait_queue_uses_adjusted_reverse_order();
        test_wait_unit_requeues_active_unit_after_non_waiters();
        test_movement_shooting_and_retaliation_rules();
        test_resurrection_targeting_rejects_blocked_two_hex_corpse();
        test_summon_spell_auto_places_near_caster_and_rejects_when_full();

        if(failures != 0) {
                std::cerr << failures << " combat core test(s) failed.\n";
                return EXIT_FAILURE;
        }

        std::cout << "All combat core tests passed.\n";
        return EXIT_SUCCESS;
}
