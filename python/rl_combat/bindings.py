"""Low-level ctypes bindings to the combat session C API."""

from __future__ import annotations

import ctypes
import os
from ctypes import (c_char_p, c_int, c_int16, c_int8, c_size_t, c_uint8,
                     c_uint16, c_uint64, c_void_p, POINTER)
from typing import Iterable, Optional

__all__ = [
        "BattlefieldPointer",
        "CombatObservationC",
        "CombatActionC",
        "CombatScenarioC",
        "GameHandle",
        "HeroLoadoutC",
        "Library",  # alias for the loaded CDLL
        "RL_COMBAT_MAX_ARMY_TROOPS",
        "SessionHandle",
        "StackObservationC",
        "TroopStackC",
        "ensure_library",
]

Library = ctypes.CDLL
BattlefieldPointer = c_void_p
GameHandle = c_void_p
SessionHandle = c_void_p

RL_COMBAT_MAX_ARMY_TROOPS = 16


class TroopStackC(ctypes.Structure):
        _fields_ = [
                ("unit_type", c_uint16),
                ("stack_size", c_uint16),
        ]


class HeroLoadoutC(ctypes.Structure):
        _fields_ = [
                ("player", c_uint8),
                ("hero_class", c_uint8),
                ("hero_id", c_uint16),
                ("name", c_char_p),
                ("level", c_uint8),
                ("experience", c_uint64),
                ("attack", c_uint8),
                ("defense", c_uint8),
                ("power", c_uint8),
                ("knowledge", c_uint8),
                ("mana", c_uint16),
                ("dark_energy", c_uint16),
                ("has_ballista", c_uint8),
                ("human_controlled", c_uint8),
                ("troops", POINTER(TroopStackC)),
                ("troop_count", c_size_t),
                ("talents", POINTER(c_uint8)),
                ("talent_count", c_size_t),
                ("spells", POINTER(c_uint16)),
                ("spell_count", c_size_t),
                ("skills", POINTER(c_uint8)),
                ("skill_count", c_size_t),
        ]


class CombatScenarioC(ctypes.Structure):
        _fields_ = [
                ("attacker", HeroLoadoutC),
                ("defender", HeroLoadoutC),
                ("is_deathmatch", c_uint8),
                ("environment", c_uint8),
                ("quick_combat", c_uint8),
        ]


class StackObservationC(ctypes.Structure):
        _fields_ = [
                ("unit_type", c_uint16),
                ("stack_size", c_uint16),
                ("unit_health", c_uint16),
                ("original_stack_size", c_uint16),
                ("retaliations_remaining", c_int8),
                ("x", c_int8),
                ("y", c_int8),
                ("is_attacker", c_uint8),
                ("is_alive", c_uint8),
                ("has_waited", c_uint8),
                ("has_moved", c_uint8),
                ("has_defended", c_uint8),
                ("has_cast_spell", c_uint8),
                ("has_moraled", c_uint8),
                ("is_disabled", c_uint8),
        ]


class CombatObservationC(ctypes.Structure):
        _fields_ = [
                ("round", c_uint16),
                ("attacker_moved_last", c_uint8),
                ("is_siege", c_uint8),
                ("is_quick_combat", c_uint8),
                ("active_unit_id", c_int8),
                ("active_unit_is_attacker", c_uint8),
                ("active_unit_x", c_int8),
                ("active_unit_y", c_int8),
                ("attacker_mana", c_uint16),
                ("defender_mana", c_uint16),
                ("attacker_morale", c_int16),
                ("defender_morale", c_int16),
                ("attacker_luck", c_int16),
                ("defender_luck", c_int16),
                ("attacker_stack_count", c_uint8),
                ("defender_stack_count", c_uint8),
                ("attacker_stacks", StackObservationC * RL_COMBAT_MAX_ARMY_TROOPS),
                ("defender_stacks", StackObservationC * RL_COMBAT_MAX_ARMY_TROOPS),
        ]


class CombatActionC(ctypes.Structure):
        _fields_ = [
                ("type", c_uint8),
        ]


def _default_library_candidates() -> Iterable[str]:
        yield from (
                os.environ.get("COF_CORE_LIB"),
                "libcof_core.so",
                "libcof_core.dylib",
                "cof_core.dll",
        )


def ensure_library(path: Optional[str] = None) -> Library:
        """Load the combat shared library, returning the cached handle."""

        if not hasattr(ensure_library, "_cached"):
                ensure_library._cached = None  # type: ignore[attr-defined]

        lib = ensure_library._cached  # type: ignore[attr-defined]
        if lib is not None:
                return lib

        candidates = []
        if path:
                candidates.append(path)
        candidates.extend(filter(None, _default_library_candidates()))

        last_error: Optional[Exception] = None
        for candidate in candidates:
                try:
                        lib = ctypes.CDLL(candidate)
                        ensure_library._cached = lib  # type: ignore[attr-defined]
                        _configure_prototypes(lib)
                        return lib
                except OSError as exc:
                        last_error = exc
                        continue

        raise RuntimeError(
                "Unable to locate the Clash of Fantasy shared library; set COF_CORE_LIB"
                " or provide an explicit path"
        ) from last_error


def _configure_prototypes(lib: Library) -> None:
        lib.rl_combat_create_game.restype = GameHandle
        lib.rl_combat_create_game.argtypes = []

        lib.rl_combat_destroy_game.restype = None
        lib.rl_combat_destroy_game.argtypes = [GameHandle]

        lib.rl_combat_create_session.restype = SessionHandle
        lib.rl_combat_create_session.argtypes = [GameHandle]

        lib.rl_combat_destroy_session.restype = None
        lib.rl_combat_destroy_session.argtypes = [SessionHandle]

        lib.rl_combat_session_configure.restype = c_int
        lib.rl_combat_session_configure.argtypes = [SessionHandle, POINTER(CombatScenarioC)]

        lib.rl_combat_session_reset.restype = c_int
        lib.rl_combat_session_reset.argtypes = [SessionHandle, POINTER(c_int)]

        lib.rl_combat_session_step.restype = c_int
        lib.rl_combat_session_step.argtypes = [SessionHandle, POINTER(c_int)]

        lib.rl_combat_session_observation.restype = c_int
        lib.rl_combat_session_observation.argtypes = [SessionHandle, POINTER(CombatObservationC)]

        lib.rl_combat_session_apply_action.restype = c_int
        lib.rl_combat_session_apply_action.argtypes = [SessionHandle, POINTER(CombatActionC)]

        lib.rl_combat_session_battlefield.restype = BattlefieldPointer
        lib.rl_combat_session_battlefield.argtypes = [SessionHandle]

        lib.rl_combat_session_attacker.restype = c_void_p
        lib.rl_combat_session_attacker.argtypes = [SessionHandle]

        lib.rl_combat_session_defender.restype = c_void_p
        lib.rl_combat_session_defender.argtypes = [SessionHandle]

        lib.rl_combat_session_game.restype = c_void_p
        lib.rl_combat_session_game.argtypes = [SessionHandle]
