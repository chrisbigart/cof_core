#include "rl/combat_agent.h"

#include "rl/combat_actions.h"
#include "rl/combat_observation.h"

#include <algorithm>
#include <memory>
#include <utility>

namespace rl {
namespace combat {

combat_agent_t::combat_agent_t(std::size_t observation_dim,
                               std::size_t action_dim,
                               const CombatNetworkOptions& options,
                               torch::Device device)
        : device(device) {
        auto network = torch::nn::Sequential();

        std::vector<std::size_t> hidden_layers = options.hidden_layers;
        if(hidden_layers.empty())
                hidden_layers = {512, 256, 128};

        long previous_dim = static_cast<long>(observation_dim);
        for(std::size_t index = 0; index < hidden_layers.size(); ++index) {
                const auto hidden_dim = static_cast<long>(hidden_layers[index]);
                network->push_back(torch::nn::Linear(previous_dim, hidden_dim));
                if(options.use_layer_norm)
                        network->push_back(torch::nn::LayerNorm(torch::nn::LayerNormOptions({hidden_dim})));
                network->push_back(torch::nn::Functional(torch::relu));
                previous_dim = hidden_dim;
        }

        network->push_back(torch::nn::Linear(previous_dim, static_cast<long>(action_dim)));

        policy_network = network;
        policy_network->to(device);
}

torch::Tensor combat_agent_t::evaluate(const combat_observation_t& observation) const {
        torch::NoGradGuard guard;
        auto input = observation_to_tensor(observation, device);
        input = input.unsqueeze(0);
        auto output = policy_network->forward(input);
        return output.squeeze(0).detach();
}

int64_t combat_agent_t::select_action(const combat_observation_t& observation) const {
        auto scores = evaluate(observation);
        auto action_index = scores.argmax().item<int64_t>();
        const auto max_index = static_cast<int64_t>(ACTION_COUNT) - 1;
        if(max_index >= 0)
                action_index = std::clamp<int64_t>(action_index, 0, max_index);
        return action_index;
}

} // namespace combat
} // namespace rl

