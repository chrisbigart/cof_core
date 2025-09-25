"""Training scaffolding for reinforcement-learning agents."""

from .config import DQNConfig, SelfPlayConfig
from .dqn import DQNTrainer, DiscreteActionSpace, EpsilonSchedule, TrainingMetrics
from .encoders import (
    batch_observations_to_tensor,
    encoded_observation_size,
    encoded_observation_to_tensor,
)
from .networks import CombatMLP, build_mlp_q_network, combat_observation_dim
from .policies import BasePolicy, RandomPolicy
from .replay_buffer import ReplayBuffer, Transition
from .self_play import EpisodeRecord, SelfPlayRunner

__all__ = [
    "BasePolicy",
    "CombatMLP",
    "DQNConfig",
    "DQNTrainer",
    "DiscreteActionSpace",
    "EpsilonSchedule",
    "EpisodeRecord",
    "RandomPolicy",
    "ReplayBuffer",
    "SelfPlayConfig",
    "SelfPlayRunner",
    "TrainingMetrics",
    "Transition",
    "batch_observations_to_tensor",
    "build_mlp_q_network",
    "combat_observation_dim",
    "encoded_observation_size",
    "encoded_observation_to_tensor",
]
