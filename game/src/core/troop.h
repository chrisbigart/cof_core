#pragma once

#include "core/utils.h"
#include "core/game_config.h"
#include "core/creature.h"
#include "core/spell.h"
#include "core/qt_headers.h"

#include <array>

struct buff_t {
	buff_e buff_id = BUFF_NONE;
	int8_t duration = 0;
	uint16_t magnitude = 0;
};

QDataStream& operator<<(QDataStream& stream, const buff_t& buff);
QDataStream& operator>>(QDataStream& stream, buff_t& buff);

struct troop_t {
	troop_t() {}
	troop_t(unit_type_e type, uint16_t size = 0) : stack_size(size), unit_type(type) {}
	uint16_t stack_size = 0;
	unit_type_e unit_type = UNIT_UNKNOWN;

	//std::string get_unit_name() const;
	//uint get_base_max_hitpoints() const;
	//uint get_base_attack() const;
	//uint get_base_defense() const;
	//uint get_base_speed() const;
	//uint get_base_initiative() const;
	//uint get_base_min_damage() const;
	//uint get_base_max_damage() const;

	void clear() { unit_type = UNIT_UNKNOWN; stack_size = 0; }
	bool is_empty() const { return /*unit_type == UNIT_UNKNOWN || */stack_size == 0; }
	bool is_turret() const {
		return (unit_type == UNIT_TURRET_MAIN || unit_type == UNIT_TURRET_LEFT || unit_type == UNIT_TURRET_RIGHT);
	}

	bool is_war_machine() const {
		return (unit_type == UNIT_CATAPULT || unit_type == UNIT_BALLISTA);
	}

	bool is_turret_or_war_machine() const { return is_turret() || is_war_machine(); }
	//bool operator==(const troop_t&) const = default;

	uint get_base_initiative() const {
		return game_config::get_creature(unit_type).initiative;
	}
	
	uint get_base_max_hitpoints() const {
		return game_config::get_creature(unit_type).health;
	}
	
	uint get_base_attack() const {
		return game_config::get_creature(unit_type).attack;
	}
	
	uint get_base_defense() const {
		return game_config::get_creature(unit_type).defense;
	}
	
	uint get_base_max_damage() const {
		return game_config::get_creature(unit_type).max_damage;
	}
	
	uint get_base_min_damage() const {
		return game_config::get_creature(unit_type).min_damage;
	}
	
	uint get_base_speed() const {
		return game_config::get_creature(unit_type).speed;
	}
	
	uint get_base_resistance(const magic_damage_e magic_type = MAGIC_DAMAGE_ALL) const {
		const auto& cr = game_config::get_creature(unit_type);
		uint value = 0;
		
		if(cr.has_inherent_buff(BUFF_MAGIC_IMMUNITY))
			return 100;
		if(magic_type == MAGIC_DAMAGE_FIRE && cr.has_inherent_buff(BUFF_FIRE_IMMUNITY))
			return 100;
		if(magic_type == MAGIC_DAMAGE_FROST && cr.has_inherent_buff(BUFF_FIRE_IMMUNITY))
			return 100;
		if(magic_type == MAGIC_DAMAGE_LIGHTNING && cr.has_inherent_buff(BUFF_LIGHTNING_IMMUNITY))
			return 100;
		if(magic_type == MAGIC_DAMAGE_EARTH && cr.has_inherent_buff(BUFF_EARTH_IMMUNITY))
			return 100;
		if(magic_type == MAGIC_DAMAGE_CHAOS && cr.has_inherent_buff(BUFF_CHAOS_IMMUNITY))
			return 100;
		if(magic_type == MAGIC_DAMAGE_HOLY && cr.has_inherent_buff(BUFF_HOLY_IMMUNITY))
			return 100;
		
		if(cr.has_inherent_buff(BUFF_UNICORN_RESISTANCE))
			return 20;
		if(cr.has_inherent_buff(BUFF_MAGIC_RESISTANCE)) //fixme
			return 30;
			
		return value;
	}
	
	std::string get_unit_name() const {
		if(stack_size < 2)
			return get_unit_name(false);
		else
			return get_unit_name(true);
	}
	
	std::string get_unit_name(bool plural) const {
		if(plural)
			return game_config::get_creature(unit_type).name_plural;
		else
			return game_config::get_creature(unit_type).name;
	}

};

QDataStream& operator<<(QDataStream& stream, const troop_t& troop);
QDataStream& operator>>(QDataStream& stream, troop_t& troop);

using troop_group_t = std::array<troop_t, game_config::HERO_TROOP_SLOTS>;

inline bool is_troop_group_empty(const troop_group_t& troops) {
	for(const auto& tr : troops) {
		if(!tr.is_empty())
			return false;
	}
	
	return true;
}
