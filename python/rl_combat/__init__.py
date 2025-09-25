"""Python bindings for the Clash of Fantasy combat reinforcement-learning scaffolding."""

from . import training
from .environment import CombatEnvironment, ControlledSide
from .observation import (EncodedCombatObservation, encode_combat_observation,
                          flatten_observation)
from .scenario import CombatScenario, HeroLoadout, TroopStack
from .session import (BattleResult, CombatAction, CombatActionType,
                      CombatObservation, CombatSession, StackObservation)

__all__ = [
        "CombatEnvironment",
        "BattleResult",
        "CombatAction",
        "CombatActionType",
        "CombatObservation",
        "CombatScenario",
        "CombatSession",
        "ControlledSide",
        "EncodedCombatObservation",
        "HeroLoadout",
        "StackObservation",
        "TroopStack",
        "encode_combat_observation",
        "flatten_observation",
        "training",
]
