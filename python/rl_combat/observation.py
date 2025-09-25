"""Observation encoding helpers for the combat reinforcement-learning stack."""

from __future__ import annotations

from dataclasses import dataclass
from itertools import chain
from typing import Iterable, Sequence, Tuple

from . import bindings
from .session import CombatObservation, StackObservation

__all__ = [
    "EncodedCombatObservation",
    "encode_combat_observation",
    "flatten_observation",
    "STACK_FEATURE_COUNT",
    "GLOBAL_FEATURE_COUNT",
]

STACK_FEATURE_COUNT = 16
GLOBAL_FEATURE_COUNT = 16


def _bool(value: bool) -> float:
    return 1.0 if bool(value) else 0.0


@dataclass(frozen=True)
class EncodedCombatObservation:
    """Structured representation of encoded combat features."""

    raw: CombatObservation
    steps: int
    global_features: Tuple[float, ...]
    attacker_stack_features: Tuple[Tuple[float, ...], ...]
    defender_stack_features: Tuple[Tuple[float, ...], ...]

    @property
    def vector(self) -> Tuple[float, ...]:
        """Return the flattened observation vector."""

        return tuple(
            chain(
                self.global_features,
                chain.from_iterable(self.attacker_stack_features),
                chain.from_iterable(self.defender_stack_features),
            )
        )


def flatten_observation(observation: EncodedCombatObservation) -> Tuple[float, ...]:
    """Convenience wrapper returning the flattened observation vector."""

    return observation.vector


def encode_combat_observation(
    observation: CombatObservation,
    *,
    steps: int = 0,
) -> EncodedCombatObservation:
    """Encode a raw combat observation into fixed-size feature tuples."""

    global_features = _encode_global_features(observation, steps)
    attacker_features = _encode_stack_block(observation.attacker_stacks)
    defender_features = _encode_stack_block(observation.defender_stacks)

    return EncodedCombatObservation(
        raw=observation,
        steps=steps,
        global_features=global_features,
        attacker_stack_features=attacker_features,
        defender_stack_features=defender_features,
    )


def _encode_global_features(
    observation: CombatObservation, steps: int
) -> Tuple[float, ...]:
    active_exists = observation.active_unit_id >= 0
    active_x, active_y = observation.active_unit_position
    if not active_exists:
        active_x = -1
        active_y = -1
    active_is_attacker = (
        _bool(observation.active_unit_is_attacker)
        if observation.active_unit_is_attacker is not None
        else 0.0
    )

    features: Sequence[float] = (
        float(observation.round),
        float(steps),
        _bool(observation.attacker_moved_last),
        _bool(observation.is_siege),
        _bool(observation.is_quick_combat),
        float(observation.active_unit_id),
        _bool(active_exists),
        active_is_attacker,
        float(active_x),
        float(active_y),
        float(observation.attacker_mana),
        float(observation.defender_mana),
        float(observation.attacker_morale),
        float(observation.defender_morale),
        float(observation.attacker_luck),
        float(observation.defender_luck),
    )
    return tuple(features)


def _encode_stack_block(
    stacks: Iterable[StackObservation],
) -> Tuple[Tuple[float, ...], ...]:
    rows = [[0.0] * STACK_FEATURE_COUNT for _ in range(bindings.RL_COMBAT_MAX_ARMY_TROOPS)]

    for index, stack in enumerate(stacks):
        if index >= bindings.RL_COMBAT_MAX_ARMY_TROOPS:
            break
        rows[index] = list(_encode_stack(stack))

    return tuple(tuple(row) for row in rows)


def _encode_stack(stack: StackObservation) -> Tuple[float, ...]:
    return (
        1.0,
        float(stack.unit_type),
        float(stack.stack_size),
        float(stack.unit_health),
        float(stack.original_stack_size),
        float(stack.retaliations_remaining),
        float(stack.position[0]),
        float(stack.position[1]),
        _bool(stack.is_attacker),
        _bool(stack.is_alive),
        _bool(stack.has_waited),
        _bool(stack.has_moved),
        _bool(stack.has_defended),
        _bool(stack.has_cast_spell),
        _bool(stack.has_moraled),
        _bool(stack.is_disabled),
    )
