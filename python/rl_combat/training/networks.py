"""Neural network helpers for Clash of Fantasy combat training."""

from __future__ import annotations

from typing import Any, Callable, Optional, Sequence

from .. import bindings
from ..observation import GLOBAL_FEATURE_COUNT, STACK_FEATURE_COUNT
from .dqn import DiscreteActionSpace

try:  # Optional dependency; resolved at runtime for tooling without torch.
    import torch
    from torch import Tensor
    import torch.nn as nn
except ImportError:  # pragma: no cover - torch is optional for tooling installs
    torch = None  # type: ignore
    nn = None  # type: ignore
    Tensor = Any  # type: ignore

__all__ = [
    "combat_observation_dim",
    "CombatMLP",
    "build_mlp_q_network",
]


def _ensure_torch_nn() -> None:
    if nn is None:
        raise RuntimeError(
            "torch is required to construct neural networks; "
            "install torch or provide custom network implementations"
        )


def combat_observation_dim(max_army_troops: Optional[int] = None) -> int:
    """Return the flattened observation dimension consumed by MLP models."""

    troops = max_army_troops or bindings.RL_COMBAT_MAX_ARMY_TROOPS
    return GLOBAL_FEATURE_COUNT + 2 * troops * STACK_FEATURE_COUNT


if nn is not None:

    class CombatMLP(nn.Module):
        """Simple fully-connected network for approximating combat Q-values."""

        def __init__(
            self,
            input_dim: int,
            output_dim: int,
            *,
            hidden_layers: Sequence[int] = (256, 128),
            activation: Optional[Callable[[], "nn.Module"]] = None,
            layer_norm: bool = False,
        ) -> None:
            _ensure_torch_nn()
            super().__init__()

            act_factory: Callable[[], "nn.Module"]
            if activation is None:
                act_factory = nn.ReLU  # type: ignore[assignment]
            else:
                act_factory = activation

            layers: list["nn.Module"] = []
            last_dim = input_dim
            for hidden in hidden_layers:
                if hidden <= 0:
                    continue
                layers.append(nn.Linear(last_dim, hidden))
                if layer_norm:
                    layers.append(nn.LayerNorm(hidden))
                layers.append(act_factory())
                last_dim = hidden

            layers.append(nn.Linear(last_dim, output_dim))
            self.model = nn.Sequential(*layers)

        def forward(self, inputs: "Tensor") -> "Tensor":  # type: ignore[override]
            return self.model(inputs)

else:

    class CombatMLP:  # type: ignore[misc]
        """Placeholder that raises if torch is unavailable."""

        def __init__(self, *args: Any, **kwargs: Any) -> None:  # noqa: D401 - runtime guard
            _ensure_torch_nn()

        def forward(self, inputs: Any) -> Any:
            _ensure_torch_nn()


def build_mlp_q_network(
    action_space: DiscreteActionSpace,
    *,
    input_dim: Optional[int] = None,
    hidden_layers: Sequence[int] = (256, 128),
    activation: Optional[Callable[[], "nn.Module"]] = None,
    layer_norm: bool = False,
) -> "CombatMLP":
    """Construct a default CombatMLP sized to the action space."""

    _ensure_torch_nn()
    dim = input_dim or combat_observation_dim()
    return CombatMLP(
        dim,
        action_space.size,
        hidden_layers=hidden_layers,
        activation=activation,
        layer_norm=layer_norm,
    )
