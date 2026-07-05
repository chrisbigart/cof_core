#pragma once

#include "adventure_map.h"

enum tree_type_e {
	TREE_TYPE_UNKNOWN,
	TREE_TYPE_PALM_01,
	TREE_TYPE_PALM_02,
	TREE_TYPE_PALM_03,
	TREE_TYPE_PALM_04,
	TREE_TYPE_PALM_05, //5
	TREE_TYPE_PALM_06_UNUSED,
	TREE_TYPE_PALM_07,
	TREE_TYPE_DEAD_OAK,
	TREE_TYPE_FOREST_GREEN_01,
	TREE_TYPE_FOREST_GREEN_02, //10
	TREE_TYPE_FOREST_GREEN_03,
	TREE_TYPE_FOREST_GREEN_04,
	TREE_TYPE_FOREST_GREEN_05,
	TREE_TYPE_FOREST_GREEN_06,
	TREE_TYPE_FOREST_GREEN_07, //15
	TREE_TYPE_FOREST_STUMP_01,
	TREE_TYPE_FOREST_STUMP_02,
	TREE_TYPE_FOREST_STUMP_03,
	TREE_TYPE_FOREST_STUMP_04,
	TREE_TYPE_BIRCH_01, //20
	TREE_TYPE_BIRCH_02,
	TREE_TYPE_BIRCH_03,
	TREE_TYPE_BIRCH_04,
	TREE_TYPE_BIRCH_05,
	TREE_TYPE_BIRCH_06, //25
	TREE_TYPE_BIRCH_WINTER_01,
	TREE_TYPE_BIRCH_WINTER_02,
	TREE_TYPE_BIRCH_WINTER_03,
	TREE_TYPE_SPRUCE_01,
	TREE_TYPE_SPRUCE_02, //30
	TREE_TYPE_SPRUCE_03,
	TREE_TYPE_SPRUCE_04,
	TREE_TYPE_SPRUCE_05,
	TREE_TYPE_SPRUCE_06,
	TREE_TYPE_SPRUCE_07, //35
	TREE_TYPE_SPRUCE_08,
	TREE_TYPE_SPRUCE_AUTUMN_01,
	TREE_TYPE_SPRUCE_AUTUMN_02,
	TREE_TYPE_SPRUCE_AUTUMN_03,
	TREE_TYPE_SPRUCE_AUTUMN_04, //40
	TREE_TYPE_SPRUCE_AUTUMN_05,
	TREE_TYPE_SPRUCE_AUTUMN_06,
	TREE_TYPE_SPRUCE_AUTUMN_07,
	TREE_TYPE_SPRUCE_WINTER_01,
	TREE_TYPE_SPRUCE_WINTER_02, //45
	TREE_TYPE_SPRUCE_WINTER_03,
	TREE_TYPE_SPRUCE_WINTER_04,
	TREE_TYPE_SPRUCE_WINTER_05,
	TREE_TYPE_SPRUCE_WINTER_06,
	TREE_TYPE_SPRUCE_WINTER_07, //50
	TREE_TYPE_DEAD_WINTER_01,
	TREE_TYPE_DEAD_WINTER_02,
	TREE_TYPE_GREEN_LARGE_01,
	TREE_TYPE_GREEN_LARGE_02,
	TREE_TYPE_AUTUMN_LARGE_01, //55
	TREE_TYPE_AUTUMN_LARGE_02,
	TREE_TYPE_CACTUS_01,
	TREE_TYPE_CACTUS_02,
	TREE_TYPE_CACTUS_03,
	TREE_TYPE_CACTUS_04, //60
	TREE_TYPE_CACTUS_05,
	TREE_TYPE_PALM_08,
	TREE_TYPE_PALM_09,
	TREE_TYPE_PALM_10,
	TREE_TYPE_PALM_11 //65
};

enum mountain_type_e {
	MOUNTAIN_TYPE_UNKNOWN,
	MOUNTAIN_TYPE_SNOW_MEDIUM_01,
	MOUNTAIN_TYPE_SNOW_LARGE_01,
	MOUNTAIN_TYPE_MESA_MEDIUM_01,
	MOUNTAIN_TYPE_MESA_MEDIUM_02,
	MOUNTAIN_TYPE_MESA_LARGE_01,
	MOUNTAIN_TYPE_GRASSY_SNOWY_MEDIUM_01,
	MOUNTAIN_TYPE_GRASSY_SNOWY_LARGE_01
};

struct tree_info_t {
	float x, y, z;
	float pitch, yaw, roll;
	float sx, sy, sz;
	tree_type_e tree_type = TREE_TYPE_UNKNOWN;
	bool rotation_locked = false;
};

struct tree_brush_t {
	std::bitset<16> native_terrain_types;
	std::vector<tree_info_t> tree_placement_info;
	int density_level = 0;
};

struct mountain_info_t {
	float x, y, z;
	float pitch, yaw, roll;
	float sx, sy, sz;
	int width = 0;
	int height = 0;
	mountain_type_e mountain_type = MOUNTAIN_TYPE_UNKNOWN;
	bool rotation_locked = false;
};

struct mountain_brush_t {
	std::bitset<16> native_terrain_types;
	std::vector<mountain_info_t> mountain_placement_info;
	int width = 0;
	int height = 0;
	std::bitset<64> passability = 0;
	std::bitset<64> tree_placement = 0;
	int density_level = 0;
};

struct decoration_info_t {
	float x, y, z;
	float pitch, yaw, roll;
	float sx, sy, sz;
	//tree_type_e tree_type = TREE_TYPE_UNKNOWN;
	int width = 0;
	int height = 0;
	bool rotation_locked = false;
};

struct decoration_brush_t {
	std::bitset<16> native_terrain_types;
	std::vector<decoration_info_t> decoration_placement_info;
	int width = 0;
	int height = 0;
	std::bitset<64> passability = 0;
	std::bitset<64> tree_placement = 0;
	int density_level = 0;
};