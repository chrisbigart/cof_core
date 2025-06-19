#pragma once

#include <string>

struct hero_t;
struct town_t;
struct adventure_map_t;
struct creature_t;
struct interactable_object_t;

struct script_t {
	int validate();
	int execute(int timeout_us = 1000);
	
	union parent_u {
		hero_t* hero = nullptr;
		town_t* town;
		adventure_map_t* map;
		interactable_object_t* object;
		creature_t* creature;
		//tile?
	};
	
	enum parent_type_e {
		SCRIPT_HERO,
		SCRIPT_TOWN,
		SCRIPT_MAP,
		SCRIPT_OBJECT,
		SCRIPT_CREATURE,
		SCRIPT_TILE
	};
	
	enum run_condition_e {
		CONDITION_NONE = 0,
		CONDITION_START_OF_TURN,
		CONDITION_END_OF_TURN,
		CONDITION_START_OF_DAY,
		CONDITION_START_OF_WEEK,
		CONDITION_HERO_LEVEL_UP,
		CONDITION_HERO_GAINS_XP,
		CONDITION_HERO_MOVES,
		CONDITION_BATTLE_STARTED,
		CONDITION_BATTLE_ENDED
		
	};
	
	int script_id = 0;
	bool enabled = false;
	bool finished = false;
	std::string name;
	std::string text;
	run_condition_e run_condition = CONDITION_NONE;
	parent_u parent;// = nullptr;
	parent_type_e parent_type = SCRIPT_MAP;
	
	bool operator==(const script_t& rhs) const {
		return (script_id == rhs.script_id &&
				enabled == rhs.enabled &&
				finished == rhs.finished &&
				name == rhs.name &&
				text == rhs.text &&
				parent_type == rhs.parent_type &&
				parent.object == rhs.parent.object
				);
	}
	
	//lua jit compiled blob?
};


inline QDataStream& operator<<(QDataStream& stream, const script_t& script) {
	stream << script.script_id << script.enabled << script.finished;
	stream_write_string(stream, script.name, 64);
	stream_write_string(stream, script.text, 4096);
	return stream;
}


inline QDataStream& operator>>(QDataStream& stream, script_t& script) {
	stream >> script.script_id >> script.enabled >> script.finished;
	script.name = stream_read_string(stream, 64);
	script.text = stream_read_string(stream, 4096);
	
	return stream;
}
