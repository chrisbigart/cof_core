#pragma once

#include "core/game_config.h"
#include "player_configuration.h"
#include "lua_api.h"
#include "core/hero.h"
#include "core/adventure_map.h"
#include "core/battlefield.h"
#include "core/qt_headers.h"

#include <string>
#include <array>
#include <thread>
#include <functional>

//#pragma push_macro("check")
//#undef check
//
//#pragma push_macro("verify")
//#undef verify
//
//#include <QDateTime>
//#include <QBitArray>
//#include <QUuid>
//
//#pragma pop_macro("check")
//#pragma pop_macro("verify")

enum hero_class_e : uint8_t;
enum player_color_e : uint8_t;


enum object_metatype_e {
	OBJECT_METATYPE_UNKNOWN,
	OBJECT_METATYPE_TERRAIN,
	OBJECT_METATYPE_OBSTACLE,
	OBJECT_METATYPE_INTERACTABLE,
	OBJECT_METATYPE_HERO
};

enum editor_brush_category_e {
	CATEGORY_UNKNOWN,
	CATEGORY_TERRAIN,
	CATEGORY_TREE,
	CATEGORY_MOUNTAIN,
	CATEGORY_OBJECT,
	CATEGORY_WATER_OBJECT,
	CATEGORY_RESOURCE,
	CATEGORY_HERO,
	CATEGORY_TOWN
};

struct object_info_t {
	int16_t asset_id = -1;
	std::string name;
	std::string filename;
	object_metatype_e object_metatype = OBJECT_METATYPE_UNKNOWN;
	interactable_object_e interactable_object_type = OBJECT_UNKNOWN;
	terrain_type_e terrain_type = TERRAIN_UNKNOWN;
	editor_brush_category_e editor_brush_category = CATEGORY_UNKNOWN;
	int8_t center_x = 0;
	int8_t center_y = 0;
	quint64 passability = 0;
	quint64 interactability = 0;
};

struct save_game_header_t {
	//game version number
	std::string save_name;
	std::string map_name;
	QDateTime started_date;
	QDateTime last_save_time;
	uint16_t total_days = 0;
	uint16_t save_points = 0;
	QUuid session_uuid;
	//fixme
	//std::vector<qint64> save_point_offsets;
	//std::array<qint64, 64> save_point_offsets = {0}; //fixme
	
};

struct save_checkpoint_t {
	//uint32_t checkpoint_size;
	uint16_t date = 0;
	std::vector<interactable_object_t*> objects;
	std::vector<hero_t> heroes;
	std::vector<QBitArray> visibility_map;
	//QBitArray observability_map;
	//movement plans for heroes
	
};

struct difficulty_configuration_t {
	uint8_t difficulty = 1;
	bool king_of_the_hill = false;
	bool challenge = false;
	bool hardcore = false;
};

struct game_configuration_t {
	difficulty_configuration_t difficulty;
	uint player_count = 1;
	std::array<player_configuration_t, game_config::MAX_NUMBER_OF_PLAYERS> player_configs;
};

struct movement_stats_t {
	uint64_t total_hero_movement_points_spent = 0;
	uint64_t total_hero_movement_points_remaining_at_end_of_turns = 0;
	uint32_t total_hero_steps_taken = 0;
	uint32_t total_hero_steps_taken_on_roads = 0;
	uint32_t total_hero_steps_taken_on_grass = 0;
	uint32_t total_hero_steps_taken_on_dirt = 0;
	uint32_t total_hero_steps_taken_on_lava = 0;
	uint32_t total_hero_steps_taken_on_desert = 0;
	uint32_t total_hero_steps_taken_on_beach = 0;
	uint32_t total_hero_steps_taken_on_jungle = 0;
	uint32_t total_hero_steps_taken_on_swamp = 0;
	uint32_t total_hero_steps_taken_on_wasteland = 0;
	uint32_t total_hero_steps_taken_on_snow = 0;
	uint32_t total_hero_steps_taken_on_water = 0;
};

struct exploration_stats_t {
	uint16_t fog_tiles_revealed = 0;
	uint32_t fog_tiles_revealed_watchtower = 0;
	uint32_t fog_tiles_revealed_eyes = 0;
	uint16_t maps_fully_revealed_small = 0;
	uint16_t maps_fully_revealed_medium = 0;
	uint16_t maps_fully_revealed_large = 0;
	uint32_t watchtowers_visited = 0;
	uint32_t keymaster_tents_visited = 0;
	uint32_t campfires_visited = 0;
	uint16_t portals_used = 0;
};

struct resource_stats_t {
	resource_group_t total_resources_picked_up;
	resource_group_t total_resources_from_mines;
	resource_group_t total_resources_from_towns;
	resource_group_t total_resources_spent;
	resource_group_t total_resources_spent_on_buildings;
	resource_group_t total_resources_spent_on_creatures;
	uint32_t total_experience_from_chests = 0;
	uint32_t total_gold_from_chests = 0;
};

struct construction_stats_t {
	uint16_t total_forts_built = 0;
	uint16_t total_castles_upgraded = 0;
	uint16_t total_castles_fully_upgraded = 0;
	uint16_t total_marketplaces_built = 0;
	uint16_t total_tier1_dwellings_built = 0;
	uint16_t total_tier2_dwellings_built = 0;
	uint16_t total_tier3_dwellings_built = 0;
	uint16_t total_tier4_dwellings_built = 0;
	uint16_t total_tier5_dwellings_built = 0;
	uint16_t total_tier6_dwellings_built = 0;
	uint16_t total_special_buildings_built = 0;
	uint16_t total_captains_quarters_built = 0;
	uint16_t total_turrets_built = 0;
	uint16_t paladin_buildings = 0;
	uint16_t enchantress_buildings = 0;
	uint16_t wizard_buildings = 0;
	uint16_t barbarian_buildings = 0;
	uint16_t warlock_buildings = 0;
	uint16_t necromancer_buildings = 0;
};

struct game_stats_t {
	movement_stats_t movement;
	resource_stats_t resources;
	exploration_stats_t exploration;
	construction_stats_t construction;
	std::map<unit_type_e, uint32_t> units_recruited;
	uint32_t total_units_recruited = 0;
	uint16_t total_heroes_recruited = 0;
	uint16_t total_heroes_dismissed = 0;
	uint32_t total_creatures_recruited = 0;
	uint16_t total_towns_captured = 0;
	uint16_t total_towns_lost = 0;
	uint16_t total_mines_flagged = 0;
	uint16_t total_mines_lost = 0;
	uint16_t total_hero_levels_gained = 0;
	uint16_t total_hero_talents_acquired = 0;
	uint16_t total_battles_fought = 0;
	uint16_t total_battles_won = 0;
	uint16_t total_battles_lost = 0;
	uint16_t total_battles_drawn = 0;
	uint16_t total_shrines_visited = 0;
	uint16_t total_artifacts_found = 0;
	uint16_t total_artifacts_found_rarity1 = 0;
	uint16_t total_artifacts_found_rarity2 = 0;
	uint16_t total_artifacts_found_rarity3 = 0;
	uint16_t total_artifacts_found_rarity4 = 0;
	uint16_t total_artifacts_found_rarity5 = 0;
	uint16_t total_witches_huts_visited = 0;
	uint16_t total_graveyards_pillaged = 0;
	uint16_t towns_defended_with_one_unit = 0;
	uint8_t total_players_defeated = 0;
	uint16_t total_creatures_rescued_from_banks = 0;
	uint16_t total_dragons_defeated_from_topes = 0;

	combat_stats_t cumulative_combat_stats;
};

struct player_t {
	player_e player_number= PLAYER_NONE;
	bool is_human = false;
	bool is_alive = true;
	int days_left_to_capture_castle = 7;
	static constexpr int NUMBER_OF_RECRUITABLE_HEROES = 3;
	std::array<hero_t, NUMBER_OF_RECRUITABLE_HEROES> heroes_available_to_recruit;
	bool has_completed_turn = false;
	resource_group_t resources;
	game_stats_t player_stats;
	QBitArray tile_visibility;
	QBitArray tile_observability;
};

QDataStream& operator<<(QDataStream& stream, const player_t& player);
QDataStream& operator>>(QDataStream& stream, player_t& player);

enum game_status_e : uint8_t {
	GAME_STATUS_INCOMPLETE,
	GAME_STATUS_HUMAN_VICTORY,
	GAME_STATUS_HUMAN_LOSS
};

enum game_state_update_e : uint8_t {
	GAME_STATE_UNKNOWN,
	GAME_STATE_WON_GAME,
	GAME_STATE_LOST_GAME,
	GAME_STATE_TIME_EXPIRED,
	GAME_STATE_PLAYER_ELIMINATED,
	GAME_STATE_WILL_BE_BANISHED_WITHOUT_CASTLE
};

enum replay_action_e {
	ACTION_REPLAY_NONE,
	ACTION_REPLAY_SET_INITIAL_HERO_POSITION,
	ACTION_MOVE_TO_TILE,
	ACTION_PICKUP_ITEM,
	ACTION_FLAG_STRUCTURE,
	ACTION_DEFEAT_MONSTER,
	ACTION_CAPTURE_TOWN,
	ACTION_REMOVE_HERO
};

enum ai_turn_progress_e {
	AI_TURN_PROGRESS_UNKNOWN,
	AI_TURN_PROGRESS,
	AI_TURN_COMPLETE
};

struct ai_turn_progress_t {
	ai_turn_progress_e progress_type = AI_TURN_PROGRESS_UNKNOWN;
	int progress_percent = -1;
	player_e player_num = PLAYER_NONE;
};

struct map_tile_t;

struct replay_action_t {
	hero_t* hero = nullptr;
	replay_action_e action = ACTION_REPLAY_NONE;
	bool processed = false;
	int16_t tile_x = -1;
	int16_t tile_y = -1;
	interactable_object_e object_type = OBJECT_UNKNOWN;
	uint16_t target_object_id = 0;
	union {
		artifact_e art_type = ARTIFACT_NONE;
		resource_e res_type;
	} target_object_type;
};

enum dialog_type_e {
	DIALOG_TYPE_NONE,
	DIALOG_TYPE_PICKUP_ARTIFACT,
	DIALOG_TYPE_PICKUP_RESOURCE,
	DIALOG_TYPE_PICKUP_CAMPFIRE,
	DIALOG_TYPE_PICKUP_SEA_CHEST,
	DIALOG_TYPE_PICKUP_SEA_BARRELS,
	DIALOG_TYPE_PICKUP_FLOATSAM,
	DIALOG_TYPE_PICKUP_SHIPWRECK_SURVIVOR,
	DIALOG_TYPE_TREASURE_CHEST_CHOICE,
	DIALOG_TYPE_WARRIORS_TOMB_CHOICE,
	DIALOG_TYPE_WARRIORS_TOMB_REWARD,
	DIALOG_TYPE_WITCH_HUT_CHOICE,
	DIALOG_TYPE_ARENA_VISIT,
	DIALOG_TYPE_PYRAMID_VISIT,
	DIALOG_TYPE_PYRAMID_REWARD,
	DIALOG_TYPE_MAGIC_SHRINE_VISIT,
	DIALOG_TYPE_HUT_OF_THE_MAGI,
	DIALOG_TYPE_GAZEBO_VISIT,
	DIALOG_TYPE_GRAVEYARD_VISIT,
	DIALOG_TYPE_GRAVEYARD_RESULT,
	DIALOG_TYPE_SCHOOL_OF_WAR_CHOICE,
	DIALOG_TYPE_SCHOOL_OF_MAGIC_CHOICE,
	DIALOG_TYPE_PANDORAS_BOX_CHOICE,
	DIALOG_TYPE_PANDORAS_BOX_REWARD,
	DIALOG_TYPE_PANDORAS_BOX_ATTACKED,
	DIALOG_TYPE_SCHOLAR_CHOICE,
	DIALOG_TYPE_REFUGEE_CAMP_CHOICE,
	DIALOG_TYPE_REFUGEE_CAMP_RECRUIT,
	DIALOG_TYPE_MINE,
	DIALOG_TYPE_WATERWHEEL,
	DIALOG_TYPE_WINDMILL,
	DIALOG_TYPE_SIGN,
	DIALOG_TYPE_STANDING_STONES,
	DIALOG_TYPE_MERCENARY_CAMP,
	DIALOG_TYPE_WELL,
	DIALOG_TYPE_MONUMENT,
	DIALOG_TYPE_GAME_EVENT_VICTORY,
	DIALOG_TYPE_GAME_EVENT_LOSS,
	DIALOG_TYPE_EVENT,
	DIALOG_TYPE_EVENT_REWARD,
	DIALOG_TYPE_EVENT_ATTACKED,
	DIALOG_TYPE_CAPTURED_ARTIFACTS,
	DIALOG_TYPE_QUICK_COMBAT_CONFIRMATION,
	DIALOG_TYPE_QUIT_GAME_CONFIRMATION
};

enum well_visit_result_e {
	WELL_VISIT_RESULT_NONE,
	WELL_VISIT_RESULT_MANA_RESTORED,
	WELL_VISIT_RESULT_FULL_MANA,
	WELL_VISIT_RESULT_ALREADY_VISITED,
	WELL_VISIT_RESULT_BARBARIAN_MOVEMENT
};

enum witch_hut_visit_result_e {
	WITCH_HUT_VISIT_RESULT_NONE,
	WITCH_HUT_VISIT_RESULT_ALREADY_VISITED,
	WITCH_HUT_VISIT_RESULT_HERO_SKILL_LEVEL_MAXED,
	WITCH_HUT_VISIT_RESULT_SKILL_NOT_UPGRADABLE,
	WITCH_HUT_VISIT_RESULT_NO_EMPTY_SKILL_SLOTS,
	WITCH_HUT_VISIT_RESULT_HERO_CANNOT_LEARN_SKILL,
	WITCH_HUT_VISIT_RESULT_NEW_SKILL_OFFERED,
	WITCH_HUT_VISIT_RESULT_SKILL_UPGRADE_OFFERED
};

enum dialog_choice_e {
	DIALOG_CHOICE_NONE,
	DIALOG_CHOICE_CANCEL,
	DIALOG_CHOICE_ACCEPT,
	DIALOG_CHOICE_1,
	DIALOG_CHOICE_2,
	DIALOG_CHOICE_3,
	DIALOG_CHOICE_4,
	DIALOG_CHOICE_5
};

bool resource_valid(resource_e resource_type);
bool is_magic_resource(resource_e resource_type);
bool is_basic_resource(resource_e resource_type);

enum frequency_e : uint8_t {
	FREQUENCY_DAILY,
	FREQUENCY_WEEKLY,
	FREQUENCY_MONTHLY
};

struct ai_goal_t;

struct game_t {
	uint16_t date = 0;

	adventure_map_t map;
	battlefield_t battle;
	game_configuration_t game_configuration;

	std::vector<player_t> players;
	lua_State* _lua_state = nullptr;

	std::map<player_e, std::vector<replay_action_t>> last_turn_actions;
	std::vector<std::pair<hero_t*, uint64_t>> hero_unallocated_xp;


	int get_day() { return (date % 7) + 1; }
	int get_week() { return (((date) / 7) % 4) + 1; }
	int get_month() { return (date) / 28 + 1; }
	
	static int get_day(uint day) { return (day % 7) + 1; }
	static int get_week(uint day) { return (((day) / 7) % 4) + 1; }
	static int get_month(uint day) { return (day) / 28 + 1; }
	
	std::string get_date();
	
	void setup(game_configuration_t& game_settings, uint64_t seed);
	bool setup_scripting();
	
	//ai
	void process_all_ai_turns();
	void process_ai_turn(player_e player_num);
	
	//ai functions
	bool ai_move_hero_to_object(hero_t* hero, interactable_object_t* object);
	bool ai_pickup_object(hero_t* hero, interactable_object_t* object);
	bool ai_recruit_troops(hero_t* hero, town_t* town);
	bool ai_should_recruit_troops(const hero_t* hero, const town_t* town) const;
	bool ai_execute_hero_step(hero_t* hero, ai_goal_t& current_goal);
	building_e ai_determine_which_building_to_build(town_t* town, bool is_main_town);

	game_status_e get_scenario_status();
	void check_for_game_status_updates();
	bool end_turn(player_e player);
	void next_day();
	void next_week();
	void accept_battle_results();
	bool battle_retreat();
	void apply_battle_necromancy_results(hero_t* hero);
	void replay_battle();
	bool buy_troops_at_town(player_e player, town_t* town, uint slot_num, uint16_t count);
	hero_t* recruit_hero_at_town(player_e player, int selection, town_t* town);
	bool buy_troops_at_object(player_e player_num, interactable_object_t* object, uint16_t count);
	bool give_hero_xp(hero_t* hero, uint64_t xp);
	bool process_hero_unallocated_xp(hero_t* hero);
	map_action_e interact_with_object(hero_t* hero, interactable_object_t* object);
	bool pickup_object(hero_t* hero, interactable_object_t* object, int x, int y);
	map_action_e make_dialog_choice(hero_t* hero, interactable_object_t* object, dialog_type_e dialog_type, dialog_choice_e choice);
	bool make_hero_levelup_selection(hero_t* hero, skill_e skill_selection, talent_e talent_selection);
	spell_result_e cast_adventure_spell(hero_t* caster, spell_e spell_id, coord_t target = { -1, -1 });
	bool meet_heroes(hero_t* left, hero_t* right);
	hero_t generate_new_recruitable_hero(const player_t& player);
	void generate_new_week_recruitable_heroes();
	int16_t get_new_hero_id() const;
	hero_t* get_hero_by_id(uint16_t hero_id);
	bool player_valid(player_e player) const;
	player_t& get_player(player_e player);
	player_e get_first_human_player() const;
	const player_t& get_player(player_e player) const;
	player_configuration_t& get_player_configuration(player_e player);
	const player_configuration_t& get_player_configuration(player_e player) const;
	bool are_players_allied(player_e player1, player_e player2) const;
	
	int get_player_marketplace_count(player_e player_number);
	int get_marketplace_trade_rate(player_e player_number, resource_e resource_type_to_trade, resource_e resource_type_to_receive);
	bool trade_resources_via_marketplace(player_e player, resource_e resource_type_to_trade, uint32_t quantity_to_trade, resource_e resource_type_to_receive);
	bool give_resources_from_player_to_player(player_e from_player, player_e to_player, resource_e resource_type, uint32_t quantity_to_trade);
	bool give_resource_to_player(player_e player, resource_e resource_type, uint32_t quantity);
	
	//bool request_player_to_player_resource_trade(); //do we really want to enable this?
	
	bool is_tile_visible(int x, int y, player_e player_num) const;
	bool is_tile_observable(int x, int y, player_e player) const;
	bool is_tile_visible_and_observable(int x, int y, player_e player) const;
	void update_visibility();
	void update_visibility(player_e player);
	void reveal_area(player_e player, int x, int y, int reveal_radius, int observe_radius);
	uint save_game(const std::string& filename);
	static uint read_save_game_header_stream(QDataStream& stream, save_game_header_t& header);
	static uint write_save_game_header_stream(QDataStream& stream, const save_game_header_t& header);
	static uint load_saved_game_info(QDataStream& stream, save_game_header_t& header, game_t* game_info, std::vector<save_checkpoint_t>& checkpoints);
	static uint read_game_info(QDataStream& stream, game_t* game);
	static uint write_game_info(QDataStream& stream, const game_t* game);
	//uint write_save_point_stream(QDataStream& stream, const game_t* game);

	std::function<void(player_e)> game_state_update_callback_fn;
	std::function<void(game_state_update_e, player_e, int)> game_event_callback_fn;
	std::function<void(player_e)> ai_turn_completion_callback_fn;
	std::function<void(player_e, int)> ai_turn_progress_callback_fn;
	std::function<void(hero_t*, bool)> remove_hero_callback_fn;
	std::function<void(interactable_object_t*, bool)> remove_interactable_object_callback_fn;
	std::function<void(interactable_object_t*)> update_interactable_object_callback_fn;
	std::function<void(hero_t*, skill_e, skill_e, skill_e, stat_e, bool)> show_hero_levelup_callback_fn;
	std::function<void(dialog_type_e, interactable_object_t*, hero_t*, int, int)> show_dialog_callback_fn;
};
