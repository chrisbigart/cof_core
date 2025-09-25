"""Reinforcement-learning environment scaffolding for Clash of Fantasy combat."""

from __future__ import annotations

import copy
import random
from dataclasses import dataclass
from enum import Enum
from typing import Any, Callable, Dict, Mapping, Optional, Tuple, Union

from .observation import EncodedCombatObservation, encode_combat_observation
from .scenario import CombatScenario
from .session import (BattleResult, CombatAction, CombatActionType,
                      CombatSession)

__all__ = [
        "ControlledSide",
        "EpisodeState",
        "ScenarioGenerator",
        "CombatEnvironment",
]


class ControlledSide(str, Enum):
        """Indicate which combatant the agent controls."""

        ATTACKER = "attacker"
        DEFENDER = "defender"


ScenarioGenerator = Union[
        CombatScenario,
        Callable[[random.Random], CombatScenario],
        Callable[[random.Random, Optional[Mapping[str, Any]]], CombatScenario],
]


@dataclass
class EpisodeState:
        """Track metadata about the currently running episode."""

        scenario: CombatScenario
        seed: Optional[int]
        steps: int = 0
        result: BattleResult = BattleResult.IN_PROGRESS


class CombatEnvironment:
        """High-level environment wrapper that mirrors the Gymnasium API."""

        def __init__(
                self,
                scenario: ScenarioGenerator,
                *,
                controlled_side: ControlledSide = ControlledSide.ATTACKER,
                library_path: Optional[str] = None,
        ) -> None:
                self._scenario_factory = scenario
                self._controlled_side = ControlledSide(controlled_side)
                self._session = CombatSession(library_path)
                self._rng = random.Random()
                self._episode: Optional[EpisodeState] = None
                self._last_action_invalid: bool = False

        def close(self) -> None:
                """Release native resources associated with the combat session."""

                if self._session is not None:
                        self._session.close()

        def __enter__(self) -> "CombatEnvironment":
                return self

        def __exit__(self, exc_type, exc, tb) -> None:  # type: ignore[override]
                self.close()

        @property
        def session(self) -> CombatSession:
                """Expose the underlying session for advanced integrations."""

                return self._session

        @property
        def controlled_side(self) -> ControlledSide:
                return self._controlled_side

        def reset(
                self,
                *,
                seed: Optional[int] = None,
                options: Optional[Mapping[str, Any]] = None,
        ) -> Tuple[EncodedCombatObservation, Dict[str, Any]]:
                """Configure a new combat scenario and restart the battle."""

                if seed is not None:
                        self._rng.seed(seed)

                scenario = self._resolve_scenario(options)
                # Store a defensive copy to guard against mutation during training loops.
                scenario_copy = copy.deepcopy(scenario)
                self._apply_control_flags(scenario_copy)

                self._session.configure(scenario_copy)
                result = self._session.reset()

                self._episode = EpisodeState(
                        scenario=scenario_copy,
                        seed=seed,
                        steps=0,
                        result=result,
                )
                self._last_action_invalid = False

                observation = self._observe_controlled_state()
                info = self._build_info(terminated=self._episode.result != BattleResult.IN_PROGRESS)
                return observation, info

        def step(
                self,
                action: Any,
        ) -> Tuple[EncodedCombatObservation, float, bool, bool, Dict[str, Any]]:
                """Advance the combat by a single agent decision."""

                if self._episode is None:
                        raise RuntimeError("Environment has not been reset")

                if self._episode.result != BattleResult.IN_PROGRESS:
                        raise RuntimeError("Episode already terminated; call reset() before step().")

                self._apply_action(action)
                result = self._session.step()

                self._episode.steps += 1
                self._episode.result = result

                observation = self._observe_controlled_state()
                reward = self._compute_reward(self._episode.result)
                terminated = self._episode.result != BattleResult.IN_PROGRESS
                info = self._build_info(terminated=terminated)

                return observation, reward, terminated, False, info

        def battlefield_pointer(self) -> int:
                return self._session.battlefield_pointer()

        def attacker_pointer(self) -> int:
                return self._session.attacker_pointer()

        def defender_pointer(self) -> int:
                return self._session.defender_pointer()

        def _resolve_scenario(self, options: Optional[Mapping[str, Any]]) -> CombatScenario:
                if options and "scenario" in options:
                        scenario = options["scenario"]
                        if not isinstance(scenario, CombatScenario):
                                raise TypeError("reset(options['scenario']) must be a CombatScenario instance")
                        return scenario

                factory = self._scenario_factory
                if isinstance(factory, CombatScenario):
                        return factory

                if not callable(factory):
                        raise TypeError("scenario must be a CombatScenario or callable")

                try:
                        return factory(self._rng, options)
                except TypeError:
                        return factory(self._rng)

        def _encode_observation(self) -> EncodedCombatObservation:
                observation = self._session.observation()
                steps = self._episode.steps if self._episode is not None else 0
                return encode_combat_observation(observation, steps=steps)

        def _observe_controlled_state(self) -> EncodedCombatObservation:
                """Advance the simulation until the controlled side is active and encode it."""

                observation = self._session.observation()
                while (
                        self._episode is not None
                        and self._episode.result == BattleResult.IN_PROGRESS
                        and not self._is_controlled_turn(observation)
                ):
                        result = self._session.step()
                        self._episode.result = result
                        observation = self._session.observation()
                        if result != BattleResult.IN_PROGRESS:
                                break

                steps = self._episode.steps if self._episode is not None else 0
                return encode_combat_observation(observation, steps=steps)

        def _apply_action(self, action: Any) -> None:
                if action is None:
                        resolved = CombatAction.auto_resolve()
                elif isinstance(action, CombatAction):
                        resolved = action
                elif isinstance(action, CombatActionType):
                        resolved = CombatAction(action)
                elif isinstance(action, str):
                        normalized = action.strip().lower()
                        if normalized in {"wait"}:
                                resolved = CombatAction.wait()
                        elif normalized in {"defend"}:
                                resolved = CombatAction.defend()
                        elif normalized in {"auto", "auto_resolve", "auto-resolve"}:
                                resolved = CombatAction.auto_resolve()
                        else:
                                raise ValueError(f"Unsupported action string: {action}")
                else:
                        raise TypeError("Action must be a CombatAction, CombatActionType, string, or None")

                success = self._session.apply_action(resolved)
                self._last_action_invalid = not success
                if not success:
                        if resolved.action_type is CombatActionType.AUTO_RESOLVE:
                                raise RuntimeError("Auto-resolve action was rejected by the engine")
                        self._session.apply_action(CombatAction.auto_resolve())

        def _apply_control_flags(self, scenario: CombatScenario) -> None:
                """Mark the non-controlled hero as AI-driven for the upcoming episode."""

                controls_attacker = self._controlled_side is ControlledSide.ATTACKER
                scenario.attacker.human_controlled = controls_attacker
                scenario.defender.human_controlled = not controls_attacker

        def _is_controlled_turn(self, observation: Any) -> bool:
                active_side = getattr(observation, "active_unit_is_attacker", None)
                if active_side is None:
                        return True
                if self._controlled_side is ControlledSide.ATTACKER:
                        return bool(active_side)
                return not bool(active_side)

        def _compute_reward(self, result: BattleResult) -> float:
                if result == BattleResult.IN_PROGRESS:
                        return 0.0

                if result == BattleResult.BOTH_LOSE:
                        return -1.0

                if result in (BattleResult.ATTACKER_HAS_FLED, BattleResult.ATTACKER_VICTORY):
                        return 1.0 if self._controlled_side is ControlledSide.ATTACKER else -1.0

                if result in (BattleResult.DEFENDER_HAS_FLED, BattleResult.DEFENDER_VICTORY):
                        return 1.0 if self._controlled_side is ControlledSide.DEFENDER else -1.0

                return 0.0

        def _build_info(self, *, terminated: bool) -> Dict[str, Any]:
                assert self._episode is not None

                info: Dict[str, Any] = {
                        "battle_result": self._episode.result,
                        "controlled_side": self._controlled_side,
                        "steps": self._episode.steps,
                        "terminated": terminated,
                }

                if self._episode.seed is not None:
                        info["seed"] = self._episode.seed

                info["battlefield_ptr"] = self.battlefield_pointer()
                info["attacker_ptr"] = self.attacker_pointer()
                info["defender_ptr"] = self.defender_pointer()
                info["invalid_action"] = self._last_action_invalid

                return info
