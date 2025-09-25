#pragma once

#include "core/game.h"

#include <functional>
#include <vector>

namespace rl {
namespace combat {

struct battle_sim_options_t {
        bool single_step = true;
        std::function<void(const battle_action_t&)> emit_action;
};

class battle_sim_t {
public:
        explicit battle_sim_t(game_t& game_instance);

        void set_options(const battle_sim_options_t& options);
        void set_single_step(bool single_step);
        void set_emit_callback(std::function<void(const battle_action_t&)> callback);
        void clear_emit_callback();

        void set_player_control(player_e player, bool is_human);
        void set_players_controlled(const std::vector<player_e>& players);

        battle_result_e step();

        battlefield_t& battlefield();
        const battlefield_t& battlefield() const;

        game_t& game();
        const game_t& game() const;

private:
        void initialize_players_if_needed();
        void update_emit_callback();

        game_t* game_instance = nullptr;
        battle_sim_options_t current_options;
};

/// Convenience helper to mark every player as AI-controlled except the ones provided in `human_players`.
void assign_human_players(game_t& game_instance, const std::vector<player_e>& human_players);

} // namespace combat
} // namespace rl

