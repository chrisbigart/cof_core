#pragma once

#include "core/adventure_map.h"

struct biome_t {
	int zone_id = -1;
	coord_t terrain_center;
	terrain_type_e terrain_type = TERRAIN_UNKNOWN;
	hero_class_e faction = HERO_CLASS_NONE;
	float terrain_center_range = 1.f;
	float scale = 1.f;
	int total_value = 5000;
	player_e player = PLAYER_NONE;
	std::array<int, 8> zone_connections = { -1 };
};

enum rmg_result_e {
	RMG_RESULT_OK,
	RMG_RESULT_ERROR
};

rmg_result_e generate_random_map(adventure_map_t& map, std::vector<biome_t>& biomes);
