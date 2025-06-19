#pragma once

#include "core/adventure_map.h"
#include "utils.h"
#include "core/game_config.h"
#include "core/magic_enum.hpp"

#include <cstdio>

#include "core/qt_headers.h"

/*
Structure of a map file: (OUTDATED!!)

header
    -version information [major, minor]
    -map size [width, height]
    -map name [32 utf8 characters]
    -map description [256 utf8 characters]
    -players
    -difficulty
    -win/loss conditions
    -minimap 'image' []


[ -zip 'file'
    /models -custom models for map objects or creatures
    /creatures -custom creatures
    /images -custom images for heroes, etc.
    /scripts -custom scripts for map objects, events, etc.

    map_data -map data
        -map tiles
            -terrain
            -doodads
        -events
		-rumors
		-castles/towns
		-heroes
		
]
*/

struct map_file_header_t {
	QUuid map_uuid;
    uint8_t version_major;
    uint8_t version_minor;
    uint16_t width;
    uint16_t height;
    uint8_t players;
    uint8_t difficulty;
    win_condition_e win_condition;
    loss_condition_e loss_condition;
	char name[32];
	char description[256];
};

const int MAP_FILE_HEADER_SIZE = sizeof(map_file_header_t);
const int MINIMUM_MAP_WIDTH = 32;
const int MINIMUM_MAP_HEIGHT = 32;
const int MAXIMUM_MAP_WIDTH = 512;
const int MAXIMUM_MAP_HEIGHT = 512;
const int MAXIMUM_DOODAD_COUNT = 100000;
const int MAXIMUM_OBJECTS_COUNT = 50000;
const int MAXIMUM_HERO_COUNT = 64;
const int MAXIMUM_TOWN_COUNT = 128;

//todo: add error codes
enum map_error_e {
	SUCCESS = 0,
	ERROR_UNKNOWN,// = 100,
	MAP_COULD_NOT_OPEN_FILE,
	MAP_COULD_NOT_READ_HEADER,
	MAP_COULD_NOT_WRITE_HEADER,
	MAP_WIDTH_TOO_SMALL,
    MAP_WIDTH_TOO_LARGE,
    MAP_HEIGHT_TOO_SMALL,
    MAP_HEIGHT_TOO_LARGE,
	MAP_NAME_TOO_LONG,
	MAP_DESCRIPTION_TOO_LONG,
	MAP_NAME_TOO_SHORT,
	MAP_DESCRIPTION_TOO_SHORT,
	MAP_NAME_INVALID_CHARACTERS,
	MAP_DESCRIPTION_INVALID_CHARACTERS,
	MAP_NAME_EMPTY,
	MAP_DESCRIPTION_EMPTY,
	MAP_VERSION_INVALID,
	MAP_PLAYER_COUNT_INVALID,
	MAP_DIFFICULTY_INVALID,
	MAP_WIN_CONDITION_INVALID,
	MAP_LOSS_CONDITION_INVALID,
	MAP_MINIMAP_INVALID,
	MAP_TILE_DATA_INVALID,
	MAP_EVENT_DATA_INVALID,
	MAP_RUMOR_DATA_INVALID,
	MAP_CASTLE_DATA_INVALID,
	MAP_HERO_DATA_INVALID,
	MAP_CUSTOM_MODEL_INVALID,
	MAP_CUSTOM_IMAGE_INVALID,
	MAP_CUSTOM_SCRIPT_INVALID,
	MAP_CUSTOM_DATA_INVALID,
	MAP_DOODAD_DATA_INVALID,
    MAP_TOO_MANY_DOODADS,
    MAP_TOO_MANY_OBJECTS,
    MAP_TOO_MANY_HEROES,
    MAP_TOO_MANY_TOWNS,
	MAP_DUPLICATE_HERO_ID,
	MAP_JSON_FORMAT_ERROR
};

const int file_magic_value = 0xB16A57;

std::string get_error_string_from_error_code(map_error_e error_code);
map_error_e validate_map_file_header(const map_file_header_t& header);
map_error_e read_map_file_header_c(const std::string& filename, map_file_header_t& header);
map_error_e read_map_file_header(QDataStream& stream, map_file_header_t& header);
map_error_e read_map_file_header_json(const std::string& filepath, map_file_header_t& header);
map_error_e write_map_file_header(QDataStream& stream, const map_file_header_t& header);
map_error_e write_map_file_header_c(const std::string& filename, const map_file_header_t& header);
map_error_e read_map_file(const std::string& filename, adventure_map_t& map);
map_error_e read_map_file(const std::string& filename, adventure_map_t& map, map_file_header_t& header);
map_error_e read_map_file_stream(QDataStream& stream, adventure_map_t& map, map_file_header_t& header);
map_error_e read_map_file_json(const std::string& filename, adventure_map_t& map);
map_error_e write_map_file(const std::string& filename, const adventure_map_t& map);
map_error_e write_map_file_stream(QDataStream& stream, const adventure_map_t& map);
map_error_e write_map_file_json(const std::string& path, const adventure_map_t& map);
