#pragma once

#include "rl/combat_agent.h"
#include "rl/combat_environment.h"

#include <cstddef>
#include <deque>
#include <functional>
#include <memory>
#include <optional>
#include <random>
#include <vector>

namespace rl {
namespace combat {

struct dqn_config_t {
        double discount = 0.99;
        std::size_t batch_size = 64;
        std::size_t minimum_buffer_size = 1'000;
        std::size_t training_frequency = 1;
        std::size_t target_update_frequency = 1'000;
        std::optional<std::size_t> max_steps_per_episode;
        std::optional<double> gradient_clip_norm = 5.0;
        double learning_rate = 1e-3;
};

struct epsilon_schedule_t {
        double start = 1.0;
        double end = 0.05;
        std::size_t decay_steps = 100'000;

        [[nodiscard]] double value(std::size_t step) const;
};

struct training_metrics_t {
        std::vector<float> episode_rewards;
        std::vector<float> losses;
        std::vector<double> epsilon_values;
        std::size_t total_steps = 0;
};

using action_mask_t = std::vector<uint8_t>;

struct transition_t {
        torch::Tensor state;
        int64_t action = 0;
        float reward = 0.0F;
        torch::Tensor next_state;
        bool done = false;
        action_mask_t legal_actions_mask;
        action_mask_t next_legal_actions_mask;
};

class replay_buffer_t {
public:
        explicit replay_buffer_t(std::size_t capacity, std::optional<uint32_t> seed = std::nullopt);

        [[nodiscard]] std::size_t size() const { return data.size(); }
        [[nodiscard]] std::size_t capacity() const { return max_capacity; }

        void clear();
        void append(const transition_t& transition);
        [[nodiscard]] std::vector<transition_t> sample(std::size_t batch_size);
        [[nodiscard]] bool ready_for_training(std::size_t minimum_size) const;

private:
        std::size_t max_capacity;
        std::deque<transition_t> data;
        std::mt19937 rng;
};

class discrete_action_space_t {
public:
        explicit discrete_action_space_t(std::vector<std::size_t> mapping);

        [[nodiscard]] std::size_t size() const { return actions.size(); }
        [[nodiscard]] std::size_t to_native(std::size_t index) const;
        [[nodiscard]] const std::vector<std::size_t>& all() const { return actions; }

private:
        std::vector<std::size_t> actions;
};

using scenario_generator_t = std::function<combat_scenario_spec_t(std::mt19937& rng)>;
using legal_action_fn_t = std::function<std::optional<action_mask_t>(const combat_observation_t&)>;

class dqn_trainer_t {
public:
        dqn_trainer_t(combat_environment_t& environment,
                      combat_agent_t& policy_agent,
                      combat_agent_t& target_agent,
                      replay_buffer_t& replay_buffer,
                      discrete_action_space_t action_space,
                      dqn_config_t config,
                      epsilon_schedule_t epsilon_schedule,
                      scenario_generator_t scenario_generator,
                      legal_action_fn_t legal_action_fn = {},
                      torch::Device device = torch::kCPU,
                      std::optional<uint32_t> seed = std::nullopt);

        [[nodiscard]] training_metrics_t train(std::size_t episodes);

private:
        [[nodiscard]] double sample_epsilon() const;
        [[nodiscard]] std::optional<action_mask_t> compute_legal_mask(const combat_observation_t& observation) const;
        [[nodiscard]] int64_t select_action(const torch::Tensor& state,
                                            double epsilon,
                                            const std::optional<action_mask_t>& legal_mask);
        [[nodiscard]] int64_t sample_random_action(const std::optional<action_mask_t>& legal_mask);
        [[nodiscard]] std::vector<std::size_t> resolve_legal_indices(const std::optional<action_mask_t>& legal_mask) const;
        [[nodiscard]] torch::Tensor apply_legal_mask(const torch::Tensor& q_values,
                                                     const std::optional<action_mask_t>& legal_mask) const;
        [[nodiscard]] torch::Tensor apply_legal_mask_batch(const torch::Tensor& q_values,
                                                           const std::vector<action_mask_t>& masks) const;
        [[nodiscard]] torch::Tensor mask_to_tensor(const action_mask_t& mask, torch::Device target_device) const;
        [[nodiscard]] std::optional<float> optimise_model();
        void update_target_network();

        combat_environment_t* environment;
        combat_agent_t* policy_agent;
        combat_agent_t* target_agent;
        replay_buffer_t* replay_buffer;
        discrete_action_space_t action_space;
        dqn_config_t config;
        epsilon_schedule_t epsilon_schedule;
        scenario_generator_t scenario_generator;
        legal_action_fn_t legal_action_fn;
        torch::Device device;
        std::unique_ptr<torch::optim::Adam> optimizer;
        std::mt19937 rng;
        std::size_t global_step = 0;
};

} // namespace combat
} // namespace rl

