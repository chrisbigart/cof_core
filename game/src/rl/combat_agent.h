#pragma once

#include "rl/combat_environment.h"

#include <torch/torch.h>

#include <vector>

namespace rl {
namespace combat {

struct CombatNetworkOptions {
        std::vector<std::size_t> hidden_layers{256, 128};
        bool use_layer_norm = false;
};

class combat_agent_t {
public:
        combat_agent_t(std::size_t observation_dim,
                       std::size_t action_dim,
                       const CombatNetworkOptions& options = CombatNetworkOptions(),
                       torch::Device device = torch::kCPU);

        int64_t select_action(const combat_observation_t& observation) const;
        torch::Tensor evaluate(const combat_observation_t& observation) const;

        torch::nn::Sequential& model() { return policy_network; }
        const torch::nn::Sequential& model() const { return policy_network; }

private:
        mutable torch::nn::Sequential policy_network{nullptr};
        torch::Device device;
};

} // namespace combat
} // namespace rl

