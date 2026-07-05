#include "battle_sim.h"

#include <algorithm>
#include <utility>

void ensure_player_table(game_t& game_instance) {
        if(game_instance.players.size() < game_config::MAX_NUMBER_OF_PLAYERS) {
                game_instance.players.resize(game_config::MAX_NUMBER_OF_PLAYERS);
        }

        for(uint i = 0; i < game_config::MAX_NUMBER_OF_PLAYERS; ++i) {
                const auto player_id = static_cast<player_e>(i + 1);
                auto& player_entry = game_instance.players[i];
                player_entry.player_number = player_id;

                auto& config_entry = game_instance.game_configuration.player_configs[i];
                config_entry.player_number = player_id;
        }
}


battle_sim_t::battle_sim_t(game_t& game_instance) : game_instance(&game_instance) {
        initialize_players_if_needed();
        update_emit_callback();
}

void battle_sim_t::set_options(const battle_sim_options_t& options) {
        current_options = options;
        update_emit_callback();
}

void battle_sim_t::set_single_step(bool single_step) {
        current_options.single_step = single_step;
}

void battle_sim_t::set_emit_callback(std::function<void(const battle_action_t&)> callback) {
        current_options.emit_action = std::move(callback);
        update_emit_callback();
}

void battle_sim_t::clear_emit_callback() {
        current_options.emit_action = nullptr;
        update_emit_callback();
}

void battle_sim_t::set_player_control(player_e player, bool is_human) {
        initialize_players_if_needed();

        if(!game_instance || !game_instance->player_valid(player))
                return;

        auto& player_entry = game_instance->get_player(player);
        player_entry.player_number = player;
        player_entry.is_human = is_human;

        auto index = static_cast<size_t>(player) - 1;
        auto& config_entry = game_instance->game_configuration.player_configs[index];
        config_entry.player_number = player;
        config_entry.is_human = is_human;
}

void battle_sim_t::set_players_controlled(const std::vector<player_e>& players) {
        initialize_players_if_needed();

        if(!game_instance)
                return;

        for(uint i = 0; i < game_config::MAX_NUMBER_OF_PLAYERS; ++i) {
                const auto player_id = static_cast<player_e>(i + 1);
                const bool is_human = std::find(players.begin(), players.end(), player_id) != players.end();
                set_player_control(player_id, is_human);
        }
}

battle_result_e battle_sim_t::step() {
        if(!game_instance)
                return BATTLE_IN_PROGRESS;

        return game_instance->battle.update_battle(game_instance, current_options.single_step);
}

battlefield_t& battle_sim_t::battlefield() {
        return game_instance->battle;
}

const battlefield_t& battle_sim_t::battlefield() const {
        return game_instance->battle;
}

game_t& battle_sim_t::game() {
        return *game_instance;
}

const game_t& battle_sim_t::game() const {
        return *game_instance;
}

void battle_sim_t::initialize_players_if_needed() {
        if(!game_instance)
                return;

        ensure_player_table(*game_instance);
}

void battle_sim_t::update_emit_callback() {
        if(!game_instance)
                return;

        if(current_options.emit_action) {
                game_instance->battle.fn_emit_combat_action = [this](const battle_action_t& action) {
                        if(current_options.emit_action)
                                current_options.emit_action(action);
                };
        } else {
                game_instance->battle.fn_emit_combat_action = [](const battle_action_t&) {};
        }
}

void assign_human_players(game_t& game_instance, const std::vector<player_e>& human_players) {
        ensure_player_table(game_instance);

        for(uint i = 0; i < game_config::MAX_NUMBER_OF_PLAYERS; ++i) {
                const auto player_id = static_cast<player_e>(i + 1);
                const bool is_human = std::find(human_players.begin(), human_players.end(), player_id) != human_players.end();

                auto& player_entry = game_instance.players[i];
                player_entry.player_number = player_id;
                player_entry.is_human = is_human;

                auto& config_entry = game_instance.game_configuration.player_configs[i];
                config_entry.player_number = player_id;
                config_entry.is_human = is_human;
        }
}

