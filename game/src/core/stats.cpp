#include "game.h"

#include "qt_headers.h"

static inline QDataStream& operator<<(QDataStream& stream, const movement_stats_t& v) {
	stream
		<< quint64(v.total_hero_movement_points_spent)
		<< quint64(v.total_hero_movement_points_remaining_at_end_of_turns)
		<< quint32(v.total_hero_steps_taken)
		<< quint32(v.total_hero_steps_taken_on_roads)
		<< quint32(v.total_hero_steps_taken_on_grass)
		<< quint32(v.total_hero_steps_taken_on_dirt)
		<< quint32(v.total_hero_steps_taken_on_lava)
		<< quint32(v.total_hero_steps_taken_on_desert)
		<< quint32(v.total_hero_steps_taken_on_beach)
		<< quint32(v.total_hero_steps_taken_on_jungle)
		<< quint32(v.total_hero_steps_taken_on_swamp)
		<< quint32(v.total_hero_steps_taken_on_wasteland)
		<< quint32(v.total_hero_steps_taken_on_snow)
		<< quint32(v.total_hero_steps_taken_on_water);
	return stream;
}

static inline QDataStream& operator>>(QDataStream& stream, movement_stats_t& v) {
	quint64 mp_spent = 0;
	quint64 mp_rem = 0;
	quint32 tmp32 = 0;

	stream >> mp_spent; v.total_hero_movement_points_spent = uint64_t(mp_spent);
	stream >> mp_rem;   v.total_hero_movement_points_remaining_at_end_of_turns = uint64_t(mp_rem);

	stream >> tmp32; v.total_hero_steps_taken = uint32_t(tmp32);
	stream >> tmp32; v.total_hero_steps_taken_on_roads = uint32_t(tmp32);
	stream >> tmp32; v.total_hero_steps_taken_on_grass = uint32_t(tmp32);
	stream >> tmp32; v.total_hero_steps_taken_on_dirt = uint32_t(tmp32);
	stream >> tmp32; v.total_hero_steps_taken_on_lava = uint32_t(tmp32);
	stream >> tmp32; v.total_hero_steps_taken_on_desert = uint32_t(tmp32);
	stream >> tmp32; v.total_hero_steps_taken_on_beach = uint32_t(tmp32);
	stream >> tmp32; v.total_hero_steps_taken_on_jungle = uint32_t(tmp32);
	stream >> tmp32; v.total_hero_steps_taken_on_swamp = uint32_t(tmp32);
	stream >> tmp32; v.total_hero_steps_taken_on_wasteland = uint32_t(tmp32);
	stream >> tmp32; v.total_hero_steps_taken_on_snow = uint32_t(tmp32);
	stream >> tmp32; v.total_hero_steps_taken_on_water = uint32_t(tmp32);
	return stream;
}

static inline QDataStream& operator<<(QDataStream& stream, const exploration_stats_t& v) {
	stream
		<< quint16(v.fog_tiles_revealed)
		<< quint32(v.fog_tiles_revealed_watchtower)
		<< quint32(v.fog_tiles_revealed_eyes)
		<< quint16(v.maps_fully_revealed_small)
		<< quint16(v.maps_fully_revealed_medium)
		<< quint16(v.maps_fully_revealed_large)
		<< quint32(v.watchtowers_visited)
		<< quint32(v.keymaster_tents_visited)
		<< quint32(v.signs_visited)
		<< quint32(v.campfires_visited)
		<< quint16(v.portals_used);
	return stream;
}

static inline QDataStream& operator>>(QDataStream& stream, exploration_stats_t& v) {
	quint16 tmp16 = 0;
	quint32 tmp32 = 0;

	stream >> tmp16; v.fog_tiles_revealed = uint16_t(tmp16);
	stream >> tmp32; v.fog_tiles_revealed_watchtower = uint32_t(tmp32);
	stream >> tmp32; v.fog_tiles_revealed_eyes = uint32_t(tmp32);
	stream >> tmp16; v.maps_fully_revealed_small = uint16_t(tmp16);
	stream >> tmp16; v.maps_fully_revealed_medium = uint16_t(tmp16);
	stream >> tmp16; v.maps_fully_revealed_large = uint16_t(tmp16);
	stream >> tmp32; v.watchtowers_visited = uint32_t(tmp32);
	stream >> tmp32; v.keymaster_tents_visited = uint32_t(tmp32);
	stream >> tmp32; v.signs_visited = uint32_t(tmp32);
	stream >> tmp32; v.campfires_visited = uint32_t(tmp32);
	stream >> tmp16; v.portals_used = uint16_t(tmp16);
	return stream;
}

static inline QDataStream& operator<<(QDataStream& stream, const resource_stats_t& v) {
	stream
		<< v.total_resources_picked_up
		<< v.total_resources_from_mines
		<< v.total_resources_from_towns
		<< v.total_resources_spent
		<< v.total_resources_spent_on_buildings
		<< v.total_resources_spent_on_creatures
		<< quint32(v.total_experience_from_chests)
		<< quint32(v.total_gold_from_chests);
	return stream;
}

static inline QDataStream& operator>>(QDataStream& stream, resource_stats_t& v) {
	quint32 tmp32 = 0;

	stream
		>> v.total_resources_picked_up
		>> v.total_resources_from_mines
		>> v.total_resources_from_towns
		>> v.total_resources_spent
		>> v.total_resources_spent_on_buildings
		>> v.total_resources_spent_on_creatures;

	stream >> tmp32; v.total_experience_from_chests = uint32_t(tmp32);
	stream >> tmp32; v.total_gold_from_chests = uint32_t(tmp32);
	return stream;
}

static inline QDataStream& operator<<(QDataStream& stream, const construction_stats_t& v) {
	stream
		<< quint16(v.total_forts_built)
		<< quint16(v.total_castles_upgraded)
		<< quint16(v.total_castles_fully_upgraded)
		<< quint16(v.total_marketplaces_built)
		<< quint16(v.total_tier1_dwellings_built)
		<< quint16(v.total_tier2_dwellings_built)
		<< quint16(v.total_tier3_dwellings_built)
		<< quint16(v.total_tier4_dwellings_built)
		<< quint16(v.total_tier5_dwellings_built)
		<< quint16(v.total_tier6_dwellings_built)
		<< quint16(v.total_special_buildings_built)
		<< quint16(v.total_captains_quarters_built)
		<< quint16(v.total_turrets_built)
		<< quint16(v.paladin_buildings)
		<< quint16(v.enchantress_buildings)
		<< quint16(v.wizard_buildings)
		<< quint16(v.barbarian_buildings)
		<< quint16(v.warlock_buildings)
		<< quint16(v.necromancer_buildings);
	return stream;
}

static inline QDataStream& operator>>(QDataStream& stream, construction_stats_t& v) {
	quint16 tmp16 = 0;

	stream >> tmp16; v.total_forts_built = uint16_t(tmp16);
	stream >> tmp16; v.total_castles_upgraded = uint16_t(tmp16);
	stream >> tmp16; v.total_castles_fully_upgraded = uint16_t(tmp16);
	stream >> tmp16; v.total_marketplaces_built = uint16_t(tmp16);
	stream >> tmp16; v.total_tier1_dwellings_built = uint16_t(tmp16);
	stream >> tmp16; v.total_tier2_dwellings_built = uint16_t(tmp16);
	stream >> tmp16; v.total_tier3_dwellings_built = uint16_t(tmp16);
	stream >> tmp16; v.total_tier4_dwellings_built = uint16_t(tmp16);
	stream >> tmp16; v.total_tier5_dwellings_built = uint16_t(tmp16);
	stream >> tmp16; v.total_tier6_dwellings_built = uint16_t(tmp16);
	stream >> tmp16; v.total_special_buildings_built = uint16_t(tmp16);
	stream >> tmp16; v.total_captains_quarters_built = uint16_t(tmp16);
	stream >> tmp16; v.total_turrets_built = uint16_t(tmp16);
	stream >> tmp16; v.paladin_buildings = uint16_t(tmp16);
	stream >> tmp16; v.enchantress_buildings = uint16_t(tmp16);
	stream >> tmp16; v.wizard_buildings = uint16_t(tmp16);
	stream >> tmp16; v.barbarian_buildings = uint16_t(tmp16);
	stream >> tmp16; v.warlock_buildings = uint16_t(tmp16);
	stream >> tmp16; v.necromancer_buildings = uint16_t(tmp16);
	return stream;
}

static inline QDataStream& operator<<(QDataStream& stream, const combat_stats_t& v) {
	stream
		<< quint64(v.total_damage_done)
		<< quint64(v.total_physical_damage_done)
		<< quint32(v.total_units_killed)
		<< quint32(v.total_units_lost)
		<< quint64(v.total_damage_received)
		<< quint64(v.total_physical_damage_received)
		<< quint64(v.total_healing_done)
		<< quint16(v.total_attacks_made)
		<< quint16(v.total_attacks_received)
		<< quint16(v.total_retaliations_made)
		<< quint32(v.total_life_stolen)
		<< quint16(v.total_spells_cast)
		<< quint16(v.total_spells_cast_level1)
		<< quint16(v.total_spells_cast_level2)
		<< quint16(v.total_spells_cast_level3)
		<< quint16(v.total_spells_cast_level4)
		<< quint16(v.total_spells_cast_level5)
		<< quint16(v.total_spells_cast_nature)
		<< quint16(v.total_spells_cast_arcane)
		<< quint16(v.total_spells_cast_holy)
		<< quint16(v.total_spells_cast_destruction)
		<< quint16(v.total_spells_cast_death)
		<< quint32(v.total_spell_damage_done)
		<< quint32(v.total_spell_damage_enemy_resisted)
		<< quint32(v.total_spell_damage_received)
		<< quint32(v.total_spell_damage_resisted)
		<< quint16(v.total_positive_morale_procs)
		<< quint16(v.total_negative_morale_procs)
		<< quint16(v.total_positive_luck_procs)
		<< quint16(v.total_negative_luck_procs)
		<< quint32(v.total_units_resurrected)
		<< quint16(v.total_units_summoned)
		<< quint32(v.total_mana_spent)
		<< quint32(v.total_mana_restored)
		<< quint16(v.total_critical_hits)
		<< quint16(v.total_hexes_walked)
		<< quint16(v.total_hexes_flown)
		<< quint16(v.total_defends)
		<< quint16(v.total_waits)
		<< quint16(v.total_breath_targets_hit)
		<< quint16(v.total_breath_attacks_received)
		<< quint32(v.total_breath_damage_done)
		<< quint32(v.total_breath_damage_received)
		<< quint32(v.fire_spell_damage)
		<< quint32(v.frost_spell_damage)
		<< quint32(v.lightning_spell_damage)
		<< quint32(v.earth_spell_damage)
		<< quint32(v.chaos_spell_damage)
		<< quint32(v.holy_spell_damage)
		<< quint16(v.death_cloud_7units)
		<< quint16(v.implosion_finish_necromancer)
		<< quint16(v.armageddon_5000_damage)
		<< quint16(v.resurrection_1000hp)
		<< quint32(v.max_damage_single_melee_attack)
		<< quint16(v.chain_lightning_5kills)
		<< quint16(v.meteor_shower_75kills)
		<< quint16(v.frost_kill_fire_immune)
		<< quint16(v.friendly_fire_spell_hits)
		<< quint32(v.friendly_fire_spell_damage)
		<< quint16(v.unique_spells_cast);

	return stream;
}

static inline QDataStream& operator>>(QDataStream& stream, combat_stats_t& v) {
	quint64 tmp64 = 0;
	quint32 tmp32 = 0;
	quint16 tmp16 = 0;

	stream >> tmp64; v.total_damage_done = uint64_t(tmp64);
	stream >> tmp64; v.total_physical_damage_done = uint64_t(tmp64);
	stream >> tmp32; v.total_units_killed = uint32_t(tmp32);
	stream >> tmp32; v.total_units_lost = uint32_t(tmp32);
	stream >> tmp64; v.total_damage_received = uint64_t(tmp64);
	stream >> tmp64; v.total_physical_damage_received = uint64_t(tmp64);
	stream >> tmp64; v.total_healing_done = uint64_t(tmp64);
	stream >> tmp16; v.total_attacks_made = uint16_t(tmp16);
	stream >> tmp16; v.total_attacks_received = uint16_t(tmp16);
	stream >> tmp16; v.total_retaliations_made = uint16_t(tmp16);
	stream >> tmp32; v.total_life_stolen = uint32_t(tmp32);
	stream >> tmp16; v.total_spells_cast = uint16_t(tmp16);
	stream >> tmp16; v.total_spells_cast_level1 = uint16_t(tmp16);
	stream >> tmp16; v.total_spells_cast_level2 = uint16_t(tmp16);
	stream >> tmp16; v.total_spells_cast_level3 = uint16_t(tmp16);
	stream >> tmp16; v.total_spells_cast_level4 = uint16_t(tmp16);
	stream >> tmp16; v.total_spells_cast_level5 = uint16_t(tmp16);
	stream >> tmp16; v.total_spells_cast_nature = uint16_t(tmp16);
	stream >> tmp16; v.total_spells_cast_arcane = uint16_t(tmp16);
	stream >> tmp16; v.total_spells_cast_holy = uint16_t(tmp16);
	stream >> tmp16; v.total_spells_cast_destruction = uint16_t(tmp16);
	stream >> tmp16; v.total_spells_cast_death = uint16_t(tmp16);
	stream >> tmp32; v.total_spell_damage_done = uint32_t(tmp32);
	stream >> tmp32; v.total_spell_damage_enemy_resisted = uint32_t(tmp32);
	stream >> tmp32; v.total_spell_damage_received = uint32_t(tmp32);
	stream >> tmp32; v.total_spell_damage_resisted = uint32_t(tmp32);
	stream >> tmp16; v.total_positive_morale_procs = uint16_t(tmp16);
	stream >> tmp16; v.total_negative_morale_procs = uint16_t(tmp16);
	stream >> tmp16; v.total_positive_luck_procs = uint16_t(tmp16);
	stream >> tmp16; v.total_negative_luck_procs = uint16_t(tmp16);
	stream >> tmp32; v.total_units_resurrected = uint32_t(tmp32);
	stream >> tmp16; v.total_units_summoned = uint16_t(tmp16);
	stream >> tmp32; v.total_mana_spent = uint32_t(tmp32);
	stream >> tmp32; v.total_mana_restored = uint32_t(tmp32);
	stream >> tmp16; v.total_critical_hits = uint16_t(tmp16);
	stream >> tmp16; v.total_hexes_walked = uint16_t(tmp16);
	stream >> tmp16; v.total_hexes_flown = uint16_t(tmp16);
	stream >> tmp16; v.total_defends = uint16_t(tmp16);
	stream >> tmp16; v.total_waits = uint16_t(tmp16);
	stream >> tmp16; v.total_breath_targets_hit = uint16_t(tmp16);
	stream >> tmp16; v.total_breath_attacks_received = uint16_t(tmp16);
	stream >> tmp32; v.total_breath_damage_done = uint32_t(tmp32);
	stream >> tmp32; v.total_breath_damage_received = uint32_t(tmp32);
	stream >> tmp32; v.fire_spell_damage = uint32_t(tmp32);
	stream >> tmp32; v.frost_spell_damage = uint32_t(tmp32);
	stream >> tmp32; v.lightning_spell_damage = uint32_t(tmp32);
	stream >> tmp32; v.earth_spell_damage = uint32_t(tmp32);
	stream >> tmp32; v.chaos_spell_damage = uint32_t(tmp32);
	stream >> tmp32; v.holy_spell_damage = uint32_t(tmp32);
	stream >> tmp16; v.death_cloud_7units = uint16_t(tmp16);
	stream >> tmp16; v.implosion_finish_necromancer = uint16_t(tmp16);
	stream >> tmp16; v.armageddon_5000_damage = uint16_t(tmp16);
	stream >> tmp16; v.resurrection_1000hp = uint16_t(tmp16);
	stream >> tmp32; v.max_damage_single_melee_attack = uint32_t(tmp32);
	stream >> tmp16; v.chain_lightning_5kills = uint16_t(tmp16);
	stream >> tmp16; v.meteor_shower_75kills = uint16_t(tmp16);
	stream >> tmp16; v.frost_kill_fire_immune = uint16_t(tmp16);
	stream >> tmp16; v.friendly_fire_spell_hits = uint16_t(tmp16);
	stream >> tmp32; v.friendly_fire_spell_damage = uint32_t(tmp32);
	stream >> tmp16; v.unique_spells_cast = uint16_t(tmp16);

	return stream;
}

static inline combat_stats_t operator+(const combat_stats_t& a, const combat_stats_t& b) {
	combat_stats_t out = a;

	out.total_damage_done += b.total_damage_done;
	out.total_physical_damage_done += b.total_physical_damage_done;
	out.total_units_killed += b.total_units_killed;
	out.total_units_lost += b.total_units_lost;
	out.total_damage_received += b.total_damage_received;
	out.total_physical_damage_received += b.total_physical_damage_received;
	out.total_healing_done += b.total_healing_done;
	out.total_attacks_made += b.total_attacks_made;
	out.total_attacks_received += b.total_attacks_received;
	out.total_retaliations_made += b.total_retaliations_made;
	out.total_life_stolen += b.total_life_stolen;
	out.total_spells_cast += b.total_spells_cast;
	out.total_spells_cast_level1 += b.total_spells_cast_level1;
	out.total_spells_cast_level2 += b.total_spells_cast_level2;
	out.total_spells_cast_level3 += b.total_spells_cast_level3;
	out.total_spells_cast_level4 += b.total_spells_cast_level4;
	out.total_spells_cast_level5 += b.total_spells_cast_level5;
	out.total_spells_cast_nature += b.total_spells_cast_nature;
	out.total_spells_cast_arcane += b.total_spells_cast_arcane;
	out.total_spells_cast_holy += b.total_spells_cast_holy;
	out.total_spells_cast_destruction += b.total_spells_cast_destruction;
	out.total_spells_cast_death += b.total_spells_cast_death;
	out.total_spell_damage_done += b.total_spell_damage_done;
	out.total_spell_damage_enemy_resisted += b.total_spell_damage_enemy_resisted;
	out.total_spell_damage_received += b.total_spell_damage_received;
	out.total_spell_damage_resisted += b.total_spell_damage_resisted;
	out.total_positive_morale_procs += b.total_positive_morale_procs;
	out.total_negative_morale_procs += b.total_negative_morale_procs;
	out.total_positive_luck_procs += b.total_positive_luck_procs;
	out.total_negative_luck_procs += b.total_negative_luck_procs;
	out.total_units_resurrected += b.total_units_resurrected;
	out.total_units_summoned += b.total_units_summoned;
	out.total_mana_spent += b.total_mana_spent;
	out.total_mana_restored += b.total_mana_restored;
	out.total_critical_hits += b.total_critical_hits;
	out.total_hexes_walked += b.total_hexes_walked;
	out.total_hexes_flown += b.total_hexes_flown;
	out.total_defends += b.total_defends;
	out.total_waits += b.total_waits;
	out.total_breath_targets_hit += b.total_breath_targets_hit;
	out.total_breath_attacks_received += b.total_breath_attacks_received;
	out.total_breath_damage_done += b.total_breath_damage_done;
	out.total_breath_damage_received += b.total_breath_damage_received;
	out.fire_spell_damage += b.fire_spell_damage;
	out.frost_spell_damage += b.frost_spell_damage;
	out.lightning_spell_damage += b.lightning_spell_damage;
	out.earth_spell_damage += b.earth_spell_damage;
	out.chaos_spell_damage += b.chaos_spell_damage;
	out.holy_spell_damage += b.holy_spell_damage;
	out.death_cloud_7units += b.death_cloud_7units;
	out.implosion_finish_necromancer += b.implosion_finish_necromancer;
	out.armageddon_5000_damage += b.armageddon_5000_damage;
	out.resurrection_1000hp += b.resurrection_1000hp;
	out.max_damage_single_melee_attack += b.max_damage_single_melee_attack;
	out.chain_lightning_5kills += b.chain_lightning_5kills;
	out.meteor_shower_75kills += b.meteor_shower_75kills;
	out.frost_kill_fire_immune += b.frost_kill_fire_immune;
	out.friendly_fire_spell_hits += b.friendly_fire_spell_hits;
	out.friendly_fire_spell_damage += b.friendly_fire_spell_damage;
	out.unique_spells_cast += b.unique_spells_cast;

	return out;
}


static inline QDataStream& operator<<(QDataStream& stream, const game_stats_t& v) {
	stream
		<< v.movement
		<< v.resources
		<< v.exploration
		<< v.construction;

	//write_std_map(stream, v.units_recruited);

	stream
		<< quint32(v.total_units_recruited)
		<< quint16(v.total_heroes_recruited)
		<< quint16(v.total_heroes_dismissed)
		<< quint32(v.total_creatures_recruited)
		<< quint16(v.total_towns_captured)
		<< quint16(v.total_towns_lost)
		<< quint16(v.total_mines_flagged)
		<< quint16(v.total_mines_lost)
		<< quint16(v.total_hero_levels_gained)
		<< quint16(v.total_hero_talents_acquired)
		<< quint16(v.total_battles_fought)
		<< quint16(v.total_battles_won)
		<< quint16(v.total_battles_lost)
		<< quint16(v.total_battles_drawn)
		<< quint16(v.total_shrines_visited)
		<< quint16(v.total_artifacts_found)
		<< quint16(v.total_artifacts_found_rarity1)
		<< quint16(v.total_artifacts_found_rarity2)
		<< quint16(v.total_artifacts_found_rarity3)
		<< quint16(v.total_artifacts_found_rarity4)
		<< quint16(v.total_artifacts_found_rarity5)
		<< quint16(v.total_witches_huts_visited)
		<< quint16(v.total_graveyards_pillaged)
		<< quint16(v.towns_defended_with_one_unit)
		<< quint8(v.total_players_defeated)
		<< quint16(v.total_creatures_rescued_from_banks)
		<< quint16(v.total_dragons_defeated_from_topes)
		<< v.cumulative_combat_stats;

	return stream;
}

static inline QDataStream& operator>>(QDataStream& stream, game_stats_t& v) {
	quint32 tmp32 = 0;
	quint16 tmp16 = 0;
	quint8  tmp8  = 0;

	stream
		>> v.movement
		>> v.resources
		>> v.exploration
		>> v.construction;

	//read_std_map(stream, v.units_recruited);

	stream >> tmp32; v.total_units_recruited = uint32_t(tmp32);
	stream >> tmp16; v.total_heroes_recruited = uint16_t(tmp16);
	stream >> tmp16; v.total_heroes_dismissed = uint16_t(tmp16);
	stream >> tmp32; v.total_creatures_recruited = uint32_t(tmp32);
	stream >> tmp16; v.total_towns_captured = uint16_t(tmp16);
	stream >> tmp16; v.total_towns_lost = uint16_t(tmp16);
	stream >> tmp16; v.total_mines_flagged = uint16_t(tmp16);
	stream >> tmp16; v.total_mines_lost = uint16_t(tmp16);
	stream >> tmp16; v.total_hero_levels_gained = uint16_t(tmp16);
	stream >> tmp16; v.total_hero_talents_acquired = uint16_t(tmp16);
	stream >> tmp16; v.total_battles_fought = uint16_t(tmp16);
	stream >> tmp16; v.total_battles_won = uint16_t(tmp16);
	stream >> tmp16; v.total_battles_lost = uint16_t(tmp16);
	stream >> tmp16; v.total_battles_drawn = uint16_t(tmp16);
	stream >> tmp16; v.total_shrines_visited = uint16_t(tmp16);
	stream >> tmp16; v.total_artifacts_found = uint16_t(tmp16);
	stream >> tmp16; v.total_artifacts_found_rarity1 = uint16_t(tmp16);
	stream >> tmp16; v.total_artifacts_found_rarity2 = uint16_t(tmp16);
	stream >> tmp16; v.total_artifacts_found_rarity3 = uint16_t(tmp16);
	stream >> tmp16; v.total_artifacts_found_rarity4 = uint16_t(tmp16);
	stream >> tmp16; v.total_artifacts_found_rarity5 = uint16_t(tmp16);
	stream >> tmp16; v.total_witches_huts_visited = uint16_t(tmp16);
	stream >> tmp16; v.total_graveyards_pillaged = uint16_t(tmp16);
	stream >> tmp16; v.towns_defended_with_one_unit = uint16_t(tmp16);
	stream >> tmp8;  v.total_players_defeated = uint8_t(tmp8);
	stream >> tmp16; v.total_creatures_rescued_from_banks = uint16_t(tmp16);
	stream >> tmp16; v.total_dragons_defeated_from_topes = uint16_t(tmp16);
	stream >> v.cumulative_combat_stats;

	return stream;
}

// -----------------------------
// Addition for game_stats_t
// -----------------------------
static inline game_stats_t operator+(const game_stats_t& a, const game_stats_t& b) {
	game_stats_t out = a;

	// movement
	out.movement.total_hero_movement_points_spent += b.movement.total_hero_movement_points_spent;
	out.movement.total_hero_movement_points_remaining_at_end_of_turns += b.movement.total_hero_movement_points_remaining_at_end_of_turns;
	out.movement.total_hero_steps_taken += b.movement.total_hero_steps_taken;
	out.movement.total_hero_steps_taken_on_roads += b.movement.total_hero_steps_taken_on_roads;
	out.movement.total_hero_steps_taken_on_grass += b.movement.total_hero_steps_taken_on_grass;
	out.movement.total_hero_steps_taken_on_dirt += b.movement.total_hero_steps_taken_on_dirt;
	out.movement.total_hero_steps_taken_on_lava += b.movement.total_hero_steps_taken_on_lava;
	out.movement.total_hero_steps_taken_on_desert += b.movement.total_hero_steps_taken_on_desert;
	out.movement.total_hero_steps_taken_on_beach += b.movement.total_hero_steps_taken_on_beach;
	out.movement.total_hero_steps_taken_on_jungle += b.movement.total_hero_steps_taken_on_jungle;
	out.movement.total_hero_steps_taken_on_swamp += b.movement.total_hero_steps_taken_on_swamp;
	out.movement.total_hero_steps_taken_on_wasteland += b.movement.total_hero_steps_taken_on_wasteland;
	out.movement.total_hero_steps_taken_on_snow += b.movement.total_hero_steps_taken_on_snow;
	out.movement.total_hero_steps_taken_on_water += b.movement.total_hero_steps_taken_on_water;

	// exploration
	out.exploration.fog_tiles_revealed += b.exploration.fog_tiles_revealed;
	out.exploration.fog_tiles_revealed_watchtower += b.exploration.fog_tiles_revealed_watchtower;
	out.exploration.fog_tiles_revealed_eyes += b.exploration.fog_tiles_revealed_eyes;
	out.exploration.maps_fully_revealed_small += b.exploration.maps_fully_revealed_small;
	out.exploration.maps_fully_revealed_medium += b.exploration.maps_fully_revealed_medium;
	out.exploration.maps_fully_revealed_large += b.exploration.maps_fully_revealed_large;
	out.exploration.watchtowers_visited += b.exploration.watchtowers_visited;
	out.exploration.keymaster_tents_visited += b.exploration.keymaster_tents_visited;
	out.exploration.signs_visited += b.exploration.signs_visited;
	out.exploration.campfires_visited += b.exploration.campfires_visited;
	out.exploration.portals_used += b.exploration.portals_used;

	// resources (assumes resource_group_t has operator+ or operator+=)
	out.resources.total_resources_picked_up = out.resources.total_resources_picked_up + b.resources.total_resources_picked_up;
	out.resources.total_resources_from_mines = out.resources.total_resources_from_mines + b.resources.total_resources_from_mines;
	out.resources.total_resources_from_towns = out.resources.total_resources_from_towns + b.resources.total_resources_from_towns;
	out.resources.total_resources_spent = out.resources.total_resources_spent + b.resources.total_resources_spent;
	out.resources.total_resources_spent_on_buildings = out.resources.total_resources_spent_on_buildings + b.resources.total_resources_spent_on_buildings;
	out.resources.total_resources_spent_on_creatures = out.resources.total_resources_spent_on_creatures + b.resources.total_resources_spent_on_creatures;
	out.resources.total_experience_from_chests += b.resources.total_experience_from_chests;
	out.resources.total_gold_from_chests += b.resources.total_gold_from_chests;

	// construction
	out.construction.total_forts_built += b.construction.total_forts_built;
	out.construction.total_castles_upgraded += b.construction.total_castles_upgraded;
	out.construction.total_castles_fully_upgraded += b.construction.total_castles_fully_upgraded;
	out.construction.total_marketplaces_built += b.construction.total_marketplaces_built;
	out.construction.total_tier1_dwellings_built += b.construction.total_tier1_dwellings_built;
	out.construction.total_tier2_dwellings_built += b.construction.total_tier2_dwellings_built;
	out.construction.total_tier3_dwellings_built += b.construction.total_tier3_dwellings_built;
	out.construction.total_tier4_dwellings_built += b.construction.total_tier4_dwellings_built;
	out.construction.total_tier5_dwellings_built += b.construction.total_tier5_dwellings_built;
	out.construction.total_tier6_dwellings_built += b.construction.total_tier6_dwellings_built;
	out.construction.total_special_buildings_built += b.construction.total_special_buildings_built;
	out.construction.total_captains_quarters_built += b.construction.total_captains_quarters_built;
	out.construction.total_turrets_built += b.construction.total_turrets_built;
	out.construction.paladin_buildings += b.construction.paladin_buildings;
	out.construction.enchantress_buildings += b.construction.enchantress_buildings;
	out.construction.wizard_buildings += b.construction.wizard_buildings;
	out.construction.barbarian_buildings += b.construction.barbarian_buildings;
	out.construction.warlock_buildings += b.construction.warlock_buildings;
	out.construction.necromancer_buildings += b.construction.necromancer_buildings;

	// units recruited map + total
	for(const auto& [k, v] : b.units_recruited)
		out.units_recruited[k] += v;

	out.total_units_recruited += b.total_units_recruited;
	out.total_heroes_recruited += b.total_heroes_recruited;
	out.total_heroes_dismissed += b.total_heroes_dismissed;
	out.total_creatures_recruited += b.total_creatures_recruited;
	out.total_towns_captured += b.total_towns_captured;
	out.total_towns_lost += b.total_towns_lost;
	out.total_mines_flagged += b.total_mines_flagged;
	out.total_mines_lost += b.total_mines_lost;
	out.total_hero_levels_gained += b.total_hero_levels_gained;
	out.total_hero_talents_acquired += b.total_hero_talents_acquired;
	out.total_battles_fought += b.total_battles_fought;
	out.total_battles_won += b.total_battles_won;
	out.total_battles_lost += b.total_battles_lost;
	out.total_battles_drawn += b.total_battles_drawn;
	out.total_shrines_visited += b.total_shrines_visited;
	out.total_artifacts_found += b.total_artifacts_found;
	out.total_artifacts_found_rarity1 += b.total_artifacts_found_rarity1;
	out.total_artifacts_found_rarity2 += b.total_artifacts_found_rarity2;
	out.total_artifacts_found_rarity3 += b.total_artifacts_found_rarity3;
	out.total_artifacts_found_rarity4 += b.total_artifacts_found_rarity4;
	out.total_artifacts_found_rarity5 += b.total_artifacts_found_rarity5;
	out.total_witches_huts_visited += b.total_witches_huts_visited;
	out.total_graveyards_pillaged += b.total_graveyards_pillaged;
	out.towns_defended_with_one_unit += b.towns_defended_with_one_unit;
	out.total_players_defeated = uint8_t(out.total_players_defeated + b.total_players_defeated);
	out.total_creatures_rescued_from_banks += b.total_creatures_rescued_from_banks;
	out.total_dragons_defeated_from_topes += b.total_dragons_defeated_from_topes;

	// combat (assumes combat_stats_t supports operator+)
	out.cumulative_combat_stats = out.cumulative_combat_stats + b.cumulative_combat_stats;

	return out;
}