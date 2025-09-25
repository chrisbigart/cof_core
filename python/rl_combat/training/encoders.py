"""Utilities for converting encoded combat observations into tensors."""

from __future__ import annotations

from typing import Any, Optional, Sequence

from .. import bindings
from ..observation import (
    EncodedCombatObservation,
    GLOBAL_FEATURE_COUNT,
    STACK_FEATURE_COUNT,
    flatten_observation,
)

try:  # Optional dependency; resolved lazily when used.
    import torch
    from torch import Tensor
except ImportError:  # pragma: no cover - torch is optional for tooling installs
    torch = None  # type: ignore
    Tensor = Any  # type: ignore

__all__ = [
    "encoded_observation_size",
    "encoded_observation_to_tensor",
    "batch_observations_to_tensor",
]


def _ensure_torch() -> None:
    if torch is None:
        raise RuntimeError(
            "torch is required to convert observations into tensors; "
            "install torch or provide a custom encoder"
        )


def encoded_observation_size(
    max_army_troops: Optional[int] = None,
) -> int:
    """Return the flattened observation dimension for a single snapshot."""

    troops = max_army_troops or bindings.RL_COMBAT_MAX_ARMY_TROOPS
    return GLOBAL_FEATURE_COUNT + 2 * troops * STACK_FEATURE_COUNT


def encoded_observation_to_tensor(
    observation: EncodedCombatObservation,
    *,
    device: Optional[str] = None,
    dtype: Optional["torch.dtype"] = None,
) -> "Tensor":
    """Convert a single encoded observation into a 1D tensor."""

    _ensure_torch()
    vector = flatten_observation(observation)
    tensor = torch.tensor(vector, device=device, dtype=dtype or torch.float32)  # type: ignore[attr-defined]
    return tensor


def batch_observations_to_tensor(
    observations: Sequence[EncodedCombatObservation],
    *,
    device: Optional[str] = None,
    dtype: Optional["torch.dtype"] = None,
) -> "Tensor":
    """Stack multiple encoded observations into a 2D tensor."""

    if not observations:
        raise ValueError("observations must contain at least one element")

    _ensure_torch()
    matrix = [flatten_observation(obs) for obs in observations]
    tensor = torch.tensor(matrix, device=device, dtype=dtype or torch.float32)  # type: ignore[attr-defined]
    return tensor
