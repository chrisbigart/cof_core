#include "rl/combat_training.h"

#include "rl/combat_observation.h"

#include <algorithm>
#include <limits>
#include <numeric>
#include <sstream>
#include <stdexcept>

#include <torch/nn/utils/clip_grad.h>
#include <torch/serialize.h>

namespace rl {
namespace combat {

double epsilon_schedule_t::value(std::size_t step) const {
        if(step >= decay_steps)
                return end;

        if(decay_steps == 0)
                return end;

        const double fraction = static_cast<double>(step) / static_cast<double>(decay_steps);
        return start + fraction * (end - start);
}

replay_buffer_t::replay_buffer_t(std::size_t capacity, std::optional<uint32_t> seed)
        : max_capacity(capacity)
        , rng(seed ? *seed : std::random_device{}()) {
        if(capacity == 0)
                throw std::invalid_argument("Replay buffer capacity must be positive");
}

void replay_buffer_t::clear() {
        data.clear();
}

void replay_buffer_t::append(const transition_t& transition) {
        if(data.size() >= max_capacity)
                data.pop_front();
        data.push_back(transition);
}

std::vector<transition_t> replay_buffer_t::sample(std::size_t batch_size) {
        if(batch_size == 0)
                throw std::invalid_argument("batch_size must be positive");
        if(batch_size > data.size())
                throw std::invalid_argument("Cannot sample more elements than stored in buffer");

        std::vector<std::size_t> indices(data.size());
        std::iota(indices.begin(), indices.end(), 0);
        std::shuffle(indices.begin(), indices.end(), rng);
        indices.resize(batch_size);

        std::vector<transition_t> batch;
        batch.reserve(batch_size);
        for(auto index : indices)
                batch.push_back(data[index]);
        return batch;
}

bool replay_buffer_t::ready_for_training(std::size_t minimum_size) const {
        return data.size() >= minimum_size;
}

discrete_action_space_t::discrete_action_space_t(std::vector<combat_action_type_t> mapping)
        : actions(std::move(mapping)) {
        if(actions.empty())
                throw std::invalid_argument("Action space must contain at least one action");
}

combat_action_type_t discrete_action_space_t::to_native(std::size_t index) const {
        if(index >= actions.size())
                throw std::out_of_range("Action index out of range");
        return actions[index];
}

dqn_trainer_t::dqn_trainer_t(combat_environment_t& environment,
                             combat_agent_t& policy_agent,
                             combat_agent_t& target_agent,
                             replay_buffer_t& replay_buffer,
                             discrete_action_space_t action_space,
                             dqn_config_t config,
                             epsilon_schedule_t epsilon_schedule,
                             scenario_generator_t scenario_generator,
                             legal_action_fn_t legal_action_fn,
                             torch::Device device,
                             std::optional<uint32_t> seed)
        : environment(&environment)
        , policy_agent(&policy_agent)
        , target_agent(&target_agent)
        , replay_buffer(&replay_buffer)
        , action_space(std::move(action_space))
        , config(std::move(config))
        , epsilon_schedule(std::move(epsilon_schedule))
        , scenario_generator(std::move(scenario_generator))
        , legal_action_fn(std::move(legal_action_fn))
        , device(device)
        , rng(seed ? *seed : std::random_device{}()) {
        if(this->config.batch_size == 0)
                throw std::invalid_argument("Batch size must be positive");
        if(this->config.training_frequency == 0)
                throw std::invalid_argument("Training frequency must be positive");
        if(this->config.target_update_frequency == 0)
                throw std::invalid_argument("Target update frequency must be positive");

        this->policy_agent->model()->train();
        this->target_agent->model()->eval();

        optimizer = std::make_unique<torch::optim::Adam>(
                this->policy_agent->model()->parameters(),
                torch::optim::AdamOptions(this->config.learning_rate));

        update_target_network();
}

training_metrics_t dqn_trainer_t::train(std::size_t episodes) {
        training_metrics_t metrics;

        for(std::size_t episode = 0; episode < episodes; ++episode) {
                auto scenario = scenario_generator(rng);
                environment->configure(scenario);
                auto observation = environment->reset();
                auto state_tensor = observation_to_tensor(observation, device);

                const double epsilon = sample_epsilon();
                float cumulative_reward = 0.0F;
                std::vector<float> episode_losses;

                bool terminated = false;
                std::size_t steps = 0;

                while(!terminated) {
                        auto legal_mask = compute_legal_mask(observation);
                        const int64_t action_index = select_action(state_tensor, epsilon, legal_mask);
                        const auto action = action_space.to_native(static_cast<std::size_t>(action_index));

                        auto [next_observation, reward, done, result] = environment->step(action);
                        (void)result;
                        auto next_state_tensor = observation_to_tensor(next_observation, device);
                        auto next_legal_mask = compute_legal_mask(next_observation);

                        bool episode_done = done;
                        bool truncated = false;
                        if(config.max_steps_per_episode && steps + 1 >= *config.max_steps_per_episode && !episode_done) {
                                episode_done = true;
                                truncated = true;
                        }

                        transition_t transition;
                        transition.state = state_tensor.detach().cpu();
                        transition.action = action_index;
                        transition.reward = reward;
                        transition.next_state = next_state_tensor.detach().cpu();
                        transition.done = episode_done;
                        if(legal_mask)
                                transition.legal_actions_mask = *legal_mask;
                        if(next_legal_mask)
                                transition.next_legal_actions_mask = *next_legal_mask;
                        replay_buffer->append(transition);

                        state_tensor = next_state_tensor;
                        observation = next_observation;
                        cumulative_reward += reward;
                        ++steps;
                        ++global_step;

                        if(replay_buffer->ready_for_training(config.minimum_buffer_size)
                           && global_step % config.training_frequency == 0) {
                                if(auto loss = optimise_model())
                                        episode_losses.push_back(*loss);
                        }

                        if(global_step % config.target_update_frequency == 0)
                                update_target_network();

                        if(episode_done || truncated)
                                terminated = true;
                }

                metrics.episode_rewards.push_back(cumulative_reward);
                metrics.epsilon_values.push_back(epsilon);
                metrics.losses.insert(metrics.losses.end(), episode_losses.begin(), episode_losses.end());
                metrics.total_steps = global_step;
        }

        return metrics;
}

double dqn_trainer_t::sample_epsilon() const {
        return epsilon_schedule.value(global_step);
}

std::optional<action_mask_t> dqn_trainer_t::compute_legal_mask(const combat_observation_t& observation) const {
        if(!legal_action_fn)
                return std::nullopt;

        auto mask = legal_action_fn(observation);
        if(mask && mask->size() != action_space.size())
                mask->resize(action_space.size(), 0);
        return mask;
}

int64_t dqn_trainer_t::select_action(const torch::Tensor& state,
                                      double epsilon,
                                      const std::optional<action_mask_t>& legal_mask) {
        std::uniform_real_distribution<double> distribution(0.0, 1.0);
        if(distribution(rng) < epsilon)
                return sample_random_action(legal_mask);

        torch::NoGradGuard guard;
        policy_agent->model()->eval();
        auto q_values = policy_agent->model()->forward(state.unsqueeze(0)).squeeze(0);
        q_values = apply_legal_mask(q_values, legal_mask);
        const int64_t action_index = q_values.argmax().item<int64_t>();
        policy_agent->model()->train();
        return action_index;
}

int64_t dqn_trainer_t::sample_random_action(const std::optional<action_mask_t>& legal_mask) {
        auto indices = resolve_legal_indices(legal_mask);
        if(indices.empty()) {
                indices.resize(action_space.size());
                std::iota(indices.begin(), indices.end(), 0);
        }

        std::uniform_int_distribution<std::size_t> distribution(0, indices.size() - 1);
        return static_cast<int64_t>(indices[distribution(rng)]);
}

std::vector<std::size_t> dqn_trainer_t::resolve_legal_indices(const std::optional<action_mask_t>& legal_mask) const {
        if(!legal_mask || legal_mask->empty())
                return {};

        std::vector<std::size_t> indices;
        indices.reserve(action_space.size());
        for(std::size_t index = 0; index < action_space.size() && index < legal_mask->size(); ++index) {
                if((*legal_mask)[index])
                        indices.push_back(index);
        }
        return indices;
}

torch::Tensor dqn_trainer_t::apply_legal_mask(const torch::Tensor& q_values,
                                               const std::optional<action_mask_t>& legal_mask) const {
        if(!legal_mask || legal_mask->empty())
                return q_values;

        auto mask = mask_to_tensor(*legal_mask, q_values.device());
        return q_values.masked_fill(~mask, -std::numeric_limits<float>::infinity());
}

torch::Tensor dqn_trainer_t::apply_legal_mask_batch(const torch::Tensor& q_values,
                                                     const std::vector<action_mask_t>& masks) const {
        if(std::all_of(masks.begin(), masks.end(), [](const action_mask_t& mask) { return mask.empty(); }))
                return q_values;

        std::vector<torch::Tensor> tensors;
        tensors.reserve(masks.size());
        for(const auto& mask : masks)
                tensors.push_back(mask_to_tensor(mask, q_values.device()));

        auto stacked = torch::stack(tensors);
        return q_values.masked_fill(~stacked, -std::numeric_limits<float>::infinity());
}

torch::Tensor dqn_trainer_t::mask_to_tensor(const action_mask_t& mask, torch::Device target_device) const {
        std::vector<int64_t> values(action_space.size(), 0);
        for(std::size_t index = 0; index < values.size() && index < mask.size(); ++index)
                values[index] = mask[index] ? 1 : 0;

        bool any_allowed = std::any_of(values.begin(), values.end(), [](int64_t value) { return value != 0; });
        if(!any_allowed)
                std::fill(values.begin(), values.end(), 1);

        auto tensor = torch::tensor(values, torch::TensorOptions().dtype(torch::kInt64).device(target_device));
        return tensor.to(torch::kBool);
}

std::optional<float> dqn_trainer_t::optimise_model() {
        if(replay_buffer->size() < config.batch_size)
                return std::nullopt;

        auto transitions = replay_buffer->sample(config.batch_size);

        std::vector<torch::Tensor> state_batch;
        std::vector<int64_t> action_batch;
        std::vector<float> reward_batch;
        std::vector<torch::Tensor> next_state_batch;
        std::vector<float> done_batch;
        std::vector<action_mask_t> next_masks;
        state_batch.reserve(transitions.size());
        action_batch.reserve(transitions.size());
        reward_batch.reserve(transitions.size());
        next_state_batch.reserve(transitions.size());
        done_batch.reserve(transitions.size());
        next_masks.reserve(transitions.size());

        for(const auto& transition : transitions) {
                state_batch.push_back(transition.state.to(device));
                action_batch.push_back(transition.action);
                reward_batch.push_back(transition.reward);
                next_state_batch.push_back(transition.next_state.to(device));
                done_batch.push_back(transition.done ? 1.0F : 0.0F);
                next_masks.push_back(transition.next_legal_actions_mask);
        }

        auto states = torch::stack(state_batch);
        auto actions = torch::tensor(action_batch, torch::TensorOptions().dtype(torch::kLong).device(device));
        auto rewards = torch::tensor(reward_batch, torch::TensorOptions().dtype(torch::kFloat32).device(device));
        auto next_states = torch::stack(next_state_batch);
        auto dones = torch::tensor(done_batch, torch::TensorOptions().dtype(torch::kFloat32).device(device));

        auto q_values = policy_agent->model()->forward(states);
        auto action_q = q_values.gather(1, actions.unsqueeze(1)).squeeze(1);

        torch::Tensor targets;
        {
                torch::NoGradGuard guard;
                auto next_q_values = target_agent->model()->forward(next_states);
                next_q_values = apply_legal_mask_batch(next_q_values, next_masks);
                auto max_next_q = std::get<0>(next_q_values.max(1));
                targets = rewards + config.discount * (1.0F - dones) * max_next_q;
        }

        auto loss = torch::nn::MSELoss()(action_q, targets);

        optimizer->zero_grad();
        loss.backward();

        if(config.gradient_clip_norm)
                torch::nn::utils::clip_grad_norm_(policy_agent->model()->parameters(), *config.gradient_clip_norm);

        optimizer->step();

        return loss.item<float>();
}

void dqn_trainer_t::update_target_network() {
        torch::NoGradGuard guard;
        torch::serialize::OutputArchive archive;
        policy_agent->model()->save(archive);
        std::stringstream stream;
        archive.save_to(stream);

        torch::serialize::InputArchive input;
        input.load_from(stream);
        target_agent->model()->load(input);
        target_agent->model()->eval();
}

} // namespace combat
} // namespace rl

