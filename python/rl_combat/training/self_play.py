"""Self-play orchestration utilities for reinforcement-learning agents."""

from __future__ import annotations

import random
from dataclasses import dataclass, field
from typing import Any, Callable, Dict, Iterable, List, Optional

from ..environment import CombatEnvironment, ControlledSide
from ..session import BattleResult, CombatAction
from .config import SelfPlayConfig
from .policies import BasePolicy


@dataclass
class EpisodeRecord:
    """Full trajectory data for a single combat episode."""

    controlled_side: ControlledSide
    observations: List[Any] = field(default_factory=list)
    actions: List[CombatAction] = field(default_factory=list)
    rewards: List[float] = field(default_factory=list)
    next_observations: List[Any] = field(default_factory=list)
    dones: List[bool] = field(default_factory=list)
    infos: List[Dict[str, Any]] = field(default_factory=list)
    result: BattleResult = BattleResult.IN_PROGRESS

    def __len__(self) -> int:
        return len(self.actions)

    def transitions(self) -> Iterable["Transition"]:
        """Yield replay buffer transitions for downstream consumption."""

        from .replay_buffer import Transition

        for obs, action, reward, next_obs, done in zip(
            self.observations,
            self.actions,
            self.rewards,
            self.next_observations,
            self.dones,
        ):
            yield Transition(
                state=obs,
                action=int(action.action_type),
                reward=reward,
                next_state=next_obs,
                done=done,
                legal_actions_mask=None,
                next_legal_actions_mask=None,
            )


class SelfPlayRunner:
    """Drive repeated combat episodes using a supplied policy."""

    def __init__(
        self,
        env_factory: Callable[[ControlledSide], CombatEnvironment],
        *,
        config: Optional[SelfPlayConfig] = None,
        rng: Optional[random.Random] = None,
    ) -> None:
        self._env_factory = env_factory
        self._config = config or SelfPlayConfig()
        self._rng = rng or random.Random(self._config.random_seed)
        self._next_side = ControlledSide.ATTACKER

    def run_episode(
        self,
        policy: BasePolicy,
        *,
        controlled_side: Optional[ControlledSide] = None,
        scenario_options: Optional[Dict[str, Any]] = None,
        seed: Optional[int] = None,
        max_steps: Optional[int] = None,
    ) -> EpisodeRecord:
        side = controlled_side or self._select_side()

        episode_seed = seed
        if episode_seed is None and self._config.random_seed is not None:
            episode_seed = self._rng.randint(0, 2**31 - 1)

        env = self._env_factory(side)
        with env:
            options = dict(self._config.scenario_options)
            if scenario_options:
                options.update(scenario_options)

            observation, _ = env.reset(seed=episode_seed, options=options)

            record = EpisodeRecord(controlled_side=side)
            terminated = False
            steps = 0
            max_episode_steps = max_steps
            if max_episode_steps is None:
                max_episode_steps = self._config.max_steps_per_episode

            while not terminated:
                action = policy.select_action(observation)
                next_observation, reward, terminated, truncated, step_info = env.step(action)

                record.observations.append(observation)
                record.actions.append(action)
                record.rewards.append(float(reward))
                record.next_observations.append(next_observation)
                record.dones.append(bool(terminated or truncated))
                record.infos.append(step_info)

                observation = next_observation
                steps += 1

                if max_episode_steps is not None and steps >= max_episode_steps:
                    break

                if truncated:
                    break

            record.result = (
                BattleResult(record.infos[-1]["battle_result"])
                if record.infos
                else BattleResult.IN_PROGRESS
            )

        if self._config.alternate_controlled_side and controlled_side is None:
            self._next_side = (
                ControlledSide.DEFENDER
                if side is ControlledSide.ATTACKER
                else ControlledSide.ATTACKER
            )

        return record

    def run_batch(
        self,
        policy: BasePolicy,
        *,
        episodes: Optional[int] = None,
        scenario_options: Optional[Dict[str, Any]] = None,
    ) -> List[EpisodeRecord]:
        total = episodes or self._config.episodes_per_cycle
        results: List[EpisodeRecord] = []
        for _ in range(total):
            record = self.run_episode(policy, scenario_options=scenario_options)
            results.append(record)
        return results

    def _select_side(self) -> ControlledSide:
        if not self._config.alternate_controlled_side:
            return self._next_side
        side = self._next_side
        self._next_side = (
            ControlledSide.DEFENDER
            if self._next_side is ControlledSide.ATTACKER
            else ControlledSide.ATTACKER
        )
        return side
