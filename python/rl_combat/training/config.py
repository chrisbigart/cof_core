"""Configuration dataclasses for reinforcement-learning training loops."""

from __future__ import annotations

from dataclasses import dataclass, field
from typing import Any, Dict, Optional


@dataclass
class DQNConfig:
    """Hyperparameters controlling Deep Q-Network optimisation."""

    discount: float = 0.99
    batch_size: int = 64
    minimum_buffer_size: int = 1_000
    training_frequency: int = 1
    target_update_frequency: int = 1_000
    evaluation_interval: Optional[int] = 10_000
    gradient_clip_norm: Optional[float] = 5.0
    max_steps_per_episode: Optional[int] = None
    replay_buffer_capacity: int = 200_000
    logging_interval: int = 1_000


@dataclass
class SelfPlayConfig:
    """Parameters governing how self-play episodes are generated."""

    episodes_per_cycle: int = 1
    attacker_advantage: float = 0.0
    defender_advantage: float = 0.0
    scenario_options: Dict[str, Any] = field(default_factory=dict)
    alternate_controlled_side: bool = True
    random_seed: Optional[int] = None
    max_steps_per_episode: Optional[int] = None
