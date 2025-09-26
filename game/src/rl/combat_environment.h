#pragma once

#include "rl/battle_session.h"
#include "rl/combat_actions.h"
#include "rl/combat_observation.h"

#include <tuple>
#include <vector>

namespace rl {
namespace combat {

constexpr std::size_t ACTION_COUNT = static_cast<std::size_t>(combat_action_type_t::COUNT);

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

        std::vector<battle_action_t> consume_action_log();
        const std::vector<battle_action_t>& action_log() const { return action_history; }

private:
        void ensure_agent_turn();

        game_t* game_instance = nullptr;
        combat_session_t session_instance;
        bool scenario_ready = false;
        controlled_side_t side_controlled = controlled_side_t::ATTACKER;
        std::vector<battle_action_t> action_history;
};

combat_action_spec_t action_from_type(combat_action_type_t type);

} // namespace combat
} // namespace rl

