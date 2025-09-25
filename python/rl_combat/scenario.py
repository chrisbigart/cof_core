"""High-level data structures for configuring combat scenarios via Python."""

from __future__ import annotations

import ctypes
from dataclasses import dataclass, field
from typing import List, Sequence, Tuple

from . import bindings

__all__ = ["CombatScenario", "HeroLoadout", "TroopStack"]


@dataclass(frozen=True)
class TroopStack:
        """Describe a single troop stack to assign to a hero."""

        unit_type: int
        stack_size: int

        def to_c(self) -> bindings.TroopStackC:
                stack = bindings.TroopStackC()
                stack.unit_type = int(self.unit_type)
                stack.stack_size = int(self.stack_size)
                return stack


@dataclass
class HeroLoadout:
        """Specification of a hero loadout for a combat scenario."""

        player: int = 1
        hero_class: int = 0
        hero_id: int = 0
        name: str = ""
        level: int = 1
        experience: int = 0
        attack: int = 0
        defense: int = 0
        power: int = 1
        knowledge: int = 1
        mana: int = 10
        dark_energy: int = 0
        has_ballista: bool = False
        human_controlled: bool = True
        troops: Sequence[TroopStack] = field(default_factory=tuple)
        talents: Sequence[int] = field(default_factory=tuple)
        spells: Sequence[int] = field(default_factory=tuple)
        skills: Sequence[int] = field(default_factory=tuple)

        def to_c_struct(self) -> Tuple[bindings.HeroLoadoutC, List[object]]:
                loadout = bindings.HeroLoadoutC()
                loadout.player = int(self.player) & 0xFF
                loadout.hero_class = int(self.hero_class) & 0xFF
                loadout.hero_id = int(self.hero_id) & 0xFFFF
                name_bytes = self.name.encode("utf-8") if self.name else None
                loadout.name = name_bytes
                loadout.level = int(self.level) & 0xFF
                loadout.experience = int(self.experience) & 0xFFFFFFFFFFFFFFFF
                loadout.attack = int(self.attack) & 0xFF
                loadout.defense = int(self.defense) & 0xFF
                loadout.power = int(self.power) & 0xFF
                loadout.knowledge = int(self.knowledge) & 0xFF
                loadout.mana = int(self.mana) & 0xFFFF
                loadout.dark_energy = int(self.dark_energy) & 0xFFFF
                loadout.has_ballista = 1 if self.has_ballista else 0
                loadout.human_controlled = 1 if self.human_controlled else 0

                buffers: List[object] = []
                if name_bytes:
                        buffers.append(name_bytes)

                if self.troops:
                        troop_array_type = bindings.TroopStackC * len(self.troops)
                        troop_array = troop_array_type(*[troop.to_c() for troop in self.troops])
                        loadout.troops = ctypes.cast(troop_array, ctypes.POINTER(bindings.TroopStackC))
                        loadout.troop_count = len(self.troops)
                        buffers.append(troop_array)
                else:
                        loadout.troops = ctypes.POINTER(bindings.TroopStackC)()
                        loadout.troop_count = 0

                if self.talents:
                        talent_array_type = ctypes.c_uint8 * len(self.talents)
                        talent_array = talent_array_type(*[int(t) & 0xFF for t in self.talents])
                        loadout.talents = ctypes.cast(talent_array, ctypes.POINTER(ctypes.c_uint8))
                        loadout.talent_count = len(self.talents)
                        buffers.append(talent_array)
                else:
                        loadout.talents = ctypes.POINTER(ctypes.c_uint8)()
                        loadout.talent_count = 0

                if self.spells:
                        spell_array_type = ctypes.c_uint16 * len(self.spells)
                        spell_array = spell_array_type(*[int(s) & 0xFFFF for s in self.spells])
                        loadout.spells = ctypes.cast(spell_array, ctypes.POINTER(ctypes.c_uint16))
                        loadout.spell_count = len(self.spells)
                        buffers.append(spell_array)
                else:
                        loadout.spells = ctypes.POINTER(ctypes.c_uint16)()
                        loadout.spell_count = 0

                if self.skills:
                        skill_array_type = ctypes.c_uint8 * len(self.skills)
                        skill_array = skill_array_type(*[int(s) & 0xFF for s in self.skills])
                        loadout.skills = ctypes.cast(skill_array, ctypes.POINTER(ctypes.c_uint8))
                        loadout.skill_count = len(self.skills)
                        buffers.append(skill_array)
                else:
                        loadout.skills = ctypes.POINTER(ctypes.c_uint8)()
                        loadout.skill_count = 0

                return loadout, buffers


@dataclass
class CombatScenario:
        """Aggregate attacker/defender hero specifications for combat."""

        attacker: HeroLoadout
        defender: HeroLoadout
        is_deathmatch: bool = False
        environment: int = 0
        quick_combat: bool = False

        def to_c_struct(self) -> Tuple[bindings.CombatScenarioC, List[object]]:
                attacker_c, attacker_buffers = self.attacker.to_c_struct()
                defender_c, defender_buffers = self.defender.to_c_struct()

                scenario_c = bindings.CombatScenarioC()
                scenario_c.attacker = attacker_c
                scenario_c.defender = defender_c
                scenario_c.is_deathmatch = 1 if self.is_deathmatch else 0
                scenario_c.environment = int(self.environment) & 0xFF
                scenario_c.quick_combat = 1 if self.quick_combat else 0

                buffers: List[object] = []
                buffers.extend(attacker_buffers)
                buffers.extend(defender_buffers)
                return scenario_c, buffers
