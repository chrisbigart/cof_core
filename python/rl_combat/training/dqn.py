"""Deep Q-Network training scaffolding for Clash of Fantasy combat."""

from __future__ import annotations

import random
from dataclasses import dataclass, field
from typing import Any, Callable, Iterable, List, Optional, Sequence, Tuple, Union

from ..environment import CombatEnvironment, ControlledSide
from ..session import CombatAction
from .config import DQNConfig
from .replay_buffer import ReplayBuffer, Transition

try:  # Optional at import time; actual training will require torch installed.
    import torch
    from torch import Tensor
    from torch.nn import Module
    from torch.optim import Optimizer
except ImportError:  # pragma: no cover - torch is optional for static analysis
    torch = None  # type: ignore
    Tensor = Any  # type: ignore
    Module = Any  # type: ignore
    Optimizer = Any  # type: ignore

ObservationEncoder = Callable[[Any], "Tensor"]
LegalActionFunction = Callable[[Any], Optional[Union[Sequence[int], Sequence[bool], "Tensor"]]]


@dataclass
class DiscreteActionSpace:
    """Map integer action indices to native combat commands."""

    actions: Sequence[CombatAction]

    def __post_init__(self) -> None:
        if not self.actions:
            raise ValueError("actions must contain at least one CombatAction")

    @property
    def size(self) -> int:
        return len(self.actions)

    def to_native(self, index: int) -> CombatAction:
        if index < 0 or index >= len(self.actions):
            raise IndexError(f"Action index {index} out of range")
        return self.actions[index]


@dataclass
class TrainingMetrics:
    """Aggregated statistics captured during training."""

    episode_rewards: List[float] = field(default_factory=list)
    losses: List[float] = field(default_factory=list)
    epsilon_values: List[float] = field(default_factory=list)
    total_steps: int = 0

    def record_episode(self, reward: float, epsilon: float) -> None:
        self.episode_rewards.append(reward)
        self.epsilon_values.append(epsilon)

    def record_losses(self, values: Iterable[float]) -> None:
        self.losses.extend(values)


@dataclass
class EpsilonSchedule:
    """Linearly anneal epsilon-greedy exploration over time."""

    start: float = 1.0
    end: float = 0.05
    decay_steps: int = 100_000

    def value(self, step: int) -> float:
        if step >= self.decay_steps:
            return self.end
        fraction = step / max(1, self.decay_steps)
        return self.start + fraction * (self.end - self.start)


class DQNTrainer:
    """Coordinate data collection and optimisation for a DQN agent."""

    def __init__(
        self,
        policy_network: "Module",
        target_network: "Module",
        optimizer: "Optimizer",
        replay_buffer: ReplayBuffer,
        env_factory: Callable[[ControlledSide], CombatEnvironment],
        observation_encoder: ObservationEncoder,
        action_space: DiscreteActionSpace,
        *,
        legal_action_fn: Optional[LegalActionFunction] = None,
        config: Optional[DQNConfig] = None,
        epsilon_schedule: Optional[EpsilonSchedule] = None,
        policy_side: ControlledSide = ControlledSide.ATTACKER,
        device: Optional[str] = None,
        rng: Optional[random.Random] = None,
    ) -> None:
        if torch is None:
            raise RuntimeError("torch is required to instantiate DQNTrainer")

        self._policy_net = policy_network
        self._target_net = target_network
        self._optimizer = optimizer
        self._replay_buffer = replay_buffer
        self._env_factory = env_factory
        self._encoder = observation_encoder
        self._action_space = action_space
        self._legal_action_fn = legal_action_fn
        self._config = config or DQNConfig()
        self._epsilon_schedule = epsilon_schedule or EpsilonSchedule()
        self._side = policy_side
        self._device = torch.device(device) if device else torch.device("cuda" if torch.cuda.is_available() else "cpu")
        self._rng = rng or random.Random()

        self._policy_net.to(self._device)
        self._target_net.to(self._device)
        self._target_net.load_state_dict(self._policy_net.state_dict())
        self._target_net.eval()

        self._global_step = 0

    def train(self, episodes: int) -> TrainingMetrics:
        metrics = TrainingMetrics()

        for _ in range(episodes):
            reward, epsilon, losses = self._run_episode()
            metrics.record_episode(reward, epsilon)
            metrics.record_losses(losses)
            metrics.total_steps = self._global_step

        return metrics

    def _run_episode(self) -> Tuple[float, float, List[float]]:
        epsilon = self._epsilon_schedule.value(self._global_step)
        cumulative_reward = 0.0
        episode_losses: List[float] = []

        env = self._env_factory(self._side)
        with env:
            observation, info = env.reset()
            state_tensor = self._ensure_tensor(self._encoder(observation))

            terminated = bool(info.get("terminated", False))
            steps = 0
            max_steps = self._config.max_steps_per_episode

            while not terminated:
                legal_mask = self._compute_legal_mask(observation)
                action_index = self._select_action(state_tensor, epsilon, legal_mask)
                action = self._action_space.to_native(action_index)

                next_observation, reward, terminated, truncated, _ = env.step(action)
                done = bool(terminated or truncated)
                next_state_tensor = self._ensure_tensor(self._encoder(next_observation))
                next_legal_mask = self._compute_legal_mask(next_observation)

                transition = Transition(
                    state=state_tensor.detach().cpu(),
                    action=action_index,
                    reward=float(reward),
                    next_state=next_state_tensor.detach().cpu(),
                    done=done,
                    legal_actions_mask=legal_mask,
                    next_legal_actions_mask=next_legal_mask,
                )
                self._replay_buffer.append(transition)

                state_tensor = next_state_tensor
                observation = next_observation
                cumulative_reward += float(reward)
                steps += 1
                self._global_step += 1

                if (
                    self._replay_buffer.ready_for_training(self._config.minimum_buffer_size)
                    and self._global_step % self._config.training_frequency == 0
                ):
                    loss = self._optimise_model()
                    if loss is not None:
                        episode_losses.append(loss)

                if self._global_step % self._config.target_update_frequency == 0:
                    self._target_net.load_state_dict(self._policy_net.state_dict())

                if max_steps is not None and steps >= max_steps:
                    break

                if truncated:
                    break

        return cumulative_reward, epsilon, episode_losses

    def _optimise_model(self) -> Optional[float]:
        if torch is None:
            return None
        batch_size = self._config.batch_size
        if len(self._replay_buffer) < batch_size:
            return None

        transitions = self._replay_buffer.sample(batch_size)
        states = torch.stack([self._ensure_tensor(t.state) for t in transitions])
        actions = torch.tensor([t.action for t in transitions], dtype=torch.long, device=self._device)
        rewards = torch.tensor([t.reward for t in transitions], dtype=torch.float32, device=self._device)
        next_states = torch.stack([self._ensure_tensor(t.next_state) for t in transitions])
        dones = torch.tensor([t.done for t in transitions], dtype=torch.float32, device=self._device)

        q_values = self._policy_net(states)
        action_q = q_values.gather(1, actions.unsqueeze(1)).squeeze(1)

        with torch.no_grad():
            next_q_values = self._target_net(next_states)
            next_q_values = self._apply_legal_mask_batch(next_q_values, [t.next_legal_actions_mask for t in transitions])
            max_next_q, _ = next_q_values.max(dim=1)
            targets = rewards + self._config.discount * (1.0 - dones) * max_next_q

        criterion = torch.nn.MSELoss()
        loss_tensor = criterion(action_q, targets)

        self._optimizer.zero_grad()
        loss_tensor.backward()

        if self._config.gradient_clip_norm is not None:
            torch.nn.utils.clip_grad_norm_(self._policy_net.parameters(), self._config.gradient_clip_norm)

        self._optimizer.step()

        return float(loss_tensor.item())

    def _select_action(
        self,
        state: "Tensor",
        epsilon: float,
        legal_mask: Optional[Union[Sequence[int], Sequence[bool], "Tensor"]],
    ) -> int:
        if torch is None:
            raise RuntimeError("torch is required for action selection")

        if self._rng.random() < epsilon:
            return self._sample_random_action(legal_mask)

        self._policy_net.eval()
        with torch.no_grad():
            q_values = self._policy_net(state.unsqueeze(0).to(self._device)).squeeze(0)
            q_values = self._apply_legal_mask(q_values, legal_mask)
            action = int(torch.argmax(q_values).item())
        self._policy_net.train()
        return action

    def _sample_random_action(
        self,
        legal_mask: Optional[Union[Sequence[int], Sequence[bool], "Tensor"]],
    ) -> int:
        available = self._resolve_legal_indices(legal_mask)
        if not available:
            available = list(range(self._action_space.size))
        return self._rng.choice(available)

    def _compute_legal_mask(self, observation: Any) -> Optional[Union[Sequence[int], Sequence[bool], "Tensor"]]:
        if self._legal_action_fn is None:
            return None
        return self._legal_action_fn(observation)

    def _resolve_legal_indices(
        self,
        legal_mask: Optional[Union[Sequence[int], Sequence[bool], "Tensor"]],
    ) -> List[int]:
        if legal_mask is None:
            return list(range(self._action_space.size))

        if torch is not None and isinstance(legal_mask, torch.Tensor):
            mask_tensor = legal_mask.detach().cpu()
            if mask_tensor.dtype == torch.bool:
                indices = [i for i, allowed in enumerate(mask_tensor.tolist()) if allowed]
            else:
                values = mask_tensor.tolist()
                indices = [i for i, allowed in enumerate(values) if allowed > 0]
            return indices

        if isinstance(legal_mask, Sequence) and legal_mask:
            first = legal_mask[0]
            if isinstance(first, bool):
                return [i for i, allowed in enumerate(legal_mask) if allowed]
            if isinstance(first, int):
                return [int(idx) for idx in legal_mask]
            if isinstance(first, float):
                return [i for i, allowed in enumerate(legal_mask) if allowed > 0]

        # Fallback that treats the iterable as explicit indices
        try:
            return [int(idx) for idx in legal_mask]  # type: ignore[arg-type]
        except TypeError:
            return list(range(self._action_space.size))

    def _apply_legal_mask(
        self,
        q_values: "Tensor",
        legal_mask: Optional[Union[Sequence[int], Sequence[bool], "Tensor"]],
    ) -> "Tensor":
        if legal_mask is None:
            return q_values

        mask = self._to_mask_tensor(legal_mask, q_values.shape[-1])
        return q_values.masked_fill(~mask, float("-inf"))

    def _apply_legal_mask_batch(
        self,
        q_values: "Tensor",
        masks: Iterable[Optional[Union[Sequence[int], Sequence[bool], "Tensor"]]],
    ) -> "Tensor":
        if all(mask is None for mask in masks):
            return q_values

        mask_tensors = [self._to_mask_tensor(mask, q_values.shape[-1]) if mask is not None else None for mask in masks]
        stacked_masks = torch.stack([
            mask if mask is not None else torch.ones(q_values.shape[-1], dtype=torch.bool, device=q_values.device)
            for mask in mask_tensors
        ])
        return q_values.masked_fill(~stacked_masks, float("-inf"))

    def _to_mask_tensor(
        self,
        mask: Union[Sequence[int], Sequence[bool], "Tensor"],
        width: int,
    ) -> "Tensor":
        if torch is None:
            raise RuntimeError("torch is required for mask conversion")

        if isinstance(mask, torch.Tensor):
            if mask.dtype == torch.bool:
                tensor_mask = mask.to(self._device)
            else:
                tensor_mask = mask.to(self._device) > 0
        else:
            tensor_mask = torch.zeros(width, dtype=torch.bool, device=self._device)
            try:
                iterable = list(mask)  # type: ignore[arg-type]
            except TypeError:
                iterable = []

            if iterable:
                first = iterable[0]
                if isinstance(first, bool):
                    tensor_mask[: len(iterable)] = torch.tensor(iterable, dtype=torch.bool, device=self._device)
                elif isinstance(first, int):
                    for idx in iterable:
                        if 0 <= int(idx) < width:
                            tensor_mask[int(idx)] = True
                elif isinstance(first, float):
                    for idx, value in enumerate(iterable):
                        if value > 0 and idx < width:
                            tensor_mask[idx] = True
                else:
                    tensor_mask[:] = True
            else:
                tensor_mask[:] = True

        if tensor_mask.shape[-1] != width:
            padded = torch.zeros(width, dtype=torch.bool, device=self._device)
            limit = min(width, tensor_mask.shape[-1])
            padded[:limit] = tensor_mask[:limit]
            tensor_mask = padded

        if not torch.any(tensor_mask):
            tensor_mask = torch.ones(width, dtype=torch.bool, device=self._device)

        return tensor_mask

    def _ensure_tensor(self, value: Any) -> "Tensor":
        if torch is None:
            raise RuntimeError("torch is required for tensor conversion")

        if isinstance(value, torch.Tensor):
            return value.to(self._device)

        return torch.as_tensor(value, dtype=torch.float32, device=self._device)
