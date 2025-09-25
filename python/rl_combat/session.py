"""High-level session wrapper for driving combat simulations from Python."""

from __future__ import annotations

import ctypes
from ctypes import c_int, c_uint8
from dataclasses import dataclass
from enum import IntEnum
from typing import Any, Dict, List, Optional, Tuple

from . import bindings
from .scenario import CombatScenario

__all__ = [
        "BattleResult",
        "CombatAction",
        "CombatActionType",
        "CombatObservation",
        "CombatSession",
        "StackObservation",
]


class BattleResult(IntEnum):
        """Mirror the `battle_result_e` enum from the engine."""

        IN_PROGRESS = 0
        ATTACKER_VICTORY = 1
        DEFENDER_VICTORY = 2
        ATTACKER_HAS_FLED = 3
        DEFENDER_HAS_FLED = 4
        BOTH_LOSE = 5


class CombatActionType(IntEnum):
        """Enumerate the high-level combat commands exposed through the C API."""

        WAIT = 0
        DEFEND = 1
        AUTO_RESOLVE = 2


@dataclass(frozen=True)
class CombatAction:
        """Wrap a combat command issued by the agent."""

        action_type: CombatActionType

        @classmethod
        def wait(cls) -> "CombatAction":
                return cls(CombatActionType.WAIT)

        @classmethod
        def defend(cls) -> "CombatAction":
                return cls(CombatActionType.DEFEND)

        @classmethod
        def auto_resolve(cls) -> "CombatAction":
                return cls(CombatActionType.AUTO_RESOLVE)


@dataclass
class StackObservation:
        """Lightweight representation of a battlefield unit stack."""

        unit_type: int
        stack_size: int
        unit_health: int
        original_stack_size: int
        retaliations_remaining: int
        position: Tuple[int, int]
        is_attacker: bool
        is_alive: bool
        has_waited: bool
        has_moved: bool
        has_defended: bool
        has_cast_spell: bool
        has_moraled: bool
        is_disabled: bool

        @staticmethod
        def from_c_struct(data: bindings.StackObservationC) -> "StackObservation":
                return StackObservation(
                        unit_type=int(data.unit_type),
                        stack_size=int(data.stack_size),
                        unit_health=int(data.unit_health),
                        original_stack_size=int(data.original_stack_size),
                        retaliations_remaining=int(data.retaliations_remaining),
                        position=(int(data.x), int(data.y)),
                        is_attacker=bool(data.is_attacker),
                        is_alive=bool(data.is_alive),
                        has_waited=bool(data.has_waited),
                        has_moved=bool(data.has_moved),
                        has_defended=bool(data.has_defended),
                        has_cast_spell=bool(data.has_cast_spell),
                        has_moraled=bool(data.has_moraled),
                        is_disabled=bool(data.is_disabled),
                )

        def to_dict(self) -> Dict[str, Any]:
                return {
                        "unit_type": self.unit_type,
                        "stack_size": self.stack_size,
                        "unit_health": self.unit_health,
                        "original_stack_size": self.original_stack_size,
                        "retaliations_remaining": self.retaliations_remaining,
                        "position": self.position,
                        "is_attacker": self.is_attacker,
                        "is_alive": self.is_alive,
                        "has_waited": self.has_waited,
                        "has_moved": self.has_moved,
                        "has_defended": self.has_defended,
                        "has_cast_spell": self.has_cast_spell,
                        "has_moraled": self.has_moraled,
                        "is_disabled": self.is_disabled,
                }


@dataclass
class CombatObservation:
        """Snapshot of the current battle state suitable for policy consumption."""

        round: int
        attacker_moved_last: bool
        is_siege: bool
        is_quick_combat: bool
        active_unit_id: int
        active_unit_is_attacker: Optional[bool]
        active_unit_position: Tuple[int, int]
        attacker_mana: int
        defender_mana: int
        attacker_morale: int
        defender_morale: int
        attacker_luck: int
        defender_luck: int
        attacker_stacks: List[StackObservation]
        defender_stacks: List[StackObservation]

        @staticmethod
        def from_c_struct(data: bindings.CombatObservationC) -> "CombatObservation":
                attacker_stacks = [
                        StackObservation.from_c_struct(data.attacker_stacks[i])
                        for i in range(int(data.attacker_stack_count))
                ]
                defender_stacks = [
                        StackObservation.from_c_struct(data.defender_stacks[i])
                        for i in range(int(data.defender_stack_count))
                ]
                active_id = int(data.active_unit_id)
                active_is_attacker: Optional[bool]
                if active_id < 0:
                        active_is_attacker = None
                else:
                        active_is_attacker = bool(data.active_unit_is_attacker)

                return CombatObservation(
                        round=int(data.round),
                        attacker_moved_last=bool(data.attacker_moved_last),
                        is_siege=bool(data.is_siege),
                        is_quick_combat=bool(data.is_quick_combat),
                        active_unit_id=active_id,
                        active_unit_is_attacker=active_is_attacker,
                        active_unit_position=(int(data.active_unit_x), int(data.active_unit_y)),
                        attacker_mana=int(data.attacker_mana),
                        defender_mana=int(data.defender_mana),
                        attacker_morale=int(data.attacker_morale),
                        defender_morale=int(data.defender_morale),
                        attacker_luck=int(data.attacker_luck),
                        defender_luck=int(data.defender_luck),
                        attacker_stacks=attacker_stacks,
                        defender_stacks=defender_stacks,
                )

        def to_dict(self) -> Dict[str, Any]:
                return {
                        "round": self.round,
                        "attacker_moved_last": self.attacker_moved_last,
                        "is_siege": self.is_siege,
                        "is_quick_combat": self.is_quick_combat,
                        "active_unit_id": self.active_unit_id,
                        "active_unit_is_attacker": self.active_unit_is_attacker,
                        "active_unit_position": self.active_unit_position,
                        "attacker_mana": self.attacker_mana,
                        "defender_mana": self.defender_mana,
                        "attacker_morale": self.attacker_morale,
                        "defender_morale": self.defender_morale,
                        "attacker_luck": self.attacker_luck,
                        "defender_luck": self.defender_luck,
                        "attacker_stacks": [stack.to_dict() for stack in self.attacker_stacks],
                        "defender_stacks": [stack.to_dict() for stack in self.defender_stacks],
                }


class CombatSession:
        """Manage the lifecycle of a combat session via the shared library."""

        def __init__(self, library_path: Optional[str] = None) -> None:
                self._lib = bindings.ensure_library(library_path)
                self._game = self._lib.rl_combat_create_game()
                if not self._game:
                        raise RuntimeError("Failed to allocate combat game handle")

                self._session = self._lib.rl_combat_create_session(self._game)
                if not self._session:
                        self._lib.rl_combat_destroy_game(self._game)
                        raise RuntimeError("Failed to allocate combat session handle")

                self._closed = False

        def __enter__(self) -> "CombatSession":
                return self

        def __exit__(self, exc_type, exc, tb) -> None:  # type: ignore[override]
                self.close()

        def __del__(self) -> None:
                try:
                        self.close()
                except Exception:
                        pass

        def close(self) -> None:
                if self._closed:
                        return

                if getattr(self, "_session", None):
                        self._lib.rl_combat_destroy_session(self._session)
                        self._session = None

                if getattr(self, "_game", None):
                        self._lib.rl_combat_destroy_game(self._game)
                        self._game = None

                self._closed = True

        @property
        def closed(self) -> bool:
                return self._closed

        def configure(self, scenario: CombatScenario) -> None:
                if self._closed:
                        raise RuntimeError("CombatSession has already been closed")

                scenario_c, buffers = scenario.to_c_struct()
                try:
                        result = self._lib.rl_combat_session_configure(self._session, ctypes.byref(scenario_c))
                finally:
                        buffers.clear()

                if result != 0:
                        raise RuntimeError(f"Failed to configure combat session (error {result})")

        def reset(self) -> BattleResult:
                if self._closed:
                        raise RuntimeError("CombatSession has already been closed")

                result_value = c_int(0)
                error = self._lib.rl_combat_session_reset(self._session, ctypes.byref(result_value))
                if error != 0:
                        raise RuntimeError(f"Combat reset failed (error {error})")

                return BattleResult(result_value.value)

        def step(self) -> BattleResult:
                if self._closed:
                        raise RuntimeError("CombatSession has already been closed")

                result_value = c_int(0)
                error = self._lib.rl_combat_session_step(self._session, ctypes.byref(result_value))
                if error != 0:
                        raise RuntimeError(f"Combat step failed (error {error})")

                return BattleResult(result_value.value)

        def observation(self) -> CombatObservation:
                if self._closed:
                        raise RuntimeError("CombatSession has already been closed")

                observation_c = bindings.CombatObservationC()
                error = self._lib.rl_combat_session_observation(self._session, ctypes.byref(observation_c))
                if error != 0:
                        raise RuntimeError(f"Failed to extract combat observation (error {error})")

                return CombatObservation.from_c_struct(observation_c)

        def apply_action(self, action: "CombatAction") -> bool:
                if self._closed:
                        raise RuntimeError("CombatSession has already been closed")

                if not isinstance(action, CombatAction):
                        raise TypeError("action must be a CombatAction instance")

                action_c = bindings.CombatActionC()
                action_c.type = c_uint8(action.action_type.value)
                error = self._lib.rl_combat_session_apply_action(self._session, ctypes.byref(action_c))
                if error == 0:
                        return True
                if error == -3:
                        return False
                raise RuntimeError(f"Combat action failed (error {error})")

        def battlefield_pointer(self) -> int:
                if self._closed:
                        raise RuntimeError("CombatSession has already been closed")
                return int(self._lib.rl_combat_session_battlefield(self._session))

        def attacker_pointer(self) -> int:
                if self._closed:
                        raise RuntimeError("CombatSession has already been closed")
                return int(self._lib.rl_combat_session_attacker(self._session))

        def defender_pointer(self) -> int:
                if self._closed:
                        raise RuntimeError("CombatSession has already been closed")
                return int(self._lib.rl_combat_session_defender(self._session))

        def game_pointer(self) -> int:
                if self._closed:
                        raise RuntimeError("CombatSession has already been closed")
                return int(self._lib.rl_combat_session_game(self._session))
