#pragma once

#include "core/interactable_object.h"
#include "core/player_configuration.h"
#include "core/town.h"
#include "core/hero.h"

#include <string>
#include <bitset>
#include <vector>
#include <map>
#include <deque>

enum win_condition_e : uint8_t {
	WIN_CONDITION_DEFEAT_ALL_ENEMIES,
	WIN_CONDITION_DEFEAT_HERO,
	WIN_CONDITION_CAPTURE_TOWN,
	WIN_CONDITION_FIND_LEGENDARY_ARTIFACT,
	WIN_CONDITION_ACQUIRE_SPECIFIC_ARTIFACT,
	WIN_CONDITION_ACQUIRE_RESOURCES
};

enum loss_condition_e : uint8_t {
	LOSS_CONDITION_LOSE_ALL_TOWNS_HEROES,
	LOSS_CONDITION_LOSE_HERO,
	LOSS_CONDITION_LOSE_TOWN,
	LOSS_CONDITION_LOSE_ARTIFACT,
	LOSS_CONDITION_TIME_EXPIRES
};

enum adventure_map_direction_e : uint8_t {
	DIRECTION_NONE = 0,
	DIRECTION_NORTH,
	DIRECTION_NORTHEAST,
	DIRECTION_EAST,
	DIRECTION_SOUTHEAST,
	DIRECTION_SOUTH,
	DIRECTION_SOUTHWEST,
	DIRECTION_WEST,
	DIRECTION_NORTHWEST
};

enum terrain_type_e : uint8_t {
	TERRAIN_UNKNOWN = 0,
	TERRAIN_WATER,
	TERRAIN_GRASS,
	TERRAIN_DIRT,
	TERRAIN_WASTELAND,
	TERRAIN_LAVA,
	TERRAIN_DESERT,
	TERRAIN_BEACH,
	TERRAIN_SNOW,
	TERRAIN_SWAMP,
	TERRAIN_JUNGLE
};

enum road_type_e : uint8_t {
	ROAD_NONE = 0,
	ROAD_DIRT,
	ROAD_GRAVEL,
	ROAD_COBBLESTONE
};

enum { TERRAIN_ANY = TERRAIN_UNKNOWN };

enum map_action_e : uint8_t {
	MAP_ACTION_NONE = 0,
	MAP_ACTION_VALID_MOVE,
	MAP_ACTION_BATTLE_INITIATED,
	MAP_ACTION_SHOW_SECONDARY_INFO_DIALOG, //dialogs only shown if option 'show messages' is true
	MAP_ACTION_SHOW_PRIMARY_INFO_DIALOG, //dialogs we always show the player
	MAP_ACTION_SHOW_PLAYER_CHOICE_DIALOG,
	MAP_ACTION_SHOW_EYES_OF_THE_MAGI,
	MAP_ACTION_HERO_TELEPORTED,
	MAP_ACTION_HERO_BOARDED_SHIP,
	MAP_ACTION_HERO_UNBOARDED_SHIP,
	MAP_ACTION_OBJECT_UPDATED
};

enum spell_result_e : uint8_t {
	SPELL_RESULT_OK = 0,
	SPELL_RESULT_ERROR,
	SPELL_RESULT_INSUFFICIENT_MANA,
	SPELL_RESULT_INVALID_TARGET,
	SPELL_RESULT_INSUFFICIENT_MOVEMENT,
	SPELL_RESULT_NOT_NEAR_WATER,
	SPELL_RESULT_NO_VALID_DESTINATION
};

struct town_t;
struct hero_t;

//any non-interactive object drawn on top of terrain (trees, mountains, lakes, etc)

struct doodad_t {
	asset_id_t asset_id = 0;
	uint16_t z;
	int32_t x;
	int32_t y;
	uint8_t width = 0;
	uint8_t height = 0;
};

#include "core/qt_headers.h"

QDataStream& operator<<(QDataStream& stream, const doodad_t& doodad);
QDataStream& operator>>(QDataStream& stream, doodad_t& doodad);

struct map_tile_t {
	asset_id_t asset_id = 0;
	terrain_type_e terrain_type = TERRAIN_UNKNOWN;
	road_type_e road_type = ROAD_NONE;
	uint8_t passability = 1;
	int8_t zone_id = -1;
	uint16_t interactable_object = 0;
	const std::string get_name() const;
	bool is_interactable() const { return interactable_object != 0; }
	bool is_passable() const { return passability == 1; }
};

QDataStream& operator<<(QDataStream& stream, const map_tile_t& tile);
QDataStream& operator>>(QDataStream& stream, map_tile_t& tile);

struct game_t;

struct coord_t {
	int x = 0;
	int y = 0;
	bool operator==(const coord_t& other) const { return x == other.x && y == other.y; }
	bool operator!=(const coord_t& other) const { return !(*this == other); }
	bool operator<(const coord_t& other) const {
		if (x == other.x)
			return y < other.y;

		return x < other.x;
	}
};

struct step_t {
	coord_t tile;
	int total_cost = 0;
	int penalty = 0;
};

typedef std::deque<step_t> route_t;

QColor get_qcolor_for_player_color(player_color_e pcolor);
terrain_type_e get_faction_native_terrain(hero_class_e faction);
hero_class_e get_faction_from_native_terrain_type(terrain_type_e terrain_type);
std::string get_color_string_for_player_color(player_color_e pcolor);
std::string get_color_name_for_player_color(player_color_e pcolor);
QColor get_color_for_tile(terrain_type_e type);
void draw_minimap(QImage& image, uint pixel_size, const adventure_map_t& map, const game_t* game = nullptr, player_e = PLAYER_NONE, const std::string& path_prefix = std::string(), bool reveal_map = false);

template<typename T> T get_skill_value(const T& base, const T& per_level, int skill_level) {
	if(skill_level == 0)
		return (T)0;

	return (T)(base + (per_level * (skill_level - 1)));
}

struct game_configuration_t;

struct adventure_map_t {
private:
	adventure_map_t& operator=(adventure_map_t& other);
public:
    adventure_map_t(uint width = 0, uint height = 0);
	~adventure_map_t() { for(auto o : objects) delete o; }
    bool tile_valid(int x, int y) const;

    //int read_map_from_file(const std::string& filename);
    int initialize_map(const game_configuration_t& game_config, int seed = 0);
    void clear(bool delete_objects = true, bool clear_tiles = true);

	static artifact_e get_random_artifact_of_rarity(artifact_rarity_e rarity);
    //const map_tile_t& get_tile(int x, int y) const;
	map_tile_t& get_tile(int x, int y);
	const map_tile_t& get_tile(int x, int y) const;
	map_tile_t& get_tile_for_interactable_object(interactable_object_t* object);
	interactable_object_t* get_interactable_object_for_tile(int x, int y) const;
	bool remove_interactable_object(interactable_object_t* object);
	bool remove_hero(hero_t* hero);
	map_monster_t* get_monster_guarding_tile(int x, int y) const;
	bool is_tile_guarded_by_monster(int x, int y) const;
	mutable QBitArray monster_guarded_cache;
	mutable bool monster_guarded_cache_valid = false;
	void update_guarded_monster_cache() const;
	void populate_refugee_camp(refugee_camp_t* camp);
	
	route_t get_route(const hero_t* hero, int x2, int y2, const game_t* game) const;
	route_t get_route_ignoring_blockables(const hero_t* hero, int x2, int y2, const game_t* game) const;
	
	//route_t get_route_astar(hero_t* hero, int x2, int y2) const;
	//route_t get_route_astar1(hero_t* hero, int x2, int y2) const;
	int heuristic(int x1, int y1, int x2, int y2) const;
	float get_terrain_movement_base_cost(const hero_t* hero, int x, int y) const;
	int get_terrain_movement_cost(const hero_t* hero, int x, int y, bool is_diagonal = false) const;
    bool is_route_passable(hero_t* hero, int x2, int y2, game_t* game) const;
	bool get_passability(int x, int y) const;
    int get_time_to_tile(const hero_t* hero, int x, int y, game_t* game) const;
	int get_route_cost_to_tile(const hero_t* hero, int x, int y, const game_t* game) const;
    town_t& get_nearest_town(int x, int y) const;
    static adventure_map_direction_e get_direction(int startx, int starty, int endx, int endy);
	static int direction_to_offset(adventure_map_direction_e direction);
	bool are_tiles_adjacent(int x1, int y1, int x2, int y2) const;
	bool are_tiles_diagonal(int x1, int y1, int x2, int y2) const;
	
	//todo probably move/refactor this
	coord_t get_interactable_offset_for_object(const interactable_object_t* object) const;
	coord_t get_interactable_coordinate_for_object(const interactable_object_t* object) const;
	interactable_object_t* get_object_by_id(uint16_t object_id, interactable_object_e match_type = OBJECT_UNKNOWN);
	uint16_t get_object_id(interactable_object_t* object) const;
	bool is_offset_passable(const interactable_object_t* object, int x, int y);
	bool is_offset_passable(const doodad_t* doodad, int x, int y);
	bool is_offset_interactable(const interactable_object_t* object, int x, int y);
	const hero_t* get_hero_at_tile(int x, int y) const;
	hero_t* get_hero_at_tile(int x, int y);
	std::pair<std::string, std::string> get_troop_count_strings(uint troop_count, uint scouting_level, bool use_prefix = false) const; //crystal ball?
	int get_owned_mines_count(resource_e mine_type, player_e player = PLAYER_NONE);
	
	//actions from player.  probably needs to be moved to client_t
	bool can_hero_move_to_tile(hero_t* hero, int x, int y, game_t* game);
	map_action_e move_hero_to_tile(hero_t& hero, int x, int y, game_t& game);
	map_action_e interact_with_object(hero_t* hero, interactable_object_t* object, game_t& game);
	bool zero_hero_movement_points_if_low(hero_t* hero, game_t& game);
	//troop management
	int get_next_open_troop_slot(const troop_group_t& troops);
	int get_number_of_open_slots(const troop_group_t& troops);
	bool swap_troop_slots(troop_group_t& troops, uint slot1, uint slot2);
	bool combine_troop_slots(troop_group_t& troops, uint slot1, uint slot2);
	bool split_troops(troop_group_t& troops, uint slot, split_mode_e mode, int count = -1);
	bool merge_troops(troop_group_t& troops, uint slot);
	void clear_troop_slots(troop_group_t& troops);
	bool dismiss_troops(hero_t* hero, int troop_index);
	bool move_troop_stack_from_hero_to_hero(hero_t* hero_from, hero_t* hero_to, uint stack, bool move_only_one_troop = false);
	bool move_troops_between_heroes(hero_t* source_hero, uint source_slot, hero_t* dest_hero, uint dest_slot);
	bool sort_hero_backpack(hero_t* hero);
	bool swap_hero_armies(hero_t* hero1, hero_t* hero2);
	bool swap_hero_everything(hero_t* hero1, hero_t* hero2);
	bool swap_hero_artifacts(hero_t* hero1, hero_t* hero2, bool include_backpack = true, bool backpack_only = false);
	bool move_all_artifacts_from_hero_to_hero(hero_t* hero_from, hero_t* hero_to,  bool include_backpack = true, bool backpack_only = false);
	bool move_artifact_from_hero_to_hero(hero_t* hero_from, hero_t* hero_to, uint from_slot, uint to_slot);
	bool move_artifact_from_hero_slot_to_hero_backpack(hero_t* hero_from, hero_t* hero_to, uint from_slot, uint to_slot);
	bool move_artifact_from_hero_backpack_to_hero_slot(hero_t* hero_from, hero_t* hero_to, uint from_slot, uint to_slot);
	bool move_artifact_from_hero_backpack_to_hero_backpack(hero_t* hero_from, hero_t* hero_to, uint from_slot, uint to_slot);
	bool move_army_from_hero_to_hero(hero_t* hero_from, hero_t* hero_to);
	bool move_troops_to_garrison(hero_t* source_hero, uint source_slot, town_t* dest_town, uint dest_slot);
	bool move_troops_from_garrison(town_t* source_town, uint source_slot, hero_t* dest_hero, uint dest_slot);
	bool move_troops_within_garrison(town_t* town, uint source_slot, uint dest_slot);
	bool move_troops_within_army(troop_group_t& troops, uint source_slot, uint dest_slot);
	bool move_troops_from_army_to_army(troop_group_t& source, uint source_slot, troop_group_t& dest, uint dest_slot);
	bool move_all_troops_from_garrison_to_visiting_hero(town_t* town);
	bool move_all_troops_from_visiting_hero_to_garrison(town_t* town);
	bool can_add_troop_to_group(const troop_t& troop, const troop_group_t& group);
	bool add_troop_to_group_combining(const troop_t& troop, troop_group_t& group);
	bool garrison_hero(hero_t* hero, town_t* town);
	bool ungarrison_hero(hero_t* hero, town_t* town);
	bool hero_visit_town(hero_t* hero, town_t* town);
	bool hero_board_ship(hero_t* hero, interactable_object_t* ship);
	bool hero_unboard_ship(hero_t* hero);
	hero_t* get_hero_on_ship(interactable_object_t* ship);
	bool swap_visiting_and_garrisoned_heroes(town_t* town);

	///
	
    uint16_t width;
    uint16_t height;

	QUuid map_uuid;
    std::string name;
    std::string description;

    uint8_t players;
    uint8_t difficulty;
	win_condition_e win_condition;
    loss_condition_e loss_condition;
	
    //probably want to change this
	static std::map<int, std::bitset<64>> interactivity_map;
	std::map<int, std::bitset<64>> passability_map;

    std::vector<map_tile_t> tiles;

	std::vector<doodad_t> doodads;
    std::vector<interactable_object_t*> objects;
    std::map<int, hero_t> heroes;
	
	std::array<player_configuration_t, game_config::MAX_NUMBER_OF_PLAYERS> player_configurations;
	
};

//bool operator==(const adventure_map_t& lhs, const adventure_map_t& rhs);
