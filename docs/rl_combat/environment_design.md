# RL Combat Environment Design

This document specifies the observation, action, reward, and orchestration scaffolding required to train a Deep Q-Network agent for the standalone combat mode.  It builds on the combat audit to ensure the wrapper stays aligned with the in-game rules.

## Environment Goals

- Mirror the native combat state transitions and `battle_action_t` semantics so trained policies remain valid inside the full game.
- Support self-play by swapping attacker/defender roles across episodes and allowing simultaneous learning/evaluation loops.
- Provide deterministic resets by reusing existing army/hero generators with explicit RNG seeds.
- Surface detailed logging (actions, RNG seeds, emitted `battle_action_t` records) for replay inspection and debugging.

## Observation Specification

Observations are a concatenation of global battle metadata, per-hex features, and per-unit attributes.  The layout is designed to plug into convolutional or attention-based encoders.

### Global Features

| Field | Source | Notes |
| --- | --- | --- |
| Round number | `battlefield_t::round` | Normalized by `MAX_BATTLE_ROUNDS` |
| Active side | `battlefield_t::attacker_moved_last` and queue head | Binary attacker/defender flag |
| Siege metadata | Wall HP, gate status, moat presence | Derived from `castle_layout_t` and `check_wall_section()` state |
| Hero stats | Primary attributes, morale/luck, mana | Pulled from `hero_t` and `battlefield_t::attacking_hero_*` fields |
| Spell cooldowns | `attacking_hero_used_cast`, intervals | Track cast availability per hero |
| Weather/terrain flags | `battlefield_t::is_siege`, `is_quick_combat`, etc. | Provide context for movement penalties |

### Hex Grid Tensor

- Shape: `BATTLEFIELD_WIDTH × BATTLEFIELD_HEIGHT × F_hex`.
- Features per hex:
  - Occupancy (empty, attacker unit id, defender unit id).
  - Passability and moat presence (`battlefield_hex_grid_t::hex::passable`, `is_moat`).
  - Obstacle type (walls, turrets, towers) encoded as categorical embeddings.
  - Ranged cover modifier (from `hex_grid.get_ranged_penalty`).
  - Distance-to-gate for siege attackers (precomputed breadth-first values).

### Per-Unit Table

- Shape: `MAX_TROOPS_PER_SIDE × 2 × F_unit` (attacker and defender slots).
- Features per unit:
  - Unit template id (`troop_t::creature_id`).
  - Current stack size & total HP (derived from `battlefield_unit_t::stack_size`, `hit_points`).
  - Position (x, y) and facing (for two-hex units).
  - Action state flags (`has_moved`, `has_waited`, `has_retaliated`, `has_buff(BUFF_BERSERK)`, etc.).
  - Active buffs/debuffs encoded as multi-hot vector over `buff_e` values.
  - Ammo count and shots remaining for shooters.
  - Last action summary (attack, move, defend, wait) from prior `battle_action_t` for temporal context.

### Queue Snapshot

- Include the next `BATTLE_QUEUE_DEPTH` entries with unit ids, remaining wait flags, and round offsets.
- Provide `time_to_act` scalar for the agent's unit (0 if currently active, positive for queued turns).

## Action Space

Actions are decomposed into a discrete head plus structured parameters to accommodate targeted spells and movement.

### Discrete Action Head

| Index | Meaning | Parameters |
| --- | --- | --- |
| 0 | Wait | none |
| 1 | Defend | none |
| 2 | Move | Target hex (x, y) |
| 3 | Melee attack | Target unit id + destination hex |
| 4 | Ranged attack | Target unit id |
| 5 | Hero spell | Spell id + optional target hex/unit |
| 6 | Catapult/turret choice | Wall section or target unit |
| 7 | Surrender/retreat (if enabled) | confirmation flag |

Invalid combinations are masked using legality checks from `battlefield_t` (e.g., `can_troop_shoot`, `can_troop_act`, `is_spell_cast_allowed`).

### Parameter Encoders

- **Target unit id:** mapped via `battlefield_unit_t::troop_id`; mask unavailable ids.
- **Target hex:** enumerated over passable hexes; mask unreachable cells using `get_unit_route()` distance checks.
- **Spell id:** limited to spells known by the casting hero and filtered by mana cost and battlefield conditions.
- **Wall section:** enumerated via `castle_wall_section_e` with mask when destroyed.

## Reward Shaping

- **Terminal reward:** +1 for victory, -1 for defeat, 0 for stalemate/timeout.
- **Intermediate signals:**
  - Scaled damage dealt minus damage taken per action.
  - Bonuses for eliminating stacks, destroying walls, or capturing siege objectives.
  - Penalties for illegal actions (should be masked but retained for diagnostics) and idling (e.g., repeated waits when action is available).
- **Regularization:** Encourage shorter battles by applying a small step penalty after a configurable round threshold.

## Episode Lifecycle

1. **Reset:**
   - Sample or load attacker/defender armies (JSON templates) and heroes.
   - Initialize `battlefield_t` through `init_*_battle()` and `start_combat()`.
   - Serialize the initial `battle_action_t` queue for logging.
2. **Step:**
   - Retrieve the current active unit via `get_active_unit()`.
   - If the unit is agent-controlled, translate the chosen RL action into the appropriate `battlefield_t` method call.
   - Call `update_battle(game, /*single_step=*/true)` to advance exactly one decision.
   - Capture emitted `battle_action_t` entries for replay buffers and derive rewards.
3. **Termination:**
   - Detect `battle_result_e != BATTLE_IN_PROGRESS` and compute terminal reward.
   - Return summary stats (units lost, mana spent, wall damage) in the `info` dict.

## Self-Play Orchestration

- Alternate which policy controls attacker vs. defender every episode; allow evaluation matches with fixed scripted AI from `ai_combat.cpp`.
- Maintain an Elo-style rating or win-rate tracker to select opponents from a league table.
- Support simultaneous environments for experience generation using battle seeds and asynchronous step calls.

## Tooling and Integration Tasks

1. **C++ Harness Library**
   - Extract combat initialization and stepping helpers into a small library (`libcof_battle_sim`) exposing C-friendly bindings.
   - Provide serialization utilities for observations (`battlefield_t` → flat buffer) and actions (flat buffer → `battle_action_t`).

2. **Python Environment Wrapper**
   - Build a `gymnasium`-compatible interface (`reset`, `step`, `action_space`, `observation_space`).
   - Implement legal-action masking and epsilon-greedy helper functions for DQN exploration.
   - Integrate replay buffer writers that store serialized observations and `battle_action_t` logs.

3. **Testing Strategy**
   - Unit tests covering observation encoding correctness and action application round-trips.
   - Golden-match regression tests comparing environment outcomes to the native AI for fixed seeds.
   - Continuous integration job executing smoke battles to validate deterministic resets.

4. **Infrastructure**
   - Configuration files describing curriculum phases (army sizes, hero levels, siege vs. open field).
   - Logging hooks exporting `battle_action_t` sequences as JSON/Protobuf for visualization.
   - Documentation outlining how to extend the action space (e.g., implementing surrender) and how to plug in custom reward functions.

These specifications unblock implementation of the combat RL environment while ensuring compatibility with existing engine mechanics and providing clear testing/integration milestones.
