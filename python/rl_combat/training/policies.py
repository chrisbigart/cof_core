"""Policy abstractions for driving self-play simulations."""

from __future__ import annotations

import random
from abc import ABC, abstractmethod
from typing import Any, Iterable, Optional, Sequence

from ..session import CombatAction, CombatActionType


class BasePolicy(ABC):
    """Interface for policies that emit environment-compatible actions."""

    @abstractmethod
    def select_action(
        self,
        observation: Any,
        *,
        legal_actions: Optional[Iterable[CombatActionType]] = None,
    ) -> CombatAction:
        """Return a `CombatAction` in response to an observation."""


class RandomPolicy(BasePolicy):
    """Uniformly sample from the provided legal action set."""

    def __init__(self, *, rng: Optional[random.Random] = None) -> None:
        self._rng = rng or random.Random()

    def select_action(
        self,
        observation: Any,
        *,
        legal_actions: Optional[Iterable[CombatActionType]] = None,
    ) -> CombatAction:
        del observation  # unused but part of the interface

        candidates: Sequence[CombatActionType]
        if legal_actions:
            candidates = tuple(legal_actions)
        else:
            candidates = (
                CombatActionType.WAIT,
                CombatActionType.DEFEND,
                CombatActionType.AUTO_RESOLVE,
            )

        if not candidates:
            raise ValueError("No legal actions provided to RandomPolicy")

        choice = self._rng.choice(candidates)
        return CombatAction(choice)
