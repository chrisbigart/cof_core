#include "core/game.h"
#include "core/game_config.h"
#include "rl/combat_actions.h"
#include "rl/combat_environment.h"
#include "rl/combat_observation.h"
#include "rl/combat_training.h"

#include <algorithm>
#include <array>
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
                  << "  --hidden-layer <int>         Append a hidden layer width (can repeat, default: 512,256,128)\n"
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
                result = {512, 256, 128};
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

rl::combat::combat_scenario_spec_t build_default_scenario(std::mt19937& rng,
                                                           const std::unordered_map<std::string, std::string>& options) {
        using rl::combat::combat_scenario_spec_t;
        using rl::combat::hero_loadout_spec_t;
        using rl::combat::troop_stack_spec_t;

        const std::array<unit_type_e, 12> unit_pool = {
                UNIT_INFANTRYMAN, UNIT_ARCHER,    UNIT_CAVALIER,  UNIT_CRUSADER,
                UNIT_VAMPIRE,     UNIT_LICH,      UNIT_MINOTAUR,  UNIT_MANTICORE,
                UNIT_PIXIE,       UNIT_ELF,       UNIT_DRUID,     UNIT_TITAN,
        };
        const std::array<hero_class_e, 6> hero_classes = {
                HERO_KNIGHT, HERO_BARBARIAN, HERO_NECROMANCER, HERO_SORCERESS, HERO_WARLOCK, HERO_WIZARD,
        };
        const std::array<battlefield_environment_e, 6> environments = {
                BATTLEFIELD_ENVIRONMENT_GRASS,
                BATTLEFIELD_ENVIRONMENT_DIRT,
                BATTLEFIELD_ENVIRONMENT_DESERT,
                BATTLEFIELD_ENVIRONMENT_SNOW,
                BATTLEFIELD_ENVIRONMENT_SWAMP,
                BATTLEFIELD_ENVIRONMENT_WASTELAND,
        };

        const int min_stack_size = get_option_or_default(options, "min_stack_size", 8);
        const int max_stack_size = std::max(min_stack_size, get_option_or_default(options, "max_stack_size", 40));
        const int attacker_min_stacks = std::max(1, get_option_or_default(options, "attacker_min_stacks", 1));
        const int attacker_max_stacks = std::max(attacker_min_stacks, get_option_or_default(options, "attacker_max_stacks", 3));
        const int defender_min_stacks = std::max(1, get_option_or_default(options, "defender_min_stacks", 1));
        const int defender_max_stacks = std::max(defender_min_stacks, get_option_or_default(options, "defender_max_stacks", 3));
        const int min_spells = std::max(1, get_option_or_default(options, "min_spells", 2));
        const int max_spells = std::max(min_spells, get_option_or_default(options, "max_spells", static_cast<int>(rl::combat::SPELL_ACTION_COUNT)));

        std::uniform_int_distribution<int> stack_size_dist(min_stack_size, max_stack_size);
        std::uniform_int_distribution<int> attacker_stack_count_dist(attacker_min_stacks, attacker_max_stacks);
        std::uniform_int_distribution<int> defender_stack_count_dist(defender_min_stacks, defender_max_stacks);
        std::uniform_int_distribution<std::size_t> unit_dist(0, unit_pool.size() - 1);
        std::uniform_int_distribution<std::size_t> class_dist(0, hero_classes.size() - 1);
        std::uniform_int_distribution<int> level_dist(1, 5);
        std::uniform_int_distribution<int> stat_dist(1, 6);
        std::uniform_int_distribution<int> mana_bonus_dist(10, 30);

        auto populate_spellbook = [&](hero_loadout_spec_t& hero) {
                std::array<spell_e, rl::combat::SPELL_ACTION_COUNT> spells{};
                for(std::size_t index = 0; index < rl::combat::SPELL_ACTION_COUNT; ++index)
                        spells[index] = rl::combat::kSpellActions[index].spell;

                std::shuffle(spells.begin(), spells.end(), rng);
                const int spell_count = std::min(static_cast<int>(rl::combat::SPELL_ACTION_COUNT),
                                                 std::uniform_int_distribution<int>(min_spells, max_spells)(rng));
                hero.spellbook.assign(spells.begin(), spells.begin() + spell_count);
        };

        auto populate_army = [&](hero_loadout_spec_t& hero, bool attacker_side, int override_unit_type) {
                hero.troops.fill(troop_stack_spec_t());
                const int stack_slots = attacker_side ? attacker_stack_count_dist(rng) : defender_stack_count_dist(rng);
                for(int slot = 0; slot < stack_slots && slot < static_cast<int>(hero.troops.size()); ++slot) {
                        unit_type_e unit = (override_unit_type >= 0 && slot == 0)
                                                   ? static_cast<unit_type_e>(override_unit_type)
                                                   : unit_pool[unit_dist(rng)];
                        const int size = std::max(1, stack_size_dist(rng));
                        hero.troops[slot] = troop_stack_spec_t{unit, static_cast<uint16_t>(size)};
                }
        };

        auto configure_hero = [&](hero_loadout_spec_t& hero, bool attacker_side) {
                hero.hero_class = hero_classes[class_dist(rng)];
                hero.level = static_cast<uint8_t>(level_dist(rng));
                hero.attack = static_cast<uint8_t>(stat_dist(rng));
                hero.defense = static_cast<uint8_t>(stat_dist(rng));
                hero.power = static_cast<uint8_t>(stat_dist(rng));
                hero.knowledge = static_cast<uint8_t>(std::max(1, stat_dist(rng)));
                hero.mana = static_cast<uint16_t>(hero.knowledge * 10 + mana_bonus_dist(rng));
                hero.human_controlled = true;
                populate_spellbook(hero);
                const int override_unit = attacker_side ? get_option_or_default(options, "attacker_unit_type", -1)
                                                        : get_option_or_default(options, "defender_unit_type", -1);
                populate_army(hero, attacker_side, override_unit);
        };

        hero_loadout_spec_t attacker;
        attacker.player = PLAYER_1;
        attacker.hero_id = 1;
        attacker.name = "RL Attacker";
        configure_hero(attacker, true);

        hero_loadout_spec_t defender;
        defender.player = PLAYER_2;
        defender.hero_id = 2;
        defender.name = "RL Defender";
        configure_hero(defender, false);

        combat_scenario_spec_t scenario;
        scenario.attacker = attacker;
        scenario.defender = defender;
        const int environment_override = get_option_or_default(options, "environment", -1);
        if(environment_override >= 0) {
                scenario.environment = static_cast<battlefield_environment_e>(environment_override);
        } else {
                std::uniform_int_distribution<std::size_t> environment_dist(0, environments.size() - 1);
                scenario.environment = environments[environment_dist(rng)];
        }
        scenario.quick_combat = get_bool_option_or_default(options, "quick_combat", false);
        scenario.is_deathmatch = get_bool_option_or_default(options, "is_deathmatch", false);
        return scenario;
}

rl::combat::controlled_side_t parse_side(const std::string& value) {
        const auto normalized = to_lower(value);
        if(normalized == "attacker")
                return rl::combat::controlled_side_t::ATTACKER;
        if(normalized == "defender")
                return rl::combat::controlled_side_t::DEFENDER;
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

        rl::combat::controlled_side_t side;
        try {
                side = parse_side(options.side);
        } catch(const std::exception& ex) {
                std::cerr << "Error: " << ex.what() << "\n";
                return 1;
        }

        rl::combat::combat_environment_t environment(game_instance, side);

        const auto hidden_layers = resolve_hidden_layers(options.hidden_layers);
        rl::combat::CombatNetworkOptions policy_options;
        policy_options.hidden_layers = hidden_layers;
        policy_options.use_layer_norm = options.layer_norm;

        torch::Device device = torch::kCPU;
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

        const auto observation_dim = rl::combat::observation_feature_count();
        rl::combat::combat_agent_t policy_agent(observation_dim, environment.action_count(), policy_options, device);
        rl::combat::combat_agent_t target_agent(observation_dim, environment.action_count(), policy_options, device);

        const std::optional<uint32_t> seed_opt = options.seed ? std::optional<uint32_t>(static_cast<uint32_t>(*options.seed))
                                                              : std::optional<uint32_t>();

        rl::combat::replay_buffer_t replay_buffer(static_cast<std::size_t>(options.buffer_capacity), seed_opt);

        rl::combat::dqn_config_t config;
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

        rl::combat::epsilon_schedule_t epsilon_schedule;
        epsilon_schedule.start = options.epsilon_start;
        epsilon_schedule.end = options.epsilon_end;
        epsilon_schedule.decay_steps = static_cast<std::size_t>(std::max(options.epsilon_decay, 0));

        std::vector<std::size_t> action_mapping(rl::combat::ACTION_COUNT);
        std::iota(action_mapping.begin(), action_mapping.end(), 0);
        rl::combat::discrete_action_space_t action_space(std::move(action_mapping));

        auto scenario_options = options.scenario_options;
        auto scenario_generator = [scenario_options](std::mt19937& rng) {
                return build_default_scenario(rng, scenario_options);
        };

        auto legal_fn = [&environment](const rl::combat::combat_observation_t&) {
                return std::optional<rl::combat::action_mask_t>(environment.legal_action_mask());
        };

        rl::combat::dqn_trainer_t trainer(environment,
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

        rl::combat::training_metrics_t metrics = trainer.train(static_cast<std::size_t>(options.episodes));

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

