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

struct elemental_spell_damage_achievement_case_t {
        spell_e spell;
        unit_type_e target_unit;
        uint32_t combat_stats_t::* stat;
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

std::vector<elemental_spell_damage_achievement_case_t> elemental_spell_damage_achievement_cases() {
        return {
                {SPELL_FIRE_BALL, UNIT_SKELETON, &combat_stats_t::fire_spell_damage, ACHIEVEMENT_IN_FLAMES},
                {SPELL_FROST_RAY, UNIT_EFREETI, &combat_stats_t::frost_spell_damage, ACHIEVEMENT_WINTER_IS_COMING},
                {SPELL_LIGHTNING_BOLT, UNIT_SKELETON, &combat_stats_t::lightning_spell_damage, ACHIEVEMENT_STORM_DRINKER},
                {SPELL_METEOR_SHOWER, UNIT_SKELETON, &combat_stats_t::earth_spell_damage, ACHIEVEMENT_EARTHSHAKER},
                {SPELL_DEATH_COIL, UNIT_SKELETON, &combat_stats_t::chaos_spell_damage, ACHIEVEMENT_CHAOS_THEORY},
                {SPELL_HOLY_SHOUT, UNIT_SKELETON, &combat_stats_t::holy_spell_damage, ACHIEVEMENT_BLINDED_BY_THE_LIGHT},
        };
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

void test_elemental_spell_damage_cast_progress_and_callbacks() {
        for(const auto& test_case : elemental_spell_damage_achievement_cases()) {
                game_t game;
                hero_t attacker;
                player_t player;
                initialize_combat_achievement_game(game, attacker, player);

                attacker.power = 200;
                attacker.mana = 1000;
                attacker.spellbook.push_back(test_case.spell);

                auto& target = game.battle.defending_army.troops[0];
                target = make_battlefield_unit(test_case.target_unit, 1000, false, 1, 8, 5);
                place_battlefield_unit(game.battle, target);

                std::vector<achievement_e> earned;
                game.achievement_earned_callback_fn = [&](achievement_e achievement) {
                        earned.push_back(achievement);
                };

                const auto first_threshold = first_threshold_or_one(game_config::get_achievement(test_case.achievement));
                auto& progress = game.get_player(PLAYER_1).player_stats.cumulative_combat_stats.*(test_case.stat);
                progress = 0;

                expect_true(game.battle.cast_spell(&attacker, test_case.spell, target.x, target.y, &target) == SPELL_RESULT_OK,
                            game_config::get_achievement(test_case.achievement).name + " spell scenario should cast successfully");
                const auto spell_damage = game.battle.attacker_stats.*(test_case.stat);
                expect_true(spell_damage > 0,
                            game_config::get_achievement(test_case.achievement).name + " spell scenario should record typed spell damage");

                progress = first_threshold > spell_damage ? first_threshold - spell_damage : 0;
                const auto expected_progress = progress + spell_damage;
                game.accept_battle_results();

                expect_eq(progress, expected_progress,
                          game_config::get_achievement(test_case.achievement).name + " should add actual cast spell damage to cumulative typed spell damage");
                expect_eq(earned_count(earned, test_case.achievement), size_t(1),
                          game_config::get_achievement(test_case.achievement).name + " callback should fire exactly once after a concrete spell cast crosses the first tier threshold");
                expect_true(!earned.empty(),
                            game_config::get_achievement(test_case.achievement).name + " concrete spell scenario should fire an achievement callback");
        }
}

void set_achievement_event_progress_to_before_first_tier(game_t& game, achievement_e achievement) {
        const auto first_threshold = first_threshold_or_one(game_config::get_achievement(achievement));
        game.get_player(PLAYER_1).player_stats.achievement_event_counts[achievement] = first_threshold - 1;
}

void expect_concrete_magic_event_callback(const std::vector<achievement_e>& earned, achievement_e achievement) {
        expect_eq(earned_count(earned, achievement), size_t(1),
                  game_config::get_achievement(achievement).name + " concrete combat scenario should fire exactly one callback");
}

void test_i_dream_of_genie_unit_spellcast_progress_and_callback() {
        game_t game;
        hero_t attacker;
        player_t player;
        initialize_combat_achievement_game(game, attacker, player);

        game.battle.attacking_army.hero = &attacker;
        auto& genie = game.battle.attacking_army.troops[0];
        genie = make_battlefield_unit(UNIT_GENIE, 1, true, 1, 3, 5);
        genie.spell_casts_remaining = 1;
        place_battlefield_unit(game.battle, genie);
        auto& ally = game.battle.attacking_army.troops[1];
        ally = make_battlefield_unit(UNIT_INFANTRYMAN, 10, true, 2, 4, 5);
        place_battlefield_unit(game.battle, ally);

        std::vector<achievement_e> earned;
        game.achievement_earned_callback_fn = [&](achievement_e achievement) {
                earned.push_back(achievement);
        };

        const auto first_threshold = first_threshold_or_one(game_config::get_achievement(ACHIEVEMENT_I_DREAM_OF_GENIE));
        auto& progress = game.get_player(PLAYER_1).player_stats.cumulative_combat_stats.genie_friendly_spells_cast;
        progress = first_threshold - 1;

        expect_true(game.battle.cast_spell(&genie, SPELL_BLESS, ally.x, ally.y, &ally) == SPELL_RESULT_OK,
                    "Genie should successfully cast a friendly Bless spell");
        expect_eq(game.battle.attacker_stats.genie_friendly_spells_cast, static_cast<uint16_t>(1),
                  "Genie friendly spellcast should be recorded on attacker combat stats");

        game.accept_battle_results();

        expect_eq(progress, first_threshold,
                  "I Dream of Genie should add the concrete Genie spellcast to cumulative progress");
        expect_eq(earned_count(earned, ACHIEVEMENT_I_DREAM_OF_GENIE), size_t(1),
                  "I Dream of Genie concrete spellcast should fire exactly one callback");
}

void test_special_magic_achievement_concrete_spell_scenarios() {
        {
                game_t game;
                hero_t attacker;
                player_t player;
                initialize_combat_achievement_game(game, attacker, player);
                game.battle.result = BATTLE_ATTACKER_VICTORY;
                attacker.power = 200;
                attacker.mana = 1000;
                attacker.spellbook.push_back(SPELL_METEOR_SHOWER);

                auto& ally = game.battle.attacking_army.troops[0];
                ally = make_battlefield_unit(UNIT_INFANTRYMAN, 1000, true, 1, 7, 5);
                place_battlefield_unit(game.battle, ally);
                auto& target = game.battle.defending_army.troops[0];
                target = make_battlefield_unit(UNIT_SKELETON, 1000, false, 2, 8, 5);
                place_battlefield_unit(game.battle, target);

                std::vector<achievement_e> earned;
                game.achievement_earned_callback_fn = [&](achievement_e achievement) { earned.push_back(achievement); };
                set_achievement_event_progress_to_before_first_tier(game, ACHIEVEMENT_FRIENDLY_FIRE);

                expect_true(game.battle.cast_spell(&attacker, SPELL_METEOR_SHOWER, target.x, target.y, &target) == SPELL_RESULT_OK,
                            "Meteor Shower should successfully hit both enemy and adjacent friendly stacks");
                expect_true(game.battle.attacker_stats.friendly_fire_spell_hits > 0,
                            "Meteor Shower should record friendly-fire spell hits");
                game.accept_battle_results();
                expect_concrete_magic_event_callback(earned, ACHIEVEMENT_FRIENDLY_FIRE);
        }

        {
                game_t game;
                hero_t attacker;
                player_t player;
                initialize_combat_achievement_game(game, attacker, player);
                game.battle.result = BATTLE_ATTACKER_VICTORY;
                attacker.power = 200;
                attacker.mana = 1000;
                attacker.spellbook.push_back(SPELL_LIGHTNING_BOLT);

                auto& target = game.battle.defending_army.troops[0];
                target = make_battlefield_unit(UNIT_SKELETON, 1000, false, 1, 8, 5);
                place_battlefield_unit(game.battle, target);

                std::vector<achievement_e> earned;
                game.achievement_earned_callback_fn = [&](achievement_e achievement) { earned.push_back(achievement); };
                set_achievement_event_progress_to_before_first_tier(game, ACHIEVEMENT_BRAINS_BEATS_BRAWN);

                expect_true(game.battle.cast_spell(&attacker, SPELL_LIGHTNING_BOLT, target.x, target.y, &target) == SPELL_RESULT_OK,
                            "Lightning Bolt should successfully deal spell-only combat damage");
                expect_true(game.battle.attacker_stats.total_spell_damage_done > 0,
                            "Lightning Bolt should record spell damage for the attacker");
                expect_eq(game.battle.attacker_stats.total_physical_damage_done, static_cast<uint64_t>(0),
                          "Spell-only victory scenario should not record physical damage");
                game.accept_battle_results();
                expect_concrete_magic_event_callback(earned, ACHIEVEMENT_BRAINS_BEATS_BRAWN);
        }

        {
                game_t game;
                hero_t attacker;
                player_t player;
                initialize_combat_achievement_game(game, attacker, player);
                attacker.power = 200;
                attacker.mana = 1000;
                attacker.spellbook.push_back(SPELL_FROST_RAY);

                auto& target = game.battle.defending_army.troops[0];
                target = make_battlefield_unit(UNIT_EFREETI, 1, false, 1, 8, 5);
                place_battlefield_unit(game.battle, target);

                std::vector<achievement_e> earned;
                game.achievement_earned_callback_fn = [&](achievement_e achievement) { earned.push_back(achievement); };
                set_achievement_event_progress_to_before_first_tier(game, ACHIEVEMENT_QUENCHING_THE_FLAME);

                expect_true(game.battle.cast_spell(&attacker, SPELL_FROST_RAY, target.x, target.y, &target) == SPELL_RESULT_OK,
                            "Frost Ray should successfully target a Fire-immune Efreeti stack");
                expect_eq(game.battle.attacker_stats.frost_kill_fire_immune, static_cast<uint16_t>(1),
                          "Frost Ray should record killing a Fire-immune stack with Frost damage");
                game.accept_battle_results();
                expect_concrete_magic_event_callback(earned, ACHIEVEMENT_QUENCHING_THE_FLAME);
        }

        {
                game_t game;
                hero_t attacker;
                player_t player;
                initialize_combat_achievement_game(game, attacker, player);
                attacker.power = 200;
                attacker.mana = 1000;
                attacker.spellbook.push_back(SPELL_METEOR_SHOWER);

                auto& target = game.battle.defending_army.troops[0];
                target = make_battlefield_unit(UNIT_SKELETON, 75, false, 1, 8, 5);
                place_battlefield_unit(game.battle, target);

                std::vector<achievement_e> earned;
                game.achievement_earned_callback_fn = [&](achievement_e achievement) { earned.push_back(achievement); };
                set_achievement_event_progress_to_before_first_tier(game, ACHIEVEMENT_METEORIC_RISE);

                expect_true(game.battle.cast_spell(&attacker, SPELL_METEOR_SHOWER, target.x, target.y, &target) == SPELL_RESULT_OK,
                            "Meteor Shower should successfully hit a 75-unit enemy stack");
                expect_eq(game.battle.attacker_stats.meteor_shower_75kills, static_cast<uint16_t>(1),
                          "Meteor Shower should record killing at least 75 units in one cast");
                game.accept_battle_results();
                expect_concrete_magic_event_callback(earned, ACHIEVEMENT_METEORIC_RISE);
        }

        {
                game_t game;
                hero_t attacker;
                hero_t defender;
                player_t player;
                initialize_combat_achievement_game(game, attacker, player);
                defender.hero_class = HERO_CLASS_NECROMANCER;
                game.battle.defending_hero = &defender;
                attacker.power = 200;
                attacker.mana = 1000;
                attacker.spellbook.push_back(SPELL_IMPLOSION);

                auto& target = game.battle.defending_army.troops[0];
                target = make_battlefield_unit(UNIT_SKELETON, 1, false, 1, 8, 5);
                place_battlefield_unit(game.battle, target);

                std::vector<achievement_e> earned;
                game.achievement_earned_callback_fn = [&](achievement_e achievement) { earned.push_back(achievement); };
                set_achievement_event_progress_to_before_first_tier(game, ACHIEVEMENT_GRAVE_MISTAKE);

                expect_true(game.battle.cast_spell(&attacker, SPELL_IMPLOSION, target.x, target.y, &target) == SPELL_RESULT_OK,
                            "Implosion should successfully kill the Necromancer defender's last stack");
                expect_eq(game.battle.attacker_stats.implosion_finish_necromancer, static_cast<uint16_t>(1),
                          "Implosion should record finishing off a Necromancer hero");
                game.accept_battle_results();
                expect_concrete_magic_event_callback(earned, ACHIEVEMENT_GRAVE_MISTAKE);
        }


        {
                game_t game;
                hero_t attacker;
                player_t player;
                initialize_combat_achievement_game(game, attacker, player);
                attacker.power = 10;
                attacker.mana = 1000;
                attacker.spellbook = {SPELL_SWIFTNESS, SPELL_ARCANE_BOLT, SPELL_BLESS, SPELL_FIRE_BALL, SPELL_DEATH_COIL, SPELL_SLOW};

                auto& ally = game.battle.attacking_army.troops[0];
                ally = make_battlefield_unit(UNIT_INFANTRYMAN, 1000, true, 1, 3, 5);
                place_battlefield_unit(game.battle, ally);
                auto& target = game.battle.defending_army.troops[0];
                target = make_battlefield_unit(UNIT_SKELETON, 10000, false, 2, 8, 5);
                place_battlefield_unit(game.battle, target);

                std::vector<achievement_e> earned;
                game.achievement_earned_callback_fn = [&](achievement_e achievement) { earned.push_back(achievement); };
                set_achievement_event_progress_to_before_first_tier(game, ACHIEVEMENT_CREATIVE_CASTER);
                set_achievement_event_progress_to_before_first_tier(game, ACHIEVEMENT_MASTER_OF_THE_ARCANE);

                expect_true(game.battle.cast_spell(&attacker, SPELL_SWIFTNESS, ally.x, ally.y, &ally) == SPELL_RESULT_OK,
                            "Swiftness should cast as the Nature school spell");
                game.battle.attacking_hero_used_cast = false;
                expect_true(game.battle.cast_spell(&attacker, SPELL_ARCANE_BOLT, target.x, target.y, &target) == SPELL_RESULT_OK,
                            "Arcane Bolt should cast as the Arcane school spell");
                game.battle.attacking_hero_used_cast = false;
                expect_true(game.battle.cast_spell(&attacker, SPELL_BLESS, ally.x, ally.y, &ally) == SPELL_RESULT_OK,
                            "Bless should cast as the Holy school spell");
                game.battle.attacking_hero_used_cast = false;
                expect_true(game.battle.cast_spell(&attacker, SPELL_FIRE_BALL, target.x, target.y, &target) == SPELL_RESULT_OK,
                            "Fire Ball should cast as the Destruction school spell");
                game.battle.attacking_hero_used_cast = false;
                expect_true(game.battle.cast_spell(&attacker, SPELL_DEATH_COIL, target.x, target.y, &target) == SPELL_RESULT_OK,
                            "Death Coil should cast as the Death school spell");
                game.battle.attacking_hero_used_cast = false;
                expect_true(game.battle.cast_spell(&attacker, SPELL_SLOW, target.x, target.y, &target) == SPELL_RESULT_OK,
                            "Slow should cast as a sixth unique spell");
                expect_eq(game.battle.attacker_stats.unique_spells_cast, static_cast<uint16_t>(6),
                          "Six distinct concrete spell casts should be recorded");
                expect_eq(game.battle.attacker_stats.total_spells_cast_nature, static_cast<uint16_t>(1),
                          "Nature spell cast should be recorded");
                expect_eq(game.battle.attacker_stats.total_spells_cast_arcane, static_cast<uint16_t>(2),
                          "Arcane spell casts should be recorded");
                expect_eq(game.battle.attacker_stats.total_spells_cast_holy, static_cast<uint16_t>(1),
                          "Holy spell cast should be recorded");
                expect_eq(game.battle.attacker_stats.total_spells_cast_destruction, static_cast<uint16_t>(1),
                          "Destruction spell cast should be recorded");
                expect_eq(game.battle.attacker_stats.total_spells_cast_death, static_cast<uint16_t>(1),
                          "Death spell cast should be recorded");
                game.accept_battle_results();
                expect_concrete_magic_event_callback(earned, ACHIEVEMENT_CREATIVE_CASTER);
                expect_concrete_magic_event_callback(earned, ACHIEVEMENT_MASTER_OF_THE_ARCANE);
        }

        {
                game_t game;
                hero_t attacker;
                player_t player;
                initialize_combat_achievement_game(game, attacker, player);
                attacker.power = 200;
                attacker.mana = 1000;
                attacker.spellbook.push_back(SPELL_ARMAGEDDON);

                auto& ally = game.battle.attacking_army.troops[0];
                ally = make_battlefield_unit(UNIT_INFANTRYMAN, 1000, true, 1, 3, 5);
                place_battlefield_unit(game.battle, ally);
                auto& target = game.battle.defending_army.troops[0];
                target = make_battlefield_unit(UNIT_SKELETON, 1000, false, 2, 8, 5);
                place_battlefield_unit(game.battle, target);

                std::vector<achievement_e> earned;
                game.achievement_earned_callback_fn = [&](achievement_e achievement) { earned.push_back(achievement); };
                set_achievement_event_progress_to_before_first_tier(game, ACHIEVEMENT_WATCH_THE_WORLD_BURN);

                expect_true(game.battle.cast_spell(&attacker, SPELL_ARMAGEDDON) == SPELL_RESULT_OK,
                            "Armageddon should successfully hit battlefield units");
                expect_eq(game.battle.attacker_stats.armageddon_5000_damage, static_cast<uint16_t>(1),
                          "Armageddon should record dealing at least 5000 damage in one cast");
                game.accept_battle_results();
                expect_concrete_magic_event_callback(earned, ACHIEVEMENT_WATCH_THE_WORLD_BURN);
        }
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


void prepare_adjacent_melee_attack(game_t& game, hero_t& attacker_hero, battlefield_unit_t& attacker, battlefield_unit_t& defender) {
        game.battle.attacking_army.hero = &attacker_hero;
        game.battle.defending_army.hero = nullptr;
        place_battlefield_unit(game.battle, attacker);
        place_battlefield_unit(game.battle, defender);
}

void test_reaper_one_punch_and_titanic_smackdown_concrete_melee_attack() {
        game_t game;
        hero_t attacker_hero;
        player_t player;
        initialize_combat_achievement_game(game, attacker_hero, player);

        auto& attacker = game.battle.attacking_army.troops[0];
        attacker = make_battlefield_unit(UNIT_TITAN, 1000, true, 1, 7, 5);
        auto& defender = game.battle.defending_army.troops[0];
        defender = make_battlefield_unit(UNIT_SKELETON, 1, false, 2, 8, 5);
        prepare_adjacent_melee_attack(game, attacker_hero, attacker, defender);

        std::vector<achievement_e> earned;
        game.achievement_earned_callback_fn = [&](achievement_e achievement) { earned.push_back(achievement); };

        auto& cumulative = game.get_player(PLAYER_1).player_stats.cumulative_combat_stats;
        cumulative.total_units_killed = first_threshold_or_one(game_config::get_achievement(ACHIEVEMENT_THE_REAPER)) - 1;
        cumulative.max_damage_single_melee_attack = 0;
        set_achievement_event_progress_to_before_first_tier(game, ACHIEVEMENT_TITANIC_SMACKDOWN);
        set_achievement_event_progress_to_before_first_tier(game, ACHIEVEMENT_MAXIMUM_OVERKILL);

        expect_true(game.battle.attack_unit(attacker, &defender, false, attacker.x, attacker.y),
                    "Titan should successfully make a concrete adjacent melee attack");
        expect_eq(game.battle.attacker_stats.total_units_killed, static_cast<uint32_t>(1),
                  "Titan melee attack should record one killed unit");
        expect_true(game.battle.attacker_stats.max_damage_single_melee_attack >= 5000,
                    "Titan melee attack should record at least 5000 melee damage");
        expect_eq(game.battle.attacker_stats.titanic_smackdowns, static_cast<uint16_t>(1),
                  "Titan melee attack should record a Titanic Smackdown event");
        expect_eq(game.battle.attacker_stats.maximum_overkill_hits, static_cast<uint16_t>(1),
                  "Titan melee attack should record a Maximum Overkill event");

        const auto expected_one_punch_progress = game.battle.attacker_stats.max_damage_single_melee_attack;
        game.accept_battle_results();

        expect_eq(cumulative.total_units_killed, first_threshold_or_one(game_config::get_achievement(ACHIEVEMENT_THE_REAPER)),
                  "The Reaper should add concrete melee kills to cumulative progress");
        expect_eq(cumulative.max_damage_single_melee_attack, expected_one_punch_progress,
                  "One Punch Hero should store the concrete melee attack's damage");
        expect_eq(earned_count(earned, ACHIEVEMENT_THE_REAPER), size_t(1),
                  "The Reaper concrete melee attack should fire exactly one callback");
        expect_eq(earned_count(earned, ACHIEVEMENT_ONE_PUNCH_HERO), size_t(1),
                  "One Punch Hero concrete melee attack should fire exactly one callback");
        expect_eq(earned_count(earned, ACHIEVEMENT_TITANIC_SMACKDOWN), size_t(1),
                  "Titanic Smackdown concrete melee attack should fire exactly one callback");
        expect_eq(earned_count(earned, ACHIEVEMENT_MAXIMUM_OVERKILL), size_t(1),
                  "Maximum Overkill concrete melee attack should fire exactly one callback");
}

void test_bigger_they_are_concrete_tier1_melee_kill() {
        game_t game;
        hero_t attacker_hero;
        player_t player;
        initialize_combat_achievement_game(game, attacker_hero, player);

        auto& attacker = game.battle.attacking_army.troops[0];
        attacker = make_battlefield_unit(UNIT_INFANTRYMAN, 1000, true, 1, 7, 5);
        auto& defender = game.battle.defending_army.troops[0];
        defender = make_battlefield_unit(UNIT_BEHEMOTH, 1, false, 2, 8, 5);
        prepare_adjacent_melee_attack(game, attacker_hero, attacker, defender);

        std::vector<achievement_e> earned;
        game.achievement_earned_callback_fn = [&](achievement_e achievement) { earned.push_back(achievement); };
        set_achievement_event_progress_to_before_first_tier(game, ACHIEVEMENT_THE_BIGGER_THEY_ARE);

        expect_true(game.battle.attack_unit(attacker, &defender, false, attacker.x, attacker.y),
                    "Tier-1-only army should successfully make a concrete melee attack against a tier-6 stack");
        expect_eq(game.battle.attacker_stats.tier6_killed_by_tier1_army, static_cast<uint16_t>(1),
                  "Tier-1-only melee attack should record killing a tier-6 stack");
        game.accept_battle_results();
        expect_eq(earned_count(earned, ACHIEVEMENT_THE_BIGGER_THEY_ARE), size_t(1),
                  "The Bigger They Are concrete melee attack should fire exactly one callback");
}

void test_crunching_numbers_concrete_exact_melee_kill() {
        game_t game;
        hero_t attacker_hero;
        player_t player;
        initialize_combat_achievement_game(game, attacker_hero, player);

        auto& attacker = game.battle.attacking_army.troops[0];
        attacker = make_battlefield_unit(UNIT_BISHOP, 1, true, 1, 7, 5);
        auto& defender = game.battle.defending_army.troops[0];
        defender = make_battlefield_unit(UNIT_SKELETON, 1, false, 2, 8, 5);
        prepare_adjacent_melee_attack(game, attacker_hero, attacker, defender);

        const auto exact_damage = game.battle.calculate_damage(attacker, defender, false, false);
        const auto defender_hp = game.battle.get_unit_adjusted_hp(defender);
        defender.stack_size = static_cast<uint16_t>((exact_damage + defender_hp - 1) / defender_hp);
        defender.unit_health = exact_damage % defender_hp;
        if(defender.unit_health == 0)
                defender.unit_health = defender_hp;

        std::vector<achievement_e> earned;
        game.achievement_earned_callback_fn = [&](achievement_e achievement) { earned.push_back(achievement); };
        set_achievement_event_progress_to_before_first_tier(game, ACHIEVEMENT_CRUNCHING_NUMBERS);

        expect_true(game.battle.attack_unit(attacker, &defender, false, attacker.x, attacker.y),
                    "Bishop should successfully make a concrete exact-damage melee attack");
        expect_eq(game.battle.attacker_stats.exact_melee_kills, static_cast<uint16_t>(1),
                  "Exact-damage melee attack should record a Crunching Numbers event");
        game.accept_battle_results();
        expect_eq(earned_count(earned, ACHIEVEMENT_CRUNCHING_NUMBERS), size_t(1),
                  "Crunching Numbers concrete melee attack should fire exactly one callback");
}

void test_shot_through_the_heart_and_high_roller_concrete_ranged_critical() {
        game_t game;
        hero_t attacker_hero;
        player_t player;
        initialize_combat_achievement_game(game, attacker_hero, player);
        attacker_hero.skills[0] = {SKILL_LUCK, 5};

        auto& attacker = game.battle.attacking_army.troops[0];
        attacker = make_battlefield_unit(UNIT_ORC, 10, true, 1, 3, 5);
        attacker.add_buff(BUFF_INCREASED_LUCK, -1, 255);
        auto& defender = game.battle.defending_army.troops[0];
        defender = make_battlefield_unit(UNIT_SKELETON, 100, false, 2, 8, 5);
        prepare_adjacent_melee_attack(game, attacker_hero, attacker, defender);

        std::vector<achievement_e> earned;
        game.achievement_earned_callback_fn = [&](achievement_e achievement) { earned.push_back(achievement); };

        auto& cumulative = game.get_player(PLAYER_1).player_stats.cumulative_combat_stats.total_critical_hits;
        cumulative = first_threshold_or_one(game_config::get_achievement(ACHIEVEMENT_SHOT_THROUGH_THE_HEART)) - 1;
        game.get_player(PLAYER_1).player_stats.achievement_event_counts[ACHIEVEMENT_HIGH_ROLLER] = 0;

        expect_true(game.battle.attack_unit(attacker, &defender, true, attacker.x, attacker.y),
                    "High-luck Orc should successfully make a concrete ranged attack");
        expect_eq(game.battle.attacker_stats.total_attacks_made, static_cast<uint16_t>(1),
                  "Concrete ranged attack should record one attack made");
        expect_eq(game.battle.attacker_stats.total_critical_hits, static_cast<uint16_t>(1),
                  "High-luck concrete ranged attack should record one critical hit");

        game.accept_battle_results();

        expect_eq(cumulative, first_threshold_or_one(game_config::get_achievement(ACHIEVEMENT_SHOT_THROUGH_THE_HEART)),
                  "Shot Through the Heart should add the concrete critical hit to cumulative progress");
        expect_eq(earned_count(earned, ACHIEVEMENT_SHOT_THROUGH_THE_HEART), size_t(1),
                  "Shot Through the Heart concrete critical hit should fire exactly one callback");
        expect_eq(earned_count(earned, ACHIEVEMENT_HIGH_ROLLER), size_t(1),
                  "High Roller concrete all-critical battle should fire exactly one callback");
}

void test_unbreakable_concrete_attacks_received() {
        game_t game;
        hero_t attacker_hero;
        player_t player;
        initialize_combat_achievement_game(game, attacker_hero, player);
        game.battle.result = BATTLE_ATTACKER_VICTORY;

        auto& attacker = game.battle.attacking_army.troops[0];
        attacker = make_battlefield_unit(UNIT_CRUSADER, 100, true, 1, 7, 5);
        auto& defender = game.battle.defending_army.troops[0];
        defender = make_battlefield_unit(UNIT_SKELETON, 1, false, 2, 8, 5);
        prepare_adjacent_melee_attack(game, attacker_hero, attacker, defender);

        std::vector<achievement_e> earned;
        game.achievement_earned_callback_fn = [&](achievement_e achievement) { earned.push_back(achievement); };
        game.get_player(PLAYER_1).player_stats.achievement_event_counts[ACHIEVEMENT_FLAWLESS_EXECUTION] = 1;
        set_achievement_event_progress_to_before_first_tier(game, ACHIEVEMENT_UNBREAKABLE);

        for(int i = 0; i < 10; ++i) {
                defender.stack_size = 1;
                defender.unit_health = game.battle.get_unit_adjusted_hp(defender);
                place_battlefield_unit(game.battle, defender);
                expect_true(game.battle.attack_unit(defender, &attacker, false, defender.x, defender.y),
                            "Defender should successfully make a concrete melee attack received by the attacker");
                defender.has_moved = false;
                defender.has_waited = false;
        }
        expect_true(game.battle.attacker_stats.total_attacks_received >= 10,
                    "Concrete defender attacks should record at least ten attacks received by the attacker");

        game.accept_battle_results();
        expect_eq(earned_count(earned, ACHIEVEMENT_UNBREAKABLE), size_t(1),
                  "Unbreakable concrete attacks-received scenario should fire exactly one callback");
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
        test_elemental_spell_damage_cast_progress_and_callbacks();
        test_i_dream_of_genie_unit_spellcast_progress_and_callback();
        test_special_magic_achievement_concrete_spell_scenarios();
        test_magic_combat_stat_progress_and_callbacks();
        test_magic_event_progress_and_callbacks();
        test_all_magic_achievements_have_specific_test_case();
        test_might_combat_stat_progress_and_callbacks();
        test_reaper_one_punch_and_titanic_smackdown_concrete_melee_attack();
        test_bigger_they_are_concrete_tier1_melee_kill();
        test_crunching_numbers_concrete_exact_melee_kill();
        test_shot_through_the_heart_and_high_roller_concrete_ranged_critical();
        test_unbreakable_concrete_attacks_received();
        test_might_event_progress_and_callbacks();
        test_all_might_achievements_have_specific_test_case();
        return failures;
}
