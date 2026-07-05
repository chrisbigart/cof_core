#pragma once

#include "battle_session.h"
#include "combat_observation.h"

#include <tuple>

constexpr std::size_t ACTION_COUNT = 3;

enum class controlled_side_t { ATTACKER, DEFENDER };

class combat_environment_t {
public:
        explicit combat_environment_t(game_t& game_instance, controlled_side_t side = controlled_side_t::ATTACKER);

        void configure(const combat_scenario_spec_t& spec);
        combat_observation_t reset();

        std::tuple<combat_observation_t, float, bool, battle_result_e> step(combat_action_type_t action);

        std::size_t action_count() const { return ACTION_COUNT; }

        void set_controlled_side(controlled_side_t side);
        controlled_side_t controlled_side() const { return side_controlled; }

        combat_session_t& session() { return session_instance; }
        const combat_session_t& session() const { return session_instance; }
        std::vector<battle_action_t> action_history;

        bool record_actions = false;

private:
        void ensure_agent_turn();

        game_t* game_instance = nullptr;
        combat_session_t session_instance;
        bool scenario_ready = false;
        controlled_side_t side_controlled = controlled_side_t::ATTACKER;
};

