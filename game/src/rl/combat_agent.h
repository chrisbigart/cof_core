#pragma once

#include "rl/combat_environment.h"

#include <torch/torch.h>

#include <vector>

namespace rl {
namespace combat {

struct CombatPolicyOptions {
        std::vector<std::size_t> hidden_layers{256, 128};
        bool use_layer_norm = false;
};

struct CombatPolicyImpl : torch::nn::Module {
        CombatPolicyImpl(std::size_t input_dim, std::size_t action_dim, CombatPolicyOptions options = {});
        torch::Tensor forward(torch::Tensor input);

        CombatPolicyOptions options;
        std::vector<torch::nn::Linear> hidden_layers;
        std::vector<torch::nn::LayerNorm> layer_norms;
        torch::nn::Linear output_layer{nullptr};
};
TORCH_MODULE(CombatPolicy);

class combat_agent_t {
public:
        combat_agent_t(std::size_t observation_dim,
                       std::size_t action_dim,
                       const CombatPolicyOptions& options = CombatPolicyOptions(),
                       torch::Device device = torch::kCPU);

        combat_action_type_t select_action(const combat_observation_t& observation) const;
        torch::Tensor evaluate(const combat_observation_t& observation) const;

        CombatPolicy& policy() { return policy_network; }
        const CombatPolicy& policy() const { return policy_network; }

private:
        mutable CombatPolicy policy_network;
        torch::Device device;
};

} // namespace combat
} // namespace rl

