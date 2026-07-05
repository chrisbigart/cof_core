#include "core/game_config.h"
#include "core/resource.h"
#include "core/troop.h"
#include "core/utils.h"

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

void test_resource_group_arithmetic() {
        resource_group_t stockpile(10, 5, 2, 3, 4, 6, 1000);
        const resource_group_t cost(3, 2, 1, 1, 1, 2, 250);

        expect_true(stockpile.covers_cost(cost), "stockpile should cover smaller resource cost");

        stockpile -= cost;
        expect_true(stockpile.get_value_for_type(RESOURCE_WOOD) == 7, "wood should be subtracted");
        expect_true(stockpile.get_value_for_type(RESOURCE_ORE) == 3, "ore should be subtracted");
        expect_true(stockpile.get_value_for_type(RESOURCE_GOLD) == 750, "gold should be subtracted");

        stockpile.add_value(RESOURCE_GOLD, MAX_RESOURCE_VALUE);
        expect_true(stockpile.get_value_for_type(RESOURCE_GOLD) == MAX_RESOURCE_VALUE, "gold addition should saturate at max value");

        stockpile.sub_value(RESOURCE_GEMS, 100);
        expect_true(stockpile.get_value_for_type(RESOURCE_GEMS) == 0, "resource subtraction should clamp at zero");
}

void test_troop_group_empty_state() {
        troop_group_t troops{};
        expect_true(is_troop_group_empty(troops), "default troop group should be empty");

        troops[0] = troop_t(UNIT_SKELETON, 12);
        expect_true(!is_troop_group_empty(troops), "troop group with a stack should not be empty");

        troops[0].clear();
        expect_true(is_troop_group_empty(troops), "cleared troop stack should make the group empty again");
}

void test_distance_helpers() {
        expect_true(utils::mh_dist(0, 0, 3, -4) == 7, "Manhattan distance should sum axis deltas");
        expect_true(utils::eu_dist(0, 0, 3, 4) == 25, "squared Euclidean distance should use squared deltas");
}
}

int main() {
        test_resource_group_arithmetic();
        test_troop_group_empty_state();
        test_distance_helpers();

        if(failures != 0) {
                std::cerr << failures << " basic core test(s) failed.\n";
                return EXIT_FAILURE;
        }

        std::cout << "All basic core tests passed.\n";
        return EXIT_SUCCESS;
}
