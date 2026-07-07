#include "core/game.h"
#include "core/game_config.h"

#include <algorithm>
#include <iostream>
#include <set>
#include <string>
#include <vector>

namespace {
int failures = 0;

struct magic_combat_stat_achievement_case_t {
        uint32_t combat_stats_t::* stat;
        achievement_e achievement;
};

struct small_magic_combat_stat_achievement_case_t {
        uint16_t combat_stats_t::* stat;
        achievement_e achievement;
};

struct might_combat_stat_achievement_case_t {
        uint32_t combat_stats_t::* stat;
        achievement_e achievement;
};

struct small_might_combat_stat_achievement_case_t {
        uint16_t combat_stats_t::* stat;
        achievement_e achievement;
};

void expect_true(bool condition, const std::string& message) {
        if(!condition) {
                std::cerr << "FAIL: " << message << '\n';
                ++failures;
        }
}

void expect_eq(size_t actual, size_t expected, const std::string& message) {
        if(actual != expected) {
                std::cerr << "FAIL: " << message << " (expected " << expected << ", got " << actual << ")\n";
                ++failures;
        }
}

size_t earned_count(const std::vector<achievement_e>& earned, achievement_e achievement) {
        return static_cast<size_t>(std::count(earned.begin(), earned.end(), achievement));
}

uint64_t first_threshold_or_one(const achievement_t& achievement) {
        for(const auto& tier : achievement.tiers) {
                if(tier.value > 0)
                        return tier.value;
        }
        return 1;
}

battlefield_unit_t make_battlefield_unit(unit_type_e type, uint16_t stack_size, bool is_attacker, int8_t troop_id, int8_t x, int8_t y) {
        battlefield_unit_t unit(troop_t(type, stack_size));
        unit.is_attacker = is_attacker;
        unit.troop_id = troop_id;
        unit.x = x;
        unit.y = y;
        return unit;
}

void place_battlefield_unit(battlefield_t& battlefield, battlefield_unit_t& unit) {
        battlefield.hex_grid.get_hex(unit.x, unit.y)->unit = &unit;
}

std::vector<small_magic_combat_stat_achievement_case_t> small_magic_combat_stat_achievement_cases() {
        return {
                {&combat_stats_t::genie_friendly_spells_cast, ACHIEVEMENT_I_DREAM_OF_GENIE},
        };
}

std::vector<magic_combat_stat_achievement_case_t> magic_combat_stat_achievement_cases() {
        return {
                {&combat_stats_t::fire_spell_damage, ACHIEVEMENT_IN_FLAMES},
                {&combat_stats_t::frost_spell_damage, ACHIEVEMENT_WINTER_IS_COMING},
                {&combat_stats_t::lightning_spell_damage, ACHIEVEMENT_STORM_DRINKER},
                {&combat_stats_t::earth_spell_damage, ACHIEVEMENT_EARTHSHAKER},
                {&combat_stats_t::chaos_spell_damage, ACHIEVEMENT_CHAOS_THEORY},
                {&combat_stats_t::holy_spell_damage, ACHIEVEMENT_BLINDED_BY_THE_LIGHT},
        };
}

std::vector<small_might_combat_stat_achievement_case_t> small_might_combat_stat_achievement_cases() {
        return {
                {&combat_stats_t::total_critical_hits, ACHIEVEMENT_SHOT_THROUGH_THE_HEART},
        };
}

std::vector<might_combat_stat_achievement_case_t> might_combat_stat_achievement_cases() {
        return {
                {&combat_stats_t::total_units_killed, ACHIEVEMENT_THE_REAPER},
                {&combat_stats_t::max_damage_single_melee_attack, ACHIEVEMENT_ONE_PUNCH_HERO},
        };
}

std::vector<achievement_e> might_event_achievement_cases() {
        return {
                ACHIEVEMENT_MAXED_OUT,
                ACHIEVEMENT_GLASS_CANNON,
                ACHIEVEMENT_IMMOVABLE_OBJECT,
                ACHIEVEMENT_OVERMATCHED,
                ACHIEVEMENT_EQUAL_GROUNDS,
                ACHIEVEMENT_MASSACRE,
                ACHIEVEMENT_TITANIC_SMACKDOWN,
                ACHIEVEMENT_THE_BIGGER_THEY_ARE,
                ACHIEVEMENT_UNBREAKABLE,
                ACHIEVEMENT_OUTLAST,
                ACHIEVEMENT_FLAWLESS_EXECUTION,
                ACHIEVEMENT_HIGH_ROLLER,
                ACHIEVEMENT_MAXIMUM_OVERKILL,
                ACHIEVEMENT_CRUNCHING_NUMBERS,
        };
}

std::vector<achievement_e> magic_event_achievement_cases() {
        return {
                ACHIEVEMENT_ANCIENT_SECRETS,
                ACHIEVEMENT_FRIENDLY_FIRE,
                ACHIEVEMENT_QUENCHING_THE_FLAME,
                ACHIEVEMENT_CONDUCTIVE_CRITTERS,
                ACHIEVEMENT_BRAINS_BEATS_BRAWN,
                ACHIEVEMENT_METEORIC_RISE,
                ACHIEVEMENT_GRAVE_MISTAKE,
                ACHIEVEMENT_SECOND_LIFE,
                ACHIEVEMENT_CREATIVE_CASTER,
                ACHIEVEMENT_WATCH_THE_WORLD_BURN,
                ACHIEVEMENT_TURTLING_UP,
                ACHIEVEMENT_MASTER_OF_THE_ARCANE,
                ACHIEVEMENT_ENERGIZER_BUNNY,
        };
}

void initialize_combat_achievement_game(game_t& game, hero_t& attacker, player_t& player) {
        player.player_number = PLAYER_1;
        player.is_human = true;
        game.players.push_back(player);
        attacker.player = PLAYER_1;
        game.battle.attacking_hero = &attacker;
        game.battle.result = BATTLE_BOTH_LOSE;
        game.battle.fn_emit_combat_action = [](const battle_action_t&) {};
        game.remove_hero_callback_fn = [](hero_t*, bool) {};
        game.remove_interactable_object_callback_fn = [](interactable_object_t*, bool) {};
        game.show_dialog_callback_fn = [](dialog_type_e, interactable_object_t*, hero_t*, int, int) {};
        game.show_hero_levelup_callback_fn = [](hero_t*, skill_e, skill_e, skill_e, stat_e, bool) {};
        game.game_event_callback_fn = [](game_state_update_e, player_e, int) {};
        game.update_interactable_object_callback_fn = [](interactable_object_t*) {};
}

void apply_magic_event_condition(game_t& game, hero_t& attacker, achievement_e achievement) {
        switch(achievement) {
                case ACHIEVEMENT_FRIENDLY_FIRE:
                        game.battle.result = BATTLE_ATTACKER_VICTORY;
                        game.battle.attacker_stats.friendly_fire_spell_hits = 1;
                        game.battle.attacker_stats.total_units_lost = 1;
                        break;
                case ACHIEVEMENT_QUENCHING_THE_FLAME:
                        game.battle.attacker_stats.frost_kill_fire_immune = 1;
                        break;
                case ACHIEVEMENT_CONDUCTIVE_CRITTERS:
                        game.battle.attacker_stats.chain_lightning_5kills = 1;
                        break;
                case ACHIEVEMENT_BRAINS_BEATS_BRAWN:
                        game.battle.result = BATTLE_ATTACKER_VICTORY;
                        game.battle.attacker_stats.total_spell_damage_done = 1;
                        game.battle.attacker_stats.total_units_lost = 1;
                        break;
                case ACHIEVEMENT_METEORIC_RISE:
                        game.battle.attacker_stats.meteor_shower_75kills = 1;
                        break;
                case ACHIEVEMENT_GRAVE_MISTAKE:
                        game.battle.attacker_stats.implosion_finish_necromancer = 1;
                        break;
                case ACHIEVEMENT_SECOND_LIFE:
                        game.battle.attacker_stats.resurrection_1000hp = 1;
                        break;
                case ACHIEVEMENT_CREATIVE_CASTER:
                        game.battle.attacker_stats.unique_spells_cast = 6;
                        break;
                case ACHIEVEMENT_WATCH_THE_WORLD_BURN:
                        game.battle.attacker_stats.armageddon_5000_damage = 1;
                        break;
                case ACHIEVEMENT_TURTLING_UP:
                        game.update_achievement_stats(game.get_player(PLAYER_1).player_stats.achievement_event_counts[achievement], 1, achievement);
                        break;
                case ACHIEVEMENT_MASTER_OF_THE_ARCANE:
                        game.battle.attacker_stats.total_spells_cast_nature = 1;
                        game.battle.attacker_stats.total_spells_cast_arcane = 1;
                        game.battle.attacker_stats.total_spells_cast_holy = 1;
                        game.battle.attacker_stats.total_spells_cast_destruction = 1;
                        game.battle.attacker_stats.total_spells_cast_death = 1;
                        break;
                case ACHIEVEMENT_ENERGIZER_BUNNY:
                        game.battle.result = BATTLE_ATTACKER_VICTORY;
                        attacker.knowledge = 1;
                        attacker.mana = attacker.get_maximum_mana() * 2;
                        game.battle.attacker_stats.total_units_lost = 1;
                        break;
                default:
                        break;
        }
}

void apply_might_event_condition(game_t& game, hero_t& attacker, hero_t& defender, achievement_e achievement) {
        switch(achievement) {
                case ACHIEVEMENT_MAXED_OUT:
                        game.battle.result = BATTLE_ATTACKER_VICTORY;
                        attacker.attack = 99;
                        attacker.defense = 99;
                        game.battle.attacker_stats.total_units_lost = 1;
                        break;
                case ACHIEVEMENT_GLASS_CANNON:
                        game.battle.result = BATTLE_ATTACKER_VICTORY;
                        attacker.attack = 25;
                        attacker.defense = 0;
                        game.battle.attacker_stats.total_units_lost = 1;
                        break;
                case ACHIEVEMENT_IMMOVABLE_OBJECT:
                        game.battle.result = BATTLE_ATTACKER_VICTORY;
                        attacker.attack = 0;
                        attacker.defense = 25;
                        game.battle.attacker_stats.total_units_lost = 1;
                        break;
                case ACHIEVEMENT_OVERMATCHED:
                        game.battle.result = BATTLE_ATTACKER_VICTORY;
                        defender.player = PLAYER_NONE;
                        game.battle.defending_hero = &defender;
                        attacker.attack = 10;
                        attacker.defense = 10;
                        defender.attack = 1;
                        defender.defense = 1;
                        game.battle.attacker_stats.total_units_lost = 1;
                        break;
                case ACHIEVEMENT_EQUAL_GROUNDS:
                        game.battle.result = BATTLE_ATTACKER_VICTORY;
                        defender.player = PLAYER_NONE;
                        game.battle.defending_hero = &defender;
                        attacker.attack = 5;
                        attacker.defense = 5;
                        defender.attack = 5;
                        defender.defense = 5;
                        game.battle.attacker_stats.total_units_lost = 1;
                        break;
                case ACHIEVEMENT_MASSACRE:
                        game.battle.result = BATTLE_ATTACKER_VICTORY;
                        game.get_player(PLAYER_1).player_stats.cumulative_combat_stats.total_units_killed = 1;
                        game.battle.attacker_stats.total_units_killed = 1000;
                        game.battle.attacker_stats.total_units_lost = 1;
                        break;
                case ACHIEVEMENT_TITANIC_SMACKDOWN:
                        game.battle.attacker_stats.titanic_smackdowns = 1;
                        break;
                case ACHIEVEMENT_THE_BIGGER_THEY_ARE:
                        game.battle.attacker_stats.tier6_killed_by_tier1_army = 1;
                        break;
                case ACHIEVEMENT_UNBREAKABLE:
                        game.get_player(PLAYER_1).player_stats.achievement_event_counts[ACHIEVEMENT_FLAWLESS_EXECUTION] = 1;
                        game.battle.result = BATTLE_ATTACKER_VICTORY;
                        game.battle.attacker_stats.total_attacks_received = 10;
                        break;
                case ACHIEVEMENT_OUTLAST:
                        game.battle.result = BATTLE_ATTACKER_VICTORY;
                        game.battle.round = 20;
                        game.battle.attacker_stats.total_units_lost = 1;
                        break;
                case ACHIEVEMENT_FLAWLESS_EXECUTION:
                        game.battle.result = BATTLE_ATTACKER_VICTORY;
                        break;
                case ACHIEVEMENT_HIGH_ROLLER:
                        game.get_player(PLAYER_1).player_stats.cumulative_combat_stats.total_critical_hits = 1;
                        game.battle.attacker_stats.total_attacks_made = 1;
                        game.battle.attacker_stats.total_critical_hits = 1;
                        break;
                case ACHIEVEMENT_MAXIMUM_OVERKILL:
                        game.battle.attacker_stats.maximum_overkill_hits = 1;
                        break;
                case ACHIEVEMENT_CRUNCHING_NUMBERS:
                        game.battle.attacker_stats.exact_melee_kills = 1;
                        break;
                default:
                        break;
        }
}

void test_storm_drinker_lightning_spell_damage_cast_progress_and_callback() {
        game_t game;
        hero_t attacker;
        player_t player;
        initialize_combat_achievement_game(game, attacker, player);

        attacker.power = 200;
        attacker.mana = 1000;
        attacker.spellbook.push_back(SPELL_LIGHTNING_BOLT);

        auto& target = game.battle.defending_army.troops[0];
        target = make_battlefield_unit(UNIT_SKELETON, 1000, false, 1, 8, 5);
        place_battlefield_unit(game.battle, target);

        std::vector<achievement_e> earned;
        game.achievement_earned_callback_fn = [&](achievement_e achievement) {
                earned.push_back(achievement);
        };

        const auto first_threshold = first_threshold_or_one(game_config::get_achievement(ACHIEVEMENT_STORM_DRINKER));
        auto& progress = game.get_player(PLAYER_1).player_stats.cumulative_combat_stats.lightning_spell_damage;
        progress = first_threshold - 1;

        expect_true(game.battle.cast_spell(&attacker, SPELL_LIGHTNING_BOLT, target.x, target.y, &target) == SPELL_RESULT_OK,
                    "Lightning Bolt should successfully hit the enemy stack in the Storm Drinker scenario");
        expect_true(game.battle.attacker_stats.lightning_spell_damage > 0,
                    "Lightning Bolt should record lightning spell damage for the attacking hero");

        const auto lightning_damage = game.battle.attacker_stats.lightning_spell_damage;
        game.accept_battle_results();

        expect_eq(progress, first_threshold - 1 + lightning_damage,
                  "Storm Drinker should add actual Lightning Bolt damage to cumulative lightning spell damage");
        expect_eq(earned_count(earned, ACHIEVEMENT_STORM_DRINKER), size_t(1),
                  "Storm Drinker callback should fire exactly once after Lightning Bolt crosses the first tier threshold");
        expect_true(!earned.empty(),
                    "Lightning Bolt scenario should fire at least the Storm Drinker achievement callback");
}

template<typename TestCase>
void test_magic_combat_stat_case(const TestCase& test_case) {
        game_t game;
        hero_t attacker;
        player_t player;
        initialize_combat_achievement_game(game, attacker, player);

        std::vector<achievement_e> earned;
        game.achievement_earned_callback_fn = [&](achievement_e achievement) {
                earned.push_back(achievement);
        };

        const auto first_threshold = first_threshold_or_one(game_config::get_achievement(test_case.achievement));
        auto& progress = game.get_player(PLAYER_1).player_stats.cumulative_combat_stats.*(test_case.stat);
        progress = first_threshold - 1;

        game.battle.attacker_stats.*(test_case.stat) = 1;
        game.accept_battle_results();

        expect_eq(progress, first_threshold,
                  game_config::get_achievement(test_case.achievement).name + " should increment cumulative magic combat stat exactly");
        expect_eq(earned_count(earned, test_case.achievement), size_t(1),
                  game_config::get_achievement(test_case.achievement).name + " callback should fire exactly once when crossing the first tier threshold");
        expect_eq(earned.size(), size_t(1),
                  game_config::get_achievement(test_case.achievement).name + " should be the only achievement callback in this combat scenario");
}

void test_magic_combat_stat_progress_and_callbacks() {
        for(const auto& test_case : small_magic_combat_stat_achievement_cases())
                test_magic_combat_stat_case(test_case);
        for(const auto& test_case : magic_combat_stat_achievement_cases()) {
                test_magic_combat_stat_case(test_case);
        }
}

void test_magic_event_progress_and_callbacks() {
        for(const auto achievement : magic_event_achievement_cases()) {
                game_t game;
                hero_t attacker;
                player_t player;
                initialize_combat_achievement_game(game, attacker, player);

                std::vector<achievement_e> earned;
                game.achievement_earned_callback_fn = [&](achievement_e earned_achievement) {
                        earned.push_back(earned_achievement);
                };

                const auto first_threshold = first_threshold_or_one(game_config::get_achievement(achievement));
                auto& progress = game.get_player(PLAYER_1).player_stats.achievement_event_counts[achievement];
                progress = first_threshold - 1;

                if(achievement == ACHIEVEMENT_ANCIENT_SECRETS)
                        game.update_achievement_stats(progress, 1, achievement);
                else {
                        apply_magic_event_condition(game, attacker, achievement);
                        game.accept_battle_results();
                }

                expect_eq(progress, first_threshold,
                          game_config::get_achievement(achievement).name + " should increment magic event progress exactly");
                expect_eq(earned_count(earned, achievement), size_t(1),
                          game_config::get_achievement(achievement).name + " callback should fire exactly once when crossing the first tier threshold");
                expect_eq(earned.size(), size_t(1),
                          game_config::get_achievement(achievement).name + " should be the only achievement callback in this magic event scenario");
        }
}

void test_all_magic_achievements_have_specific_test_case() {
        std::set<achievement_e> covered;
        for(const auto& test_case : small_magic_combat_stat_achievement_cases())
                covered.insert(test_case.achievement);
        for(const auto& test_case : magic_combat_stat_achievement_cases())
                covered.insert(test_case.achievement);
        for(const auto achievement : magic_event_achievement_cases())
                covered.insert(achievement);

        for(const auto& achievement : game_config::get_achievements()) {
                if(achievement.type != ACHIEVEMENT_TYPE_MAGIC)
                        continue;

                expect_true(covered.count(achievement.id) == 1, achievement.name + " should have an explicit magic achievement test case");
        }
}


template<typename TestCase>
void test_might_combat_stat_case(const TestCase& test_case) {
        game_t game;
        hero_t attacker;
        player_t player;
        initialize_combat_achievement_game(game, attacker, player);

        std::vector<achievement_e> earned;
        game.achievement_earned_callback_fn = [&](achievement_e achievement) {
                earned.push_back(achievement);
        };

        const auto first_threshold = first_threshold_or_one(game_config::get_achievement(test_case.achievement));
        auto& progress = game.get_player(PLAYER_1).player_stats.cumulative_combat_stats.*(test_case.stat);
        progress = first_threshold - 1;

        game.battle.attacker_stats.*(test_case.stat) = 1;
        game.accept_battle_results();

        expect_eq(progress, first_threshold,
                  game_config::get_achievement(test_case.achievement).name + " should increment cumulative might combat stat exactly");
        expect_eq(earned_count(earned, test_case.achievement), size_t(1),
                  game_config::get_achievement(test_case.achievement).name + " callback should fire exactly once when crossing the first tier threshold");
        expect_eq(earned.size(), size_t(1),
                  game_config::get_achievement(test_case.achievement).name + " should be the only achievement callback in this might combat scenario");
}

void test_might_combat_stat_progress_and_callbacks() {
        for(const auto& test_case : small_might_combat_stat_achievement_cases())
                test_might_combat_stat_case(test_case);
        for(const auto& test_case : might_combat_stat_achievement_cases())
                test_might_combat_stat_case(test_case);
}

void test_might_event_progress_and_callbacks() {
        for(const auto achievement : might_event_achievement_cases()) {
                game_t game;
                hero_t attacker;
                hero_t defender;
                player_t player;
                initialize_combat_achievement_game(game, attacker, player);

                std::vector<achievement_e> earned;
                game.achievement_earned_callback_fn = [&](achievement_e earned_achievement) {
                        earned.push_back(earned_achievement);
                };

                const auto first_threshold = first_threshold_or_one(game_config::get_achievement(achievement));
                auto& progress = game.get_player(PLAYER_1).player_stats.achievement_event_counts[achievement];
                progress = first_threshold - 1;

                apply_might_event_condition(game, attacker, defender, achievement);
                game.accept_battle_results();

                expect_eq(progress, first_threshold,
                          game_config::get_achievement(achievement).name + " should increment might event progress exactly");
                expect_eq(earned_count(earned, achievement), size_t(1),
                          game_config::get_achievement(achievement).name + " callback should fire exactly once when crossing the first tier threshold");
                expect_eq(earned.size(), size_t(1),
                          game_config::get_achievement(achievement).name + " should be the only achievement callback in this might event scenario");
        }
}

void test_all_might_achievements_have_specific_test_case() {
        std::set<achievement_e> covered;
        for(const auto& test_case : small_might_combat_stat_achievement_cases())
                covered.insert(test_case.achievement);
        for(const auto& test_case : might_combat_stat_achievement_cases())
                covered.insert(test_case.achievement);
        for(const auto achievement : might_event_achievement_cases())
                covered.insert(achievement);

        for(const auto& achievement : game_config::get_achievements()) {
                if(achievement.type != ACHIEVEMENT_TYPE_MIGHT)
                        continue;

                expect_true(covered.count(achievement.id) == 1, achievement.name + " should have an explicit might achievement test case");
        }
}

} // namespace

int run_combat_achievement_tests() {
        failures = 0;
        test_storm_drinker_lightning_spell_damage_cast_progress_and_callback();
        test_magic_combat_stat_progress_and_callbacks();
        test_magic_event_progress_and_callbacks();
        test_all_magic_achievements_have_specific_test_case();
        test_might_combat_stat_progress_and_callbacks();
        test_might_event_progress_and_callbacks();
        test_all_might_achievements_have_specific_test_case();
        return failures;
}
