"""Command-line helpers for running basic combat DQN training loops."""

from __future__ import annotations

import argparse
import random
import statistics
from typing import Any, Callable, Mapping, Optional, Sequence

from ..environment import CombatEnvironment, ControlledSide
from ..observation import EncodedCombatObservation
from ..scenario import CombatScenario, HeroLoadout, TroopStack
from ..session import CombatAction, CombatActionType, StackObservation
from .config import DQNConfig
from .dqn import DQNTrainer, DiscreteActionSpace, EpsilonSchedule
from .encoders import encoded_observation_to_tensor
from .networks import build_mlp_q_network, combat_observation_dim
from .replay_buffer import ReplayBuffer

try:  # torch is an optional dependency for the base package
    import torch
    from torch.optim import Adam
except ImportError:  # pragma: no cover - handled at runtime
    torch = None  # type: ignore[assignment]
    Adam = None  # type: ignore[assignment]

__all__ = [
    "build_default_scenario",
    "create_environment_factory",
    "main",
]

_DEFAULT_ATTACKER_UNIT = 16  # UNIT_INFANTRYMAN
_DEFAULT_DEFENDER_UNIT = 16  # UNIT_INFANTRYMAN
_DEFAULT_STACK_SIZE = 20


def build_default_scenario(
    rng: random.Random,
    options: Optional[Mapping[str, Any]] = None,
) -> CombatScenario:
    """Return a simple duel scenario for quick smoke tests."""

    opts = dict(options or {})
    attacker_unit = int(opts.get("attacker_unit_type", _DEFAULT_ATTACKER_UNIT))
    defender_unit = int(opts.get("defender_unit_type", _DEFAULT_DEFENDER_UNIT))
    base_attacker_stack = int(opts.get("attacker_stack_size", _DEFAULT_STACK_SIZE))
    base_defender_stack = int(opts.get("defender_stack_size", _DEFAULT_STACK_SIZE))
    variation = int(opts.get("stack_size_variation", 0))

    if variation > 0:
        attacker_delta = rng.randint(-variation, variation)
        defender_delta = rng.randint(-variation, variation)
    else:
        attacker_delta = 0
        defender_delta = 0

    attacker_stack = max(1, base_attacker_stack + attacker_delta)
    defender_stack = max(1, base_defender_stack + defender_delta)

    attacker = HeroLoadout(
        player=1,
        hero_class=0,
        hero_id=0,
        name="RL Attacker",
        level=1,
        attack=2,
        defense=2,
        power=1,
        knowledge=1,
        mana=12,
        human_controlled=True,
        troops=(TroopStack(unit_type=attacker_unit, stack_size=attacker_stack),),
    )
    defender = HeroLoadout(
        player=2,
        hero_class=0,
        hero_id=0,
        name="RL Defender",
        level=1,
        attack=2,
        defense=2,
        power=1,
        knowledge=1,
        mana=12,
        human_controlled=True,
        troops=(TroopStack(unit_type=defender_unit, stack_size=defender_stack),),
    )

    environment = int(opts.get("environment", 0))
    quick_combat = bool(opts.get("quick_combat", False))
    is_deathmatch = bool(opts.get("is_deathmatch", False))

    return CombatScenario(
        attacker=attacker,
        defender=defender,
        environment=environment,
        quick_combat=quick_combat,
        is_deathmatch=is_deathmatch,
    )


def create_environment_factory(
    *,
    library_path: Optional[str],
    scenario_options: Optional[Mapping[str, Any]] = None,
) -> Callable[[ControlledSide], CombatEnvironment]:
    """Build a factory that produces combat environments on demand."""

    options = dict(scenario_options or {})

    def scenario_generator(rng: random.Random, user_options: Optional[Mapping[str, Any]] = None) -> CombatScenario:
        merged = dict(options)
        if user_options:
            merged.update(user_options)
        return build_default_scenario(rng, merged)

    def factory(side: ControlledSide) -> CombatEnvironment:
        return CombatEnvironment(
            scenario_generator,
            controlled_side=ControlledSide(side),
            library_path=library_path,
        )

    return factory


def _build_action_space() -> DiscreteActionSpace:
    return DiscreteActionSpace(
        actions=(
            CombatAction(CombatActionType.WAIT),
            CombatAction(CombatActionType.DEFEND),
            CombatAction(CombatActionType.AUTO_RESOLVE),
        )
    )


def _find_active_stack(observation: EncodedCombatObservation) -> Optional[StackObservation]:
    raw = observation.raw
    if raw.active_unit_id < 0:
        return None

    active_side = raw.active_unit_is_attacker
    if active_side is None:
        return None

    active_position = tuple(raw.active_unit_position)
    stacks = raw.attacker_stacks if active_side else raw.defender_stacks
    for stack in stacks:
        if tuple(stack.position) == active_position:
            return stack

    return None


def _legal_action_mask(observation: EncodedCombatObservation) -> Sequence[bool]:
    """Return a mask indicating which discrete actions are currently valid."""

    active_stack = _find_active_stack(observation)
    if active_stack is None:
        # When no unit is active we can only request auto-resolve to let the
        # simulator advance until a new decision is required.
        return (False, False, True)

    if not active_stack.is_alive or active_stack.is_disabled:
        return (False, False, True)

    can_wait = not active_stack.has_waited
    can_defend = not active_stack.has_defended

    # Auto-resolve is always available as a fallback.
    return (can_wait, can_defend, True)


def _resolve_hidden_layers(values: Optional[Sequence[int]]) -> Sequence[int]:
    if not values:
        return (256, 128)
    return tuple(int(v) for v in values if int(v) > 0)


def _make_observation_encoder(device: Optional[str]) -> Callable[[EncodedCombatObservation], "torch.Tensor"]:
    if torch is None:
        raise RuntimeError("PyTorch is required to encode observations for training")

    def encoder(observation: EncodedCombatObservation) -> "torch.Tensor":
        return encoded_observation_to_tensor(observation, device=device)

    return encoder


def _create_trainer(args: argparse.Namespace) -> DQNTrainer:
    if torch is None or Adam is None:
        raise RuntimeError(
            "PyTorch is required to run training. Install torch before executing this script."
        )

    action_space = _build_action_space()
    hidden_layers = _resolve_hidden_layers(args.hidden_layer)
    config = DQNConfig(
        discount=args.discount,
        batch_size=args.batch_size,
        minimum_buffer_size=args.minimum_buffer,
        training_frequency=args.training_frequency,
        target_update_frequency=args.target_update,
        gradient_clip_norm=args.gradient_clip,
        max_steps_per_episode=args.max_steps,
        replay_buffer_capacity=args.buffer_capacity,
    )

    replay_buffer = ReplayBuffer(config.replay_buffer_capacity, rng=random.Random(args.seed))

    policy_net = build_mlp_q_network(
        action_space,
        input_dim=combat_observation_dim(),
        hidden_layers=hidden_layers,
        layer_norm=args.layer_norm,
    )
    target_net = build_mlp_q_network(
        action_space,
        input_dim=combat_observation_dim(),
        hidden_layers=hidden_layers,
        layer_norm=args.layer_norm,
    )

    optimizer = Adam(policy_net.parameters(), lr=args.learning_rate)

    env_factory = create_environment_factory(
        library_path=args.library,
        scenario_options=args.scenario_option,
    )

    epsilon_schedule = EpsilonSchedule(
        start=args.epsilon_start,
        end=args.epsilon_end,
        decay_steps=args.epsilon_decay,
    )

    observation_encoder = _make_observation_encoder(args.device)

    rng = random.Random(args.seed) if args.seed is not None else None

    trainer = DQNTrainer(
        policy_network=policy_net,
        target_network=target_net,
        optimizer=optimizer,
        replay_buffer=replay_buffer,
        env_factory=env_factory,
        observation_encoder=observation_encoder,
        action_space=action_space,
        legal_action_fn=_legal_action_mask,
        config=config,
        epsilon_schedule=epsilon_schedule,
        policy_side=ControlledSide(args.side),
        device=args.device,
        rng=rng,
    )

    return trainer


def _print_summary(metrics: Any) -> None:
    print(f"Completed {len(metrics.episode_rewards)} episodes / {metrics.total_steps} environment steps")
    if metrics.episode_rewards:
        reward_mean = statistics.mean(metrics.episode_rewards)
        reward_std = statistics.pstdev(metrics.episode_rewards) if len(metrics.episode_rewards) > 1 else 0.0
        print(f"Episode reward: mean={reward_mean:.3f} std={reward_std:.3f}")
    if metrics.losses:
        loss_mean = statistics.mean(metrics.losses)
        print(f"Average loss: {loss_mean:.6f}")
    if metrics.epsilon_values:
        print(
            f"Epsilon schedule: start={metrics.epsilon_values[0]:.3f} "
            f"final={metrics.epsilon_values[-1]:.3f}"
        )


def _parse_scenario_options(option_values: Optional[Sequence[str]]) -> Mapping[str, Any]:
    options: dict[str, Any] = {}
    if not option_values:
        return options

    for raw in option_values:
        if "=" not in raw:
            raise ValueError(f"Scenario option '{raw}' must be of the form key=value")
        key, value = raw.split("=", 1)
        key = key.strip()
        value = value.strip()
        if not key:
            raise ValueError("Scenario option keys must be non-empty")
        if value.lower() in {"true", "false"}:
            options[key] = value.lower() == "true"
            continue
        try:
            if "." in value:
                options[key] = float(value)
            else:
                options[key] = int(value)
            continue
        except ValueError:
            pass
        options[key] = value
    return options


def _build_arg_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Run a basic Clash of Fantasy combat DQN training loop")
    parser.add_argument("--episodes", type=int, default=5, help="Number of training episodes to run")
    parser.add_argument("--library", type=str, default=None, help="Path to libcof_core shared library")
    parser.add_argument("--learning-rate", type=float, default=1e-3, help="Optimizer learning rate")
    parser.add_argument("--batch-size", type=int, default=32, help="Training batch size")
    parser.add_argument("--minimum-buffer", type=int, default=64, help="Minimum replay buffer size before training")
    parser.add_argument("--buffer-capacity", type=int, default=10_000, help="Replay buffer capacity")
    parser.add_argument("--training-frequency", type=int, default=1, help="How often to run optimisation steps")
    parser.add_argument("--target-update", type=int, default=250, help="Target network update frequency")
    parser.add_argument("--max-steps", type=int, default=128, help="Maximum steps per episode")
    parser.add_argument("--discount", type=float, default=0.99, help="Discount factor for future rewards")
    parser.add_argument("--gradient-clip", type=float, default=5.0, help="Gradient clipping norm")
    parser.add_argument("--device", type=str, default=None, help="Torch device to run on (e.g. 'cpu' or 'cuda')")
    parser.add_argument("--hidden-layer", type=int, action="append", help="Add a hidden layer width to the MLP")
    parser.add_argument("--layer-norm", action="store_true", help="Apply layer norm after each hidden layer")
    parser.add_argument("--epsilon-start", type=float, default=1.0, help="Initial epsilon for epsilon-greedy exploration")
    parser.add_argument("--epsilon-end", type=float, default=0.05, help="Final epsilon value")
    parser.add_argument("--epsilon-decay", type=int, default=5_000, help="Steps to anneal epsilon")
    parser.add_argument("--side", choices=[side.value for side in ControlledSide], default=ControlledSide.ATTACKER.value, help="Which side the policy controls")
    parser.add_argument("--seed", type=int, default=42, help="Random seed")
    parser.add_argument(
        "--scenario-option",
        action="append",
        help="Override default scenario parameters (key=value). Can be supplied multiple times.",
    )
    return parser


def main(argv: Optional[Sequence[str]] = None) -> int:
    parser = _build_arg_parser()
    args = parser.parse_args(argv)

    if torch is None or Adam is None:
        parser.error("PyTorch is required to run this training script. Install torch and try again.")

    if args.episodes <= 0:
        parser.error("--episodes must be positive")

    if args.batch_size <= 0:
        parser.error("--batch-size must be positive")

    if args.minimum_buffer <= 0:
        parser.error("--minimum-buffer must be positive")

    if args.buffer_capacity <= 0:
        parser.error("--buffer-capacity must be positive")

    if args.training_frequency <= 0:
        parser.error("--training-frequency must be positive")

    if args.target_update <= 0:
        parser.error("--target-update must be positive")

    if args.max_steps is not None and args.max_steps <= 0:
        parser.error("--max-steps must be positive")

    if not 0.0 <= args.epsilon_end <= args.epsilon_start <= 1.0:
        parser.error("Require 0 <= epsilon_end <= epsilon_start <= 1")

    args.scenario_option = _parse_scenario_options(args.scenario_option)

    random.seed(args.seed)
    if torch is not None and args.seed is not None:
        torch.manual_seed(args.seed)
        if torch.cuda.is_available():
            torch.cuda.manual_seed_all(args.seed)

    trainer = _create_trainer(args)
    metrics = trainer.train(args.episodes)
    _print_summary(metrics)
    return 0


if __name__ == "__main__":  # pragma: no cover - CLI entry point
    raise SystemExit(main())
