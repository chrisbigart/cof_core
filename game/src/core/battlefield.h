#pragma once

#include "core/utils.h"
#include "core/game_config.h"
#include "core/battlefield_hex_grid.h"
#include "core/troop.h"
#include "core/hero.h"
#include "core/adventure_map.h"

enum battle_action_e {
	ACTION_NONE,
	ACTION_ROUND_ENDED,
	ACTION_WAIT,
	ACTION_DEFEND,
	ACTION_WALK_TO_HEX,
	ACTION_FLY_TO_HEX,
	ACTION_MELEE_ATTACK,
	ACTION_RANGED_ATTACK,
	ACTION_RANGED_AOE_ATTACK,
	ACTION_RETALIATION,
	ACTION_MORALED,
	ACTION_MISMORALED,
	ACTION_ATTACKER_SPELLCAST,
	ACTION_DEFENDER_SPELLCAST,
	ACTION_UNIT_SPELLCAST,
	ACTION_MANA_RESTORED,
	ACTION_BUFF_APPLIED,
	ACTION_BUFF_EXPIRED,
	ACTION_UNIT_UPDATE,
	ACTION_UNIT_LIFESTEAL,
	ACTION_CATAPULT_FIRED_AT_WALL
};

enum castle_wall_section_e : uint8_t {
	CASTLE_WALL_SECTION_NONE,
	CASTLE_WALL_SECTION_MAIN_TURRET,
	CASTLE_WALL_SECTION_LEFT_TURRET,
	CASTLE_WALL_SECTION_RIGHT_TURRET,
	CASTLE_WALL_SECTION_TOP_INNER_WALL,
	CASTLE_WALL_SECTION_TOP_OUTER_WALL,
	CASTLE_WALL_SECTION_BOTTOM_INNER_WALL,
	CASTLE_WALL_SECTION_BOTTOM_OUTER_WALL,
	CASTLE_WALL_SECTION_GATE
};

struct battlefield_unit_t : troop_t {
	battlefield_unit_t() {
		//unit_health = get_base_max_hitpoints();
		original_stack_size = stack_size;
	}
	battlefield_unit_t(const troop_t& troop) {
		unit_health = troop.get_base_max_hitpoints();
		original_stack_size = troop.stack_size;
		unit_type = troop.unit_type;
		stack_size = troop.stack_size;
	}
	
	void clear() {
		original_stack_size = 0;
		for(auto& b : buffs)
			b = buff_t();
		
		troop_t::clear();
	}
	
	int8_t troop_id = -1;
	int8_t x = -1;
	int8_t y = -1;
	int8_t retaliations_remaining = 1;
	uint16_t original_stack_size = 0;
	uint16_t unit_health = 0;
	uint8_t resurrection_count = 0;
	//bitfield / enum
	bool was_summoned = false;
	bool has_waited = false;
	bool has_moved = false;
	bool has_cast_spell = false;
	bool has_moraled = false;
	bool has_defended = false;
	bool is_attacker = false;
	std::array<buff_t, game_config::MAX_UNIT_BUFFS> buffs;

	bool is_disabled(int round_offset = 0) const {
		for(const auto& b : buffs) {
			if((b.buff_id == BUFF_FROZEN || b.buff_id == BUFF_STUNNED || b.buff_id == BUFF_SEDUCED || b.buff_id == BUFF_BLINDED
				|| b.buff_id == BUFF_PARALYZED || b.buff_id == BUFF_PACIFIED || b.buff_id == BUFF_FEARED)
			   && b.duration > round_offset)
				return true;
		}

		return false;
	}
	
	bool is_two_hex() const {
		return game_config::get_creature(unit_type).two_hex;
	}

	bool is_flyer() const {
		return game_config::get_creature(unit_type).has_inherent_buff(BUFF_FLYER) || has_buff(BUFF_ANGELS_WINGS);
	}
	
	bool is_shooter() const {
		return game_config::get_creature(unit_type).has_inherent_buff(BUFF_SHOOTER);
	}

	bool is_undead() const {
		const auto& cr = game_config::get_creature(unit_type);
		return cr.has_inherent_buff(BUFF_UNDEAD);
	}

	bool is_living() const {
		const auto& cr = game_config::get_creature(unit_type);
		return !(cr.has_inherent_buff(BUFF_UNDEAD) || cr.has_inherent_buff(BUFF_ANIMATED));
	}
	
	bool add_buff(buff_e buff_id, int8_t duration = -1, uint8_t magnitude = 0) {
		bool added = false;
		//some buffs negate other buffs, ex. bless/curse
		if(buff_id == BUFF_BLESSED && has_buff(BUFF_CURSED))
			remove_buff(BUFF_CURSED);
		else if(buff_id == BUFF_CURSED && has_buff(BUFF_BLESSED))
			 remove_buff(BUFF_BLESSED);
		//BUFF_HASTENED BUFF_SLOWED
		if(buff_id == BUFF_MAGIC_IMMUNITY) {
			//what to do here?  keep existing magical buffs or remove them?
		}		
		
		//replace existing buff if applicable
		if(has_buff(buff_id)) {
			auto& existing = get_buff(buff_id);
			existing.magnitude = magnitude;

			if(existing.duration != -1 && existing.duration < duration)
				existing.duration = duration;
			
			return true;
		}

		for(auto& b : buffs) {
			if(b.buff_id == BUFF_NONE) {
				b.buff_id = buff_id;
				b.duration = duration;
				b.magnitude = magnitude;
				break;
			}
		}
		
		return added;
	}
	
	bool remove_buff(buff_e buff) {
		bool removed = false;

		for(auto& b : buffs) {
			if(b.buff_id == buff) {
				b = buff_t();
				removed = true;
			}
		}
		
		return removed;
	}
	
	bool has_buff(buff_e buff) const { //includes inherent buffs
		for(auto& b : buffs) {
			if(b.buff_id == buff)
				return true;
		}
		auto& creature = game_config::get_creature(unit_type);
		for(auto b : creature.inherent_buffs) {
			if(b == buff)
				return true;
		}
		return false;
	}
	
	bool has_aoe_attack() const {
		return has_buff(BUFF_LICH_DEATH_CLOUD_ATTACK) || has_buff(BUFF_MAGOG_FIREBALL_ATTACK);
	}

	buff_t& get_buff(buff_e buff) { //only applies to temporary (de)buffs
		for(auto& b : buffs) {
			if(b.buff_id == buff)
				return b;
		}
		
		return buffs[0];
	}
	
	const buff_t& get_buff(buff_e buff) const { //only applies to temporary (de)buffs
		for(auto& b : buffs) {
			if(b.buff_id == buff)
				return b;
		}
		
		return buffs[0];
	}
};

QDataStream& operator<<(QDataStream& stream, const battlefield_unit_t& unit);
QDataStream& operator>>(QDataStream& stream, battlefield_unit_t& unit);

typedef int8_t unit_id;

struct battle_action_t {
	battle_action_e action = ACTION_NONE;
	/*battlefield_hex_t* */ hex_location_t source_hex;// = nullptr;
	/*battlefield_hex_t* */ hex_location_t target_hex;// = nullptr;
	route_t route;
	battlefield_unit_t acting_unit;// = nullptr;
	struct target_t {
		battlefield_unit_t target_unit;// = nullptr;
		uint32_t damage = 0;
		uint16_t kills = 0;
		int8_t/*bool*/ was_fatal = false;
		bool is_healing = false;
	};
	hero_t* applicable_hero = nullptr;
	std::vector<target_t> affected_units;
	spell_e spell_id = SPELL_UNKNOWN;
	buff_e buff_id = BUFF_NONE;
	talent_e applicable_talent = TALENT_NONE;
	artifact_e applicable_artifact = ARTIFACT_NONE;
	castle_wall_section_e affected_wall_section = CASTLE_WALL_SECTION_NONE;
	int luck_effect = 0;
	int effect_value = 0;
	//std::string log_message;
};

QDataStream& operator<<(QDataStream& stream, const battle_action_t& action);
QDataStream& operator>>(QDataStream& stream, battle_action_t& action);

struct army_t {
	const static uint MAX_BATTLEFIELD_TROOPS = 16;
	using battlefield_unit_group_t = std::array<battlefield_unit_t, MAX_BATTLEFIELD_TROOPS>;
	battlefield_unit_group_t troops;
	hero_t* hero = nullptr;
	
	void clear() {
		for(auto& troop : troops)
			troop.clear();
	}

	uint get_attack_bonus() const {
		uint bonus = 0;
		if(hero) {
			bonus += hero->get_total_attack_bonus();
		}

		//native terrain

		//kingdom-wide bonuses

		return bonus;
	}

	uint get_defense_bonus() const {
		uint bonus = 0;
		if(hero) {
			bonus += hero->get_total_defense_bonus();
		}

		//native terrain

		//kingdom-wide bonuses

		return bonus;
	}

	bool is_affected_by_skill(skill_e skill) const {
		return hero && hero->get_secondary_skill_level(skill);
	}

	bool is_affected_by_talent(talent_e talent) const {
		return hero && hero->has_talent(talent);
	}

	bool is_affected_by_specialty(hero_specialty_e specialty) const {
		return hero && (hero->specialty == specialty);
	}
	
	int get_luck_value() const {
		if(!hero)
			return 0;
		
		return hero->get_luck();
	}
	
	int get_morale_value() const {
		if(!hero)
			return 0;
		
		return hero->get_morale();
	}
	
};

QDataStream& operator<<(QDataStream& stream, const army_t& army);
QDataStream& operator>>(QDataStream& stream, army_t& army);

struct map_monster_t;
struct town_t;

enum battle_result_e {
	BATTLE_IN_PROGRESS,
	BATTLE_ATTACKER_VICTORY,
	BATTLE_DEFENDER_VICTORY,
	BATTLE_ATTACKER_HAS_FLED,
	BATTLE_DEFENDER_HAS_FLED,
	BATTLE_BOTH_LOSE
};

struct move_queue_slot_t {
	battlefield_unit_t* unit = nullptr;
	uint round_marker = 0;
	bool attacker_can_cast = false;
	bool defender_can_cast = false;
};

enum battlefield_environment_e : uint8_t {
	BATTLEFIELD_ENVIRONMENT_RANDOM = 0,
	BATTLEFIELD_ENVIRONMENT_WATER = TERRAIN_WATER,
	BATTLEFIELD_ENVIRONMENT_GRASS = TERRAIN_GRASS,
	BATTLEFIELD_ENVIRONMENT_DIRT = TERRAIN_DIRT,
	BATTLEFIELD_ENVIRONMENT_WASTELAND = TERRAIN_WASTELAND,
	BATTLEFIELD_ENVIRONMENT_LAVE = TERRAIN_LAVA,
	BATTLEFIELD_ENVIRONMENT_DESERT = TERRAIN_DESERT,
	BATTLEFIELD_ENVIRONMENT_BEACH = TERRAIN_BEACH,
	BATTLEFIELD_ENVIRONMENT_SNOW = TERRAIN_SNOW,
	BATTLEFIELD_ENVIRONMENT_SWAMP = TERRAIN_SWAMP,
	BATTLEFIELD_ENVIRONMENT_JUNGLE = TERRAIN_JUNGLE,
	///
	BATTLEFIELD_ENVIRONMENT_GRAVEYARD = 100,
	BATTLEFIELD_ENVIRONMENT_PYRAMID
};

struct combat_stats_t {
	uint64_t total_damage_done = 0;
	uint64_t total_physical_damage_done = 0;
	uint32_t total_units_killed = 0;
	uint32_t total_units_lost = 0;
	uint64_t total_damage_received = 0;
	uint64_t total_physical_damage_received = 0;
	uint64_t total_healing_done = 0;
	uint16_t total_attacks_made = 0;
	uint16_t total_attacks_received = 0;
	uint16_t total_retaliations_made = 0;
	uint32_t total_life_stolen = 0;
	uint16_t total_spells_cast = 0;
	uint16_t total_spells_cast_level1 = 0;
	uint16_t total_spells_cast_level2 = 0;
	uint16_t total_spells_cast_level3 = 0;
	uint16_t total_spells_cast_level4 = 0;
	uint16_t total_spells_cast_level5 = 0;
	uint16_t total_spells_cast_nature = 0;
	uint16_t total_spells_cast_arcane = 0;
	uint16_t total_spells_cast_holy = 0;
	uint16_t total_spells_cast_destruction = 0;
	uint16_t total_spells_cast_death = 0;
	uint32_t total_spell_damage_done = 0;
	uint32_t total_spell_damage_enemy_resisted = 0;
	uint32_t total_spell_damage_received = 0;
	uint32_t total_spell_damage_resisted = 0;
	uint16_t total_morale_procs = 0;
	uint16_t total_luck_procs = 0;
	uint32_t total_units_resurrected = 0;
	uint16_t total_units_summoned = 0;
	uint32_t total_mana_spent = 0;
	uint32_t total_mana_restored = 0;
	uint16_t total_critical_hits = 0;
	uint16_t total_hexes_walked = 0;
	uint16_t total_hexes_flown = 0;
	uint16_t total_defends = 0;
	uint16_t total_waits = 0;
	uint16_t total_breath_targets_hit = 0;
	uint16_t total_breath_attacks_received = 0;
	uint32_t total_breath_damage_done = 0;
	uint32_t total_breath_damage_received = 0;
	uint32_t fire_spell_damage = 0;
	uint32_t frost_spell_damage = 0;
	uint32_t lightning_spell_damage = 0;
	uint32_t earth_spell_damage = 0;
	uint32_t chaos_spell_damage = 0;
	uint32_t holy_spell_damage = 0;
	uint16_t death_cloud_7units = 0;
	uint16_t implosion_finish_necromancer = 0;
	uint16_t armageddon_5000_damage = 0;
	uint16_t resurrection_1000hp = 0;
	uint32_t max_damage_single_melee_attack = 0;
};

struct battlefield_t {
	static const int BATTLE_QUEUE_DEPTH = 20;

	battlefield_hex_grid_t hex_grid;
	
	army_t attacking_army;
	army_t defending_army;
	
	hero_t* attacking_hero = nullptr;
	uint16_t attacking_hero_initial_mana = 0;
	uint16_t attacking_hero_initial_dark_energy = 0;
	bool attacking_hero_used_cast = false;
	uint attacking_hero_cast_interval = 0;
	hero_t* defending_hero = nullptr;
	uint16_t defending_hero_initial_mana = 0;
	uint16_t defending_hero_initial_dark_energy = 0;
	bool defending_hero_used_cast = false;
	uint defending_hero_cast_interval = 0;
	map_monster_t* defending_monster = nullptr;
	interactable_object_t* defending_map_object = nullptr;
	town_t* defending_town = nullptr;
	battlefield_environment_e environment_type = BATTLEFIELD_ENVIRONMENT_RANDOM;

	uint round = 0;
	uint unit_actions_this_round = 0;
	uint unit_actions_per_round = 0;
	bool attacker_moved_last = false;
	battle_result_e result = BATTLE_IN_PROGRESS;
	bool is_quick_combat = false;
	bool combat_started = false;
	//uint log_level = 1;
	bool were_troops_raised = false;
	army_t necromancy_raised_troops;
	bool is_deathmatch = false;
	bool is_creature_bank_battle = false;
	std::vector<artifact_e> captured_artifacts;

	//siege vars
	int gate_hp = 2;
	int main_turret_hp = 5;
	int top_turret_hp = 3;
	int bottom_turret_hp = 3;
	int top_inner_wall_hp = 2;
	int top_outer_wall_hp = 2;
	int bottom_inner_wall_hp = 2;
	int bottom_outer_wall_hp = 2;

	//time dilation vars
	bool time_dilation_in_effect = false;
	bool time_dilation_is_attacker_effect = false; //determines if attacker or defender cast time dilation
	int time_dilation_rounds_remaining = 0;
	int time_dilation_speed_increase = 0;
	int time_dilation_initiative_increase = 0;

	combat_stats_t total_stats;
	combat_stats_t attacker_stats;
	combat_stats_t defender_stats;

	std::vector<battlefield_unit_t*> unit_move_queue;
	std::vector<move_queue_slot_t> unit_move_queue_with_markers;

	const static std::vector<std::vector<battlefield_direction_e>> obstacle_shapes;
	
	bool is_siege() const { return defending_town != nullptr; }
	bool are_any_castle_walls_remaining() const;
	castle_wall_section_e get_catapult_auto_target_wall() const;
	//std::function<void(const std::string&)> fn_combat_log;
	//void combat_log(const std::string& message) const;
	std::function<void(const battle_action_t& action)> fn_emit_combat_action;
	//std::function<void(const battlefield_unit_t& unit)> fn_update_combat_unit;

	uint get_unit_adjusted_attack(battlefield_unit_t& unit);
	uint get_unit_adjusted_defense(battlefield_unit_t& unit);
	uint get_unit_adjusted_hp(const battlefield_unit_t& unit);
	std::pair<uint, uint> get_unit_adjusted_damage_range(battlefield_unit_t& unit);

	int get_hero_adjusted_power(hero_t* hero);
	
	int get_unit_adjusted_luck(battlefield_unit_t& unit);
	int get_unit_adjusted_morale(const battlefield_unit_t& unit) const;
	int get_unit_adjusted_speed(battlefield_unit_t& unit) const;
	int get_unit_adjusted_initiative(battlefield_unit_t& unit);
	int get_unit_adjusted_resistance(battlefield_unit_t& unit, magic_damage_e damage_type = MAGIC_DAMAGE_ALL);
	
	int is_hit_lucky(battlefield_unit_t& unit);
	uint32_t apply_luck_damage_modifier(const battlefield_unit_t& unit, uint32_t base_damage, int luck_effect);
	std::pair<uint32_t, uint32_t> get_attack_damage_range(battlefield_unit_t& attacker, battlefield_unit_t& defender, bool is_ranged_attack, battlefield_hex_t* attack_from_hex = nullptr, battlefield_hex_t* source_movement_hex = nullptr);
	float get_damage_multiplier(battlefield_unit_t& attacker, battlefield_unit_t& defender, bool is_retaliation);
	uint32_t calculate_damage(battlefield_unit_t& attacker, battlefield_unit_t& defender,  bool is_ranged_attack, bool is_retaliation, battlefield_hex_t* attack_from_hex = nullptr, battlefield_hex_t* source_movement_hex = nullptr);
	uint32_t calculate_magic_damage_to_stack(uint32_t base_damage, battlefield_unit_t& defender, magic_damage_e damage_type);
	uint32_t apply_damage_adjustments(uint32_t base_damage, battlefield_unit_t& attacker, battlefield_unit_t& defender, bool is_ranged_attack, bool is_retaliation, battlefield_hex_t* attack_from_hex = nullptr, battlefield_hex_t* source_movement_hex = nullptr);
	uint16_t get_kills(uint32_t damage, battlefield_unit_t& defender);
	std::pair<uint32_t, uint32_t> get_kill_range(battlefield_unit_t& attacker, battlefield_unit_t& defender, bool is_ranged_attack, battlefield_hex_t* attack_from_hex = nullptr, battlefield_hex_t* source_movement_hex = nullptr);
	uint16_t deal_damage_to_stack(uint32_t damage, battlefield_unit_t& defender, bool is_spell_damage = false);
	uint16_t deal_magic_damage_to_stack(uint32_t damage, battlefield_unit_t& defender);
	std::pair<uint32_t, uint16_t> calculate_healing_to_stack(hero_t* caster, spell_e spell_id, uint32_t healing, battlefield_unit_t& unit, bool can_resurrect);
	std::pair<uint32_t, uint16_t> apply_healing_to_stack(uint32_t healing, battlefield_unit_t& unit, bool can_resurrect, bool simulate = false);
	
	uint cast_spell(hero_t* caster, spell_e spell_id, int target_x = -1, int target_y = -1, battlefield_unit_t* target = nullptr);
	std::vector<battlefield_unit_t*> cast_spell_on_random_troops(army_t::battlefield_unit_group_t& troops, spell_e spell_id, int duration, int number_of_troops_affected = 1);
	bool can_hero_cast_spell(hero_t* caster, spell_e spell_id);
	bool restore_hero_mana(hero_t* hero, int mana_to_restore, talent_e talent = TALENT_NONE);
	void init_hero_hero_battle(hero_t* attacker, hero_t* defender, bool is_deathmatch_battle = false);
	void init_hero_town_battle(hero_t* attacker, town_t* defender, bool is_deathmatch_battle = false);
	void init_hero_monster_battle(hero_t* attacker, map_monster_t* defender);
	void init_hero_creature_bank_battle(hero_t* attacker, army_t& defender, interactable_object_t* object);
	void setup_obstacles(bool only_clear = false);
	void reset();
	void start_combat();
	bool round_ended() const;
	void next_round();
	battle_result_e update_battle(game_t* game_instance, bool single_step = true);
	bool auto_move_troop();
	void update_all_units_overwhelm_status();
	bool is_troop_human_controlled(battlefield_unit_t* unit, game_t* game_instance);
	player_e get_player_for_troop(battlefield_unit_t* unit);
	void finish_troop_action(battlefield_unit_t* unit);
	bool catapult_shoot_wall(castle_wall_section_e wall_section);
		
	void compute_next_unit_to_move();
	bool recompute_unit_move_queue();
	battlefield_unit_t* get_active_unit();
	player_e get_player_of_active_unit();

	bool is_attackers_turn();
	/*uint*/ bool move_unit(battlefield_unit_t& unit, sint x, sint y, bool finalize_action = true);
	std::set<battlefield_hex_t*> get_movement_range(battlefield_unit_t& unit, sint radius, bool flyer);
	route_t get_unit_route(const battlefield_unit_t& unit, uint target_x, uint target_y);
	route_t get_unit_route_xy(const battlefield_unit_t& moving_unit, uint source_x, uint source_y, uint target_x, uint target_y);
	std::pair<battlefield_unit_t*, battlefield_hex_t*> get_target_in_range(battlefield_unit_t* acting_unit);
	std::pair<battlefield_unit_t*, battlefield_hex_t*> get_nearest_target(battlefield_unit_t* acting_unit, bool include_friendly = false, bool ignore_range = false);
	battlefield_hex_t* get_target_movement_hex(battlefield_unit_t* acting_unit);
	uint get_two_hex_effective_x(const battlefield_unit_t& unit, uint target_x, uint target_y);
	std::vector<battlefield_unit_t*> get_chain_lightning_targets(battlefield_unit_t* initial_target, int jump_count);
	bool is_move_valid(battlefield_unit_t& unit, uint target_x, uint target_y);
	bool is_spell_target_valid(hero_t* caster, battlefield_unit_t* unit, spell_e spell_id);
	bool is_spell_target_valid(hero_t* caster, int target_x, int target_y, spell_e spell_id);
	bool is_unit_immune_to_spell(const battlefield_unit_t* unit, spell_e spell_id) const;
	int unit_belongs_to_army(battlefield_unit_t* unit);
	bool wait_unit(battlefield_unit_t* unit);
	bool defend_unit(battlefield_unit_t* unit);
	uint attack_unit(troop_t& attacker, troop_t defender);
	bool unit_can_attack(battlefield_unit_t* attacker, battlefield_unit_t* defender, uint from_x = 0, uint from_y = 0);
	bool are_units_adjacent(battlefield_unit_t* attacker, battlefield_unit_t* defender);
	bool can_troop_shoot(const battlefield_unit_t* troop);
	bool can_troop_act(const battlefield_unit_t& unit, int round_offset = 0) const;
	//bool is_unit_disabled(const battlefield_unit_t& unit, int round_offset = 0) const;
	bool can_troop_morale(const battlefield_unit_t& unit);
	int get_unit_morale_chance(const battlefield_unit_t& unit) const;
	bool will_defender_retaliate(battlefield_unit_t& attacker, battlefield_unit_t& defender);
	bool attack_unit(battlefield_unit_t& attacker, battlefield_unit_t* target_unit, bool use_ranged_attack, int target_x, int target_y, battlefield_hex_t* source_movement_hex = nullptr);
	bool move_and_attack_unit(battlefield_unit_t& attacker, battlefield_unit_t& defender, sint from_x, sint from_y);
	void handle_post_attack_effects(battlefield_unit_t& attacker, battlefield_unit_t& defender, uint32_t damage, uint16_t kills, bool was_melee_attack, bool was_lucky_strike);
	double get_ranged_attack_penalty(battlefield_unit_t& attacker, int hex_x, int hex_y) const;
	double get_ranged_attack_penalty(battlefield_unit_t& attacker, battlefield_unit_t& defender) const;
	uint32_t apply_ranged_attack_penalty(uint32_t damage, battlefield_unit_t& attacker, battlefield_unit_t& defender);
	uint32_t apply_shooter_melee_penalty(uint32_t damage, battlefield_unit_t& attacker, battlefield_unit_t& defender);
	battlefield_unit_t* get_dead_unit_on_hex(sint x, sint y);
	battlefield_unit_t* get_unit_on_hex(sint x, sint y);
	battlefield_unit_t* get_unit_on_hex(hex_location_t point);
	battlefield_unit_t* get_unit_by_id(int8_t unit_id);
	battle_result_e compute_quick_combat();
	battle_result_e end_combat();
	bool troops_remain();
	uint32_t get_winning_hero_xp() const;
	void calculate_necromancy_results(battle_result_e result);
	void calculate_captured_artifacts(battle_result_e result);	
	troop_group_t get_remaining_troops_attacker() const;
	troop_group_t get_remaining_troops_defender() const;
};

QDataStream& operator<<(QDataStream& stream, const battlefield_t& battlefield);
QDataStream& operator>>(QDataStream& stream, battlefield_t& battlefield);

bool read_battlefield_from_stream(QDataStream& stream, battlefield_t& battlefield, game_t& game);
bool write_battlefield_to_stream(QDataStream& stream, const battlefield_t& battlefield, const game_t& game);

const std::string stringify_combat_action(const battle_action_t& action, const std::function<QString(const char*, int)>& tr);
