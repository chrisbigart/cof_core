#include "core/game.h"
#include "core/game_config.h"

#include <cstdlib>
#include <iostream>
#include <set>
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

void expect_eq(size_t actual, size_t expected, const std::string& message) {
        if(actual != expected) {
                std::cerr << "FAIL: " << message << " (expected " << expected << ", got " << actual << ")\n";
                ++failures;
        }
}

uint64_t first_threshold_or_one(const achievement_t& achievement) {
        for(const auto& tier : achievement.tiers) {
                if(tier.value != 0)
                        return tier.value;
        }

        return 1;
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

        test_achievement_config_is_well_formed();
        test_every_configured_achievement_can_fire_callback();
        test_achievement_threshold_crossing_semantics();
        test_one_shot_achievement_semantics();

        if(failures != 0) {
                std::cerr << failures << " achievement test(s) failed.\n";
                return EXIT_FAILURE;
        }

        std::cout << "All achievement tests passed.\n";
        return EXIT_SUCCESS;
}
