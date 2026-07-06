#include "core/adventure_map.h"
#include "core/game.h"

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>

namespace {
int failures = 0;

void expect_true(bool condition, const std::string& message) {
        if(!condition) {
                std::cerr << "FAIL: " << message << '\n';
                ++failures;
        }
}

void expect_eq(int actual, int expected, const std::string& message) {
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
        game.achievement_earned_callback_fn = [](achievement_e) {};
}

hero_t make_hero(int x, int y, int movement_points = 1500) {
        hero_t hero;
        hero.player = PLAYER_1;
        hero.x = static_cast<uint8_t>(x);
        hero.y = static_cast<uint8_t>(y);
        hero.movement_points = movement_points;
        hero.maximum_daily_movement_points = 1500;
        return hero;
}

void add_pickup(adventure_map_t& map, int x, int y, interactable_object_e type = OBJECT_RESOURCE) {
        auto* object = interactable_object_t::make_new_object(type);
        object->x = x;
        object->y = y;
        map.objects.push_back(object);
        map.get_tile(x, y).interactable_object = static_cast<uint16_t>(map.objects.size());
}

void test_route_rejects_invalid_or_hidden_destinations() {
        game_t game;
        initialize_visible_game(game, 5, 5);
        auto hero = make_hero(2, 2);

        expect_true(game.map.get_route(&hero, -1, 2, &game).empty(), "route to an invalid tile should be empty");

        game.get_player(PLAYER_1).tile_visibility.clearBit(4 + 2 * game.map.width);
        expect_true(game.map.get_route(&hero, 4, 2, &game).empty(), "route to a hidden tile should be empty");
}

void test_route_avoids_blocked_tiles_and_uses_diagonals() {
        game_t game;
        initialize_visible_game(game, 5, 5);
        auto hero = make_hero(0, 0);

        auto diagonal_route = game.map.get_route(&hero, 2, 2, &game);
        expect_eq(static_cast<int>(diagonal_route.size()), 2, "open diagonal route should use two diagonal steps");
        expect_eq(diagonal_route.back().tile.x, 2, "diagonal route should end at target x");
        expect_eq(diagonal_route.back().tile.y, 2, "diagonal route should end at target y");

        game.map.get_tile(1, 1).passability = 0;
        auto detour = game.map.get_route(&hero, 2, 2, &game);
        expect_true(!detour.empty(), "route should still exist around a blocked diagonal tile");
        for(const auto& step : detour)
                expect_true(step.tile != coord_t{1, 1}, "route should not include blocked tile");
}

void test_interactable_tiles_only_allowed_as_destination() {
        game_t game;
        initialize_visible_game(game, 5, 3);
        auto hero = make_hero(0, 1);
        add_pickup(game.map, 1, 1);

        auto through_pickup = game.map.get_route(&hero, 2, 1, &game);
        expect_true(through_pickup.empty() || through_pickup.front().tile != coord_t{1, 1},
                    "pathfinding should not route through an uncollected pickup object");

        auto to_pickup = game.map.get_route(&hero, 1, 1, &game);
        expect_eq(static_cast<int>(to_pickup.size()), 1, "pathfinding should allow a pickup object as the destination");
}

void test_hero_movement_costs_and_boundaries() {
        game_t game;
        initialize_visible_game(game, 4, 4);
        auto hero = make_hero(1, 1, 100);

        expect_true(!game.map.can_hero_move_to_tile(&hero, 3, 3, &game), "hero should not move to non-adjacent tiles in one step");
        expect_true(!game.map.can_hero_move_to_tile(&hero, -1, 1, &game), "hero should not move off the map");
        expect_true(game.map.can_hero_move_to_tile(&hero, 2, 1, &game), "hero should move to adjacent cardinal grass tile with exact movement");

        auto action = game.map.move_hero_to_tile(hero, 2, 1, game);
        expect_true(action == MAP_ACTION_VALID_MOVE, "valid movement should return a move action");
        expect_eq(hero.x, 2, "hero x should update after movement");
        expect_eq(hero.y, 1, "hero y should update after movement");
        expect_eq(hero.movement_points, 0, "cardinal grass move should spend 100 movement points");
}

void test_pathfinding_skill_allows_zero_movement_pickups() {
        game_t game;
        initialize_visible_game(game, 3, 3);
        auto hero = make_hero(1, 1, 0);
        hero.skills[0].skill = SKILL_PATHFINDING;
        hero.skills[0].level = 1;
        add_pickup(game.map, 2, 1);

        expect_true(game.map.can_hero_move_to_tile(&hero, 2, 1, &game),
                    "pathfinding skill should allow adjacent pickup collection with zero movement");

        auto action = game.map.move_hero_to_tile(hero, 2, 1, game);
        expect_true(action == MAP_ACTION_VALID_MOVE, "zero-movement pathfinding pickup should still move the hero");
        expect_eq(hero.x, 2, "hero should enter the pickup tile");
        expect_eq(hero.movement_points, 0, "zero-movement pathfinding pickup should not make movement negative");
}

void benchmark_pathfinding() {
        constexpr uint size = 64;
        constexpr int iterations = 100;
        game_t game;
        initialize_visible_game(game, size, size);
        auto hero = make_hero(0, 0);

        for(uint y = 0; y < size; ++y) {
                if(y == size / 2)
                        continue;
                game.map.get_tile(size / 2, y).passability = 0;
        }

        const auto start = std::chrono::steady_clock::now();
        route_t last_route;
        for(int i = 0; i < iterations; ++i)
                last_route = game.map.get_route(&hero, size - 1, size - 1, &game);
        const auto elapsed = std::chrono::steady_clock::now() - start;
        const auto micros = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();

        expect_true(!last_route.empty(), "benchmark route should be reachable");
        std::cout << "Pathfinding benchmark: " << iterations << " routes across " << size << "x" << size
                  << " map in " << micros << "us (" << (micros / iterations) << "us/route).\n";
}
}

int main() {
        game_config::load_achievements();
        test_route_rejects_invalid_or_hidden_destinations();
        test_route_avoids_blocked_tiles_and_uses_diagonals();
        test_interactable_tiles_only_allowed_as_destination();
        test_hero_movement_costs_and_boundaries();
        test_pathfinding_skill_allows_zero_movement_pickups();
        benchmark_pathfinding();

        if(failures != 0) {
                std::cerr << failures << " adventure map test(s) failed.\n";
                return EXIT_FAILURE;
        }

        std::cout << "All adventure map tests passed.\n";
        return EXIT_SUCCESS;
}
