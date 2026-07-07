#include "core/game.h"
#include "core/game_config.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <set>
#include <string>
#include <vector>

#include <QBitArray>

int run_combat_achievement_tests();

namespace {
int failures = 0;

struct resource_achievement_case_t {
        resource_e resource_type;
        achievement_e achievement;
};

struct terrain_traversal_achievement_case_t {
        terrain_type_e terrain_type;
        achievement_e achievement;
        uint32_t movement_stats_t::* stat;
};

struct full_map_reveal_achievement_case_t {
        uint width;
        uint height;
        achievement_e achievement;
        uint16_t exploration_stats_t::* stat;
};

struct construction_achievement_case_t {
        town_type_e town_type;
        building_e building;
        std::vector<building_e> already_built;
        achievement_e achievement;
        uint16_t construction_stats_t::* stat;
};

struct recruitment_achievement_case_t {
        unit_type_e unit_type;
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

void initialize_visible_game(game_t& game, uint width, uint height) {
        game.map.width = width;
        game.map.height = height;
        game.map.tiles.assign(width * height, {});
        game.map.monster_guarded_cache = QBitArray(width * height, false);
        game.map.monster_guarded_cache_valid = true;

        for(auto& tile : game.map.tiles) {
                tile.terrain_type = TERRAIN_GRASS;
                tile.passability = 1;
        }

        player_t player;
        player.player_number = PLAYER_1;
        player.is_human = true;
        player.tile_visibility = QBitArray(width * height, true);
        player.tile_observability = QBitArray(width * height, true);
        game.players.push_back(player);
        game.remove_interactable_object_callback_fn = [](interactable_object_t*, bool) {};
}

hero_t make_hero(int x, int y, int movement_points = 1500) {
        hero_t hero;
        hero.player = PLAYER_1;
        hero.x = static_cast<uint8_t>(x);
        hero.y = static_cast<uint8_t>(y);
        hero.movement_points = movement_points;
        hero.maximum_daily_movement_points = movement_points;
        return hero;
}

map_resource_t* add_resource_pickup(adventure_map_t& map, int x, int y, resource_e resource_type, uint16_t quantity) {
        auto* resource = static_cast<map_resource_t*>(interactable_object_t::make_new_object(OBJECT_RESOURCE));
        resource->x = x;
        resource->y = y;
        resource->resource_type = resource_type;
        resource->min_quantity = quantity;
        resource->max_quantity = quantity;
        map.objects.push_back(resource);
        map.get_tile(x, y).interactable_object = static_cast<uint16_t>(map.objects.size());
        return resource;
}


mine_t* add_mine(adventure_map_t& map, int x, int y, resource_e resource_type, player_e owner) {
        auto* mine = static_cast<mine_t*>(interactable_object_t::make_new_object(OBJECT_MINE));
        mine->x = x;
        mine->y = y;
        mine->mine_type = resource_type;
        mine->owner = owner;
        map.objects.push_back(mine);
        map.get_tile(x, y).interactable_object = static_cast<uint16_t>(map.objects.size());
        return mine;
}

interactable_object_t* add_object(adventure_map_t& map, int x, int y, interactable_object_e object_type) {
        auto* object = interactable_object_t::make_new_object(object_type);
        object->x = x;
        object->y = y;
        map.objects.push_back(object);
        map.get_tile(x, y).interactable_object = static_cast<uint16_t>(map.objects.size());
        return object;
}

teleporter_t* add_teleporter(adventure_map_t& map, int x, int y, uint8_t color, teleporter_t::teleporter_type_e teleporter_type) {
        auto* teleporter = static_cast<teleporter_t*>(add_object(map, x, y, OBJECT_TELEPORTER));
        teleporter->color = color;
        teleporter->teleporter_type = teleporter_type;
        return teleporter;
}

eye_of_the_magi_t* add_eye_of_the_magi(adventure_map_t& map, int x, int y, uint8_t reveal_radius) {
        auto* eye = static_cast<eye_of_the_magi_t*>(add_object(map, x, y, OBJECT_EYE_OF_THE_MAGI));
        eye->reveal_radius = reveal_radius;
        return eye;
}

void hide_map_for_player(game_t& game, player_e player) {
        auto& player_state = game.get_player(player);
        player_state.tile_visibility.fill(false);
        player_state.tile_observability.fill(false);
}

size_t earned_count(const std::vector<achievement_e>& earned, achievement_e achievement) {
        return static_cast<size_t>(std::count(earned.begin(), earned.end(), achievement));
}

void give_player_abundant_resources(game_t& game, player_e player) {
        auto& resources = game.get_player(player).resources;
        resources.set_value_for_type(RESOURCE_GOLD, 1000000);
        resources.set_value_for_type(RESOURCE_WOOD, 1000000);
        resources.set_value_for_type(RESOURCE_ORE, 1000000);
        resources.set_value_for_type(RESOURCE_GEMS, 1000000);
        resources.set_value_for_type(RESOURCE_MERCURY, 1000000);
        resources.set_value_for_type(RESOURCE_CRYSTALS, 1000000);
        resources.set_value_for_type(RESOURCE_SULFUR, 1000000);
}

town_t make_buildable_town(town_type_e town_type, building_e building, const std::vector<building_e>& already_built) {
        town_t town;
        town.player = PLAYER_1;
        town.town_type = town_type;
        town.available_buildings = already_built;
        town.available_buildings.push_back(building);
        town.built_buildings = already_built;
        return town;
}

uint32_t expected_daily_mine_income(resource_e resource_type) {
        if(resource_type == RESOURCE_GOLD)
                return 1000;
        if(resource_type == RESOURCE_WOOD || resource_type == RESOURCE_ORE)
                return 2;
        return 1;
}

std::vector<resource_achievement_case_t> adventure_map_resource_pickup_achievement_cases() {
        return {
                {RESOURCE_GOLD, ACHIEVEMENT_MONEY_ON_MY_MIND},
                {RESOURCE_GEMS, ACHIEVEMENT_SHINE_BRIGHT_LIKE_A_DIAMOND},
                {RESOURCE_MERCURY, ACHIEVEMENT_ITS_LIKE_WORKING_WITH_MERCURY},
                {RESOURCE_CRYSTALS, ACHIEVEMENT_CRYSTAL_CLEAR},
                {RESOURCE_WOOD, ACHIEVEMENT_TIMBER},
                {RESOURCE_ORE, ACHIEVEMENT_OREIGINAL_RECIPE},
        };
}

std::vector<resource_achievement_case_t> mine_income_achievement_cases() {
        return {
                {RESOURCE_ORE, ACHIEVEMENT_ORESON_WELLS},
                {RESOURCE_WOOD, ACHIEVEMENT_WOODY_HARRELSON},
                {RESOURCE_GEMS, ACHIEVEMENT_GEM_CARREY},
                {RESOURCE_MERCURY, ACHIEVEMENT_FREDDIE_MERCURY},
                {RESOURCE_CRYSTALS, ACHIEVEMENT_BILLY_CRYSTAL},
                {RESOURCE_GOLD, ACHIEVEMENT_JEFF_GOLDBLUM},
        };
}

std::vector<terrain_traversal_achievement_case_t> terrain_traversal_achievement_cases() {
        return {
                {TERRAIN_WATER, ACHIEVEMENT_SAIL_AWAY, &movement_stats_t::total_hero_steps_taken_on_water},
                {TERRAIN_GRASS, ACHIEVEMENT_TAKE_ME_HOME, &movement_stats_t::total_hero_steps_taken_on_grass},
                {TERRAIN_DIRT, ACHIEVEMENT_DIRT_ROAD_ANTHEM, &movement_stats_t::total_hero_steps_taken_on_dirt},
                {TERRAIN_WASTELAND, ACHIEVEMENT_CRACKS_IN_THE_PAVEMENT, &movement_stats_t::total_hero_steps_taken_on_wasteland},
                {TERRAIN_LAVA, ACHIEVEMENT_HIGHWAY_TO_HELL, &movement_stats_t::total_hero_steps_taken_on_lava},
                {TERRAIN_DESERT, ACHIEVEMENT_DESERT_ROSE, &movement_stats_t::total_hero_steps_taken_on_desert},
                {TERRAIN_BEACH, ACHIEVEMENT_ISLAND_IN_THE_SUN, &movement_stats_t::total_hero_steps_taken_on_beach},
                {TERRAIN_SNOW, ACHIEVEMENT_COLD_AS_ICE, &movement_stats_t::total_hero_steps_taken_on_snow},
                {TERRAIN_SWAMP, ACHIEVEMENT_WELCOME_TO_THE_JUNGLE, &movement_stats_t::total_hero_steps_taken_on_swamp},
                {TERRAIN_JUNGLE, ACHIEVEMENT_WELCOME_TO_THE_JUNGLE, &movement_stats_t::total_hero_steps_taken_on_jungle},
        };
}

std::vector<road_type_e> road_traversal_cases() {
        return {ROAD_DIRT, ROAD_GRAVEL, ROAD_COBBLESTONE};
}

std::vector<full_map_reveal_achievement_case_t> full_map_reveal_achievement_cases() {
        return {
                {10, 10, ACHIEVEMENT_EYE_SEE_YOU, &exploration_stats_t::maps_fully_revealed_small},
                {80, 80, ACHIEVEMENT_PAPER_VIEW, &exploration_stats_t::maps_fully_revealed_medium},
                {109, 109, ACHIEVEMENT_MAPQUEST, &exploration_stats_t::maps_fully_revealed_large},
        };
}

std::vector<construction_achievement_case_t> construction_achievement_cases() {
        return {
                {TOWN_KNIGHT, BUILDING_FORT, {}, ACHIEVEMENT_FORT_KNOX, &construction_stats_t::total_forts_built},
                {TOWN_KNIGHT, BUILDING_CASTLE, {BUILDING_FORT}, ACHIEVEMENT_CASTLEMANIA, &construction_stats_t::total_castles_upgraded},
                {TOWN_KNIGHT, BUILDING_CAPTAINS_QUARTERS, {BUILDING_FORT, BUILDING_CASTLE, BUILDING_TURRET_LEFT, BUILDING_TURRET_RIGHT}, ACHIEVEMENT_MAXIMUM_FORTIFICATION, &construction_stats_t::total_castles_fully_upgraded},
                {TOWN_KNIGHT, BUILDING_MARKETPLACE, {BUILDING_FORT}, ACHIEVEMENT_WOLF_OF_WALLSTREET, &construction_stats_t::total_marketplaces_built},
                {TOWN_KNIGHT, BUILDING_LIGHTHOUSE, {BUILDING_FORT}, ACHIEVEMENT_ARCHITECT_OF_DESTINY, &construction_stats_t::total_special_buildings_built},
                {TOWN_KNIGHT, BUILDING_CAPTAINS_QUARTERS, {BUILDING_FORT, BUILDING_CASTLE}, ACHIEVEMENT_OH_CAPTAIN_MY_CAPTAIN, &construction_stats_t::total_captains_quarters_built},
                {TOWN_KNIGHT, BUILDING_TURRET_LEFT, {BUILDING_FORT, BUILDING_CASTLE}, ACHIEVEMENT_TOWER_DEFENSE, &construction_stats_t::total_turrets_built},
                {TOWN_BARBARIAN, BUILDING_GOBLIN_HUT, {BUILDING_FORT}, ACHIEVEMENT_HUMBLE_BEGINNINGS, &construction_stats_t::total_tier1_dwellings_built},
                {TOWN_BARBARIAN, BUILDING_ORC_CAMP, {BUILDING_FORT}, ACHIEVEMENT_THE_WARRIORS_DEN, &construction_stats_t::total_tier2_dwellings_built},
                {TOWN_BARBARIAN, BUILDING_WOLF_DEN, {BUILDING_FORT, BUILDING_GOBLIN_HUT}, ACHIEVEMENT_BARRACKS_AND_BEYOND, &construction_stats_t::total_tier3_dwellings_built},
                {TOWN_BARBARIAN, BUILDING_TROLL_BRIDGE, {BUILDING_FORT, BUILDING_GOBLIN_HUT, BUILDING_WOLF_DEN}, ACHIEVEMENT_FORGE_OF_CHAMPIONS, &construction_stats_t::total_tier4_dwellings_built},
                {TOWN_BARBARIAN, BUILDING_YETI_CAVE, {BUILDING_FORT, BUILDING_GOBLIN_HUT, BUILDING_WOLF_DEN, BUILDING_TROLL_BRIDGE}, ACHIEVEMENT_MONSTERS_ROOST, &construction_stats_t::total_tier5_dwellings_built},
                {TOWN_BARBARIAN, BUILDING_BEHEMOTH_CRAG, {BUILDING_FORT, BUILDING_GOBLIN_HUT, BUILDING_WOLF_DEN, BUILDING_TROLL_BRIDGE}, ACHIEVEMENT_BEHEMOTHS_BIRTHPLACE, &construction_stats_t::total_tier6_dwellings_built},
                {TOWN_KNIGHT, BUILDING_TAVERN, {BUILDING_FORT}, ACHIEVEMENT_HOLY_FOUNDATIONS, &construction_stats_t::paladin_buildings},
                {TOWN_SORCERESS, BUILDING_TAVERN, {BUILDING_FORT}, ACHIEVEMENT_ENCHANTED_EXPANSION, &construction_stats_t::enchantress_buildings},
                {TOWN_WIZARD, BUILDING_TAVERN, {BUILDING_FORT}, ACHIEVEMENT_THE_GRAND_ARCHIVE, &construction_stats_t::wizard_buildings},
                {TOWN_BARBARIAN, BUILDING_TAVERN, {BUILDING_FORT}, ACHIEVEMENT_BLOOD_SWEAT_AND_STONE, &construction_stats_t::barbarian_buildings},
                {TOWN_WARLOCK, BUILDING_TAVERN, {BUILDING_FORT}, ACHIEVEMENT_MORDOR_EXPANSION_PROJECT, &construction_stats_t::warlock_buildings},
                {TOWN_NECROMANCER, BUILDING_TAVERN, {BUILDING_FORT}, ACHIEVEMENT_TOMBSTONE_BOOMTOWN, &construction_stats_t::necromancer_buildings},
        };
}

std::vector<recruitment_achievement_case_t> recruitment_achievement_cases() {
        return {
                {UNIT_SKELETON, ACHIEVEMENT_RATTLE_AND_ROLL},
                {UNIT_GHOUL, ACHIEVEMENT_GRAVE_CONSEQUENCES},
                {UNIT_VAMPIRE, ACHIEVEMENT_CHILDREN_OF_THE_NIGHT},
                {UNIT_LICH, ACHIEVEMENT_DUST_TO_DUST},
                {UNIT_ABOMINATION, ACHIEVEMENT_MONSTROSITY_UNLEASHED},
                {UNIT_BONE_WYRM, ACHIEVEMENT_WINGS_OF_DEATH},
                {UNIT_GOBLIN, ACHIEVEMENT_GREEN_TIDE_RISING},
                {UNIT_ORC, ACHIEVEMENT_WARLORDS_BROOD},
                {UNIT_WOLF, ACHIEVEMENT_HOWLING_HORDE},
                {UNIT_TROLL, ACHIEVEMENT_BRIDGE_TROLL_TOLL},
                {UNIT_YETI, ACHIEVEMENT_COLDBLOODED},
                {UNIT_BEHEMOTH, ACHIEVEMENT_MONSTER_MASH},
                {UNIT_INFANTRYMAN, ACHIEVEMENT_MARCH_OF_THE_BRAVE},
                {UNIT_ARCHER, ACHIEVEMENT_VOLLEY_FIRE},
                {UNIT_FOOTMAN, ACHIEVEMENT_IRON_DISCIPLINE},
                {UNIT_BISHOP, ACHIEVEMENT_MEDITATIONS_OF_WAR},
                {UNIT_CAVALIER, ACHIEVEMENT_CHARGE_OF_THE_VALIANT},
                {UNIT_CRUSADER, ACHIEVEMENT_HOLY_RECKONING},
                {UNIT_DEMON, ACHIEVEMENT_INFERNAL_RECRUITMENT},
                {UNIT_SUCCUBUS, ACHIEVEMENT_DEADLY_SEDUCTION},
                {UNIT_HIPPOGRIFF, ACHIEVEMENT_SKYBOUND_SENTINELS},
                {UNIT_MINOTAUR, ACHIEVEMENT_MAZE_RUNNERS},
                {UNIT_MANTICORE, ACHIEVEMENT_WINGS_OF_TERROR},
                {UNIT_RED_DRAGON, ACHIEVEMENT_BORN_OF_FIRE},
                {UNIT_PIXIE, ACHIEVEMENT_FEY_AWAKENING},
                {UNIT_DRYAD, ACHIEVEMENT_GUARDIAN_OF_THE_GLADE},
                {UNIT_ELF, ACHIEVEMENT_VERDANT_VANGUARD},
                {UNIT_DRUID, ACHIEVEMENT_NATURES_WRATH},
                {UNIT_UNICORN, ACHIEVEMENT_LIGHT_IN_THE_FOREST},
                {UNIT_PHOENIX, ACHIEVEMENT_ASHES_REBORN},
                {UNIT_GOLEM, ACHIEVEMENT_FORGED_FOR_WAR},
                {UNIT_DWARF, ACHIEVEMENT_MOUNTAINBORN},
                {UNIT_ARCANE_CONSTRUCT, ACHIEVEMENT_GEARS_OF_THE_ARCANE},
                {UNIT_GENIE, ACHIEVEMENT_WISHES_AND_WARFARE},
                {UNIT_MAGE, ACHIEVEMENT_MASTERS_OF_THE_MYSTIC_ARTS},
                {UNIT_TITAN, ACHIEVEMENT_STORMLORDS_CALL},
        };
}

uint32_t& movement_stat(game_t& game, uint32_t movement_stats_t::* stat) {
        return game.get_player(PLAYER_1).player_stats.movement.*stat;
}

uint16_t& exploration_stat(game_t& game, uint16_t exploration_stats_t::* stat) {
        return game.get_player(PLAYER_1).player_stats.exploration.*stat;
}

uint16_t& construction_stat(game_t& game, uint16_t construction_stats_t::* stat) {
        return game.get_player(PLAYER_1).player_stats.construction.*stat;
}

uint64_t first_threshold_or_one(const achievement_t& achievement) {
        for(const auto& tier : achievement.tiers) {
                if(tier.value != 0)
                        return tier.value;
        }

        return 1;
}

void test_adventure_map_resource_pickup_progress_and_callbacks() {
        for(const auto& test_case : adventure_map_resource_pickup_achievement_cases()) {
                game_t game;
                initialize_visible_game(game, 3, 3);
                auto hero = make_hero(1, 1);
                std::vector<achievement_e> earned;
                game.achievement_earned_callback_fn = [&](achievement_e achievement) {
                        earned.push_back(achievement);
                };

                constexpr uint16_t first_pickup_quantity = 7;
                auto* first_pickup = add_resource_pickup(game.map, 2, 1, test_case.resource_type, first_pickup_quantity);
                const auto before_pickup = game.get_player(PLAYER_1).player_stats.resources.total_resources_picked_up.get_value_for_type(test_case.resource_type);

                expect_true(game.pickup_object(&hero, first_pickup, first_pickup->x, first_pickup->y),
                            game_config::get_achievement(test_case.achievement).name + " pickup should succeed");

                const auto after_pickup = game.get_player(PLAYER_1).player_stats.resources.total_resources_picked_up.get_value_for_type(test_case.resource_type);
                expect_eq(after_pickup, before_pickup + first_pickup_quantity,
                          game_config::get_achievement(test_case.achievement).name + " pickup should increase achievement progress by the pile quantity");
                expect_true(earned.empty(), game_config::get_achievement(test_case.achievement).name + " should not fire before the first tier threshold");

                constexpr uint16_t threshold_crossing_quantity = 3;
                const auto first_threshold = first_threshold_or_one(game_config::get_achievement(test_case.achievement));
                game.get_player(PLAYER_1).player_stats.resources.total_resources_picked_up.set_value_for_type(test_case.resource_type, first_threshold - 1);
                auto* threshold_pickup = add_resource_pickup(game.map, 2, 1, test_case.resource_type, threshold_crossing_quantity);

                expect_true(game.pickup_object(&hero, threshold_pickup, threshold_pickup->x, threshold_pickup->y),
                            game_config::get_achievement(test_case.achievement).name + " threshold-crossing pickup should succeed");

                expect_eq(game.get_player(PLAYER_1).player_stats.resources.total_resources_picked_up.get_value_for_type(test_case.resource_type),
                          first_threshold - 1 + threshold_crossing_quantity,
                          game_config::get_achievement(test_case.achievement).name + " pickup should preserve exact progress when crossing the first tier threshold");
                expect_eq(earned.size(), size_t(1),
                          game_config::get_achievement(test_case.achievement).name + " callback should fire exactly once when crossing the first tier threshold");
                if(!earned.empty())
                        expect_true(earned.front() == test_case.achievement,
                                    game_config::get_achievement(test_case.achievement).name + " callback should report the matching achievement ID");
        }
}

void test_mine_income_progress_and_callbacks() {
        for(const auto& test_case : mine_income_achievement_cases()) {
                game_t game;
                initialize_visible_game(game, 3, 3);
                std::vector<achievement_e> earned;
                game.achievement_earned_callback_fn = [&](achievement_e achievement) {
                        earned.push_back(achievement);
                };

                add_mine(game.map, 1, 1, test_case.resource_type, PLAYER_1);
                const auto daily_income = expected_daily_mine_income(test_case.resource_type);
                const auto before_income = game.get_player(PLAYER_1).player_stats.resources.total_resources_from_mines.get_value_for_type(test_case.resource_type);

                game.next_day();

                const auto after_income = game.get_player(PLAYER_1).player_stats.resources.total_resources_from_mines.get_value_for_type(test_case.resource_type);
                expect_eq(after_income, before_income + daily_income,
                          game_config::get_achievement(test_case.achievement).name + " mine income should increase achievement progress by one day of production");
                expect_true(earned.empty(), game_config::get_achievement(test_case.achievement).name + " should not fire before the first tier threshold");

                const auto first_threshold = first_threshold_or_one(game_config::get_achievement(test_case.achievement));
                game.get_player(PLAYER_1).player_stats.resources.total_resources_from_mines.set_value_for_type(test_case.resource_type, first_threshold - daily_income);

                game.next_day();

                expect_eq(game.get_player(PLAYER_1).player_stats.resources.total_resources_from_mines.get_value_for_type(test_case.resource_type),
                          first_threshold,
                          game_config::get_achievement(test_case.achievement).name + " mine income should preserve exact progress when crossing the first tier threshold");
                expect_eq(earned.size(), size_t(1),
                          game_config::get_achievement(test_case.achievement).name + " callback should fire exactly once when crossing the first tier threshold");
                if(!earned.empty())
                        expect_true(earned.front() == test_case.achievement,
                                    game_config::get_achievement(test_case.achievement).name + " callback should report the matching achievement ID");
        }
}

void test_travelers_journey_progress_and_callback() {
        game_t game;
        initialize_visible_game(game, 3, 3);
        auto hero = make_hero(1, 1, 5000);
        std::vector<achievement_e> earned;
        game.achievement_earned_callback_fn = [&](achievement_e achievement) {
                earned.push_back(achievement);
        };

        const auto first_threshold = first_threshold_or_one(game_config::get_achievement(ACHIEVEMENT_TRAVELERS_JOURNEY));
        game.get_player(PLAYER_1).player_stats.movement.total_hero_steps_taken = first_threshold - 1;

        expect_true(game.map.move_hero_to_tile(hero, 2, 1, game) == MAP_ACTION_VALID_MOVE,
                    "Traveler's Journey movement should succeed");

        expect_eq(game.get_player(PLAYER_1).player_stats.movement.total_hero_steps_taken, first_threshold,
                  "Traveler's Journey movement should increment total hero steps exactly once");
        expect_eq(earned.size(), size_t(1),
                  "Traveler's Journey callback should fire exactly once when crossing the first tier threshold");
        if(!earned.empty())
                expect_true(earned.front() == ACHIEVEMENT_TRAVELERS_JOURNEY,
                            "Traveler's Journey callback should report ACHIEVEMENT_TRAVELERS_JOURNEY");
}

void test_terrain_traversal_progress_and_callbacks() {
        for(const auto& test_case : terrain_traversal_achievement_cases()) {
                game_t game;
                initialize_visible_game(game, 3, 3);
                game.map.get_tile(2, 1).terrain_type = test_case.terrain_type;
                game.map.get_tile(2, 1).road_type = ROAD_NONE;
                auto hero = make_hero(1, 1, 5000);
                std::vector<achievement_e> earned;
                game.achievement_earned_callback_fn = [&](achievement_e achievement) {
                        earned.push_back(achievement);
                };

                const auto first_threshold = first_threshold_or_one(game_config::get_achievement(test_case.achievement));
                movement_stat(game, test_case.stat) = first_threshold - 1;

                expect_true(game.map.move_hero_to_tile(hero, 2, 1, game) == MAP_ACTION_VALID_MOVE,
                            game_config::get_achievement(test_case.achievement).name + " movement should succeed");

                expect_eq(movement_stat(game, test_case.stat), first_threshold,
                          game_config::get_achievement(test_case.achievement).name + " movement should increment matching terrain steps exactly once");
                expect_eq(game.get_player(PLAYER_1).player_stats.movement.total_hero_steps_taken, size_t(1),
                          game_config::get_achievement(test_case.achievement).name + " movement should also increment total hero steps");
                expect_eq(earned.size(), size_t(1),
                          game_config::get_achievement(test_case.achievement).name + " callback should fire exactly once when crossing the first tier threshold");
                if(!earned.empty())
                        expect_true(earned.front() == test_case.achievement,
                                    game_config::get_achievement(test_case.achievement).name + " callback should report the matching achievement ID");
        }
}

void test_road_traversal_progress_and_callbacks() {
        for(const auto road_type : road_traversal_cases()) {
                game_t game;
                initialize_visible_game(game, 3, 3);
                game.map.get_tile(2, 1).road_type = road_type;
                auto hero = make_hero(1, 1, 5000);
                std::vector<achievement_e> earned;
                game.achievement_earned_callback_fn = [&](achievement_e achievement) {
                        earned.push_back(achievement);
                };

                const auto first_threshold = first_threshold_or_one(game_config::get_achievement(ACHIEVEMENT_THE_LONG_AND_WINDING_ROAD));
                game.get_player(PLAYER_1).player_stats.movement.total_hero_steps_taken_on_roads = first_threshold - 1;

                expect_true(game.map.move_hero_to_tile(hero, 2, 1, game) == MAP_ACTION_VALID_MOVE,
                            "road traversal movement should succeed");

                expect_eq(game.get_player(PLAYER_1).player_stats.movement.total_hero_steps_taken_on_roads, first_threshold,
                          "road traversal should increment road steps exactly once");
                expect_eq(game.get_player(PLAYER_1).player_stats.movement.total_hero_steps_taken_on_grass, size_t(0),
                          "road traversal should not increment destination terrain-specific steps");
                expect_eq(earned.size(), size_t(1),
                          "road traversal callback should fire exactly once when crossing the first tier threshold");
                if(!earned.empty())
                        expect_true(earned.front() == ACHIEVEMENT_THE_LONG_AND_WINDING_ROAD,
                                    "road traversal callback should report ACHIEVEMENT_THE_LONG_AND_WINDING_ROAD");
        }
}

void test_revealing_the_unknown_progress_and_callback() {
        game_t game;
        initialize_visible_game(game, 50, 50);
        hide_map_for_player(game, PLAYER_1);
        std::vector<achievement_e> earned;
        game.achievement_earned_callback_fn = [&](achievement_e achievement) {
                earned.push_back(achievement);
        };

        const auto first_threshold = first_threshold_or_one(game_config::get_achievement(ACHIEVEMENT_REVEALING_THE_UNKNOWN));
        game.get_player(PLAYER_1).player_stats.exploration.fog_tiles_revealed = first_threshold - 1;

        const auto revealed_tiles = game.reveal_area(PLAYER_1, 25, 25, 2, 2);

        expect_true(revealed_tiles > 0, "Revealing the Unknown test should reveal at least one hidden tile");
        expect_eq(game.get_player(PLAYER_1).player_stats.exploration.fog_tiles_revealed,
                  first_threshold - 1 + revealed_tiles,
                  "Revealing the Unknown should preserve exact revealed tile progress");
        expect_eq(earned.size(), size_t(1),
                  "Revealing the Unknown callback should fire exactly once when crossing the first tier threshold");
        if(!earned.empty())
                expect_true(earned.front() == ACHIEVEMENT_REVEALING_THE_UNKNOWN,
                            "Revealing the Unknown callback should report ACHIEVEMENT_REVEALING_THE_UNKNOWN");
}

void test_watchtower_exploration_progress_and_callbacks() {
        game_t game;
        initialize_visible_game(game, 50, 50);
        hide_map_for_player(game, PLAYER_1);
        auto hero = make_hero(25, 25);
        auto* watchtower = add_object(game.map, 25, 25, OBJECT_WATCHTOWER);
        game.show_dialog_callback_fn = [](dialog_type_e, interactable_object_t*, hero_t*, int, int) {};
        game.show_hero_levelup_callback_fn = [](hero_t*, skill_e, skill_e, skill_e, stat_e, bool) {};
        game.game_event_callback_fn = [](game_state_update_e, player_e, int) {};
        game.update_interactable_object_callback_fn = [](interactable_object_t*) {};
        std::vector<achievement_e> earned;
        game.achievement_earned_callback_fn = [&](achievement_e achievement) {
                earned.push_back(achievement);
        };

        game.get_player(PLAYER_1).player_stats.exploration.fog_tiles_revealed = 50000;
        game.get_player(PLAYER_1).player_stats.exploration.watchtowers_visited = first_threshold_or_one(game_config::get_achievement(ACHIEVEMENT_WATCHMEN)) - 1;
        game.get_player(PLAYER_1).player_stats.exploration.fog_tiles_revealed_watchtower = first_threshold_or_one(game_config::get_achievement(ACHIEVEMENT_THE_NIGHTS_WATCH)) - 1;

        game.make_dialog_choice(&hero, watchtower, DIALOG_TYPE_WATCHTOWER, DIALOG_CHOICE_NONE);

        expect_true(game.get_player(PLAYER_1).player_stats.exploration.fog_tiles_revealed_watchtower >=
                            first_threshold_or_one(game_config::get_achievement(ACHIEVEMENT_THE_NIGHTS_WATCH)),
                    "watchtower reveal should cross The Night's Watch first tier threshold");
        expect_eq(game.get_player(PLAYER_1).player_stats.exploration.watchtowers_visited,
                  first_threshold_or_one(game_config::get_achievement(ACHIEVEMENT_WATCHMEN)),
                  "Watchmen should increment watchtower visits exactly once when new tiles are revealed");
        expect_eq(earned_count(earned, ACHIEVEMENT_THE_NIGHTS_WATCH), size_t(1),
                  "The Night's Watch callback should fire exactly once");
        expect_eq(earned_count(earned, ACHIEVEMENT_WATCHMEN), size_t(1),
                  "Watchmen callback should fire exactly once");
        expect_eq(earned.size(), size_t(2),
                  "watchtower visit should only fire the two watchtower-related callbacks in this setup");
}

void test_keymaster_tent_progress_and_callback() {
        game_t game;
        initialize_visible_game(game, 3, 3);
        auto hero = make_hero(1, 1);
        auto* tent = static_cast<keymaster_tent_t*>(add_object(game.map, 1, 1, OBJECT_KEYMASTER_TENT));
        tent->color = 2;
        game.show_dialog_callback_fn = [](dialog_type_e, interactable_object_t*, hero_t*, int, int) {};
        game.show_hero_levelup_callback_fn = [](hero_t*, skill_e, skill_e, skill_e, stat_e, bool) {};
        game.game_event_callback_fn = [](game_state_update_e, player_e, int) {};
        game.update_interactable_object_callback_fn = [](interactable_object_t*) {};
        std::vector<achievement_e> earned;
        game.achievement_earned_callback_fn = [&](achievement_e achievement) {
                earned.push_back(achievement);
        };

        const auto first_threshold = first_threshold_or_one(game_config::get_achievement(ACHIEVEMENT_KEY_TO_VICTORY));
        game.get_player(PLAYER_1).player_stats.exploration.keymaster_tents_visited = first_threshold - 1;

        game.interact_with_object(&hero, tent);

        expect_eq(game.get_player(PLAYER_1).player_stats.exploration.keymaster_tents_visited, first_threshold,
                  "Key to Victory should increment keymaster tent visits exactly once for a new key color");
        expect_eq(earned.size(), size_t(1),
                  "Key to Victory callback should fire exactly once when crossing the first tier threshold");
        if(!earned.empty())
                expect_true(earned.front() == ACHIEVEMENT_KEY_TO_VICTORY,
                            "Key to Victory callback should report ACHIEVEMENT_KEY_TO_VICTORY");
}

void test_dimensional_drift_progress_and_callback() {
        game_t game;
        initialize_visible_game(game, 5, 5);
        auto hero = make_hero(1, 1);
        auto* entrance = add_teleporter(game.map, 1, 1, 1, teleporter_t::TELEPORTER_TYPE_TWO_WAY);
        add_teleporter(game.map, 3, 3, 1, teleporter_t::TELEPORTER_TYPE_TWO_WAY);
        std::vector<achievement_e> earned;
        game.achievement_earned_callback_fn = [&](achievement_e achievement) {
                earned.push_back(achievement);
        };

        const auto first_threshold = first_threshold_or_one(game_config::get_achievement(ACHIEVEMENT_DIMENSIONAL_DRIFT));
        game.get_player(PLAYER_1).player_stats.exploration.portals_used = first_threshold - 1;

        expect_true(game.interact_with_object(&hero, entrance) == MAP_ACTION_HERO_TELEPORTED,
                    "Dimensional Drift teleporter interaction should teleport the hero");

        expect_eq(game.get_player(PLAYER_1).player_stats.exploration.portals_used, first_threshold,
                  "Dimensional Drift should increment portal uses exactly once");
        expect_eq(earned.size(), size_t(1),
                  "Dimensional Drift callback should fire exactly once when crossing the first tier threshold");
        if(!earned.empty())
                expect_true(earned.front() == ACHIEVEMENT_DIMENSIONAL_DRIFT,
                            "Dimensional Drift callback should report ACHIEVEMENT_DIMENSIONAL_DRIFT");
        expect_eq(hero.x, size_t(3), "teleporter should move hero to destination x");
        expect_eq(hero.y, size_t(3), "teleporter should move hero to destination y");
}

void test_eye_of_the_magi_progress_and_callback() {
        game_t game;
        initialize_visible_game(game, 50, 50);
        hide_map_for_player(game, PLAYER_1);
        auto hero = make_hero(25, 25);
        auto* hut = add_object(game.map, 25, 25, OBJECT_HUT_OF_THE_MAGI);
        add_eye_of_the_magi(game.map, 25, 25, 10);
        std::vector<achievement_e> earned;
        game.achievement_earned_callback_fn = [&](achievement_e achievement) {
                earned.push_back(achievement);
        };

        game.get_player(PLAYER_1).player_stats.exploration.fog_tiles_revealed = 50000;
        game.get_player(PLAYER_1).player_stats.exploration.fog_tiles_revealed_eyes = first_threshold_or_one(game_config::get_achievement(ACHIEVEMENT_ALL_SEEING_EYE)) - 1;

        game.make_dialog_choice(&hero, hut, DIALOG_TYPE_HUT_OF_THE_MAGI, DIALOG_CHOICE_NONE);

        expect_true(game.get_player(PLAYER_1).player_stats.exploration.fog_tiles_revealed_eyes >=
                            first_threshold_or_one(game_config::get_achievement(ACHIEVEMENT_ALL_SEEING_EYE)),
                    "All Seeing Eye should cross its first tier threshold after revealing tiles from an eye of the magi");
        expect_eq(earned.size(), size_t(1),
                  "All Seeing Eye callback should fire exactly once when crossing the first tier threshold");
        if(!earned.empty())
                expect_true(earned.front() == ACHIEVEMENT_ALL_SEEING_EYE,
                            "All Seeing Eye callback should report ACHIEVEMENT_ALL_SEEING_EYE");
}

void test_full_map_reveal_progress_and_callbacks() {
        for(const auto& test_case : full_map_reveal_achievement_cases()) {
                game_t game;
                initialize_visible_game(game, test_case.width, test_case.height);
                hide_map_for_player(game, PLAYER_1);
                std::vector<achievement_e> earned;
                game.achievement_earned_callback_fn = [&](achievement_e achievement) {
                        earned.push_back(achievement);
                };

                game.get_player(PLAYER_1).player_stats.exploration.fog_tiles_revealed = 50000;

                game.reveal_area(PLAYER_1, test_case.width / 2, test_case.height / 2, std::max(test_case.width, test_case.height), std::max(test_case.width, test_case.height));

                expect_eq(exploration_stat(game, test_case.stat), size_t(1),
                          game_config::get_achievement(test_case.achievement).name + " should increment its fully revealed map stat exactly once");
                expect_eq(earned.size(), size_t(1),
                          game_config::get_achievement(test_case.achievement).name + " callback should fire exactly once when the map becomes fully visible");
                if(!earned.empty())
                        expect_true(earned.front() == test_case.achievement,
                                    game_config::get_achievement(test_case.achievement).name + " callback should report the matching achievement ID");
        }
}

void test_construction_achievement_progress_and_callbacks() {
        for(const auto& test_case : construction_achievement_cases()) {
                game_t game;
                initialize_visible_game(game, 3, 3);
                give_player_abundant_resources(game, PLAYER_1);
                game.update_interactable_object_callback_fn = [](interactable_object_t*) {};
                auto town = make_buildable_town(test_case.town_type, test_case.building, test_case.already_built);
                std::vector<achievement_e> earned;
                game.achievement_earned_callback_fn = [&](achievement_e achievement) {
                        earned.push_back(achievement);
                };

                const auto first_threshold = first_threshold_or_one(game_config::get_achievement(test_case.achievement));
                construction_stat(game, test_case.stat) = first_threshold - 1;

                expect_true(game.build_building(&town, test_case.building),
                            game_config::get_achievement(test_case.achievement).name + " building construction should succeed");

                expect_eq(construction_stat(game, test_case.stat), first_threshold,
                          game_config::get_achievement(test_case.achievement).name + " should increment the matching construction stat exactly once");
                expect_eq(earned_count(earned, test_case.achievement), size_t(1),
                          game_config::get_achievement(test_case.achievement).name + " callback should fire exactly once when crossing the first tier threshold");
                expect_eq(earned.size(), size_t(1),
                          game_config::get_achievement(test_case.achievement).name + " should be the only achievement callback in this construction scenario");
        }
}

void test_recruitment_achievement_progress_and_callbacks() {
        for(const auto& test_case : recruitment_achievement_cases()) {
                game_t game;
                initialize_visible_game(game, 3, 3);
                give_player_abundant_resources(game, PLAYER_1);
                town_t town;
                town.player = PLAYER_1;
                town.available_troops.push_back(troop_t(test_case.unit_type, first_threshold_or_one(game_config::get_achievement(test_case.achievement))));
                std::vector<achievement_e> earned;
                game.achievement_earned_callback_fn = [&](achievement_e achievement) {
                        earned.push_back(achievement);
                };

                const auto first_threshold = first_threshold_or_one(game_config::get_achievement(test_case.achievement));
                const auto recruit_count = first_threshold;
                game.get_player(PLAYER_1).player_stats.units_recruited[test_case.unit_type] = first_threshold - recruit_count;

                expect_true(game.buy_troops_at_town(PLAYER_1, &town, 0, recruit_count),
                            game_config::get_achievement(test_case.achievement).name + " recruitment should succeed");

                expect_eq(game.get_player(PLAYER_1).player_stats.units_recruited[test_case.unit_type], first_threshold,
                          game_config::get_achievement(test_case.achievement).name + " should increment the matching unit recruitment stat exactly");
                expect_eq(game.get_player(PLAYER_1).player_stats.total_units_recruited, recruit_count,
                          game_config::get_achievement(test_case.achievement).name + " should increment aggregate units recruited by the recruited count");
                expect_eq(earned_count(earned, test_case.achievement), size_t(1),
                          game_config::get_achievement(test_case.achievement).name + " callback should fire exactly once when crossing the first tier threshold");
                expect_eq(earned.size(), size_t(1),
                          game_config::get_achievement(test_case.achievement).name + " should be the only achievement callback in this recruitment scenario");
        }
}


void test_achievement_config_is_well_formed() {
        const auto& achievements = game_config::get_achievements();
        expect_true(!achievements.empty(), "achievement config should load at least one achievement");

        std::set<achievement_e> ids;
        std::set<std::string> names;
        for(const auto& achievement : achievements) {
                expect_true(achievement.id != ACHIEVEMENT_NONE, "achievement config should not contain ACHIEVEMENT_NONE rows");
                expect_true(ids.insert(achievement.id).second, "achievement IDs should be unique: " + achievement.name);
                expect_true(names.insert(achievement.name).second, "achievement names should be unique: " + achievement.name);
                expect_true(!achievement.name.empty(), "achievement should have a display name");
                expect_true(!achievement.description.empty(), "achievement should have a description: " + achievement.name);
                expect_true(!achievement.asset_id.empty(), "achievement should have an icon asset: " + achievement.name);
                expect_true(achievement.type != ACHIEVEMENT_TYPE_NONE, "achievement should have a type: " + achievement.name);
                expect_true(achievement.category != ACHIEVEMENT_CATEGORY_NONE, "achievement should have a category: " + achievement.name);
        }
}

void test_every_configured_achievement_can_fire_callback() {
        const auto& achievements = game_config::get_achievements();
        std::vector<achievement_e> earned;
        game_t game;
        game.achievement_earned_callback_fn = [&](achievement_e achievement) {
                earned.push_back(achievement);
        };

        for(const auto& achievement : achievements) {
                uint64_t progress = 0;
                earned.clear();
                const auto increment = first_threshold_or_one(achievement);

                const bool result = game.update_achievement_stats(progress, increment, achievement.id);

                expect_true(result, "update_achievement_stats should report an earned achievement for " + achievement.name);
                expect_eq(earned.size(), size_t(1), "achievement callback should fire exactly once for " + achievement.name);
                if(!earned.empty())
                        expect_true(earned.front() == achievement.id, "achievement callback should report the matching ID for " + achievement.name);
        }
}

void test_achievement_threshold_crossing_semantics() {
        game_t game;
        std::vector<achievement_e> earned;
        game.achievement_earned_callback_fn = [&](achievement_e achievement) {
                earned.push_back(achievement);
        };

        const auto achievement = ACHIEVEMENT_TRAVELERS_JOURNEY;
        const auto& data = game_config::get_achievement(achievement);
        const auto threshold = first_threshold_or_one(data);
        uint64_t progress = 0;

        expect_true(!game.update_achievement_stats(progress, threshold - 1, achievement), "achievement should not fire before first threshold");
        expect_true(earned.empty(), "callback should not fire before first threshold");

        expect_true(game.update_achievement_stats(progress, 1, achievement), "achievement should fire when crossing first threshold");
        expect_eq(earned.size(), size_t(1), "callback should fire once when crossing first threshold");

        expect_true(!game.update_achievement_stats(progress, 0, achievement), "zero increment should not fire duplicate callback");
        expect_eq(earned.size(), size_t(1), "callback count should remain unchanged for zero increment");
}

void test_one_shot_achievement_semantics() {
        game_t game;
        std::vector<achievement_e> earned;
        game.achievement_earned_callback_fn = [&](achievement_e achievement) {
                earned.push_back(achievement);
        };

        uint64_t progress = 0;
        expect_true(game.update_achievement_stats(progress, 1, ACHIEVEMENT_FRIENDLY_FIRE), "one-shot achievements should fire on first positive progress");
        expect_eq(earned.size(), size_t(1), "one-shot achievement should fire once on first positive progress");
        expect_true(!game.update_achievement_stats(progress, 1, ACHIEVEMENT_FRIENDLY_FIRE), "one-shot achievements should not fire repeatedly after progress already exists");
        expect_eq(earned.size(), size_t(1), "one-shot achievement should not add duplicate callbacks");
}
}

int main() {
        if(game_config::load_achievements("../") != 0) {
                std::cerr << "FAIL: could not load achievements config\n";
                return EXIT_FAILURE;
        }
        if(game_config::load_buildings("../") != 0) {
                std::cerr << "FAIL: could not load buildings config\n";
                return EXIT_FAILURE;
        }
        if(game_config::load_creatures("../") != 0) {
                std::cerr << "FAIL: could not load creatures config\n";
                return EXIT_FAILURE;
        }
        if(game_config::load_spells("../") != 0) {
                std::cerr << "FAIL: could not load spells config\n";
                return EXIT_FAILURE;
        }

        test_achievement_config_is_well_formed();
        test_every_configured_achievement_can_fire_callback();
        test_achievement_threshold_crossing_semantics();
        test_one_shot_achievement_semantics();
        test_adventure_map_resource_pickup_progress_and_callbacks();
        test_mine_income_progress_and_callbacks();
        test_travelers_journey_progress_and_callback();
        test_terrain_traversal_progress_and_callbacks();
        test_road_traversal_progress_and_callbacks();
        test_revealing_the_unknown_progress_and_callback();
        test_watchtower_exploration_progress_and_callbacks();
        test_keymaster_tent_progress_and_callback();
        test_dimensional_drift_progress_and_callback();
        test_eye_of_the_magi_progress_and_callback();
        test_full_map_reveal_progress_and_callbacks();
        test_construction_achievement_progress_and_callbacks();
        test_recruitment_achievement_progress_and_callbacks();
        failures += run_combat_achievement_tests();

        if(failures != 0) {
                std::cerr << failures << " achievement test(s) failed.\n";
                return EXIT_FAILURE;
        }

        std::cout << "All achievement tests passed.\n";
        return EXIT_SUCCESS;
}
