"""Experience replay utilities used by the training loops."""

from __future__ import annotations

from collections import deque
from dataclasses import dataclass
from typing import Any, Deque, Iterable, List, Optional

import random


@dataclass
class Transition:
    """Container describing a single environment transition."""

    state: Any
    action: int
    reward: float
    next_state: Any
    done: bool
    legal_actions_mask: Optional[Any] = None
    next_legal_actions_mask: Optional[Any] = None


class ReplayBuffer:
    """Fixed-capacity buffer for sampling i.i.d. batches of transitions."""

    def __init__(
        self,
        capacity: int,
        *,
        rng: Optional[random.Random] = None,
    ) -> None:
        if capacity <= 0:
            raise ValueError("capacity must be positive")
        self._capacity = capacity
        self._data: Deque[Transition] = deque(maxlen=capacity)
        self._rng = rng or random.Random()

    def __len__(self) -> int:
        return len(self._data)

    @property
    def capacity(self) -> int:
        return self._capacity

    def clear(self) -> None:
        self._data.clear()

    def extend(self, transitions: Iterable[Transition]) -> None:
        for transition in transitions:
            self.append(transition)

    def append(self, transition: Transition) -> None:
        self._data.append(transition)

    def sample(self, batch_size: int) -> List[Transition]:
        if batch_size <= 0:
            raise ValueError("batch_size must be positive")
        if batch_size > len(self._data):
            raise ValueError("Cannot sample more elements than present in buffer")
        return self._rng.sample(list(self._data), batch_size)

    def is_full(self) -> bool:
        return len(self._data) >= self._capacity

    def ready_for_training(self, minimum_size: int) -> bool:
        return len(self._data) >= minimum_size
