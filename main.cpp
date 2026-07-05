#include "core/game.h"
#include "core/game_config.h"
#include "rl/combat_environment.h"
#include "rl/combat_observation.h"
#include "rl/combat_training.h"

#include <algorithm>
#include <cmath>
#include <cctype>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <optional>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

struct cli_options_t {
        int episodes = 5;
        double learning_rate = 1e-3;
        int batch_size = 32;
        int minimum_buffer = 64;
        int buffer_capacity = 10'000;
        int training_frequency = 1;
        int target_update = 250;
        std::optional<int> max_steps = 128;
        double discount = 0.99;
        std::optional<double> gradient_clip = 5.0;
        double epsilon_start = 1.0;
        double epsilon_end = 0.05;
        int epsilon_decay = 5'000;
        std::vector<int> hidden_layers;
        bool layer_norm = false;
        std::optional<std::string> device;
        std::string side = "attacker";
        std::optional<int> seed = 42;
        std::unordered_map<std::string, std::string> scenario_options;
        bool show_help = false;
};

std::string to_lower(std::string value) {
        std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
        return value;
}

bool parse_bool(const std::string& value) {
        const auto normalized = to_lower(value);
        if(normalized == "true" || normalized == "1" || normalized == "yes")
                return true;
        if(normalized == "false" || normalized == "0" || normalized == "no")
                return false;
        throw std::invalid_argument("Expected boolean value (true/false)");
}

int parse_int(const std::string& value, const std::string& name) {
        try {
                return std::stoi(value);
        } catch(const std::exception&) {
                throw std::invalid_argument("Invalid integer for " + name + ": " + value);
        }
}

double parse_double(const std::string& value, const std::string& name) {
        try {
                return std::stod(value);
        } catch(const std::exception&) {
                throw std::invalid_argument("Invalid float for " + name + ": " + value);
        }
}

void print_usage(const char* program) {
        std::cout << "Usage: " << program << " [options]\n"
                  << "Options:\n"
                  << "  --episodes <int>             Number of training episodes (default: 5)\n"
                  << "  --learning-rate <float>      Optimizer learning rate (default: 1e-3)\n"
                  << "  --batch-size <int>           Training batch size (default: 32)\n"
                  << "  --minimum-buffer <int>       Minimum replay buffer size before training (default: 64)\n"
                  << "  --buffer-capacity <int>      Replay buffer capacity (default: 10000)\n"
                  << "  --training-frequency <int>   Optimisation frequency in environment steps (default: 1)\n"
                  << "  --target-update <int>        Target network update frequency (default: 250)\n"
                  << "  --max-steps <int>            Maximum steps per episode (default: 128, <=0 disables)\n"
                  << "  --discount <float>           Discount factor gamma (default: 0.99)\n"
                  << "  --gradient-clip <float>      Gradient clipping norm (default: 5.0, negative disables)\n"
                  << "  --epsilon-start <float>      Initial epsilon for exploration (default: 1.0)\n"
                  << "  --epsilon-end <float>        Final epsilon value (default: 0.05)\n"
                  << "  --epsilon-decay <int>        Steps to anneal epsilon (default: 5000)\n"
                  << "  --hidden-layer <int>         Append a hidden layer width (can repeat)\n"
                  << "  --layer-norm[=bool]          Enable layer normalization in policy network\n"
                  << "  --device <str>               Torch device (e.g. cpu or cuda:0)\n"
                  << "  --side <attacker|defender>   Controlled combat side (default: attacker)\n"
                  << "  --seed <int>                 Random seed (default: 42)\n"
                  << "  --scenario-option key=value  Override scenario parameter (repeatable)\n"
                  << "  --help                       Show this message\n";
}

void apply_option(cli_options_t& options, const std::string& key, const std::string& value) {
        if(key == "episodes") {
                options.episodes = parse_int(value, "--episodes");
        } else if(key == "learning-rate") {
                options.learning_rate = parse_double(value, "--learning-rate");
        } else if(key == "batch-size") {
                options.batch_size = parse_int(value, "--batch-size");
        } else if(key == "minimum-buffer") {
                options.minimum_buffer = parse_int(value, "--minimum-buffer");
        } else if(key == "buffer-capacity") {
                options.buffer_capacity = parse_int(value, "--buffer-capacity");
        } else if(key == "training-frequency") {
                options.training_frequency = parse_int(value, "--training-frequency");
        } else if(key == "target-update") {
                options.target_update = parse_int(value, "--target-update");
        } else if(key == "max-steps") {
                options.max_steps = parse_int(value, "--max-steps");
        } else if(key == "discount") {
                options.discount = parse_double(value, "--discount");
        } else if(key == "gradient-clip") {
                options.gradient_clip = parse_double(value, "--gradient-clip");
        } else if(key == "epsilon-start") {
                options.epsilon_start = parse_double(value, "--epsilon-start");
        } else if(key == "epsilon-end") {
                options.epsilon_end = parse_double(value, "--epsilon-end");
        } else if(key == "epsilon-decay") {
                options.epsilon_decay = parse_int(value, "--epsilon-decay");
        } else if(key == "hidden-layer") {
                options.hidden_layers.push_back(parse_int(value, "--hidden-layer"));
        } else if(key == "layer-norm") {
                options.layer_norm = parse_bool(value);
        } else if(key == "device") {
                options.device = value;
        } else if(key == "side") {
                options.side = to_lower(value);
        } else if(key == "seed") {
                options.seed = parse_int(value, "--seed");
        } else if(key == "scenario-option") {
                const auto pos = value.find('=');
                if(pos == std::string::npos)
                        throw std::invalid_argument("Scenario option must be key=value");
                auto key_name = to_lower(value.substr(0, pos));
                auto option_value = value.substr(pos + 1);
                options.scenario_options[key_name] = option_value;
        } else {
                throw std::invalid_argument("Unknown option --" + key);
        }
}

cli_options_t parse_arguments(int argc, char** argv) {
        cli_options_t options;

        for(int index = 1; index < argc; ++index) {
                std::string argument = argv[index];
                if(argument == "--help" || argument == "-h") {
                        options.show_help = true;
                        continue;
                }

                if(argument.rfind("--", 0) != 0)
                        throw std::invalid_argument("Unexpected positional argument: " + argument);

                argument.erase(0, 2);
                std::string key;
                std::string value;
                const auto equals_position = argument.find('=');
                if(equals_position != std::string::npos) {
                        key = argument.substr(0, equals_position);
                        value = argument.substr(equals_position + 1);
                } else {
                        key = argument;
                        if(key == "layer-norm") {
                                options.layer_norm = true;
                                continue;
                        }
                        if(index + 1 >= argc)
                                throw std::invalid_argument("Option --" + key + " requires a value");
                        value = argv[++index];
                }

                apply_option(options, to_lower(key), value);
        }

        return options;
}

std::vector<std::size_t> resolve_hidden_layers(const std::vector<int>& values) {
        std::vector<std::size_t> result;
        for(int value : values) {
                if(value > 0)
                        result.push_back(static_cast<std::size_t>(value));
        }
        if(result.empty())
                result = {256, 128};
        return result;
}

int get_option_or_default(const std::unordered_map<std::string, std::string>& options,
                          const std::string& key,
                          int default_value) {
        auto it = options.find(key);
        if(it == options.end())
                return default_value;
        return parse_int(it->second, key);
}

bool get_bool_option_or_default(const std::unordered_map<std::string, std::string>& options,
                                const std::string& key,
                                bool default_value) {
        auto it = options.find(key);
        if(it == options.end())
                return default_value;
        return parse_bool(it->second);
}

combat_scenario_spec_t build_default_scenario(std::mt19937& rng,
                                                           const std::unordered_map<std::string, std::string>& options) {
        const int attacker_unit = get_option_or_default(options, "attacker_unit_type", 16);
        const int defender_unit = get_option_or_default(options, "defender_unit_type", 16);
        const int attacker_base_stack = get_option_or_default(options, "attacker_stack_size", 20);
        const int defender_base_stack = get_option_or_default(options, "defender_stack_size", 20);
        const int variation = get_option_or_default(options, "stack_size_variation", 0);

        std::uniform_int_distribution<int> delta_distribution(-variation, variation);
        const int attacker_delta = variation > 0 ? delta_distribution(rng) : 0;
        const int defender_delta = variation > 0 ? delta_distribution(rng) : 0;

        const int attacker_stack = std::max(1, attacker_base_stack + attacker_delta);
        const int defender_stack = std::max(1, defender_base_stack + defender_delta);

        hero_loadout_spec_t attacker;
        attacker.player = PLAYER_1;
        attacker.hero_class = HERO_CLASS_KNIGHT;
        attacker.hero_id = 1;
        attacker.name = "RL Attacker";
        attacker.level = 1;
        attacker.attack = 2;
        attacker.defense = 2;
        attacker.power = 1;
        attacker.knowledge = 1;
        attacker.mana = 12;
        attacker.human_controlled = true;
        attacker.troops[0] = troop_stack_spec_t{static_cast<unit_type_e>(attacker_unit), static_cast<uint16_t>(attacker_stack)};

        hero_loadout_spec_t defender;
        defender.player = PLAYER_2;
        defender.hero_class = HERO_CLASS_KNIGHT;
        defender.hero_id = 2;
        defender.name = "RL Defender";
        defender.level = 1;
        defender.attack = 2;
        defender.defense = 2;
        defender.power = 1;
        defender.knowledge = 1;
        defender.mana = 12;
        defender.human_controlled = true;
        defender.troops[0] = troop_stack_spec_t{static_cast<unit_type_e>(defender_unit), static_cast<uint16_t>(defender_stack)};

        combat_scenario_spec_t scenario;
        scenario.attacker = attacker;
        scenario.defender = defender;
        scenario.environment = static_cast<battlefield_environment_e>(
                get_option_or_default(options, "environment", static_cast<int>(BATTLEFIELD_ENVIRONMENT_GRASS)));
        scenario.quick_combat = get_bool_option_or_default(options, "quick_combat", false);
        scenario.is_deathmatch = get_bool_option_or_default(options, "is_deathmatch", false);
        return scenario;
}

action_mask_t legal_action_mask(const combat_observation_t& observation) {
        action_mask_t mask(ACTION_COUNT, 0);
        const auto auto_index = static_cast<std::size_t>(combat_action_type_t::AUTO_RESOLVE);
        mask[auto_index] = 1;

        if(observation.active_unit_id < 0)
                return mask;

        const bool attacker_active = observation.active_unit_is_attacker;
        const auto x = observation.active_unit_x;
        const auto y = observation.active_unit_y;
        const auto& stacks = attacker_active ? observation.attacker_stacks : observation.defender_stacks;

        const stack_observation_t* active_stack = nullptr;
        for(const auto& stack : stacks) {
                if(!stack.is_alive)
                        continue;
                if(stack.x == x && stack.y == y) {
                        active_stack = &stack;
                        break;
                }
        }

        if(!active_stack || active_stack->is_disabled)
                return mask;

        mask[static_cast<std::size_t>(combat_action_type_t::WAIT)] = active_stack->has_waited ? 0 : 1;
        mask[static_cast<std::size_t>(combat_action_type_t::DEFEND)] = active_stack->has_defended ? 0 : 1;
        return mask;
}

controlled_side_t parse_side(const std::string& value) {
        const auto normalized = to_lower(value);
        if(normalized == "attacker")
                return controlled_side_t::ATTACKER;
        if(normalized == "defender")
                return controlled_side_t::DEFENDER;
        throw std::invalid_argument("--side must be 'attacker' or 'defender'");
}

void validate_options(const cli_options_t& options) {
        if(options.episodes <= 0)
                throw std::invalid_argument("--episodes must be positive");
        if(options.batch_size <= 0)
                throw std::invalid_argument("--batch-size must be positive");
        if(options.minimum_buffer <= 0)
                throw std::invalid_argument("--minimum-buffer must be positive");
        if(options.buffer_capacity <= 0)
                throw std::invalid_argument("--buffer-capacity must be positive");
        if(options.training_frequency <= 0)
                throw std::invalid_argument("--training-frequency must be positive");
        if(options.target_update <= 0)
                throw std::invalid_argument("--target-update must be positive");
        if(options.max_steps && *options.max_steps == 0)
                throw std::invalid_argument("--max-steps must be non-zero if provided");
        if(options.epsilon_start < options.epsilon_end)
                throw std::invalid_argument("Require epsilon-start >= epsilon-end");
        if(options.epsilon_start < 0.0 || options.epsilon_start > 1.0 || options.epsilon_end < 0.0 || options.epsilon_end > 1.0)
                throw std::invalid_argument("Epsilon values must be within [0, 1]");
        if(options.epsilon_decay < 0)
                throw std::invalid_argument("--epsilon-decay must be non-negative");
        if(options.seed && *options.seed < 0)
                throw std::invalid_argument("--seed must be non-negative");
}

double compute_mean(const std::vector<float>& values) {
        if(values.empty())
                return 0.0;
        const double sum = std::accumulate(values.begin(), values.end(), 0.0);
        return sum / static_cast<double>(values.size());
}

double compute_stddev(const std::vector<float>& values, double mean) {
        if(values.size() <= 1)
                return 0.0;
        double variance = 0.0;
        for(float value : values) {
                const double diff = static_cast<double>(value) - mean;
                variance += diff * diff;
        }
        variance /= static_cast<double>(values.size());
        return std::sqrt(variance);
}

} // namespace

int main(int argc, char** argv) {
        cli_options_t options;
        try {
                options = parse_arguments(argc, argv);
        } catch(const std::exception& ex) {
                std::cerr << "Error: " << ex.what() << "\n";
                print_usage(argv[0]);
                return 1;
        }

        if(options.show_help) {
                print_usage(argv[0]);
                return 0;
        }

        try {
                validate_options(options);
        } catch(const std::exception& ex) {
                std::cerr << "Error: " << ex.what() << "\n";
                return 1;
        }

        game_config::load_game_data();
        game_t game_instance;

        controlled_side_t side;
        try {
                side = parse_side(options.side);
        } catch(const std::exception& ex) {
                std::cerr << "Error: " << ex.what() << "\n";
                return 1;
        }

        combat_environment_t environment(game_instance, side);

        const auto hidden_layers = resolve_hidden_layers(options.hidden_layers);
        CombatNetworkOptions policy_options;
        policy_options.hidden_layers = hidden_layers;
        policy_options.use_layer_norm = options.layer_norm;

        torch::Device device = torch::kCUDA;
        if(options.device) {
                try {
                        device = torch::Device(*options.device);
                } catch(const std::exception& ex) {
                        std::cerr << "Error: invalid device string '" << *options.device << "': " << ex.what() << "\n";
                        return 1;
                }
        } else {
                device = torch::cuda::is_available() ? torch::Device(torch::kCUDA) : torch::Device(torch::kCPU);
        }

        if(options.seed) {
                torch::manual_seed(*options.seed);
                if(torch::cuda::is_available())
                        torch::cuda::manual_seed_all(*options.seed);
        }

        const auto observation_dim = observation_feature_count();
        combat_agent_t policy_agent(observation_dim, environment.action_count(), policy_options, device);
        combat_agent_t target_agent(observation_dim, environment.action_count(), policy_options, device);

        const std::optional<uint32_t> seed_opt = options.seed ? std::optional<uint32_t>(static_cast<uint32_t>(*options.seed))
                                                              : std::optional<uint32_t>();

        replay_buffer_t replay_buffer(static_cast<std::size_t>(options.buffer_capacity), seed_opt);

        dqn_config_t config;
        config.discount = options.discount;
        config.batch_size = static_cast<std::size_t>(options.batch_size);
        config.minimum_buffer_size = static_cast<std::size_t>(options.minimum_buffer);
        config.training_frequency = static_cast<std::size_t>(options.training_frequency);
        config.target_update_frequency = static_cast<std::size_t>(options.target_update);
        config.max_steps_per_episode = (options.max_steps && *options.max_steps > 0)
                                              ? std::optional<std::size_t>(static_cast<std::size_t>(*options.max_steps))
                                              : std::optional<std::size_t>();
        config.gradient_clip_norm = (options.gradient_clip && *options.gradient_clip >= 0.0)
                                            ? options.gradient_clip
                                            : std::optional<double>();
        config.learning_rate = options.learning_rate;

        epsilon_schedule_t epsilon_schedule;
        epsilon_schedule.start = options.epsilon_start;
        epsilon_schedule.end = options.epsilon_end;
        epsilon_schedule.decay_steps = static_cast<std::size_t>(std::max(options.epsilon_decay, 0));

        discrete_action_space_t action_space({
                combat_action_type_t::WAIT,
                combat_action_type_t::DEFEND,
                combat_action_type_t::AUTO_RESOLVE,
        });

        auto scenario_options = options.scenario_options;
        auto scenario_generator = [scenario_options](std::mt19937& rng) {
                return build_default_scenario(rng, scenario_options);
        };

        auto legal_fn = [](const combat_observation_t& observation) {
                return std::optional<action_mask_t>(legal_action_mask(observation));
        };

        dqn_trainer_t trainer(environment,
                                          policy_agent,
                                          target_agent,
                                          replay_buffer,
                                          action_space,
                                          config,
                                          epsilon_schedule,
                                          scenario_generator,
                                          legal_fn,
                                          device,
                                          seed_opt);

        training_metrics_t metrics = trainer.train(static_cast<std::size_t>(options.episodes));

        std::cout << "Completed " << metrics.episode_rewards.size() << " episodes / " << metrics.total_steps
                  << " environment steps\n";

        if(!metrics.episode_rewards.empty()) {
                const double mean = compute_mean(metrics.episode_rewards);
                const double stddev = compute_stddev(metrics.episode_rewards, mean);
                std::cout << std::fixed << std::setprecision(3)
                          << "Episode reward: mean=" << mean << " std=" << stddev << "\n";
        }

        if(!metrics.losses.empty()) {
                const double mean_loss = compute_mean(metrics.losses);
                std::cout << std::fixed << std::setprecision(6) << "Average loss: " << mean_loss << "\n";
        }

        if(!metrics.epsilon_values.empty()) {
                std::cout << std::fixed << std::setprecision(3)
                          << "Epsilon schedule: start=" << metrics.epsilon_values.front()
                          << " final=" << metrics.epsilon_values.back() << "\n";
        }

        return 0;
}

