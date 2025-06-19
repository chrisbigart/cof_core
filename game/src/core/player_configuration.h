#pragma once

#include <string>
#include <vector>

enum hero_class_e : uint8_t;
//enum player_color_e : uint8_t;

#include "core/hero.h"

enum player_type_e : uint8_t {
	PLAYER_TYPE_HUMAN_OR_COMPUTER,
	PLAYER_TYPE_COMPUTER_ONLY,
	PLAYER_TYPE_HUMAN_ONLY
};

enum player_color_e : uint8_t {
	PLAYER_COLOR_BLUE,
	PLAYER_COLOR_RED,
	PLAYER_COLOR_GREEN,
	PLAYER_COLOR_ORANGE,
	PLAYER_COLOR_TEAL,
	PLAYER_COLOR_PINK,
	PLAYER_COLOR_PURPLE,
	PLAYER_COLOR_YELLOW,
	PLAYER_COLOR_NEUTRAL
};

//todo: fixed-size types
struct player_configuration_t {
	player_e player_number = PLAYER_NONE;
	player_color_e color = PLAYER_COLOR_NEUTRAL;
	uint team = 0;
	bool is_human = false;
	std::string player_name;
	/*hero_class_e*/
	int selected_class = -1;
	//main town?
	std::vector<hero_class_e> allowed_classes = {HERO_CLASS_ALL};
	player_type_e allowed_player_type = PLAYER_TYPE_HUMAN_OR_COMPUTER;
	int starting_hero_id = -1;
	bool was_class_random = false;
	bool was_hero_random = false;
	
	//bool operator==(const player_configuration_t&) const = default;
};


QDataStream& operator<<(QDataStream& stream, const player_configuration_t& player_config);
QDataStream& operator>>(QDataStream& stream, player_configuration_t& player_config);
