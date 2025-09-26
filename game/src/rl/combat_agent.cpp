#include "rl/combat_agent.h"

#include "rl/combat_observation.h"

#include <algorithm>
#include <memory>
#include <utility>

namespace rl {
namespace combat {

CombatPolicyImpl::CombatPolicyImpl(std::size_t input_dim, std::size_t action_dim, CombatPolicyOptions options)
        : options(std::move(options)) {
        if(this->options.hidden_layers.empty())
                this->options.hidden_layers = {256, 128};

        hidden_layers.reserve(this->options.hidden_layers.size());
        if(this->options.use_layer_norm)
                layer_norms.reserve(this->options.hidden_layers.size());

        std::size_t previous_dim = input_dim;
        for(std::size_t index = 0; index < this->options.hidden_layers.size(); ++index) {
                const std::size_t hidden_dim = this->options.hidden_layers[index];
                auto layer = register_module(
                        "fc" + std::to_string(index + 1),
                        torch::nn::Linear(static_cast<long>(previous_dim), static_cast<long>(hidden_dim)));
                hidden_layers.push_back(std::move(layer));

                if(this->options.use_layer_norm) {
                        auto norm = register_module(
                                "ln" + std::to_string(index + 1),
                                torch::nn::LayerNorm(torch::nn::LayerNormOptions({static_cast<long>(hidden_dim)})));
                        layer_norms.push_back(std::move(norm));
                }

                previous_dim = hidden_dim;
        }

        output_layer = register_module(
                "output",
                torch::nn::Linear(static_cast<long>(previous_dim), static_cast<long>(action_dim)));
}

torch::Tensor CombatPolicyImpl::forward(torch::Tensor input) {
        auto activation = input;
        for(std::size_t index = 0; index < hidden_layers.size(); ++index) {
                activation = hidden_layers[index]->forward(activation);
                if(options.use_layer_norm && index < layer_norms.size())
                        activation = layer_norms[index]->forward(activation);
                activation = torch::relu(activation);
        }

        activation = output_layer->forward(activation);
        return activation;
}

combat_agent_t::combat_agent_t(std::size_t observation_dim,
                               std::size_t action_dim,
                               const CombatPolicyOptions& options,
                               torch::Device device)
        : policy_network(std::make_shared<CombatPolicyImpl>(observation_dim, action_dim, options))
        , device(device) {
        policy_network->to(device);
}

torch::Tensor combat_agent_t::evaluate(const combat_observation_t& observation) const {
        torch::NoGradGuard guard;
        auto input = observation_to_tensor(observation, device);
        input = input.unsqueeze(0);
        auto output = policy_network->forward(input);
        return output.squeeze(0).to(torch::kCPU);
}

combat_action_type_t combat_agent_t::select_action(const combat_observation_t& observation) const {
        auto scores = evaluate(observation);
        auto action_index = scores.argmax().item<int64_t>();
        action_index = std::clamp<int64_t>(action_index, 0, static_cast<int64_t>(ACTION_COUNT) - 1);
        return static_cast<combat_action_type_t>(action_index);
}

} // namespace combat
} // namespace rl

