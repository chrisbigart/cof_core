# Combat Engine Audit Overview

This document summarizes the current combat engine structures and entry points that must be wrapped to create a reinforcement-learning (RL) environment dedicated to battlefield encounters.

## Key Source Files
- `game/src/core/battlefield.h` / `.cpp`: primary combat state container (`battlefield_t`), action definitions, combat loop utilities, spell/damage resolution, and serialization helpers.
- `game/src/core/battlefield_hex_grid.h`: hex-grid geometry, neighborhood queries, and coordinate conversions used by movement and range calculations.
- `game/src/core/ai_combat.cpp`: scripted AI utilities for controlling units (future reference for baseline behaviors).
- `game/src/core/hero.h`, `troop.h`, `spell.h`, `artifact.h`: hero and unit stats, spell metadata, and buff definitions referenced throughout combat resolution.

These files have no additional AGENTS guidelines, so all coding conventions follow existing project style.

## Core Data Structures

### `battlefield_unit_t`
Extends `troop_t` with battlefield-specific state such as `troop_id`, grid coordinates, retaliation counters, per-stack health, resurrection tracking, and status flags for waits, moves, spells, morale, and defender/attacker allegiance.【F:game/src/core/battlefield.h†L38-L112】【F:game/src/core/battlefield.h†L392-L436】  It also embeds a fixed-size `buff_t` array and helper methods (`is_disabled`, `is_two_hex`, `is_flyer`, etc.) that enforce ability and status logic used when validating RL actions.

### `battle_action_t`
Represents an atomic combat event emitted through `fn_emit_combat_action`.  It records the acting unit, optional source/target hexes, movement route, list of affected units (with damage, kills, and healing flags), spell or buff metadata, artifacts/talents applied, wall-section hits, and luck modifiers.【F:game/src/core/battlefield.h†L205-L239】  Serialization helpers in `battlefield.cpp` allow saving/loading action logs for replay datasets.【F:game/src/core/battlefield.cpp†L10-L103】

### `battlefield_hex_grid_t`
Owns the hex array backing the battlefield and implements axial/offset conversions, distance checks, neighborhood queries, and pixel-mapping helpers.【F:game/src/core/battlefield_hex_grid.h†L1-L124】  Movement validation and pathfinding rely on this API, so the RL wrapper can query legal moves via the same utilities.

### `battlefield_t`
The central combat state machine storing armies, heroes, environment flags, siege wall health, time dilation effects, accumulated combat stats, and the unit move queue.【F:game/src/core/battlefield.h†L392-L520】  Its methods span combat initialization (`init_hero_*_battle`), spellcasting, movement, attacks, morale checks, queue recomputation, and battle termination.  Most RL-facing APIs will delegate to these methods to ensure parity with the in-game rules.

## Combat Loop Entry Points

### Initialization
Combat setup functions populate armies and map objects before calling `start_combat()`, which handles artifact-triggered pre-battle spellcasts and seeds the first active unit via `compute_next_unit_to_move()`.【F:game/src/core/battlefield.cpp†L2308-L2366】  During setup, each unit receives a unique `troop_id` used throughout serialization and action logs, so RL observations must preserve this mapping.

### Turn Selection
`compute_next_unit_to_move()` repeatedly calls `recompute_unit_move_queue()` until it finds a controllable unit or processes auto-resolved entities (catapults, turrets, ballistae).【F:game/src/core/battlefield.cpp†L3224-L3296】  The queue builder groups attacker/defender units, respects wait actions, and interleaves future rounds until it accumulates `BATTLE_QUEUE_DEPTH` entries.【F:game/src/core/battlefield.cpp†L3304-L3389】  This is the natural hook for the RL `step()` routine to fetch the pending actor.

### Action Resolution
Movement flows through `move_unit()` and optionally triggers morale-based extra turns or `finish_troop_action()` when the action completes.【F:game/src/core/battlefield.cpp†L3389-L3479】  Attacks call `attack_unit()` / `move_and_attack_unit()`, which in turn invoke damage, retaliation, AOE handling, post-attack effects, and action emission.【F:game/src/core/battlefield.cpp†L3609-L3773】【F:game/src/core/battlefield.cpp†L3774-L3962】  Spells route through `cast_spell()` and related helpers (not detailed here) but follow the same `battle_action_t` emission pattern.

### Combat Advancement
`update_battle()` advances the encounter by repeatedly selecting the active unit, yielding to human players, and otherwise letting the AI automation resolve the turn before checking for completion through `end_combat()`.【F:game/src/core/battlefield.h†L474-L520】【F:game/src/core/battlefield.cpp†L2869-L2888】【F:game/src/core/battlefield.cpp†L3240-L3264】  The loop keeps returning `BATTLE_IN_PROGRESS` until either side runs out of units or the caller requests a single-step advance.

### `update_battle` Control Flow
When the active unit is AI-controlled, `auto_move_troop()` orchestrates targeting, pathing, attacks, and fallback actions before chaining into queue recomputation helpers.【F:game/src/core/battlefield.cpp†L2890-L3035】  The relationships among the primary helpers are summarized below to guide RL interception points.

```
update_battle
  └── auto_move_troop
        ├── get_nearest_target / get_target_in_range
        ├── move_and_attack_unit → attack_unit → resolve_damage / retaliation
        ├── move_unit → finish_troop_action → compute_next_unit_to_move → recompute_unit_move_queue → next_round
        ├── defend_unit / wait_unit
        └── catapult/turret/ballista auto-fire → compute_next_unit_to_move
```

`compute_next_unit_to_move()` also handles morale checks, siege engine automation, and re-entry into the loop when auto-actions finish, so an RL wrapper can either supply a `battle_action_t` directly or let these helpers run for scripted entities.【F:game/src/core/battlefield.cpp†L3058-L3315】  Preserving the queue recomputation logic ensures the environment remains consistent with in-game turn ordering.

### Spell Resolution Touchpoints
Pre-battle effects and artifact triggers share `cast_spell_on_random_troops()` to pick affected stacks, while interactive hero casting flows through `cast_spell()` with optional unit and hex parameters before emitting `battle_action_t` payloads.【F:game/src/core/battlefield.cpp†L1382-L1426】【F:game/src/core/battlefield.cpp†L2319-L2358】  Both paths eventually call `fn_emit_combat_action`, making them important hooks for logging and for exposing buff changes in RL observations.

## Next Steps
- Implement the observation encoder and action translator defined in the environment design document, reusing combat serialization helpers.
- Build the reinforcement-learning environment wrapper around `update_battle()` with reset/step APIs and deterministic seeding.
- Prototype a lightweight harness that instantiates `battlefield_t`, seeds armies, and exercises `compute_next_unit_to_move()` to confirm we can drive battles without the full game loop.

This foundation captures the primary combat data flow required to wrap the engine in an RL environment while maintaining compatibility with existing mechanics.
