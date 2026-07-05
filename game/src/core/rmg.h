#pragma once

#include "core/adventure_map.h"

struct biome_object_info_t {
	interactable_object_e interactable_object_type = OBJECT_UNKNOWN;
	asset_id_t variation = 0;
	int min_spawned = 0;
	int max_spawned = 0;
	int value = 0;
	bool guarded = false;
	bool is_value_modified_by_proximity_to_road = true;
};

struct biome_t {
	int zone_id = -1;
	coord_t terrain_center;
	terrain_type_e terrain_type = TERRAIN_UNKNOWN;
	hero_class_e faction = HERO_CLASS_NONE;
	float terrain_center_range = 1.f;
	float scale = 1.f;
	int total_value = 5000;
	float guard_toughness_multi = 1.0f;
	bool spawn_basic_mines = true;
	bool spawn_roads_to_basic_mines = false;
	float resource_frequency = 10.f;
	std::bitset<8> allowed_resources = 0xFF;
	player_e player = PLAYER_NONE;
	int owned_town_count = 1;
	int neutral_town_count = 0;
	std::array<int, 8> zone_connections = { -1 };
	std::vector<biome_object_info_t> placeable_objects;
};

enum rmg_result_e {
	RMG_RESULT_OK,
	RMG_RESULT_ERROR
};

bool add_object_to_map(interactable_object_t* object, adventure_map_t& map);
bool add_object_to_map(interactable_object_t* object, adventure_map_t& map, QBitArray& obstacles);
rmg_result_e generate_random_map(adventure_map_t& map, std::vector<biome_t>& biomes, int64_t map_seed = -1);
