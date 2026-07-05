#include "combat_environment.h"

#include "battle_sim.h"

#include <algorithm>

float terminal_reward(battle_result_e result, controlled_side_t side) {
        switch(result) {
        case BATTLE_ATTACKER_VICTORY:
        case BATTLE_ATTACKER_HAS_FLED:
                return side == controlled_side_t::ATTACKER ? 1.0F : -1.0F;
        case BATTLE_DEFENDER_VICTORY:
        case BATTLE_DEFENDER_HAS_FLED:
                return side == controlled_side_t::DEFENDER ? 1.0F : -1.0F;
        case BATTLE_BOTH_LOSE:
                return -1.0F;
        case BATTLE_IN_PROGRESS:
        default:
                return 0.0F;
        }
}

combat_environment_t::combat_environment_t(game_t& game_instance, controlled_side_t side)
        : game_instance(&game_instance)
        , session_instance(game_instance)
        , side_controlled(side) {
        session_instance.simulator.game().battle.fn_emit_combat_action = [this] (const battle_action_t& action) {
            if(record_actions)
                action_history.push_back(action);
        };
}

void combat_environment_t::configure(const combat_scenario_spec_t& spec) {
        auto adjusted = spec;
        const bool controls_attacker = side_controlled == controlled_side_t::ATTACKER;
        adjusted.attacker.human_controlled = controls_attacker;
        adjusted.defender.human_controlled = !controls_attacker;
        session_instance.configure(adjusted);
        scenario_ready = true;
}

combat_observation_t combat_environment_t::reset() {
        if(!scenario_ready)
                return combat_observation_t();
        
        action_history.clear();
        session_instance.reset();
        ensure_agent_turn();
        return capture_observation(session_instance);
}

std::tuple<combat_observation_t, float, bool, battle_result_e> combat_environment_t::step(combat_action_type_t action_type) {
        if(!scenario_ready)
                return std::make_tuple(combat_observation_t(), 0.0F, true, BATTLE_IN_PROGRESS);

        float reward = 0.0F;
        bool applied = session_instance.apply_action(action_type);
        if(!applied)
                reward -= 0.1F;

        auto result = session_instance.step();
        bool done = result != BATTLE_IN_PROGRESS;
        if(done) {
                reward += terminal_reward(result, side_controlled);
                return std::make_tuple(capture_observation(session_instance), reward, true, result);
        }

        ensure_agent_turn();
        auto observation = capture_observation(session_instance);
        return std::make_tuple(observation, reward, false, result);
}

void combat_environment_t::ensure_agent_turn() {
        if(!scenario_ready)
                return;

        auto& battlefield = session_instance.simulator.battlefield();
        while(true) {
                const auto* active = battlefield.get_active_unit();
                if(!active)
                        break;
                const bool controls_attacker = side_controlled == controlled_side_t::ATTACKER;
                if((active->is_attacker && controls_attacker) || (!active->is_attacker && !controls_attacker))
                        break;

                auto result = session_instance.step();
                if(result != BATTLE_IN_PROGRESS)
                        break;
        }
}

void combat_environment_t::set_controlled_side(controlled_side_t side) {
        side_controlled = side;
}

